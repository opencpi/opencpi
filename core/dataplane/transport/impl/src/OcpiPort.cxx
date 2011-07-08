
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * Abstact:
 *   This file contains the Implementation for the OCPI port.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <DtTransferInternal.h>
#include <DtHandshakeControl.h>
#include <OcpiPort.h>
#include <OcpiOutputBuffer.h>
#include <OcpiInputBuffer.h>
#include <OcpiPortSet.h>
#include <OcpiTransport.h>
#include <OcpiOsAssert.h>
#include <OcpiCircuit.h>
#include <OcpiRDTInterface.h>
#include <OcpiTransferController.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace OCPI::DataTransport;
using namespace DataTransfer;
using namespace DtI;
using namespace DtOsDataTypes;
using namespace OCPI::OS;

// Buffer allignment
#define BUF_ALIGNMENT 7

void OCPI::DataTransport::Port::reset()
{
  if ( ! this->isShadow() ) {
    m_externalState = WaitingForUpdate;
  }
  if ( ! isOutput() ) {  // input
    if ( getPortSet() &&  getPortSet()->getTxController() ) {
      getPortSet()->getTxController()->freeAllBuffersLocal(this);
    }
  }
  else {  // output
    if ( getPortSet() &&  getPortSet()->getTxController() ) {  
      getPortSet()->getTxController()->consumeAllBuffersLocal(this);
    }
    m_initialized = false;
  }
  m_sequence = 0;
  m_lastBufferOrd=-1;
}

/**********************************
 * Internal port initialization
 *********************************/
void OCPI::DataTransport::Port::initialize()
{
  if ( m_initialized ) {
    return;
  }

  m_sequence = 0;
  m_lastBufferOrd=-1;

  m_realSMemResources = XferFactoryManager::getFactoryManager().getSMBResources( m_data->real_location_string );
  if ( !  m_realSMemResources ) {
    throw OCPI::Util::EmbeddedException ( UNSUPPORTED_ENDPOINT, 
                                        m_data->real_location_string.c_str() );
  }

  // If we are a shadow, map the local shared memory
  if ( m_data->shadow_location_string.length() ) {
    m_shadowSMemResources = XferFactoryManager::getFactoryManager().getSMBResources( m_data->shadow_location_string );
  }
  else {
    m_shadowSMemResources = m_realSMemResources;
  }
        
  // Set the SMB name
  m_data->m_real_location = m_realSMemResources->sMemServices->endpoint();

  // Determine if we are a shadow port
  // Get our transport class
  Circuit *c = getCircuit();
  Transport* t = c->m_transport;

  if ( !t->isLocalEndpoint( m_data->m_real_location->end_point.c_str()) ) {

#ifdef DEBUG_L2
    printf( "We are a shadow port\n" );
#endif
    m_shadow = true;
    m_localSMemResources = m_shadowSMemResources;
  }
  else {

#ifdef DEBUG_L2
    printf("We are a real port\n" );
#endif
    m_shadow = false;
    m_localSMemResources = m_realSMemResources;
  }


  // At this point we are going to map our offset stucture into
  // SMB memory so that we can make direct requests to our remote
  // dependencies to fill in the missing data.
        
  if ( m_localSMemResources->sMemResourceMgr->alloc( 
                                    sizeof(PortMetaData::BufferOffsets)*MAX_BUFFERS, 0, &m_offsetsOffset ) ) {
    throw OCPI::Util::EmbeddedException ( 
                                      NO_MORE_SMB, 
                                      m_localSMemResources->sMemServices->endpoint()->end_point.c_str() );
  }

  // Now get our mailbox
  m_mailbox = m_localSMemResources->sMemServices->endpoint()->mailbox;

  // map our meta-data
  m_portDependencyData.offsets = static_cast<PortMetaData::BufferOffsets*>
    (m_localSMemResources->sMemServices->map 
     (m_offsetsOffset, 
      sizeof(PortMetaData::BufferOffsets)*MAX_BUFFERS ));

  memcpy(m_portDependencyData.offsets,m_data->m_bufferData,
         sizeof(PortMetaData::BufferOffsets[MAX_BUFFERS]));

  delete [] m_data->m_bufferData;
  m_data->m_bufferData = m_portDependencyData.offsets;
  m_data->m_init = true;

  // Now we need to create our buffers
  createBuffers();

  m_initialized = true;
}


/**********************************
 * Constructors
 *********************************/
OCPI::DataTransport::Port::Port( PortMetaData* data, PortSet* ps )
  : CU::Child<PortSet,Port>(*ps),
    m_initialized(false), 
    m_data( data ),
    m_realSMemResources(NULL),
    m_shadowSMemResources(NULL),
    m_localSMemResources(NULL),
    m_hsPortControl(NULL),
    m_externalState(NotExternal),
    m_pdDriver(NULL),
    m_portSet(ps)
{
  m_bufferCount=0;
  m_buffers[0]=0;
  m_zCopyBufferQ = 0;

#ifndef NDEBUG
  printf("In OCPI::DataTransport::Port::Port()\n");
#endif


  // Init member data
  m_lastBufferTidProcessed = 0;
  m_eos = false;
  m_data->rank = ps->getPortCount();

  if ( m_data->real_location_string.length() ) {
    initialize();
  }

}

/**********************************
 * Get the shared memory object
 *********************************/
SmemServices* OCPI::DataTransport::Port::getRealShemServices()
{
  if ( m_realSMemResources )
    return m_realSMemResources->sMemServices;
  else
    return NULL;
}

/**********************************
 * Sets the feedback descriptor for this port.
 *********************************/
void 
Port::
setFlowControlDescriptor( OCPI::RDT::Descriptors& d )
{
  getCircuit()->setFlowControlDescriptor(this, d);
}

/**********************************
 * Finalize the port
 *********************************/
void 
Port::
finalize( OCPI::RDT::Descriptors& d)
{
  SmemServices * smem = getRealShemServices();
  ocpiAssert( smem );

  std::string s2(d.desc.oob.oep);
  XferServices * xfers =   
    XferFactoryManager::getFactoryManager().getService( getEndpoint()->end_point, s2 );
  xfers->finalize( d.desc.oob.cookie );

}



void 
Port::
setFlowControlDescriptorInternal( OCPI::RDT::Descriptors & desc )
{

  initialize();

  // This descriptor contains the information needed by this port to tell a producer
  // when a buffer is empty.
  getMetaData()->m_shadowPortDescriptor = desc;
  getMetaData()->m_externPortDependencyData = desc;


  // For each buffer, set the descriptor
  PortSet* s_ps = static_cast<PortSet*>(getCircuit()->getOutputPortSet());
  OCPI::DataTransport::Port* output_port = 
    static_cast<OCPI::DataTransport::Port*>(s_ps->getPortFromIndex(0));
  int idx = output_port->m_realSMemResources->sMemServices->endpoint()->mailbox;
  for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {
    InputBuffer* tb = static_cast<InputBuffer*>(getBuffer(n));
    if ( desc.desc.emptyFlagPitch == 0 ) {
      tb->useTidForFlowControl(false);
    }
    else {
      tb->useTidForFlowControl(true);
    }
    tb->markBufferEmpty();
    this->getMetaData()->m_bufferData[n].inputOffsets.myShadowsRemoteStateOffsets[idx] =
      desc.desc.emptyFlagBaseAddr  + ( desc.desc.emptyFlagPitch * n );
    m_portDependencyData.offsets[n].inputOffsets.myShadowsRemoteStateOffsets[idx] =                        
      desc.desc.emptyFlagBaseAddr  + ( desc.desc.emptyFlagPitch * n );
  }

  m_externalState = DefinitionComplete;

  update();
}


/**********************************
 * Get port dependency data.  This is the data that is needed by an
 * external circuit to connect to this port.
 *********************************/
void OCPI::DataTransport::Port::getPortDescriptor( OCPI::RDT::Descriptors& desc, OCPI::RDT::Descriptors * other  )
{

  // If we are not a shadow port, we fill in all of the real descriptor dependency information
  // that is needed by a producer port to output our data.  If we are a shadow port (we only learn about
  // this after we as a output port are connected to an external consumer port) then we only have "remote 
  // buffer state" information.  This is descriptor information that is used on the consumer side to tell us
  // we a remote input buffer becomes available.
  if ( ! this->isShadow() ) {

    if ( other ) {
      // Get the connection cookie
      std::string mep(desc.desc.oob.oep);
      std::string tep(other->desc.oob.oep);
      XferServices * xfers = 
      XferFactoryManager::getFactoryManager().getService( mep, tep);
      ocpiAssert( xfers );
      desc.desc.oob.cookie = 
	xfers->getConnectionCookie();
    }

    if ( ! isOutput() ) { 

      desc.type = OCPI::RDT::ConsumerDescT;
      desc.role = OCPI::RDT::ActiveFlowControl;
      desc.options |= 1 << OCPI::RDT::MandatedRole;
      desc.desc.dataBufferBaseAddr = m_portDependencyData.offsets[0].inputOffsets.bufferOffset;
      desc.desc.dataBufferPitch = desc.desc.dataBufferSize = this->getPortSet()->getBufferLength();
      desc.desc.metaDataBaseAddr = m_portDependencyData.offsets[0].inputOffsets.metaDataOffset;
      desc.desc.metaDataPitch = sizeof(BufferMetaData) * MAX_PCONTRIBS;
      desc.desc.nBuffers = getPortSet()->getBufferCount();
      desc.desc.fullFlagBaseAddr = m_portDependencyData.offsets[0].inputOffsets.localStateOffset;
      desc.desc.fullFlagSize = sizeof(BufferState);
      desc.desc.fullFlagPitch = sizeof(BufferState) * MAX_PCONTRIBS * 2;

    }
    else {  // We are an output port

        desc.role = OCPI::RDT::ActiveFlowControl;
	desc.options |= 1 << OCPI::RDT::MandatedRole;
        desc.desc.dataBufferBaseAddr = 	m_data->m_bufferData[0].outputOffsets.bufferOffset;
        desc.desc.dataBufferPitch = desc.desc.dataBufferSize = this->getPortSet()->getBufferLength();
        desc.desc.metaDataBaseAddr = m_data->m_bufferData[0].outputOffsets.metaDataOffset;
        desc.desc.metaDataPitch = sizeof(BufferMetaData) * MAX_PCONTRIBS;
        desc.desc.nBuffers = getPortSet()->getBufferCount();
	desc.desc.fullFlagBaseAddr = m_data->m_bufferData[0].outputOffsets.localStateOffset;
        desc.desc.fullFlagSize = sizeof(BufferState);
        desc.desc.fullFlagPitch = sizeof(BufferState) * MAX_PCONTRIBS * 2;
	desc.desc.emptyFlagBaseAddr = m_data->m_bufferData[0].outputOffsets.localStateOffset;
        desc.desc.emptyFlagSize = sizeof(BufferState);
        desc.desc.emptyFlagPitch = sizeof(BufferState) * MAX_PCONTRIBS * 2;

    }

      if ( getCircuit()->m_transport->m_transportGlobal->useEvents() ) {

#ifndef NDEBUG
      printf("We are using EVENTS\n");
#endif
                
      int lr,hr;
      getCircuit()->m_transport->m_transportGlobal->getEventManager()->getEventRange(lr,hr);
      desc.desc.fullFlagValue = 1 | 
        ((OCPI::OS::uint64_t)(lr+1)<<32) | (OCPI::OS::uint64_t)1<<63;
#ifndef NDEBUG
      printf("OcpiPort: low range = %d, high range = %d, flag = 0x%llx\n", lr, hr, (long long)desc.desc.fullFlagValue);
#endif

    }
    else {
#ifndef NDEBUG
      printf("We are NOT using events \n");
#endif
      desc.desc.fullFlagValue = 1 | 
        ((OCPI::OS::uint64_t)(0xfff)<<32) | (OCPI::OS::uint64_t)1<<63;
    }

#ifndef NDEBUG
    printf("Full flag value = 0x%llx\n", (long long)desc.desc.fullFlagValue );
#endif

    desc.desc.oob.port_id = reinterpret_cast<uint64_t>(this);
    strcpy(desc.desc.oob.oep, m_realSMemResources->sMemServices->endpoint()->end_point.c_str());
    desc.desc.nBuffers = getPortSet()->getBufferCount();
    desc.desc.dataBufferSize = this->getPortSet()->getBufferLength();

    // We are being externally connected, so we need to wait for an update before
    // attempting to get our shadow information
    m_externalState = WaitingForUpdate;

  }
  else {  // We are a shadow port 
    ocpiAssert(0);
  }

}


/**********************************
 * Get the shared memory object
 *********************************/
SmemServices* OCPI::DataTransport::Port::getShadowShemServices()
{
  return m_shadowSMemResources->sMemServices;
}


/**********************************
 * Get the shared memory object
 *********************************/
SmemServices* OCPI::DataTransport::Port::getLocalShemServices()
{
  return m_localSMemResources->sMemServices;
}


/**********************************
 * Updates the port with addition meta-data information
 *********************************/
void OCPI::DataTransport::Port::update()
{
  this->initialize();
}


/**********************************
 * Advance the ports circular buffer
 *********************************/
void OCPI::DataTransport::Port::advance( OCPI::OS::uint64_t value )
{
  ( void ) value;
  if ( isOutput() ) {

#ifndef NDEBUG                
    printf("ERROR: Attemping to advance a source buffer \n");
#endif
                
    if ( isShadow() ) {
      // Nothing to do
      printf("Programming ERROR!! advancing a output shadow buffer\n");
    }
    else {
      // All output transfer objects are required to include a done flag,
      // Therfore there is nothing to do here either.  The event was generated
      // to ensure that the process woke up to process the output buffer becoming
      // empty
    }
  }
  else {
    if ( isShadow() ) {
        
#ifndef NDEBUG                                
      printf("Advancing the shadow buffer\n");
#endif
                        
      // If we are a input shadow buffer, it means that a remote input buffer
      // became empty.
      getPortSet()->getTxController()->freeBuffer(this);
    }
    else {

#ifndef NDEBUG
      printf("Advancing the REAL buffer\n");
#endif
                                                
      // One of our inputs just got filled
      getPortSet()->getTxController()->bufferFull(this);
                        
    }
  }
        
}


/**********************************
 * Can these two ports support Zero Copy transfers
 *********************************/
bool OCPI::DataTransport::Port::supportsZeroCopy( OCPI::DataTransport::Port* port )
{

  if ( getCircuit()->m_transport->isLocalEndpoint( port->m_data->m_real_location->end_point.c_str() ) &&
       getCircuit()->m_transport->isLocalEndpoint( m_data->m_real_location->end_point.c_str() )
       ) {
#ifndef NDEBUG
    printf(" NOTE: %s and %s are both local\n", m_data->m_real_location->end_point.c_str(), port->m_data->m_real_location->end_point.c_str() );
#endif
    return true;
  }



  else if ( 1  /*(test++)%3 == 0*/ ) {
    //                return TransportGlobal::m_ooPZeroCopy;
    return false;

  }

  return false;
}


/**********************************
 * Destructor
 *********************************/
void 
OCPI::DataTransport::Port::
createBuffers()
{
        
#ifndef NDEBUG
  printf("Number of buffers = %d\n", this->getBufferCount() );
#endif

  m_bufferCount = this->getBufferCount();

  // Do the resource allocation first to make sure we can habdle the request.
  allocateBufferResources();

  // Here we will create our buffers
  for ( OCPI::OS::uint32_t tid=0; tid<this->getBufferCount(); tid++ ) {
    if ( this->isOutput() ) {
      OutputBuffer* sb = new OutputBuffer( this, tid );
      addBuffer(sb);
      sb->update(true);
    }
    else {
      InputBuffer* sb = new InputBuffer( this, tid );
      addBuffer(sb);
      sb->update(true);
    }
  }



}

OCPI::OS::uint32_t 
Port::
getBufferCount()
{
  return m_portSet->getBufferCount();
}

/**********************************
 * Destructor
 *********************************/
OCPI::DataTransport::Port::
~Port()
{
  int rc;
  int index=0;
  ResourceServices* res_mgr;


  if ( ! m_initialized ) {
    return;
  }

  if ( isOutput() ) {

    if ( ! m_data->m_shadow  ) {

      res_mgr = 
        XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
      ocpiAssert( res_mgr );

      rc = res_mgr->free( m_data->m_bufferData[index].outputOffsets.bufferOffset,
                          m_data->m_bufferData[index].outputOffsets.bufferSize);
      ocpiAssert( rc == 0 );

      rc = res_mgr->free( m_data->m_bufferData[index].outputOffsets.localStateOffset,
                          sizeof(BufferState)*MAX_PCONTRIBS);
      ocpiAssert( rc == 0 );

      rc = res_mgr->free( m_data->m_bufferData[index].outputOffsets.metaDataOffset,
                          sizeof(BufferMetaData)*MAX_PCONTRIBS);
      ocpiAssert( rc == 0 );

      if ( m_data->m_localPortSetControl != 0  ) {

        rc = res_mgr->free( m_data->m_localPortSetControl,
                            sizeof(OutputPortSetControl));
        ocpiAssert( rc == 0 );
        m_data->m_localPortSetControl = 0;
      }
    }
  }
  else {

    if ( ! m_shadow ) {

      res_mgr = 
        XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
      ocpiAssert( res_mgr );

      rc = res_mgr->free( m_data->m_bufferData[index].inputOffsets.bufferOffset,
                          m_data->m_bufferData[index].inputOffsets.bufferSize);
      ocpiAssert( rc == 0 );

      rc = res_mgr->free( m_data->m_bufferData[index].inputOffsets.metaDataOffset,
                          sizeof(BufferMetaData)*MAX_PCONTRIBS);
      ocpiAssert( rc == 0 );

      rc = res_mgr->free( m_data->m_bufferData[index].inputOffsets.localStateOffset,
                          sizeof(BufferState)*MAX_PCONTRIBS);
      ocpiAssert( rc == 0 );

    }
    else {  // We are a shadow port

      res_mgr = XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_shadow_location )->sMemResourceMgr;
      ocpiAssert( res_mgr );

      rc = res_mgr->free( m_data->m_bufferData[index].inputOffsets.myShadowsRemoteStateOffsets[m_data->m_shadow_location->mailbox],
                          sizeof(BufferState));
      ocpiAssert( rc == 0 );
    }

  }


  if ( m_localSMemResources ) {
    m_localSMemResources->sMemResourceMgr->free( m_offsetsOffset,
                                                 sizeof(PortMetaData::BufferOffsets)*MAX_BUFFERS);
  }
 
  OCPI::OS::int32_t i;
  if ( m_buffers[0] ) {
    for (i=0; i<m_bufferCount; i++ ) {
      if ( this->isOutput() ) {
        delete static_cast<OutputBuffer*>(m_buffers[i]);
      }
      else {
        delete static_cast<InputBuffer*>(m_buffers[i]);        
      }
    }
  }
}

/**********************************
 * Get our associated circuit
 *********************************/
OCPI::DataTransport::Circuit* Port::getCircuit()
{
  return m_portSet->getCircuit();
}



/**********************************
 * Determines of a port is ready to go
 *********************************/
bool OCPI::DataTransport::Port::ready()
{

  if ( ! getCircuit()->m_transport->supportsMailboxes() ) {
    return true;
  }

  bool rtn=true;

  if ( m_externalState == WaitingForUpdate ) {
    return false;
  }

  // We need to be initialized first
  if ( ! m_initialized ) {
    return false;
  }


  int last_idx = getPortSet()->getBufferCount()-1;
  if ( ! isShadow() ) {  // Real port
                
    if ( ! isOutput() ) {  // Real input

      // Now make sure that all of our shadow offsets have been allocated
      for ( OCPI::OS::uint32_t n=0; n<getCircuit()->getOutputPortSet()->getPortCount(); n++ ) {

        PortSet* s_ps = static_cast<PortSet*>(getCircuit()->getOutputPortSet());
        OCPI::DataTransport::Port* output_port = static_cast<OCPI::DataTransport::Port*>(s_ps->getPortFromIndex(n));
        int idx = output_port->m_realSMemResources->sMemServices->endpoint()->mailbox;
        if ( ! m_portDependencyData.offsets[last_idx].inputOffsets.myShadowsRemoteStateOffsets[idx] ) {
                                        
          // Make sure this output port is not co-located

          if ( getCircuit()->m_transport->isLocalEndpoint( 
                                     output_port->getMetaData()->m_real_location->end_point.c_str() ) ) {

            for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {
              m_portDependencyData.offsets[n].inputOffsets.myShadowsRemoteStateOffsets[getMailbox()] =
                m_portDependencyData.offsets[n].inputOffsets.localStateOffset;
            }
            continue;
          }
                                        
          // Make the request to get our offset
          SMBResources* s_res = XferFactoryManager::getFactoryManager().getSMBResources( getEndpoint() );
          SMBResources* t_res = XferFactoryManager::getFactoryManager().getSMBResources( output_port->getEndpoint() );
          XferMailBox xmb( getMailbox() );

          if ( ! xmb.mailBoxAvailable(s_res) ) {
            return false;
          }
#ifndef NDEBUG
          printf("Real Input buffer is making a request to get shadow offsets !!\n");
#endif
                                        
          DataTransfer::ContainerComms::MailBox* mb = xmb.getMailBox( s_res );
          mb->request.reqShadowOffsets.type = DataTransfer::ContainerComms::ReqShadowRstateOffset;
                  

          if ( this->m_data->remoteCircuitId != -1 ) {
            mb->request.reqShadowOffsets.circuitId = this->m_data->remoteCircuitId;
            mb->request.reqShadowOffsets.portId    = this->m_data->remotePortId;
          }
          else {
            mb->request.reqShadowOffsets.circuitId = getCircuit()->getCircuitId();
            mb->request.reqShadowOffsets.portId    = getPortId();
          }

#ifndef NDEBUG
          printf("Making return address to %s\n", 
                 m_localSMemResources->sMemServices->endpoint()->end_point.c_str() );
#endif

          strncpy( mb->request.reqShadowOffsets.url, 
                   m_localSMemResources->sMemServices->endpoint()->end_point.c_str(), 128 );
          mb->return_offset = m_offsetsOffset;
          mb->return_size = sizeof( PortMetaData::BufferOffsets );
          mb->returnMailboxId = getMailbox();
                                        
          xmb.makeRequest( s_res, t_res );

          rtn = false;
                                        
        }
                                
      }


                
                        
    }
    else {  // Real output port

      // Nothing to do
      return true;

    }


  }
  else { // We are a shadow port

    if ( ! isOutput() ) {   // shadow input port

      // If we are a shadow input port, we need the offsets to our real port so that
      // co-located(with this shadow) source ports can know where to write.  In addition
      // we need the remote state offsets to ALL of the other shadow ports because of 
      // patterns that need to mark this buffer as used within the other shadow ports.
                        
      // We are ready when we have all of our offset data
      if ( ! m_portDependencyData.offsets[last_idx].inputOffsets.bufferOffset ||
           ! m_portDependencyData.offsets[last_idx].inputOffsets.localStateOffset ||
           ! m_portDependencyData.offsets[last_idx].inputOffsets.metaDataOffset ) {


        SMBResources* s_res = 
          XferFactoryManager::getFactoryManager().getSMBResources( m_localSMemResources->sMemServices->endpoint() );
        SMBResources* t_res = XferFactoryManager::getFactoryManager().getSMBResources( getEndpoint() );
        XferMailBox xmb( getMailbox() );
        if ( ! xmb.mailBoxAvailable(s_res) ) {
          return false;
        }
                                
#ifndef NDEBUG
        printf("Input Shadow buffer is making a request to get buffer offsets, my offset = 0x%llx!!\n",
               (long long unsigned)m_offsetsOffset);
#endif

        DataTransfer::ContainerComms::MailBox* mb = xmb.getMailBox( s_res );
        mb->request.reqInputOffsets.type = DataTransfer::ContainerComms::ReqInputOffsets;

        if ( this->m_data->remoteCircuitId != -1 ) {
          mb->request.reqInputOffsets.circuitId = this->m_data->remoteCircuitId;
          mb->request.reqInputOffsets.portId    = this->m_data->remotePortId;
        }
        else {
          mb->request.reqInputOffsets.circuitId = getCircuit()->getCircuitId();
          mb->request.reqInputOffsets.portId    = getPortId();
        }


#ifndef NDEBUG
        printf("Making return address to %s\n", 
               m_localSMemResources->sMemServices->endpoint()->end_point.c_str() );
#endif

        strncpy( mb->request.reqInputOffsets.url, 
                   m_localSMemResources->sMemServices->endpoint()->end_point.c_str(), 128 );
        mb->return_offset = m_offsetsOffset;
        mb->return_size = sizeof( PortMetaData::BufferOffsets );
        mb->returnMailboxId = getMailbox();
                
        xmb.makeRequest( s_res, t_res );
                                
        rtn = false;
      }


      // Now make sure that all of our shadow offsets have been allocated
      for ( OCPI::OS::uint32_t n=0; n<getCircuit()->getOutputPortSet()->getPortCount(); n++ ) {
                                
        PortSet* s_ps = static_cast<PortSet*>(getCircuit()->getOutputPortSet());
        OCPI::DataTransport::Port* shadow_port = static_cast<OCPI::DataTransport::Port*>(s_ps->getPortFromIndex(n));
        int idx = shadow_port->getMailbox();

        if ( ! m_portDependencyData.offsets[last_idx].inputOffsets.myShadowsRemoteStateOffsets[idx] ) {
                                        
          // Make sure this output port is not co-located

          if ( getCircuit()->m_transport->isLocalEndpoint( shadow_port->getMetaData()->m_real_location->end_point.c_str() ) ) {
            continue;
          }

          SMBResources* s_res = 
            XferFactoryManager::getFactoryManager().getSMBResources( getEndpoint() );
          SMBResources* t_res = XferFactoryManager::getFactoryManager().getSMBResources( shadow_port->getEndpoint() );
          XferMailBox xmb( getMailbox() );
          if ( ! xmb.mailBoxAvailable(s_res) ) {
            return false;
          }

#ifndef NDEBUG
          printf("Shadow Input buffer is making a request to get other shadow offsets !!\n");
#endif

          DataTransfer::ContainerComms::MailBox* mb = xmb.getMailBox(s_res);
          mb->request.reqShadowOffsets.type = DataTransfer::ContainerComms::ReqShadowRstateOffset;
                                

          if ( this->m_data->remoteCircuitId != -1 ) {
            mb->request.reqShadowOffsets.circuitId = this->m_data->remoteCircuitId;
            mb->request.reqShadowOffsets.portId    = this->m_data->remotePortId;
          }
          else {
            mb->request.reqShadowOffsets.circuitId = getCircuit()->getCircuitId();
            mb->request.reqShadowOffsets.portId    = getPortId();
          }

#ifndef NDEBUG
          printf("Making return address to %s\n", 
                 m_localSMemResources->sMemServices->endpoint()->end_point.c_str() );
#endif

          strncpy( mb->request.reqShadowOffsets.url, 
                   m_localSMemResources->sMemServices->endpoint()->end_point.c_str(), 128 );
          mb->return_offset = m_offsetsOffset;
          mb->return_size = sizeof( PortMetaData::BufferOffsets );
          mb->returnMailboxId = getMailbox();
                                        
          // Now make the request
          xmb.makeRequest( s_res, t_res );
                                        
          rtn = false;
                                        
        }

      }

    } 
    else {  // Shadow output port

      if ( ! m_portDependencyData.offsets[0].outputOffsets.portSetControlOffset ) {


        SMBResources* s_res = 
          XferFactoryManager::getFactoryManager().getSMBResources( getShadowEndpoint() );
        SMBResources* t_res = XferFactoryManager::getFactoryManager().getSMBResources( getEndpoint() );
        XferMailBox xmb( getMailbox() );
        if ( ! xmb.mailBoxAvailable(s_res) ) {
          return false;
        }

#ifndef NDEBUG
        printf("Output shadow port is making a request to get port control offsets !!\n");
#endif

        DataTransfer::ContainerComms::MailBox* mb = xmb.getMailBox(s_res);
        mb->request.reqOutputContOffset.type = DataTransfer::ContainerComms::ReqOutputControlOffset;

            
        if ( this->m_data->remoteCircuitId != -1 ) {
          mb->request.reqOutputContOffset.circuitId = this->m_data->remoteCircuitId;
          mb->request.reqOutputContOffset.portId    = this->m_data->remotePortId;
        }
        else {
          mb->request.reqOutputContOffset.circuitId = getCircuit()->getCircuitId();
          mb->request.reqOutputContOffset.portId    = getPortId();
        }

        // Need to tell it how to get back with us
        strcpy(mb->request.reqOutputContOffset.shadow_end_point,
               m_localSMemResources->sMemServices->endpoint()->end_point.c_str() );

#ifndef NDEBUG
        printf("Making return address to %s\n", 
               m_localSMemResources->sMemServices->endpoint()->end_point.c_str() );
        printf("Setting port id = %lld\n", (long long)mb->request.reqShadowOffsets.portId );
#endif

        mb->return_offset = m_offsetsOffset;
        mb->return_size = sizeof( PortMetaData::BufferOffsets );
        mb->returnMailboxId = getMailbox();
        xmb.makeRequest( s_res, t_res);

        return false;
      }
    }
  }

  return rtn;
}






/**********************************
 * writes buffer offsets to address
 *********************************/
void OCPI::DataTransport::Port::writeOffsets( PortMetaData::BufferOffsets* offset )
{
  // for all buffers
  if ( ! isOutput() ) { 
    for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {
                        
      if ( m_shadow ) {

        int idx = m_localSMemResources->sMemServices->endpoint()->mailbox;

        offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx ] =
          m_data->m_bufferData[n].inputOffsets.myShadowsRemoteStateOffsets[idx];
                                
#ifndef NDEBUG                
        printf("Wrote shadow offset %lld to address %p\n", 
               (long long)m_data->m_bufferData[n].inputOffsets.myShadowsRemoteStateOffsets[idx],
               &offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx] );
#endif
                                
                                
      }
      else {

#ifndef NDEBUG
        printf("Wrote real input offsets\n");
#endif
                                
        // Our buffer space
        offset[n].inputOffsets.bufferOffset = m_data->m_bufferData[n].inputOffsets.bufferOffset;
        offset[n].inputOffsets.bufferSize = m_data->m_bufferData[n].inputOffsets.bufferSize;
                                
        // Our state info
        offset[n].inputOffsets.localStateOffset = m_data->m_bufferData[n].inputOffsets.localStateOffset;
                                
        // Our meta-data
        offset[n].inputOffsets.metaDataOffset = m_data->m_bufferData[n].inputOffsets.metaDataOffset;
                                
      }
    }
  }
  else {

    if ( m_shadow ) {
      ocpiAssert(0);
    }

    for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {

      offset[n].outputOffsets.portSetControlOffset = 
        m_data->m_bufferData[n].outputOffsets.portSetControlOffset;

    }

#ifndef NDEBUG
    printf("Wrote output control offset 0x%llx to address %p\n", 
           (long long)m_data->m_bufferData[0].outputOffsets.portSetControlOffset,
           &offset[0].outputOffsets.portSetControlOffset );
#endif
                                
                                
  }

}


void OCPI::DataTransport::Port::releaseOffsets( OCPI::Util::VList& offsets )
{
  for ( OCPI::OS::uint32_t n=0; n<offsets.size(); n++ ) {
    ToFrom* tf = static_cast<ToFrom*>(offsets[n]);
    delete tf;
  }
  offsets.destroyList();
}

/**********************************
 * get buffer offsets to dependent data
 *********************************/
void OCPI::DataTransport::Port::getOffsets( OCPI::OS::uint32_t to_base_offset, OCPI::Util::VList& offsets )
{
  PortMetaData::BufferOffsets* from_offset = (PortMetaData::BufferOffsets*)m_offsetsOffset;
  PortMetaData::BufferOffsets* to_offset = (PortMetaData::BufferOffsets*)to_base_offset;

  // for all buffers
  if ( ! isOutput() ) { 
    for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {
                        
      if ( m_shadow ) {

        int idx = m_localSMemResources->sMemServices->endpoint()->mailbox;

        ToFrom* tf = new ToFrom;
        tf->from_offset = (OCPI::OS::uint64_t)&from_offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx];
        tf->to_offset = (OCPI::OS::uint64_t)&to_offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx];
        offsets.push_back( tf );

#ifndef NDEBUG
        printf("Wrote shadow offset 0x%llx to address 0x%llx\n", 
               (long long)tf->from_offset, (long long)tf->to_offset );
#endif
                                
      }
      else {

#ifndef NDEBUG
        printf("Wrote real input offsets\n");
#endif

        ToFrom* tf = new ToFrom;
        tf->from_offset = (OCPI::OS::uint64_t)&from_offset[n].inputOffsets.bufferOffset;
        tf->to_offset = (OCPI::OS::uint64_t)&to_offset[n].inputOffsets.bufferOffset;
        offsets.push_back( tf );

        tf = new ToFrom;
        tf->from_offset = (OCPI::OS::uint64_t)&from_offset[n].inputOffsets.bufferSize;
        tf->to_offset = (OCPI::OS::uint64_t)&to_offset[n].inputOffsets.bufferSize;
        offsets.push_back( tf );

        tf = new ToFrom;
        tf->from_offset = (OCPI::OS::uint64_t)&from_offset[n].inputOffsets.localStateOffset;
        tf->to_offset = (OCPI::OS::uint64_t)&to_offset[n].inputOffsets.localStateOffset;
        offsets.push_back( tf );

        tf = new ToFrom;
        tf->from_offset = (OCPI::OS::uint64_t)&from_offset[n].inputOffsets.metaDataOffset;
        tf->to_offset = (OCPI::OS::uint64_t)&to_offset[n].inputOffsets.metaDataOffset;
        offsets.push_back( tf );
      }
    }
  }
  else {

    if ( m_shadow ) {
      ocpiAssert(0);
    }

    for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {

      ToFrom* tf = new ToFrom;
      tf->from_offset = (OCPI::OS::uint64_t)&from_offset[n].outputOffsets.portSetControlOffset;
      tf->to_offset = (OCPI::OS::uint64_t)&to_offset[n].outputOffsets.portSetControlOffset;
      offsets.push_back( tf );
    }

#ifndef NDEBUG
    printf("Wrote output control offsets\n");
#endif
                                
  }
}



/**********************************
 * Has an End Of Stream been detcted on this port
 *********************************/
bool OCPI::DataTransport::Port::isEOS()
{
  return m_eos;
}



void OCPI::DataTransport::Port::resetEOS()
{
  m_eos=false;
}


/**********************************
 * Get/Set the SMB name
 *********************************/
void OCPI::DataTransport::Port::setEndpoint( std::string& ep)
{
  m_data->m_real_location->setEndpoint( ep );
}



void OCPI::DataTransport::Port::debugDump()
{
  printf("  Port id = %d\n", m_data->id );
  printf("    we are a %s port\n", m_data->output ? "output" : "input" );
  printf("    real location string = %s\n", m_data->real_location_string.c_str() );
  printf("    shadow location string = %s\n", m_data->shadow_location_string.c_str() );
  if ( isShadow() ) {
    printf("    remote port id = %lld\n", (long long)m_data->remotePortId );
  }
  printf("    our rank = %d\n", m_data->rank );
}



void 
Port::
createOutputOffsets()
{
  OCPI::OS::uint64_t boffset, moffset, soffset, coffset;
  OCPI::OS::uint32_t rc;
  bool local=false;
  OCPI::OS::uint32_t index;
  unsigned int bCount = m_data->m_portSetMd->bufferCount;

  // The allocation of buffer may be delayed if the circuit definition is not complete
  if ( ! m_data->m_real_location ) {
    ocpiAssert( 0 );
    return;
  }

  if ( getCircuit()->m_transport->isLocalEndpoint( m_data->m_real_location->end_point.c_str() ) ) {
    local = true;
    m_data->m_shadow = false;
  }

  if ( local ) {

    ResourceServices* res_mgr = 
      XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
    ocpiAssert( res_mgr );

    // Allocate the buffers.  We will allocate a contiguous block of memory
    // for all the buffers and the split them up
    rc = res_mgr->alloc( m_data->m_portSetMd->bufferLength * bCount, 
                         BUF_ALIGNMENT, &boffset);
    if ( rc != 0 ) {
      throw OCPI::Util::EmbeddedException( 
                                         NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->end_point.c_str() );
    }

    for ( index=0; index<bCount; index++ ) {
      m_data->m_bufferData[index].outputOffsets.bufferOffset = boffset+(index*m_data->m_portSetMd->bufferLength);
      m_data->m_bufferData[index].outputOffsets.bufferSize =  m_data->m_portSetMd->bufferLength;
    }
                
    // Allocate the local state
    rc = res_mgr->alloc( sizeof(BufferState) * MAX_PCONTRIBS * bCount, 
                         BUF_ALIGNMENT, &soffset);
    if ( rc != 0 ) {
      res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
      throw OCPI::Util::EmbeddedException( 
                                         NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->end_point.c_str() );
    }
    for ( index=0; index<bCount; index++ ) {
      m_data->m_bufferData[index].outputOffsets.localStateOffset = 
        soffset + index * MAX_PCONTRIBS * sizeof(BufferState);
    }
                
    // Allocate the meta-data structure
    rc = res_mgr->alloc( sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount, 
                         BUF_ALIGNMENT, &moffset);
    if ( rc != 0 ) {
      res_mgr->free( soffset,  sizeof(BufferState) * MAX_PCONTRIBS * bCount );
      res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
      throw OCPI::Util::EmbeddedException( 
                                         NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->end_point.c_str() );
    }
    for ( index=0; index<bCount; index++ ) {
      m_data->m_bufferData[index].outputOffsets.metaDataOffset = 
        moffset + index * MAX_PCONTRIBS * sizeof(BufferMetaData);
    }

    // Allocate the port set control structure if needed (even shadows get one of these)
    if ( m_data->m_localPortSetControl == 0 ) {
                        
      rc = res_mgr->alloc( sizeof(OutputPortSetControl), 
                           BUF_ALIGNMENT, &coffset);
      if ( rc != 0 ) {
        res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
        res_mgr->free( soffset,  sizeof(BufferState) * MAX_PCONTRIBS * bCount );
        res_mgr->free( moffset,   sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount );
        throw OCPI::Util::EmbeddedException( 
                                           NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->end_point.c_str() );
      }
      m_data->m_localPortSetControl = coffset;
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].outputOffsets.portSetControlOffset = m_data->m_localPortSetControl;
      }
    }
        
  }
}



void 
Port::
createInputOffsets() 
{
    OCPI::OS::uint64_t boffset, moffset, soffset;
    int rc;
    bool local=false;
    ResourceServices* res_mgr;
    unsigned int index;
    unsigned int bCount = m_data->m_portSetMd->bufferCount;

    // The allocation of buffer may be delayed if the circuit definition is not complete
    if ( ! m_data->m_real_location ) {
      ocpiAssert( 0 );
      return;
    }

    if ( getCircuit()->m_transport->isLocalEndpoint( m_data->m_real_location->end_point.c_str() ) ) {
      m_data->m_shadow = false;
      local = true;
    }

    // Allocate the buffer
    if ( local ) {

      res_mgr = XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
      ocpiAssert( res_mgr );
      rc = res_mgr->alloc( m_data->m_portSetMd->bufferLength * bCount, 
                           BUF_ALIGNMENT, &boffset);
      if ( rc != 0 ) {
        throw OCPI::Util::EmbeddedException( 
                                           NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->end_point.c_str() );
      }
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.bufferOffset = boffset + 
          index * m_data->m_portSetMd->bufferLength;
        m_data->m_bufferData[index].inputOffsets.bufferSize = m_data->m_portSetMd->bufferLength;
      }
                
#ifndef NDEBUG
      printf("\n\nInput buffer offset = 0x%" PRIx64 "\n", boffset );
#endif
                
      // Allocate the meta-data structure
      rc = res_mgr->alloc( sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount, 
                           BUF_ALIGNMENT, &moffset);
      if ( rc != 0 ) {
        res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
        throw OCPI::Util::EmbeddedException(  NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->end_point.c_str() );
      }
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.metaDataOffset = 
          moffset + index * sizeof(BufferMetaData) * MAX_PCONTRIBS;
      }
                
      // Allocate the local state(s)
      rc = res_mgr->alloc( 
                          sizeof(BufferState) * MAX_PCONTRIBS * bCount * 2, 
                          BUF_ALIGNMENT, &soffset);
      if ( rc != 0 ) {
        res_mgr->free( moffset,  sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount );
        res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
        throw OCPI::Util::EmbeddedException(  NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->end_point.c_str() );
      }
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.localStateOffset = 
          soffset + (index * sizeof(BufferState) * MAX_PCONTRIBS * 2);
      }
                
    }
    else {  // We are a shadow port

      res_mgr = XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_shadow_location )->sMemResourceMgr;
      ocpiAssert( res_mgr );
      rc = res_mgr->alloc( sizeof(BufferState) * bCount , BUF_ALIGNMENT, &soffset);
      if ( rc != 0 ) {
        throw OCPI::Util::EmbeddedException(
                                           NO_MORE_BUFFER_AVAILABLE, m_data->m_shadow_location->end_point.c_str());
      }
      m_data->m_shadowPortDescriptor.type = OCPI::RDT::ConsumerFlowControlDescT;
      m_data->m_shadowPortDescriptor.desc.emptyFlagBaseAddr = soffset;
      m_data->m_shadowPortDescriptor.desc.emptyFlagSize = sizeof(BufferState);
      m_data->m_shadowPortDescriptor.desc.emptyFlagPitch = sizeof(BufferState);

      if ( getCircuit()->m_transport->m_transportGlobal->useEvents() ) {
        int lr,hr;
        getCircuit()->m_transport->m_transportGlobal->getEventManager()->getEventRange(lr,hr);
        m_data->m_shadowPortDescriptor.desc.emptyFlagValue = 
          ((OCPI::OS::uint64_t)(lr)<<32) 
          | (OCPI::OS::uint64_t)1<<63;
      }
      else {
        m_data->m_shadowPortDescriptor.desc.emptyFlagValue = 
          ((OCPI::OS::uint64_t)(0xfff)<<32)
          | (OCPI::OS::uint64_t)1<<63;
      }
                
      //    printf("EmptyFlag value = 0x%llx\n", m_shadowPortDescriptor.desc.emptyFlagValue);
                
      strcpy( m_data->m_shadowPortDescriptor.desc.oob.oep,m_data->m_shadow_location->end_point.c_str());
      m_data->m_shadowPortDescriptor.desc.oob.port_id = m_data->m_externPortDependencyData.desc.oob.port_id;
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.myShadowsRemoteStateOffsets[m_data->m_shadow_location->mailbox] = 
          soffset + index * sizeof(BufferState);
      }

    }

  }

// This needed to change because OCPI requires the buffers to be located at a fixed pitch.
void 
Port::
allocateBufferResources()
{
  if ( m_data->m_portSetMd->output ) {
    createOutputOffsets();
  }
  else {
    createInputOffsets();
  }
}

bool 
OCPI::DataTransport::Port::
hasFullInputBuffer()
{

  if ( getCircuit()->isCircuitOpen() ) {
    return false;
  }
  TransferController* txc = getPortSet()->getTxController();
  if ( txc == NULL ) {
    return false;
  }

  InputBuffer* tb;
  bool available = 
    getPortSet()->getTxController()->hasFullInputBuffer(this, &tb);

  // We may need to do more work here if we are attached to a output buffer for ZCopy
  if ( ! available ) {
    if ( tb->m_zCopyPort && tb->m_attachedZBuffer ) {
      if ( tb->m_attachedZBuffer->isEmpty() && ! tb->m_attachedZBuffer->inUse() ) {
        OCPI::DataTransport::PortSet* aps = static_cast<OCPI::DataTransport::PortSet*>(tb->m_zCopyPort->getPortSet());
        aps->getTxController()->modifyOutputOffsets( tb->m_attachedZBuffer, tb, true );
        inputAvailable( tb );
        tb->m_zCopyPort = NULL;
        tb->m_attachedZBuffer->m_attachedZBuffer = NULL;
        tb->m_attachedZBuffer = NULL;
        available = true;
      }
    }
  }
  return available;
}


bool 
OCPI::DataTransport::Port::
hasEmptyOutputBuffer()
{
  if ( getCircuit()->isCircuitOpen() ) {
    return false;
  }
  if ( !getPortSet() || !getPortSet()->getTxController() ) {
    return false;
  }
  return getPortSet()->getTxController()->hasEmptyOutputBuffer(this);
}


OCPI::DataTransport::Buffer* 
OCPI::DataTransport::Port::
getNextFullInputBuffer()
{
  if ( getCircuit()->isCircuitOpen() ) {
    return NULL;
  }
  InputBuffer* buf = 
    static_cast<InputBuffer*>(getPortSet()->getTxController()->getNextFullInputBuffer(this));
  if ( buf && buf->isEOS() ) {
    setEOS();
  }
  if ( buf && buf->getMetaData()->endOfCircuit ) {
    getCircuit()->m_status = Circuit::Disconnecting;
  }
  return buf;
}



/**********************************
 * This method causes the specified input buffer to be marked
 * as available.
 *********************************/
OCPI::OS::int32_t 
OCPI::DataTransport::Port::
inputAvailable( Buffer* input_buf )
{
  int rtn=1;

  Buffer* tbuf = getPortSet()->getTxController()->consume( input_buf );

  // If this returns a input buffer, it means that this was a zcopy
  // chain that terminates with a input buffer that must also be consumed
  if ( tbuf && !tbuf->getPort()->getPortSet()->isOutput()  ) {
    inputAvailable( tbuf );
  }

  input_buf->setInUse( false );
  return rtn;
}




/**********************************
 * This method retreives the next available buffer from the local (our)
 * port set.  A NULL port indicates local context.
 *********************************/
OCPI::DataTransport::Buffer* 
OCPI::DataTransport::Port::
getNextEmptyOutputBuffer()
{
  if ( getCircuit()->isCircuitOpen() ) {
    return NULL;
  }
  OutputBuffer* buffer = static_cast<OutputBuffer*>
    (getPortSet()->getTxController()->getNextEmptyOutputBuffer(this));
  if ( isShadow() ) {
    return buffer;
  }

  // At this point we need to add some of the meta data
  if ( buffer ) {
    buffer->getMetaData()->sequence = getBufferSequence()++;
    buffer->getMetaData()->metaDataOnlyTransfer =0;
    buffer->getMetaData()->broadCast = 0;

#ifdef DEBUG_L2
    printf("Setting rank = %d\n", getRank());
#endif
    buffer->getMetaData()->srcRank = getRank();

#ifdef DEBUG_L2
    printf("Wrote sequence #%d to meta data\n", buffer->getMetaData()->sequence);
#endif
    buffer->getMetaData()->srcTemporalId = buffer->getTid();
    buffer->setMetaData();
  }

  // Set the EOS on the port if needed
  if ( getOutputControlBlock() && getOutputControlBlock()->endOfStream == 1 ) {
#ifdef DEBUG_L2
    printf("Circuit::getNextAvailableOutputBuffer, setting EOS flag !!\n");
#endif
    setEOS();
  }

  // Determine if this buffer has an associate ZCopy input buffer
  if ( buffer && buffer->m_attachedZBuffer ) {
    // Modify the output port transfer to point to the input buffer output
    OCPI::DataTransport::PortSet* aps = static_cast<OCPI::DataTransport::PortSet*>(getPortSet());
    aps->getTxController()->modifyOutputOffsets( buffer, buffer->m_attachedZBuffer, true );
    inputAvailable( buffer->m_attachedZBuffer );
    buffer->m_attachedZBuffer->m_zCopyPort = NULL;
    buffer->m_attachedZBuffer->m_attachedZBuffer = NULL;
    buffer->m_attachedZBuffer = NULL;
  }

  return static_cast<OCPI::DataTransport::Buffer*>(buffer);
}


void 
Port::
sendZcopyInputBuffer( Buffer* src_buf, unsigned int len )
{
  src_buf->getMetaData()->ocpiMetaDataWord.length = len;
  getCircuit()->sendZcopyInputBuffer( this, src_buf, len );
}


void 
Port::
advance( Buffer* buffer, unsigned int len )
{
  if ( isOutput() ) {
    sendOutputBuffer( buffer, len );
  }
  else {
    inputAvailable( buffer);
  }
}

void 
Port::
sendOutputBuffer( Buffer* b, unsigned int length )
{
  // Put the actual data length in the meta-data
  b->getMetaData()->ocpiMetaDataWord.length = length;

  // If there were no available output buffers when the worker was last run on this port, then the
  // buffer can be NULL.  The user should not be advancing all in this case, but we need to protect against it.
  Circuit * c = getCircuit();
  if ( c->canTransferBuffer( b ) ) {
    c->startBufferTransfer( b );
  }
  else {
    c->queTransfer( b );
  }

}
