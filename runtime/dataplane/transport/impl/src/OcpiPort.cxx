/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Abstract:
 *   This file contains the Implementation for the OCPI port.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */
#include <inttypes.h>
#include <ctype.h>
#include "OcpiOsAssert.h"
#include "OcpiUtilPort.h"
#include "DtHandshakeControl.h"
#include "XferManager.h"
#include "XferEndPoint.h"
#include "OcpiOutputBuffer.h"
#include "OcpiInputBuffer.h"
#include "OcpiPortSet.h"
#include "OcpiTransport.h"
#include "OcpiCircuit.h"
#include "OcpiRDTInterface.h"
#include "OcpiTransferController.h"
#include "OcpiPort.h"


//using namespace OCPI::DataTransport;
//using namespace DtI;
namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace DDT = DtOsDataTypes;
namespace XF = DataTransfer;
// Buffer allignment
#define BUF_ALIGNMENT OU::BUFFER_ALIGNMENT
namespace OCPI {
namespace DataTransport {
void Port::reset()
{
  ocpiDebug("**** In OCPI::DataTransport::Port::reset() %p", this);
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
    // m_initialized = false; this is wrong since it means allocations are not freed
  }
  m_sequence = 0;
  m_lastBufferOrd=MAXBUFORD;
  m_nextBridgeOrd=0;

  getCircuit()->release();
  // FIXME:  release actually may call the destructor on this object
  // ok since this function just returns, but still questionable
}

/**********************************
 * Internal port initialization
 *********************************/
void Port::initialize()
{
  if ( m_initialized ) {
    return;
  }

  m_sequence = 0;
  m_lastBufferOrd=MAXBUFORD;
  m_nextBridgeOrd=0;

  if (!m_data->m_real_location) {
    assert(m_data->real_location_string.size());
    m_data->m_real_location = &XF::getManager().getEndPoint(m_data->real_location_string);
  }
  if (!m_data->m_shadow_location)
    m_data->m_shadow_location =
      m_data->shadow_location_string.size() ?
      &XF::getManager().getEndPoint(m_data->shadow_location_string) :
      m_data->m_real_location;

  // Determine if we are a shadow port
  // Get our transport class
  Circuit *c = getCircuit();
  Transport* t = c->m_transport;

  if (!t->isLocalEndpoint(*m_data->m_real_location)) {

#ifdef DEBUG_L2
    ocpiDebug( "We are a shadow port" );
#endif
    m_shadow = true;
    //    m_localSMemResources = m_shadowSMemResources;
  }
  else {

#ifdef DEBUG_L2
    ocpiDebug("We are a real port" );
#endif
    m_shadow = false;
    //    m_localSMemResources = m_realSMemResources;
  }

  // At this point we are going to map our offset stucture into
  // SMB memory so that we can make direct requests to our remote
  // dependencies to fill in the missing data.
        
  if (getLocalEndPoint().resourceMgr().
      alloc(sizeof(PortMetaData::BufferOffsets)*m_bufferCount, 0, &m_offsetsOffset ))
    throw OCPI::Util::EmbeddedException (XF::NO_MORE_SMB, getLocalEndPoint().name().c_str());
  ocpiDebug("**** Alloc Port %p for initialize local offsets shadow %u circuit %p: %u",
	    this, m_shadow, c, m_offsetsOffset);

  // Now get our mailbox
  m_mailbox = getLocalEndPoint().mailBox();

  // map our meta-data
  m_portDependencyData.offsets = static_cast<PortMetaData::BufferOffsets*>
    (getLocalEndPoint().sMemServices().mapRx 
     (m_offsetsOffset, 
      sizeof(PortMetaData::BufferOffsets)*m_bufferCount));

  memcpy(m_portDependencyData.offsets,m_data->m_bufferData,
         sizeof(PortMetaData::BufferOffsets)*m_bufferCount);

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
Port::Port(PortMetaData* data, PortSet* ps)
  : CU::Child<PortSet,Port>(*ps, *this), OCPI::Time::Emit( ps, "Port", "",NULL ), 
    m_initialized(false), // reset to false in reset(), set to true in initialize()
    m_data(data),
    //    m_realSMemResources(NULL),
    //    m_shadowSMemResources(NULL),
    //    m_localSMemResources(NULL),
    m_hsPortControl(NULL),
    m_lastBufferTidProcessed(0),
    m_sequence(0),   // will be reset in reset()
    m_shadow(false), // will be set properly in initialize()
    m_busyFactor(0),
    m_eos(false),
    m_mailbox(0),    // will be set property in initialize()
    // portDependencyData
    m_offsetsOffset(),
    m_lastBufferOrd(MAXBUFORD),  // will be reset in reset()
    m_nextBridgeOrd(0),          // will be reset in reset() 
    m_externalState(NotExternal),
    m_pdDriver(NULL),
    m_portSet(ps),
    m_bufferCount(ps->getBufferCount()),
    m_buffers(new Buffer*[m_bufferCount]),
    m_zCopyBufferQ(0)
{
  ocpiCheck(m_bufferCount <= MAX_BUFFERS);
  ocpiDebug("**** In OCPI::DataTransport::Port::Port(): %p", this);
  memset(m_buffers, 0, sizeof(Buffer*) * m_bufferCount);
  m_data->rank = ps->getPortCount();
  if (m_data->real_location_string.length())
    initialize();
}

XF::XferServices &Port::
getTemplate(XF::EndPoint &source, XF::EndPoint &target) {
  XF::TemplatePair pair(&source, &target);
  OU::SelfAutoMutex guard(getCircuit());
  XF::TemplateMapIter ti = m_templates.find(pair);
  if (ti == m_templates.end()) {
    XF::XferServices &s = source.factory().getTemplate(source, target);
    // Let the above assignment cause an exception...
    return *(m_templates[pair] = &s);
  }
  return *ti->second;
}

/**********************************
 * Get the shared memory object
 *********************************/
XF::SmemServices* Port::getRealShemServices()
{
#if 1
  XF::EndPoint *ep = checkEndPoint();
  return ep ? &ep->sMemServices() : NULL;
#else
  if ( m_realSMemResources )
    return m_realSMemResources->sMemServices;
  else
    return NULL;
#endif
}

/**********************************
 * Finalize the port - called when the roles are finally established and won't change any more
 * The return value is the resulting descriptor which is either "mine" or "flow"
 * (flow is a buffer provided by the caller).
 * If "other" is NULL, it means there is no "other", and we are passive
 * If the return value is ! NULL, it means send it to the other side.
 * If the done is set to true, it means we can go operational and need no more info.
 * E.g. an input port might need to send some info back, but it is immediately done,
 * and needs no more handshaking
 *********************************/
const OCPI::RDT::Descriptors * Port::
finalize(const OCPI::RDT::Descriptors *other, OCPI::RDT::Descriptors &mine,
	 OCPI::RDT::Descriptors *flow, bool &done)
{
  const OCPI::RDT::Descriptors *result = &mine;
  Circuit &c = *getCircuit();
  Port *otherPort;
  done = true;
  if (m_data->output) {
    ocpiAssert(mine.type == OCPI::RDT::ProducerDescT);
    switch (mine.role) {
    case OCPI::RDT::ActiveFlowControl:
      // We are output w/ActiveFlowControl role, we do not need to tell the input about
      // our local input-buffer-empty flags since they do not need to indicate when input
      // buffers are consumed. We don't care because we are not managing the transfers from our
      // output to their inputs. Instead, we will send them the output descriptor so that they
      // can "pull" data from us, and indicate to us when the data transfer has completed.
      // We need to hear back from them about the 
      ocpiAssert(flow);
      *flow = mine; // we are taking a snapshot of "mine" before modifications below
      // result = flow; why not this?
      // done = false;
      break;
    case OCPI::RDT::ActiveOnly:
      ocpiDebug("Found a Passive Consumer Port !!");
      assert(other);
      attachPullDriver(c.createPullDriver(*other));
      break;
    case OCPI::RDT::ActiveMessage:
      assert(flow);
      PortMetaData *inputMeta = c.getInputPortSet(0)->getPort(0)->getMetaData();
      if (inputMeta->m_shadow) {
	*flow = c.getInputPortSet(0)->getPort(0)->getMetaData()->m_shadowPortDescriptor;
	flow->type = OCPI::RDT::ConsumerFlowControlDescT;
	result = flow;
      }
      break;
    }
    if (flow)
      flow->desc.oob.cookie = mine.desc.oob.cookie;
    otherPort = c.getInputPortSet(0)->getPort(0);
  } else {
    // We are input, other is output.
    if (other) {
      XF::EndPoint &otherEp = getCircuit()->m_transport->addRemoteEndPoint(other->desc.oob.oep);
      c.setFlowControlDescriptor(this, *other);
      ocpiAssert(getRealShemServices());
      getTemplate(getEndPoint(), otherEp).finalize( other->desc.oob.cookie );
      result = NULL;  // There is nothing to tell the other side at this point.
    }
    otherPort = c.getOutputPortSet()->getPort(0);
  }
  getPortDescriptor(mine, other);
  ocpiAssert(m_data->m_descriptor.role != 5);
  if (other)
    otherPort->m_data->m_descriptor = *other;
  ocpiAssert(otherPort->m_data->m_descriptor.role != 5);
  ocpiDebug("Circuit %s %p is closed", m_data->output ? "output" : "in", &c);
  c.m_openCircuit = false;
  c.ready();
  return result;
}
bool Port::
isFinalized() {
  Circuit* c = getCircuit();
  return c && !c->isCircuitOpen();
}


void 
Port::
setFlowControlDescriptorInternal( const OCPI::RDT::Descriptors & desc )
{

  initialize();

  // This descriptor contains the information needed by this port to tell a producer
  // when a buffer is empty.
  getMetaData()->m_shadowPortDescriptor = desc;
  getMetaData()->m_externPortDependencyData = desc;


  // For each buffer, set the descriptor
  PortSet* s_ps = static_cast<PortSet*>(getCircuit()->getOutputPortSet());
  Port* output_port = 
    static_cast<Port*>(s_ps->getPortFromIndex(0));
  int idx = output_port->getEndPoint().mailBox();
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
void Port::
getPortDescriptor(OCPI::RDT::Descriptors& desc, const OCPI::RDT::Descriptors *other) {

  // If we are not a shadow port, we fill in all of the real descriptor dependency information
  // that is needed by a producer port to output our data.  If we are a shadow port (we only learn about
  // this after we as a output port are connected to an external consumer port) then we only have "remote 
  // buffer state" information.  This is descriptor information that is used on the consumer side to tell us
  // we a remote input buffer becomes available.
  ocpiAssert(! this->isShadow());

  if (other) {
    // Get the connection cookie
    XF::EndPoint &otherEp = getCircuit()->m_transport->addRemoteEndPoint(other->desc.oob.oep);
    desc.desc.oob.cookie = getTemplate(getEndPoint(), otherEp).getConnectionCookie();
  }


  desc.desc.metaDataPitch = OCPI_UTRUNCATE(uint32_t, sizeof(BufferMetaData) * MAX_PCONTRIBS);
  desc.desc.dataBufferSize = desc.desc.dataBufferPitch = this->getPortSet()->getBufferLength();
  ocpiDebug("getPortDescriptor %p: setting %s buffer size to %zu",
	    this, isOutput() ? "output" : "input", (size_t)desc.desc.dataBufferSize);
  desc.desc.fullFlagSize = sizeof(BufferState);
  desc.desc.fullFlagPitch = OCPI_UTRUNCATE(uint32_t, sizeof(BufferState) * MAX_PCONTRIBS * 2);
  desc.desc.emptyFlagSize = sizeof(BufferState);
  desc.desc.emptyFlagPitch = OCPI_UTRUNCATE(uint32_t, sizeof(BufferState) * MAX_PCONTRIBS * 2);

  desc.desc.oob.port_id = getPortId();
  strcpy(desc.desc.oob.oep, getEndPoint().name().c_str());
  Transport::fillDescriptorFromEndPoint(getEndPoint(), desc);
  if ( ! isOutput() ) { 
    ocpiAssert(desc.type == OCPI::RDT::ConsumerDescT);
    if (desc.role == OCPI::RDT::NoRole)
      desc.role = OCPI::RDT::ActiveFlowControl;
    ocpiAssert(desc.desc.nBuffers == getPortSet()->getBufferCount());

    desc.desc.dataBufferBaseAddr = m_portDependencyData.offsets[0].inputOffsets.bufferOffset;
    desc.desc.metaDataBaseAddr = m_portDependencyData.offsets[0].inputOffsets.metaDataOffset;
    desc.desc.fullFlagBaseAddr = m_portDependencyData.offsets[0].inputOffsets.localStateOffset;

  } else {  // We are an output port
    ocpiAssert(desc.type == OCPI::RDT::ProducerDescT);
    if (desc.role == OCPI::RDT::NoRole)
      desc.role = OCPI::RDT::ActiveMessage;
    if (desc.desc.nBuffers != getPortSet()->getBufferCount())
      ocpiDebug("Buffer count mismatch.  Should be %u but will be forced to %u",
		desc.desc.nBuffers, getPortSet()->getBufferCount());

    desc.desc.dataBufferBaseAddr = m_data->m_bufferData[0].outputOffsets.bufferOffset;
    desc.desc.metaDataBaseAddr = m_data->m_bufferData[0].outputOffsets.metaDataOffset;
    desc.desc.fullFlagBaseAddr = m_data->m_bufferData[0].outputOffsets.localStateOffset;
    desc.desc.emptyFlagBaseAddr = m_data->m_bufferData[0].outputOffsets.localStateOffset;
  }

#if 1
  desc.desc.fullFlagValue = 1;
#else
  if ( getCircuit()->m_transport->m_transportGlobal->useEvents() ) {

    ocpiDebug("We are using EVENTS\n");
                
    int lr,hr;
    getCircuit()->m_transport->m_transportGlobal->getEventManager()->getEventRange(lr,hr);
    desc.desc.fullFlagValue = 1 | 
      ((OCPI::OS::uint64_t)(lr+1)<<32) | (OCPI::OS::uint64_t)1<<63;
    ocpiDebug("OcpiPort: low range = %d, high range = %d, flag = 0x%llx\n", lr, hr, (long long)desc.desc.fullFlagValue);

  }
  else {
    ocpiDebug("We are NOT using events");

    desc.desc.fullFlagValue = 1 | 
      ((OCPI::OS::uint64_t)(0xfff)<<32) | (OCPI::OS::uint64_t)1<<63;
  }
#endif
  ocpiDebug("Full flag value = 0x%" DTOSDATATYPES_FLAG_PRIx"\n", desc.desc.fullFlagValue);


  // We are being externally connected, so we need to wait for an update before
  // attempting to get our shadow information
  m_externalState = WaitingForUpdate;

  getMetaData()->m_descriptor = desc;
}


/**********************************
 * Get the shared memory object
 *********************************/
XF::SmemServices* Port::getShadowShemServices()
{
  
  return &getShadowEndPoint().sMemServices();
}


/**********************************
 * Get the shared memory object
 *********************************/
XF::SmemServices* Port::getLocalShemServices()
{
  return &getLocalEndPoint().sMemServices();
}


/**********************************
 * Updates the port with addition meta-data information
 *********************************/
void Port::update()
{
  this->initialize();
}


#if 0
/**********************************
 * Advance the ports circular buffer
 *********************************/
void Port::advance( OCPI::OS::uint64_t value )
{
  ( void ) value;
  if ( isOutput() ) {

    ocpiDebug("ERROR: Attemping to advance a source buffer \n");
                
    if ( isShadow() ) {
      // Nothing to do
      ocpiBad("Programming ERROR!! advancing a output shadow buffer");
    }
    else {
      // All output transfer objects are required to include a done flag,
      // Therfore there is nothing to do here either.  The event was generated
      // to ensure that the process woke up to process the output buffer becoming
      // empty
    }
  }
  else {
    Circuit *c = getCircuit();
    OU::SelfAutoMutex guard(c); // FIXME: refactor to make this a circuit method
    if ( isShadow() ) {
        
      ocpiDebug("Advancing the shadow buffer");
                        
      // If we are a input shadow buffer, it means that a remote input buffer
      // became empty.
      getPortSet()->getTxController()->freeBuffer(this);
    }
    else {

      ocpiDebug("Advancing the REAL buffer");
                                                
      // One of our inputs just got filled
      getPortSet()->getTxController()->bufferFull(this);
                        
    }
  }
        
}
#endif

/**********************************
 * Can these two ports support Zero Copy transfers
 *********************************/
bool Port::supportsZeroCopy( Port* port )
{

  if ( getCircuit()->m_transport->isLocalEndpoint(*port->m_data->m_real_location ) &&
       getCircuit()->m_transport->isLocalEndpoint(*m_data->m_real_location)
       ) {
    ocpiDebug(" NOTE: %s and %s are both local", m_data->m_real_location->name().c_str(),
	      port->m_data->m_real_location->name().c_str() );
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
Port::
createBuffers()
{
        
  ocpiDebug("Number of buffers = %zd", this->getBufferCount() );

  ocpiAssert(m_bufferCount == this->getBufferCount());

  ocpiDebug("Port::CreateBuffers1: port %p bmd %p offset 0x%" OCPI_UTIL_RESADDR_PRIx,
	    this, m_data->m_bufferData, m_data->m_bufferData[0].outputOffsets.bufferOffset);

  // Do the resource allocation first to make sure we can habdle the request.
  allocateBufferResources();

  ocpiDebug("Port::CreateBuffers2: port %p bmd %p offset 0x%" OCPI_UTIL_RESADDR_PRIx,
	    this, m_data->m_bufferData, m_data->m_bufferData[0].outputOffsets.bufferOffset);

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

size_t
Port::
getBufferCount()
{
  return m_portSet->getBufferCount();
}

/**********************************
 * Destructor
 *********************************/
Port::
~Port()
{
  ocpiDebug("**** In OCPI::DataTransport::Port::~Port %p init %u output %u md %p mdshadow %u shadow %u",
	    this, m_initialized, isOutput(), m_data, m_data ? m_data->m_shadow : 0, m_shadow);
  int rc;
  int index=0;
  XF::ResourceServices* res_mgr;

  if ( m_initialized ) {

    if ( isOutput() ) {

      if ( ! m_data->m_shadow  ) {

	res_mgr = &getEndPoint().resourceMgr();
	//	  XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
	ocpiAssert( res_mgr );

	rc = res_mgr->free( m_data->m_bufferData[index].outputOffsets.bufferOffset,
			    m_data->m_bufferData[index].outputOffsets.bufferSize);
	ocpiAssert( rc == 0 );

	rc = res_mgr->free( m_data->m_bufferData[index].outputOffsets.localStateOffset,
			    sizeof(BufferState)*MAX_PCONTRIBS); // FIXME shouldn't this be *2?
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

	res_mgr = &getEndPoint().resourceMgr();
	//	  XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
	ocpiAssert( res_mgr );

	rc = res_mgr->free( m_data->m_bufferData[index].inputOffsets.bufferOffset,
			    m_data->m_bufferData[index].inputOffsets.bufferSize);
	ocpiAssert( rc == 0 );

	rc = res_mgr->free( m_data->m_bufferData[index].inputOffsets.metaDataOffset,
			    sizeof(BufferMetaData)*MAX_PCONTRIBS);
	ocpiAssert( rc == 0 );

	rc = res_mgr->free( m_data->m_bufferData[index].inputOffsets.localStateOffset,
			    sizeof(BufferState)*MAX_PCONTRIBS); // FIXME shouldn't this be *2?
	ocpiAssert( rc == 0 );

      }
      else {  // We are a shadow port

	res_mgr = &getShadowEndPoint().resourceMgr();
	//	res_mgr = XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_shadow_location )->sMemResourceMgr;
	ocpiAssert( res_mgr );

	rc = res_mgr->free(OCPI_UTRUNCATE(OU::ResAddr,
					  m_data->m_bufferData[index].inputOffsets.
					  myShadowsRemoteStateOffsets[m_data->m_shadow_location->mailBox()]),
			   sizeof(BufferState));
	ocpiAssert( rc == 0 );
      }

    }
    getLocalEndPoint().resourceMgr().free(m_offsetsOffset, sizeof(PortMetaData::BufferOffsets)*m_bufferCount);
  }
  if (m_buffers)
    for (unsigned i=0; i<m_bufferCount; i++ )
      delete m_buffers[i];
  delete [] m_buffers;
  // Release our references to the transfer templates.
  for (DataTransfer::TemplateMapIter tmi = m_templates.begin(); tmi != m_templates.end(); tmi++)
    tmi->second->release();
}

/**********************************
 * Get our associated circuit
 *********************************/
Circuit* Port::getCircuit()
{
  return m_portSet->getCircuit();
}



/**********************************
 * Determines of a port is ready to go
 *********************************/
bool Port::ready()
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
      for (PortOrdinal n = 0; n < getCircuit()->getOutputPortSet()->getPortCount(); n++) {

        PortSet* s_ps = static_cast<PortSet*>(getCircuit()->getOutputPortSet());
        Port* output_port = static_cast<Port*>(s_ps->getPortFromIndex(n));
        int idx = output_port->getEndPoint().mailBox();
        if ( ! m_portDependencyData.offsets[last_idx].inputOffsets.myShadowsRemoteStateOffsets[idx] ) {
                                        
          // Make sure this output port is not co-located

          if ( getCircuit()->m_transport->isLocalEndpoint(*output_port->getMetaData()->m_real_location)) {

            for (unsigned nn=0; nn<getPortSet()->getBufferCount(); nn++)  {
              m_portDependencyData.offsets[nn].inputOffsets.myShadowsRemoteStateOffsets[getMailbox()] =
                m_portDependencyData.offsets[nn].inputOffsets.localStateOffset;
            }
            continue;
          }
                                        
          // Make the request to get our offset
	  XF::EndPoint &sep = getEndPoint();
          XferMailBox xmb( getMailbox() );

          if ( ! xmb.mailBoxAvailable(sep) ) {
            return false;
          }
          ocpiDebug("Real Input buffer is making a request to get shadow offsets. cid %x",
		    getCircuit()->getCircuitId());
                                        
          ContainerComms::MailBox* mb = xmb.getMailBox(sep);
          mb->request.header.type = ContainerComms::ReqShadowRstateOffset;
                  

          if ( this->m_data->remoteCircuitId != 0 ) {
            mb->request.header.circuitId = this->m_data->remoteCircuitId;
            mb->request.reqShadowOffsets.portId = this->m_data->remotePortId;
          }
          else {
            mb->request.header.circuitId = getCircuit()->getCircuitId();
            mb->request.reqShadowOffsets.portId = getPortId();
          }

          ocpiDebug("Making return address to %s", 
		    getLocalEndPoint().name().c_str() );

          strncpy( mb->request.reqShadowOffsets.url, 
                   getLocalEndPoint().name().c_str(), 128 );
          mb->return_offset = m_offsetsOffset;
          mb->return_size = sizeof( PortMetaData::BufferOffsets );
          mb->returnMailboxId = getMailbox();
                                        
          xmb.makeRequest(sep, output_port->getEndPoint());

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
	// The source of the request is us, the shadow
	XF::EndPoint &sep = getShadowEndPoint();
        XferMailBox xmb( getMailbox() );
        if ( ! xmb.mailBoxAvailable(sep) ) {
          return false;
        }
                                
        ocpiDebug("Input Shadow buffer is making a request to get buffer offsets, my offset = 0x%llx id %x",
		  (long long unsigned)m_offsetsOffset, getCircuit()->getCircuitId());

        ContainerComms::MailBox* mb = xmb.getMailBox(sep);
        mb->request.header.type = ContainerComms::ReqInputOffsets;

        if ( this->m_data->remoteCircuitId != 0 ) {
          mb->request.header.circuitId = this->m_data->remoteCircuitId;
          mb->request.reqInputOffsets.portId    = this->m_data->remotePortId;
        }
        else {
          mb->request.header.circuitId = getCircuit()->getCircuitId();
          mb->request.reqInputOffsets.portId    = getPortId();
        }

        ocpiDebug("Making return address to %s", 
		  getLocalEndPoint().name().c_str() );

        strncpy( mb->request.reqInputOffsets.url, 
		 getLocalEndPoint().name().c_str(), 128 );
        mb->return_offset = m_offsetsOffset;
        mb->return_size = sizeof( PortMetaData::BufferOffsets );
        mb->returnMailboxId = getMailbox();
        xmb.makeRequest(sep, getEndPoint());
        rtn = false;
      }


      // Now make sure that all of our shadow offsets have been allocated
      for (PortOrdinal n = 0; n < getCircuit()->getOutputPortSet()->getPortCount(); n++) {
                                
        PortSet* s_ps = static_cast<PortSet*>(getCircuit()->getOutputPortSet());
        Port* shadow_port = static_cast<Port*>(s_ps->getPortFromIndex(n));
        int idx = shadow_port->getMailbox();

        if ( ! m_portDependencyData.offsets[last_idx].inputOffsets.myShadowsRemoteStateOffsets[idx] ) {
                                        
          // Make sure this output port is not co-located

          if (getCircuit()->m_transport->isLocalEndpoint(*shadow_port->getMetaData()->m_real_location))
            continue;

	  XF::EndPoint &sep = getEndPoint();
          XferMailBox xmb( getMailbox() );
          if (!xmb.mailBoxAvailable(sep))
            return false;

          ocpiDebug("Shadow Input buffer is making a request to get other shadow offsets. id %x",
		    getCircuit()->getCircuitId());

          ContainerComms::MailBox* mb = xmb.getMailBox(sep);
          mb->request.header.type = ContainerComms::ReqShadowRstateOffset;
                                

          if ( this->m_data->remoteCircuitId != 0) {
            mb->request.header.circuitId = this->m_data->remoteCircuitId;
            mb->request.reqShadowOffsets.portId    = this->m_data->remotePortId;
          }
          else {
            mb->request.header.circuitId = getCircuit()->getCircuitId();
            mb->request.reqShadowOffsets.portId    = getPortId();
          }

          ocpiDebug("Making return address to %s", 
		    getLocalEndPoint().name().c_str() );

          strncpy( mb->request.reqShadowOffsets.url, 
                   getLocalEndPoint().name().c_str(), 128 );
          mb->return_offset = m_offsetsOffset;
          mb->return_size = sizeof( PortMetaData::BufferOffsets );
          mb->returnMailboxId = getMailbox();
                                        
          // Now make the request
          xmb.makeRequest(sep, shadow_port->getEndPoint());
                                        
          rtn = false;
                                        
        }

      }

    } 
    else {  // Shadow output port
      XF::EndPoint &sep = getShadowEndPoint();
      //        SMBResources* s_res = 
      //          XferFactoryManager::getFactoryManager().getSMBResources( &getShadowEndPoint() );

      if ( m_portDependencyData.offsets[0].outputOffsets.portSetControlOffset ) {
	// We have received the output offsets.  Perform any required protocol info
	// processing if there is any.  This means stashing the received protocol info
	// into the circuit object.
	size_t protocolSize;
	OCPI::Util::ResAddrType protocolOffset;
	getCircuit()->getProtocolInfo(protocolSize, protocolOffset);
	if (protocolSize) {
	  ocpiDebug("Receiving protocol info offset 0x%llx size %lu",
		    (unsigned long long)protocolOffset, (unsigned long)protocolSize);
	  // protocolSize from the circuit is set by the incoming request from the client,
	  // and cleared when the information is stashed into the circuit.
	  void *myProtocolBuffer = sep.sMemServices().mapRx(protocolOffset, protocolSize);
	  // This string constructor essentially copies the info into the string
	  char *copy = new char[protocolSize];
	  memcpy(copy, myProtocolBuffer, protocolSize);
	  ocpiDebug("Received protocol info: \"%s\"", isprint(*copy) ? copy : "unprintable");
	  getCircuit()->setProtocol(copy); // clears procotol size
	  sep.sMemServices().unMap();	  
	}
      } else {
	//        SMBResources* t_res = XferFactoryManager::getFactoryManager().getSMBResources( getEndPoint() );
        XferMailBox xmb( getMailbox() );
        if (!xmb.mailBoxAvailable(sep))
          return false;

	ocpiDebug("Output shadow port is making a request to get port control offsets. id %x",
		  getCircuit()->getCircuitId());

	ContainerComms::MailBox* mb = xmb.getMailBox(sep);
        mb->request.header.type = ContainerComms::ReqOutputControlOffset;

            
        if ( this->m_data->remoteCircuitId != 0 ) {
          mb->request.header.circuitId = this->m_data->remoteCircuitId;
          mb->request.reqOutputContOffset.portId    = this->m_data->remotePortId;
        }
        else {
          mb->request.header.circuitId = getCircuit()->getCircuitId();
          mb->request.reqOutputContOffset.portId    = getPortId();
        }

        // Need to tell it how to get back with us
        strcpy(mb->request.reqOutputContOffset.shadow_end_point,
               getLocalEndPoint().name().c_str() );

	size_t protocolSize;
	OU::ResAddr poffset;
	getCircuit()->getProtocolInfo(protocolSize, poffset);
	// convert local offset into a remote one for the other guy
	mb->request.reqOutputContOffset.protocol_offset = poffset;

        ocpiDebug("Making return address to %s", 
		  getLocalEndPoint().name().c_str());
        ocpiDebug("Setting port id = %lld", (long long)mb->request.reqShadowOffsets.portId );

        mb->return_offset = m_offsetsOffset;
        mb->return_size = sizeof( PortMetaData::BufferOffsets );
        mb->returnMailboxId = getMailbox();
        xmb.makeRequest(sep, getEndPoint());
        return false;
      }
    }
  }

  return rtn;
}






/**********************************
 * writes buffer offsets to address
 *********************************/
void Port::writeOffsets( PortMetaData::BufferOffsets* offset )
{
  // for all buffers
  if ( ! isOutput() ) { 
    for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {
                        
      if ( m_shadow ) {

        int idx = getLocalEndPoint().mailBox();

        offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx ] =
          m_data->m_bufferData[n].inputOffsets.myShadowsRemoteStateOffsets[idx];
                                
        ocpiDebug("Wrote shadow offset %lld to address %p", 
               (long long)m_data->m_bufferData[n].inputOffsets.myShadowsRemoteStateOffsets[idx],
               &offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx] );
                                
      }
      else {

        ocpiDebug("Wrote real input offsets");
                                
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

    ocpiDebug("Wrote output control offset 0x%llx to address %p\n", 
           (long long)m_data->m_bufferData[0].outputOffsets.portSetControlOffset,
           &offset[0].outputOffsets.portSetControlOffset );
                                
  }

}


void Port::releaseOffsets( OCPI::Util::VList& offsets )
{
  for ( OCPI::OS::uint32_t n=0; n<offsets.size(); n++ ) {
    ToFrom* tf = static_cast<ToFrom*>(offsets[n]);
    delete tf;
  }
  offsets.destroyList();
}

static void
addOffset(OU::VList& offsets, DDT::Offset from_base, DDT::Offset to_base,
	  size_t adjust, const char *debug) {
  Port::ToFrom* tf = new Port::ToFrom;
  DDT::Offset adj = OCPI_UTRUNCATE(DDT::Offset, adjust);
  tf->from_offset = from_base + adj;
  tf->to_offset = to_base + adj;
  offsets.push_back(tf);
  ocpiDebug("Wrote %s offsets 0x%" DTOSDATATYPES_OFFSET_PRIx
	    " to address 0x%" DTOSDATATYPES_OFFSET_PRIx, 
	    debug, tf->from_offset, tf->to_offset );
}

/**********************************
 * get buffer offsets to dependent data
 *********************************/
// g++ doesn't allow a computed address
#define myoffsetof(t,m) ((size_t)(&((t*)0)->m))

void Port::
getOffsets( DDT::Offset to_base_offset, OU::VList& offsets )
{
  DDT::Offset
    from_offset = m_offsetsOffset,
    to_offset = to_base_offset,
    increment = OCPI_UTRUNCATE(DDT::Offset, sizeof(PortMetaData::BufferOffsets));
  
  for (unsigned n = 0; n < getPortSet()->getBufferCount();
       n++, from_offset += increment, to_offset += increment) {
    size_t adjust;    
    const char *type;
    if (isOutput()) {
      adjust = myoffsetof(PortMetaData::BufferOffsets, outputOffsets.portSetControlOffset);
      type = "output";
    } else if (m_shadow) {
      adjust = myoffsetof(PortMetaData::BufferOffsets, inputOffsets.myShadowsRemoteStateOffsets[m_mailbox]);
      type = "shadow";
    } else {
      // Real input ports get three extra ones
      addOffset(offsets, from_offset, to_offset, 
		myoffsetof(PortMetaData::BufferOffsets, inputOffsets.bufferOffset), "input");		
      addOffset(offsets, from_offset, to_offset, 
		myoffsetof(PortMetaData::BufferOffsets, inputOffsets.bufferSize), "buffer size");		
      addOffset(offsets, from_offset, to_offset, 
		myoffsetof(PortMetaData::BufferOffsets, inputOffsets.localStateOffset), "local state");
      adjust = myoffsetof(PortMetaData::BufferOffsets, inputOffsets.metaDataOffset);
      type = "input";
    }
    addOffset(offsets, from_offset, to_offset, adjust, type);
  }
}

#if 0
{
  PortMetaData::BufferOffsets* from_offset = (PortMetaData::BufferOffsets*)m_offsetsOffset;
  PortMetaData::BufferOffsets* to_offset = (PortMetaData::BufferOffsets*)to_base_offset;

  // for all buffers
  if ( ! isOutput() ) { 
    for ( OCPI::OS::uint32_t n=0; n<getPortSet()->getBufferCount(); n++ )  {
                        
      if ( m_shadow ) {

        int idx = getLocalEndPoint().mailBox();

        ToFrom* tf = new ToFrom;
        tf->from_offset = (OCPI::OS::uint64_t)&from_offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx];
        tf->to_offset = (OCPI::OS::uint64_t)&to_offset[n].inputOffsets.myShadowsRemoteStateOffsets[idx];
        offsets.push_back( tf );

        ocpiDebug("Wrote shadow offset 0x%llx to address 0x%llx\n", 
               (long long)tf->from_offset, (long long)tf->to_offset );
                                
      }
      else {

        ocpiDebug("Wrote real input offsets\n");

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

    ocpiDebug("Wrote output control offsets\n");
                                
  }
}
#endif


/**********************************
 * Has an End Of Stream been detcted on this port
 *********************************/
bool Port::isEOS()
{
  return m_eos;
}



void Port::resetEOS()
{
  m_eos=false;
}


#if 0
/**********************************
 * Get/Set the SMB name
 *********************************/
void Port::setEndpoint( std::string& ep)
{
  m_data->m_real_location->setEndpoint( ep );
}
#endif

void Port::debugDump()
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
  OCPI::Util::ResAddrType boffset, moffset, soffset, coffset;
  int rc;
  bool local=false;
  uint32_t index;
  unsigned int bCount = m_data->m_portSetMd->bufferCount;

  // The allocation of buffer may be delayed if the circuit definition is not complete
  if ( ! m_data->m_real_location ) {
    ocpiAssert( 0 );
    return;
  }

  if ( getCircuit()->m_transport->isLocalEndpoint(*m_data->m_real_location)) {
    local = true;
    m_data->m_shadow = false;
  }

  if ( local ) {

    XF::ResourceServices* res_mgr = &getEndPoint().resourceMgr();
    //      XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
    ocpiAssert( res_mgr );

    // Allocate the buffers.  We will allocate a contiguous block of memory
    // for all the buffers and the split them up
    rc = res_mgr->alloc( m_data->m_portSetMd->bufferLength * bCount, 
                         BUF_ALIGNMENT, &boffset);
    ocpiDebug("**** Alloc Port %p for createOutputOffsets local buffers 0x%x", this, boffset);
    if ( rc != 0 ) {
      throw OCPI::Util::EmbeddedException( 
                                         NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->name().c_str() );
    }

    ocpiDebug("Port::createOutputOffsets1: port %p bmd %p offset 0x%" OCPI_UTIL_RESADDR_PRIx,
	      this, m_data->m_bufferData, m_data->m_bufferData[0].outputOffsets.bufferOffset);
    for ( index=0; index<bCount; index++ ) {
      m_data->m_bufferData[index].outputOffsets.bufferOffset = boffset+(index*m_data->m_portSetMd->bufferLength);
      m_data->m_bufferData[index].outputOffsets.bufferSize =  m_data->m_portSetMd->bufferLength;
    }
    ocpiDebug("Port %p bmd %p count %d boffset %" OCPI_UTIL_RESADDR_PRIx, this, m_data->m_bufferData,
	      bCount, boffset);
    ocpiDebug("Port::createOutputOffsets2: port %p bmd %p offset 0x%" OCPI_UTIL_RESADDR_PRIx,
	      this, m_data->m_bufferData, m_data->m_bufferData[0].outputOffsets.bufferOffset);
    // Allocate the local state
    rc = res_mgr->alloc( sizeof(BufferState) * MAX_PCONTRIBS * bCount * 2, 
                         BUF_ALIGNMENT, &soffset);
    ocpiDebug("**** Alloc Port %p for createOutputOffsets local state: 0x%x", this, soffset);
    if ( rc != 0 ) {
      res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
      throw OCPI::Util::EmbeddedException( 
                                         NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->name().c_str() );
    }
    for ( index=0; index<bCount; index++ ) {
      m_data->m_bufferData[index].outputOffsets.localStateOffset = 
        soffset + index * MAX_PCONTRIBS * OCPI_UTRUNCATE(OU::ResAddr, sizeof(BufferState)) * 2;
    }
                
    // Allocate the meta-data structure
    rc = res_mgr->alloc( sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount, 
                         BUF_ALIGNMENT, &moffset);
    ocpiDebug("**** Alloc Port %p for createOutputOffsets local metadata 0x%x", this, moffset);
    if ( rc != 0 ) {
      res_mgr->free( soffset,  sizeof(BufferState) * MAX_PCONTRIBS * bCount * 2 );
      res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
      throw OCPI::Util::EmbeddedException( 
                                         NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->name().c_str() );
    }
    for ( index=0; index<bCount; index++ ) {
      m_data->m_bufferData[index].outputOffsets.metaDataOffset = 
        moffset + index * MAX_PCONTRIBS * OCPI_UTRUNCATE(OU::ResAddr, sizeof(BufferMetaData));
    }

    // Allocate the port set control structure if needed (even shadows get one of these)
    if ( m_data->m_localPortSetControl == 0 ) {
                        
      rc = res_mgr->alloc( sizeof(OutputPortSetControl), 
                           BUF_ALIGNMENT, &coffset);
      ocpiDebug("**** Alloc Port %p for createOutputOffsets local outputportsetcontrol 0x%x", this, coffset);
      if ( rc != 0 ) {
        res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
        res_mgr->free( soffset,  sizeof(BufferState) * MAX_PCONTRIBS * bCount * 2);
        res_mgr->free( moffset,   sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount );
        throw OCPI::Util::EmbeddedException( 
                                           NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->name().c_str() );
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
  OCPI::Util::ResAddrType boffset, moffset, soffset;
    int rc;
    bool local=false;
    XF::ResourceServices* res_mgr;
    unsigned int index;
    unsigned int bCount = m_data->m_portSetMd->bufferCount;

    // The allocation of buffer may be delayed if the circuit definition is not complete
    if ( ! m_data->m_real_location ) {
      ocpiAssert( 0 );
      return;
    }

    if ( getCircuit()->m_transport->isLocalEndpoint(*m_data->m_real_location) ) {
      m_data->m_shadow = false;
      local = true;
    }

    // Allocate the buffer
    if ( local ) {

      res_mgr = &getEndPoint().resourceMgr();
      //      res_mgr = XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_real_location )->sMemResourceMgr;
      ocpiAssert( res_mgr );
      rc = res_mgr->alloc( m_data->m_portSetMd->bufferLength * bCount, 
                           BUF_ALIGNMENT, &boffset);
      ocpiDebug("**** Alloc Port %p for createInputOffsets local buffers 0x%" OCPI_UTIL_RESADDR_PRIx,
		this, boffset);
      if ( rc != 0 ) {
        throw OCPI::Util::EmbeddedException( 
                                           NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->name().c_str() );
      }
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.bufferOffset = boffset + 
          index * m_data->m_portSetMd->bufferLength;
        m_data->m_bufferData[index].inputOffsets.bufferSize = m_data->m_portSetMd->bufferLength;
      }
                
      ocpiDebug("***Input buffer offset = 0x%" OCPI_UTIL_RESADDR_PRIx "", boffset );
                
      // Allocate the meta-data structure
      rc = res_mgr->alloc( sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount, 
                           BUF_ALIGNMENT, &moffset);
      ocpiDebug("**** Alloc Port %p for createInputffsets local metadata 0x%" OCPI_UTIL_RESADDR_PRIx,
		this, moffset);
      if ( rc != 0 ) {
        res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
        throw OCPI::Util::EmbeddedException(  NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->name().c_str() );
      }
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.metaDataOffset = 
          moffset + index * OCPI_UTRUNCATE(OU::ResAddr, sizeof(BufferMetaData)) * MAX_PCONTRIBS;
      }
                
      // Allocate the local state(s)
      rc = res_mgr->alloc( 
                          sizeof(BufferState) * MAX_PCONTRIBS * bCount * 2, 
                          BUF_ALIGNMENT, &soffset);
      ocpiDebug("**** Alloc Port %p for createInputffsets local state 0x%" OCPI_UTIL_RESADDR_PRIx,
		this, soffset);
      if ( rc != 0 ) {
        res_mgr->free( moffset,  sizeof(BufferMetaData) * MAX_PCONTRIBS * bCount );
        res_mgr->free( boffset,  m_data->m_portSetMd->bufferLength * bCount );
        throw OCPI::Util::EmbeddedException(  NO_MORE_BUFFER_AVAILABLE, m_data->m_real_location->name().c_str() );
      }
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.localStateOffset = 
          soffset + (index * OCPI_UTRUNCATE(OU::ResAddr, sizeof(BufferState)) * MAX_PCONTRIBS * 2);
      }
                
    }
    else {  // We are a shadow port

      res_mgr = &getShadowEndPoint().resourceMgr();
      //      res_mgr = XferFactoryManager::getFactoryManager().getSMBResources( m_data->m_shadow_location )->sMemResourceMgr;
      ocpiAssert( res_mgr );
      rc = res_mgr->alloc( sizeof(BufferState) * bCount , BUF_ALIGNMENT, &soffset);
      ocpiDebug("**** Alloc Port %p for createInputffsets shadow state 0x%x", this, soffset);
      if ( rc != 0 ) {
        throw OCPI::Util::EmbeddedException(
                                           NO_MORE_BUFFER_AVAILABLE, m_data->m_shadow_location->name().c_str());
      }
      m_data->m_shadowPortDescriptor.role = OCPI::RDT::ActiveMessage;
      m_data->m_shadowPortDescriptor.type = OCPI::RDT::ConsumerFlowControlDescT;
      m_data->m_shadowPortDescriptor.desc.emptyFlagBaseAddr = soffset;
      m_data->m_shadowPortDescriptor.desc.emptyFlagSize = sizeof(BufferState);
      m_data->m_shadowPortDescriptor.desc.emptyFlagPitch = sizeof(BufferState);

#if 0
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
#else
      // We are telling the real input port what to send to us to indicate that the
      // buffer has become empty
      m_data->m_shadowPortDescriptor.desc.emptyFlagValue = EF_EMPTY_VALUE;
#endif                
      //    printf("EmptyFlag value = 0x%llx\n", m_shadowPortDescriptor.desc.emptyFlagValue);
                
      Transport::fillDescriptorFromEndPoint(*m_data->m_shadow_location, 
					    m_data->m_shadowPortDescriptor);
      m_data->m_shadowPortDescriptor.desc.oob.port_id =
	m_data->m_externPortDependencyData.desc.oob.port_id;
      for ( index=0; index<bCount; index++ ) {
        m_data->m_bufferData[index].inputOffsets.
	  myShadowsRemoteStateOffsets[m_data->m_shadow_location->mailBox()] = 
          soffset + index * OCPI_UTRUNCATE(OU::ResAddr, sizeof(BufferState));
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
Port::
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
      assert("Unexpected zero copy to input port"==0);
      if ( tb->m_attachedZBuffer->isEmpty() && ! tb->m_attachedZBuffer->inUse() ) {
        PortSet* aps = static_cast<PortSet*>(tb->m_zCopyPort->getPortSet());
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
Port::
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


BufferUserFacet* 
Port::
getNextFullInputBuffer(uint8_t *&data, size_t &length, uint8_t &opcode, bool &end)
{
  Circuit *c = getCircuit();
  OU::SelfAutoMutex guard(c); // FIXME: refactor to make this a circuit method
  if (!hasFullInputBuffer())
    return NULL;
  TransferController* txc = getPortSet()->getTxController();
  InputBuffer* buf = 
    static_cast<InputBuffer*>(txc->getNextFullInputBuffer(this));
  if (buf) {
    if (buf->isEOS())
      setEOS();
    if (buf && buf->getMetaData()->endOfCircuit)
      c->m_status = Circuit::Disconnecting;

    data = (uint8_t*)buf->getBuffer(); // cast off the volatile
    opcode = (uint8_t)buf->getMetaData()->ocpiMetaDataWord.opCode;
    end = buf->getMetaData()->ocpiMetaDataWord.end;
    length = buf->getDataLength();
    if (buf->getMetaData()->ocpiMetaDataWord.truncate)
      ocpiBad("Message was truncated to %zu bytes", length);
    OCPI_EMIT_CAT__("Data Buffer Received" , OCPI_EMIT_CAT_WORKER_DEV,OCPI_EMIT_CAT_WORKER_DEV_BUFFER_FLOW, buf);

    OCPI_EMIT_REGISTER_FULL_VAR( "Data Buffer Opcode and length", OCPI::Time::Emit::DT_u, 64, OCPI::Time::Emit::Value, dbre ); 
    OCPI_EMIT_VALUE_CAT_NR__(dbre, (uint64_t)(opcode | (uint64_t)length<<16) , OCPI_EMIT_CAT_WORKER_DEV,OCPI_EMIT_CAT_WORKER_DEV_BUFFER_VALUES, buf);
    ocpiDebug("Getting buffer %p on port %p on circuit %p on transport %p data %p op %u len %zu end %u",
	      buf, this, c, &c->parent(), data, opcode, length, buf->getMetaData()->ocpiMetaDataWord.end);

  }
  return buf;
}

// For use by bridge ports, meaning this port is passive
BufferUserFacet* Port::
getNextEmptyInputBuffer(uint8_t *&data, size_t &length) {
  OU::SelfAutoMutex guard(getCircuit());
  Buffer &b = *m_buffers[m_nextBridgeOrd];
  if (b.isEmpty()) {
    assert(!b.inUse());
    if (++m_nextBridgeOrd >= m_portSet->getBufferCount())
      m_nextBridgeOrd = 0;
    data = (uint8_t*)b.getBuffer();
    length = b.getLength();
    return &b;
  }
  return NULL;
}
#if 0
void Port::
sendInputBuffer(BufferUserFacet &ib, size_t length, uint8_t opcode) {
  OU::SelfAutoMutex guard(getCircuit());
  Buffer &b = *static_cast<Buffer *>(&ib);
  b.getMetaData()->ocpiMetaDataWord.opCode = opcode;
  b.getMetaData()->ocpiMetaDataWord.length = OCPI_UTRUNCATE(uint32_t,length);
  b.markBufferFull();
}

BufferUserFacet* Port::
getNextFullOutputBuffer(uint8_t *&data, size_t &length, uint8_t &opcode) {
  OU::SelfAutoMutex guard(getCircuit());
  Buffer &b = *m_buffers[m_nextBridgeOrd];
  if (!b.isEmpty()) {
    assert(!b.inUse());
    if (++m_nextBridgeOrd >= m_portSet->getBufferCount())
      m_nextBridgeOrd = 0;
    data = (uint8_t*)b.getBuffer();
    length = b.getDataLength();
    opcode = (uint8_t)b.getMetaData()->ocpiMetaDataWord.opCode;
    return &b;
  }
  return NULL;
}
#endif
void Port::
releaseOutputBuffer(BufferUserFacet &ob) {
  OU::SelfAutoMutex guard(getCircuit());
  Buffer &b = *static_cast<Buffer *>(&ob);
  b.markBufferEmpty();
}

/**********************************
 * This method causes the specified input buffer to be marked
 * as available.
 *********************************/
OCPI::OS::int32_t 
Port::
inputAvailable( Buffer* input_buf )
{
  Circuit *c = getCircuit();
  OU::SelfAutoMutex guard(c); // FIXME: refactor to make this a circuit method
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
BufferUserFacet* 
Port::
getNextEmptyOutputBuffer(uint8_t *&data, size_t &length)
{
  Buffer *buf = getNextEmptyOutputBuffer();
  if (buf) {
    data = (uint8_t*)buf->getBuffer(); // cast off the volatile
    length = buf->getLength(); // not really data length, but buffer length
    assert(data && length);
    ocpiDebug("getNextEmptOutputBuffer: %p %p %zu", buf, data, length);
  }
  return buf;
}
Buffer* 
Port::
getNextEmptyOutputBuffer()
{
  Circuit *c = getCircuit();
  OU::SelfAutoMutex guard(c); // FIXME: refactor to make this a circuit method
  if (c->isCircuitOpen() ) {
    return NULL;
  }
  if ( !getPortSet() || !getPortSet()->getTxController() ) {
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
    PortSet* aps = static_cast<PortSet*>(getPortSet());
    aps->getTxController()->modifyOutputOffsets( buffer, buffer->m_attachedZBuffer, true );
    inputAvailable( buffer->m_attachedZBuffer );
    buffer->m_attachedZBuffer->m_zCopyPort = NULL;
    buffer->m_attachedZBuffer->m_attachedZBuffer = NULL;
    buffer->m_attachedZBuffer = NULL;
  }

  return static_cast<Buffer*>(buffer);
}

void 
Port::
sendZcopyInputBuffer(BufferUserFacet &buf, size_t len, uint8_t op, bool end)
{
  Buffer *src_buf = static_cast<Buffer*>(&buf);
  src_buf->getMetaData()->ocpiMetaDataWord.length = (uint32_t)len;
  src_buf->getMetaData()->ocpiMetaDataWord.opCode = op;
  src_buf->getMetaData()->ocpiMetaDataWord.end = end ? 1u : 0;
  src_buf->getMetaData()->ocpiMetaDataWord.timestamp = 0x01234567;
  src_buf->getMetaData()->ocpiMetaDataWord.xferMetaData = packXferMetaData(len, op, end);
  ocpiLog(9,"METAZ: @%p %x %x %x %u %x", src_buf->getMetaData(),
	  src_buf->getMetaData()->ocpiMetaDataWord.length,
	  src_buf->getMetaData()->ocpiMetaDataWord.opCode,
	  src_buf->getMetaData()->ocpiMetaDataWord.timestamp,
	  src_buf->getMetaData()->ocpiMetaDataWord.end,
	  src_buf->getMetaData()->ocpiMetaDataWord.xferMetaData);
  Circuit *c = getCircuit();
  OU::SelfAutoMutex guard(c); // FIXME: refactor to make this a circuit method
  c->sendZcopyInputBuffer( this, src_buf, len );
}

size_t Port::
getBufferLength()
{
  return getPortSet()->getBufferLength();
}

void 
Port::
sendOutputBuffer( BufferUserFacet* buf, size_t length, uint8_t opcode, bool end,
		  bool /*data*/)
{
  if (length > getBufferLength())
    throw OU::Error("Buffer being sent with data length (%zu) exceeding buffer length (%zu)",
		    length, getBufferLength());
  Buffer *b = static_cast<Buffer *>(buf);
  // Put the actual opcode and data length in the meta-data
  b->getMetaData()->ocpiMetaDataWord.opCode = opcode;
  b->getMetaData()->ocpiMetaDataWord.length = OCPI_UTRUNCATE(uint32_t,length);
  b->getMetaData()->ocpiMetaDataWord.end = end ? 1u : 0;
  if (!b->getMetaData()->ocpiMetaDataWord.timestamp)
    b->getMetaData()->ocpiMetaDataWord.timestamp = b->getTid() << 1 | 1;
  else
    b->getMetaData()->ocpiMetaDataWord.timestamp +=
      OCPI_UTRUNCATE(uint32_t, getBufferCount() << 1);
  b->getMetaData()->ocpiMetaDataWord.xferMetaData = packXferMetaData(length, opcode, end);
  ocpiLog(9,"METAs: @%p %x %x %u %x %x sz %zu %zu", b->getMetaData(),
	  b->getMetaData()->ocpiMetaDataWord.length,
	  b->getMetaData()->ocpiMetaDataWord.opCode,
	  b->getMetaData()->ocpiMetaDataWord.end,
	  b->getMetaData()->ocpiMetaDataWord.timestamp,
	  b->getMetaData()->ocpiMetaDataWord.xferMetaData,
	  sizeof(RplMetaData), sizeof(BufferMetaData));

  Circuit * c = getCircuit();
  OU::SelfAutoMutex guard(c); // FIXME: refactor to make this a circuit method
  ocpiDebug("Sending buffer %p on port %p on circuit %p length %zu op %u", b, this, c,
	    length, opcode);
  if ( c->canTransferBuffer( b ) ) {
    c->startBufferTransfer( b );
  }
  else {
    c->queTransfer( b );
  }

}
}
}
OCPI::RDT::Descriptors::Descriptors()
  : type(0), // this is illegal
    role(NoRole),
    options(0) {
  memset(&desc, 0, sizeof(desc));
}
