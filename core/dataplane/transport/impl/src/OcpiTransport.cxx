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
 *   This file contains the Implementation for the OCPI transport.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#include <DtHandshakeControl.h>
#include <DtTransferInternal.h>
#include <OcpiIntTransportExceptions.h>
#include <DtOsDataTypes.h>
#include <OcpiTransport.h>
#include <OcpiCircuit.h>
#include <OcpiList.h>
#include <OcpiUtilHash.h>
#include <OcpiOsMutex.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiTransportExceptions.h>
#include <OcpiIntParallelDataDistribution.h>
#include "OcpiUtilAutoMutex.h"

using namespace OCPI::DataTransport;
using namespace OCPI::Util;
using namespace OCPI::OS;
using namespace DataTransfer;

struct GEndPoint {
  EndPoint*     loc;
  SMBResources* res;
  std::string   ep;
  ~GEndPoint() 
  {
  };
};


static uint32_t         g_nextCircuitId=0;
OCPI::OS::Mutex     OCPI::DataTransport::Transport::m_mutex(true);
OCPI::Util::VList   OCPI::DataTransport::Transport::m_cached_transfers;
OCPI::Util::VList   OCPI::DataTransport::Transport::active_transfers;

struct TransferDesc_ {
  XferRequest*  xfer;
  EndPoint* loc;
  int offset;
};
typedef struct TransferDesc_ TransferDesc;

std::vector<std::string> 
OCPI::DataTransport::Transport::
getListOfSupportedEndpoints()
{
  return XferFactoryManager::getFactoryManager().getListOfSupportedEndpoints();  
}


// Constructors
OCPI::DataTransport::Transport::
Transport( TransportGlobal* tpg, bool uses_mailboxes )
  : OCPI::Time::Emit("Transport"), m_uses_mailboxes(uses_mailboxes), m_transportGlobal(tpg)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  if ( g_nextCircuitId == 0 ) {
    srand( time(NULL) );
    g_nextCircuitId = getpid() + rand();
  }


  m_nextCircuitId = 0;
  m_newCircuitListener = NULL;
  m_nextCircuitId += ++g_nextCircuitId;

  // At this point we need to ask xfer factory manager for all possible endpoint
  m_endpoints = 
    XferFactoryManager::getFactoryManager().getListOfSupportedEndpoints();  

  std::vector<std::string>::iterator it;
  for ( it=m_endpoints.begin(); it!=m_endpoints.end(); it++ ) {
    printf("ep = %s\n", (*it).c_str() );
  }

}


std::string& 
OCPI::DataTransport::Transport::
getDefaultEndPoint()
{
  if ( m_defEndpoint == "" ) {
    m_defEndpoint = getEndpointFromProtocol("ocpi-smb-pio");
    if ( m_defEndpoint == "" ) {
      m_defEndpoint =  m_endpoints[0] = 
	addLocalEndpoint( m_endpoints[0].c_str() )->sMemServices->endpoint()->end_point; 
    }
  }
  return m_defEndpoint;
}


std::string
OCPI::DataTransport::Transport::
getLocalCompatibleEndpoint( const char* ep )
{

#ifndef NDEBUG
  printf("Finding compatible endpoint for %s\n", ep );
#endif

  std::string nuls;
  std::vector<std::string>::iterator it;
  for ( it=m_endpoints.begin(); it!=m_endpoints.end(); it++ ) {
#ifndef NDEBUG
    printf("ep = %s\n", (*it).c_str() );
#endif
    int m=0;
    while ( (*it).c_str()[m] && ep[m] ) {

      if ( (*it).c_str()[m] != ep[m] ) {
        break;
      }
      if ( ( (*it).c_str()[m] == ':') || ep[m] == ':' ) {

        // Make sure that the endpoint is finalized
        (*it) = 
          addLocalEndpoint( (*it).c_str() )->sMemServices->endpoint()->end_point; 
#ifndef NDEBUG
        printf("Found %s for %s\n", (*it).c_str(), ep );
#endif
        return (*it);
      }
      m++;
    }
  }
  return nuls;
}


/**********************************
 * This method gets the Node-wide mutex used to lock our mailbox
 * on the specified endpoint for mailbox communication
 *********************************/
struct MailBoxLock {
  Mutex *mutex;
  OCPI::OS::uint32_t hash_code;
  ~MailBoxLock()
  {
    mutex->unlock();
    delete mutex;
  }
};


OCPI::DataTransport::Transport::~Transport()
{

  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  OCPI::OS::uint32_t m;

  for ( m=0; m<m_cached_transfers.size(); m++ ) {
    TransferDesc* tmp_td = static_cast<TransferDesc*>(m_cached_transfers[m]);
    delete tmp_td;
  }
  m_cached_transfers.destroyList();

  for ( m=0; m<active_transfers.size();  m++ ) {
    XferRequest* xr = static_cast<XferRequest*>(active_transfers[m]);
    delete xr;
  }
  active_transfers.destroyList();

  for ( m=0; m<m_mailbox_locks.size(); m++ ) {
    MailBoxLock* mb = static_cast<MailBoxLock*>(m_mailbox_locks[m]);
    delete mb;
  }
  m_mailbox_locks.destroyList();

  // Remove our children before our reference is removed
  Circuit * c = static_cast<Circuit *>(firstChild());
  while ( c ) {
    Circuit * next_c =  static_cast<Circuit *>(nextChild( c ));
    c->release();
    c = next_c;
  }

  unsigned int n;
  for ( n=0; n<m_remoteEndpoints.getElementCount(); n++ ) {
    delete static_cast<GEndPoint*>(m_remoteEndpoints[n]);
  }
  for ( n=0; n<m_localEndpoints.getElementCount(); n++ ) { 
    delete static_cast<GEndPoint*>(m_localEndpoints[n]);
  }
}


void 
OCPI::DataTransport::Transport::
setNewCircuitRequestListener( NewCircuitRequestListener* listener )
{
  m_newCircuitListener = listener;
}



Mutex* OCPI::DataTransport::Transport::getMailBoxLock( const char* mbid )
{

  OCPI::OS::uint32_t hash = Misc::hashCode( mbid );
  OCPI::OS::int32_t len = m_mailbox_locks.getElementCount();
  for ( int n=0; n<len; n++ ) {
    MailBoxLock* mb = static_cast<MailBoxLock*>(m_mailbox_locks.getEntry(n));
    if ( mb->hash_code == hash ) {
      return mb->mutex;
    }
  }

  // We didnt find one, so create one
  MailBoxLock *mbl = new MailBoxLock;
  mbl->hash_code = hash;
  mbl->mutex = new Mutex();
  m_mailbox_locks.insert( mbl );
  return mbl->mutex;
}



void 
OCPI::DataTransport::Transport::
requestNewConnection( Circuit* circuit, bool send )
{

  std::string& input_loc = circuit->getInputPortSet(0)->getPortFromIndex(0)->getMetaData()->real_location_string;
  std::string& output_loc = circuit->getOutputPortSet()->getPortFromIndex(0)->getMetaData()->real_location_string;

  std::string*  server_loc;
  std::string*  client_loc;
  std::string   nuls;

  if ( !send ) {
    server_loc = &output_loc;
    client_loc = &input_loc;
  }
  else {
    client_loc = &output_loc;
    server_loc = &input_loc;
  }

  if ( ! isLocalEndpoint ( server_loc->c_str() ) ) {
    addRemoteEndpoint( server_loc->c_str() );
  }

  XferFactory* tfactory = 
    XferFactoryManager::getFactoryManager().find( *client_loc, nuls );
  if ( ! tfactory ) {
    throw UnsupportedEndpointEx(client_loc->c_str());
  }
  EndPoint* client_location = tfactory->getEndPoint( *client_loc );

  Mutex* mutex = getMailBoxLock(server_loc->c_str());
  mutex->lock();

  SMBResources* s_res = 
    XferFactoryManager::getFactoryManager().getSMBResources( *client_loc );
  SMBResources* t_res = 
    XferFactoryManager::getFactoryManager().getSMBResources( *server_loc );

  XferMailBox xmb(client_location->mailbox );
  while ( ! xmb.mailBoxAvailable(s_res) ) {
    dispatch();
    OCPI::OS::sleep(0);
  }

#ifdef DEBUG_L2
  printf("Client is making a request to server to establish new connection!!\n");
#endif
   
  DataTransfer::ContainerComms::MailBox* mb = xmb.getMailBox( s_res );
  mb->request.reqNewConnection.type = DataTransfer::ContainerComms::ReqNewConnection;
  mb->request.reqNewConnection.circuitId = circuit->getCircuitId();
  mb->request.reqNewConnection.buffer_size = circuit->getOutputPortSet()->getBufferLength();
  mb->request.reqNewConnection.send = send ? 1 : 0;
  strcpy(mb->request.reqNewConnection.output_end_point, m_CSendpoint->end_point.c_str() );
   
  // For now, this request does not require a return
  mb->return_offset = -1;
  mb->return_size = 0;
  mb->returnMailboxId = client_location->mailbox;

#ifndef NDEBUG
  printf("reqNewConnection: mb %p\n", mb);
  printf("reqNewConnection: type %d\n", mb->request.reqNewConnection.type);
#endif
 
  xmb.makeRequest( s_res, t_res );

  // We will lock here a prescibed amount of time to see if we are successful
  int wait_time = 9000000;
  int time = 0;
  while ( ! xmb.mailBoxAvailable(s_res) ) {
    if ( time++ > wait_time ) {
      mutex->unlock();
      throw OCPI::Util::EmbeddedException("Server Not Responding");
    }
    dispatch();
    OCPI::OS::sleep(1);
  }
  if ( mb->error_code ) {
    mutex->unlock();
    throw OCPI::Util::EmbeddedException("Failed to create client connection");
  }

  mutex->unlock();
}



Circuit *
OCPI::DataTransport::Transport::
createCircuit( 
              CircuitId&   cid,
              ConnectionMetaData* connection,
              PortOrdinal src_ports[],
              PortOrdinal dest_ports[],
              OCPI::OS::uint32_t flags
              )                        
{

  // Make sure that this circuit does not already exist
  OCPI::DataTransport::Circuit* circuit = getCircuit( cid );
  if ( circuit ) {
    deleteCircuit( circuit->getCircuitId() );
  }
  if ( supportsMailboxes() ) {
    circuit = new OCPI::DataTransport::Circuit( this, cid, connection, src_ports, dest_ports );
  }
  else {
    // NOTE:: This needs to be specialized for optimization
    circuit = new OCPI::DataTransport::Circuit( this, cid, connection, src_ports, dest_ports );
  }

  m_circuits.push_back( circuit );


  // We may need to make a new connection request
  if ( flags & NewConnectionFlag ) {
    try {
      bool send = false;
      if ( flags & SendCircuitFlag ) {
        send = true;
      }
      requestNewConnection( circuit, send );
    }
    catch( ... ) {
      deleteCircuit( circuit->getCircuitId() );
      throw;
    }
  }
  return circuit;
}



Circuit *
OCPI::DataTransport::Transport::
createCircuit( 
              const char*   id,         
              ConnectionMetaData* connection,        
              PortOrdinal src_ports[],        
              PortOrdinal dest_ports[],
              OCPI::OS::uint32_t flags
              )
{
  ( void ) id;
  CircuitId cid;
  cid = this->m_nextCircuitId++;
  return createCircuit( cid, connection, src_ports, dest_ports, flags);
}



// ports in the connection are used.
Circuit * 
OCPI::DataTransport::Transport::
createCircuit( OCPI::RDT::Descriptors& sPortDesc )
{
  Circuit * c;
  // Create the port connection meta-data
  OCPI::DataTransport::ConnectionMetaData* cmd = 
    new OCPI::DataTransport::ConnectionMetaData( sPortDesc );
  c = createCircuit(NULL ,cmd);
  return c;
}


OCPI::DataTransport::Port * 
OCPI::DataTransport::Transport::
createInputPort( Circuit * &circuit,  OCPI::RDT::Descriptors& desc )
{
  int ord=-1;
  OCPI::DataTransport::Port * dtPort=NULL;  

  // Make sure the endpoint is initialized
  std::string eps;
  DataTransfer::SMBResources* res = addLocalEndpoint( desc.desc.oob.oep );
  if ( ! res ) {
    std::string ens("Endpoint not supported ");
    ens +=  desc.desc.oob.oep;
    throw OCPI::Util::EmbeddedException( ens.c_str() );
  }
  eps = res->sMemServices->endpoint()->end_point;
  
  // For sake of efficiency we make sure to re-use the circuits that relate 
  // to the same connecton
  if ( circuit  ) {
    if ( circuit->getInputPortSetCount() ) {
      ord = 1 + circuit->getInputPortSet(0)->getPortCount();
    }
    else {
      ord = 1;
    }      

    // Create the port meta-data
    OCPI::DataTransport::PortSetMetaData* psmd;
    if ( ! circuit->getInputPortSet(0) ) {
      psmd = new OCPI::DataTransport::PortSetMetaData(  false, 1,new OCPI::DataTransport::ParallelDataDistribution(), 
                                                       desc.desc.nBuffers,
                                                       desc.desc.dataBufferSize,
                                                       circuit->getConnectionMetaData() );
    }
    else {
      psmd = circuit->getInputPortSet(0)->getPsMetaData();
    }
    strcpy( desc.desc.oob.oep, eps.c_str() );
    dtPort = circuit->addPort( new OCPI::DataTransport::PortMetaData( ord, desc, psmd) );
    circuit->updatePort( dtPort );
  }
  else {

    // Create the port meta-data
    OCPI::DataTransport::ConnectionMetaData* cmd = 
      new OCPI::DataTransport::ConnectionMetaData( NULL,
                                                     eps.c_str(),
                                                     desc.desc.nBuffers,
                                                     desc.desc.dataBufferSize );
    ord = 1;
    circuit = createCircuit(NULL ,cmd);

    OCPI::DataTransport::PortSet* ps = circuit->getInputPortSet(0);
    dtPort = ps->getPortFromOrdinal(ord);    
  }

  return dtPort;
}


OCPI::DataTransport::Circuit* 
OCPI::DataTransport::Transport::
getCircuit(  CircuitId& circuit_id )
{
  std::vector<OCPI::DataTransport::Circuit*>::iterator cit;
  for ( cit=m_circuits.begin(); cit!=m_circuits.end(); cit++) {
    if ( (*cit)->getCircuitId() == circuit_id ) {
      return (*cit);
    }
  }
  return NULL;
}


OCPI::OS::uint32_t 
OCPI::DataTransport::Transport::
getCircuitCount()
{
  return m_circuits.size();
}


void 
OCPI::DataTransport::Transport::
deleteCircuit( CircuitId circuit_ord )
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  OCPI::DataTransport::Circuit* circuit = getCircuit( circuit_ord );
  if ( ! circuit ) {
    return;
  }
  std::vector<OCPI::DataTransport::Circuit*>::iterator it = std::find(m_circuits.begin(), m_circuits.end(), circuit);
  m_circuits.erase(it);
  delete circuit;
  if ( m_circuits.size() == 0 ) {
    for ( OCPI::OS::uint32_t m=0; m<m_cached_transfers.getElementCount(); m++ ) {
      TransferDesc* tmp_td = static_cast<TransferDesc*>(m_cached_transfers[m]);
      m_cached_transfers.remove( tmp_td );
      delete tmp_td;
    }
  }
}



/**********************************
 * General house keeping 
 *********************************/
void OCPI::DataTransport::Transport::dispatch(DataTransfer::EventManager*)
{

  // move data from queue if possible
  std::vector<OCPI::DataTransport::Circuit*>::iterator cit;
  for ( cit=m_circuits.begin(); cit!=m_circuits.end(); cit++) {
    if ( (*cit) == NULL ) continue;
    if ( (*cit)->ready() ) {
      (*cit)->initializeDataTransfers();
      (*cit)->checkQueuedTransfers();
    }
  }

  // handle mailbox requests
  if ( m_uses_mailboxes )
    checkMailBoxs();

}


void OCPI::DataTransport::Transport::clearRemoteMailbox( OCPI::OS::uint32_t offset, EndPoint* loc )
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  TransferDesc* td=NULL;

#ifdef DEBUG_L2
  printf("Clearing remote mailbox address = %s, offset = 0x%x\n", loc->end_point, offset );
#endif

  for ( OCPI::OS::uint32_t m=0; m<m_cached_transfers.getElementCount(); m++ ) {
    TransferDesc* tmp_td = static_cast<TransferDesc*>(m_cached_transfers[m]);
      if ( (tmp_td->loc->end_point == loc->end_point) && ((OCPI::OS::uint32_t)tmp_td->offset == offset ) ) {
      td = tmp_td;
      while ( td->xfer->getStatus() ) {
#ifdef DEBUG_L2
        printf("Request to clear the remote mailbox has not yet completed\n");
#endif
      }
    }
  }

  if (  ! td ) {

    /* Attempt to get or make a transfer template */
    XferServices* ptemplate = 
      XferFactoryManager::getFactoryManager().getService( m_CSendpoint, 
                                      loc );
    if ( ! ptemplate ) {
      ocpiAssert(0);
    }

    XferRequest* ptransfer = ptemplate->createXferRequest();

    // Create the copy in the template

    ptransfer->copy (
		     offset + sizeof(ContainerComms::BasicReq),
		     offset + sizeof(ContainerComms::BasicReq),
		     sizeof(ContainerComms::MailBox) - sizeof(ContainerComms::BasicReq),
		     XferRequest::FirstTransfer );
                
    ptransfer->copy (
		     offset,
		     offset,
		     sizeof(ContainerComms::BasicReq),
		     XferRequest::LastTransfer );

    ptransfer->post();

    // Cache it
    TransferDesc *trd = new TransferDesc;
    trd->loc = loc;
    trd->offset = offset;
    trd->xfer = ptransfer;
    m_cached_transfers.push_back( trd );

  }
  else {
    ocpiAssert( td->xfer->getStatus() == 0 );
    td->xfer->post();
  }
}


void OCPI::DataTransport::Transport::sendOffsets( OCPI::Util::VList& offsets, std::string& remote_ep )
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  // Check active transfers to make sure they are complete

 FORSTART:

  for ( OCPI::OS::uint32_t m=0; m<active_transfers.getElementCount();  m++ ) {
    XferRequest* xr = static_cast<XferRequest*>(active_transfers[m]);
    if ( xr->getStatus() == 0 ) {
      XferRequest* for_delete = xr;
      active_transfers.remove( xr );
      delete for_delete;
      goto FORSTART;
    }
  }

#ifdef DEBUG_L2
  printf("In OCPI::DataTransport::Transport::sendOffsets, sending %d OCPI::OS::int32_ts\n", offsets.size() );
#endif

  /* Attempt to get or make a transfer template */
  XferServices* ptemplate = 
    XferFactoryManager::getFactoryManager().getService( m_CSendpoint->end_point, 
                                    remote_ep );
  if ( ! ptemplate ) {
    ocpiAssert(0);
  }

  // Create the copy in the template
  XferRequest* ptransfer = ptemplate->createXferRequest();
  int count=0;

  for ( OCPI::OS::uint32_t y=0; y<offsets.getElementCount(); y++, count++ ) {

    OCPI::DataTransport::Port::ToFrom* tf = 
      static_cast<OCPI::DataTransport::Port::ToFrom*>(offsets[y]);

#ifdef DEBUG_L3
    printf("Adding copy to transfer list, 0x%x to 0x%x\n", tf->from_offset,tf->to_offset );
#endif
    ptransfer->copy (
		     tf->from_offset,
		     tf->to_offset,
		     sizeof(OCPI::OS::uint32_t),
		     XferRequest::None );

  }
  ptransfer->post();
  active_transfers.push_back( ptransfer );
}



SMBResources* Transport::getEndpointResourcesFromMailbox(OCPI::OS::uint32_t mb )
{
  OCPI::OS::uint32_t n;
  for ( n=0; n<m_remoteEndpoints.getElementCount(); n++ ) {
    if ( mb == ((GEndPoint*)m_remoteEndpoints.getEntry(n))->loc->mailbox ) {
      return ((GEndPoint*)m_remoteEndpoints.getEntry(n))->res;
    }
  }
  for ( n=0; n<m_localEndpoints.getElementCount(); n++ ) {
    if ( mb == ((GEndPoint*)m_localEndpoints.getEntry(n))->loc->mailbox ) {
      return ((GEndPoint*)m_localEndpoints.getEntry(n))->res;
    }
  }
  return NULL;
}

                                                                

/**********************************
 * Our mailbox handler
 *********************************/
static volatile int nc=0;
void OCPI::DataTransport::Transport::checkMailBoxs()
{
  nc++;

  // Ignore our request slot
  DataTransfer::ContainerComms* comms =   getEndpointResources(m_CSendpoint->end_point.c_str())->m_comms;

  // See if we have any comms requests
  for ( OCPI::OS::uint32_t n=0; n<MAX_SYSTEM_SMBS; n++ ) {

    if ( (n != m_CSendpoint->mailbox ) && (comms->mailBox[n].request.reqBasic.type != 0) ) {

#ifndef NDEBUG
      printf("***&&& Got a mailbox request from %d, req = %d\n", n, 
             comms->mailBox[n].request.reqBasic.type);
#endif

      switch ( comms->mailBox[n].request.reqBasic.type ) {

      case DataTransfer::ContainerComms::ReqUpdateCircuit:
        {

#ifndef NDEBUG
          printf("Handling case DataTransfer::ContainerComms::ReqUpdateCircuit:\n");
#endif

          CircuitId circuit_id = comms->mailBox[n].request.reqUpdateCircuit.receiverCircuitId;
          OCPI::DataTransport::Circuit* c = 
            static_cast<OCPI::DataTransport::Circuit*>(getCircuit(circuit_id ));

          c->updateInputs( &comms->mailBox[n].request.reqUpdateCircuit );

          // Clear our mailbox
          comms->mailBox[n].error_code = 0;
          comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

          // Clear the remote mailbox
          XferFactory* tfactory = 
            XferFactoryManager::getFactoryManager().find( comms->mailBox[n].request.reqUpdateCircuit.output_end_point, NULL );
          if ( ! tfactory ) {
            throw UnsupportedEndpointEx(comms->mailBox[n].request.reqUpdateCircuit.output_end_point);
          }

          // We will copy our copy of their mailbox back to them
          int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
          std::string s(comms->mailBox[n].request.reqUpdateCircuit.output_end_point);
          clearRemoteMailbox( offset, tfactory->getEndPoint( s ) );
        }
        break;

        // New connection request
      case DataTransfer::ContainerComms::ReqNewConnection:
        {
#ifndef NDEBUG
          printf("Handling case DataTransfer::ContainerComms::ReqNewConnection:\n");
#endif
          try {

            // If we dont have a new circuit listener installed, ignore the request
            if ( ! m_newCircuitListener ) {
              nc--;
              return;
            }

            CircuitId circuit_id = comms->mailBox[n].request.reqNewConnection.circuitId;
            ConnectionMetaData* md=NULL;
            Circuit* c=NULL;
            addRemoteEndpoint( 
                              comms->mailBox[n].request.reqNewConnection.output_end_point        );
            try {

              // send flag indicates that the client is requesting a circuit to send data to me
              if ( comms->mailBox[n].request.reqNewConnection.send ) {

                std::string s(comms->mailBox[n].request.reqNewConnection.output_end_point);
                md = new ConnectionMetaData( s.c_str(), 
                                             m_CSendpoint->end_point.c_str(), 1, 
                                             comms->mailBox[n].request.reqNewConnection.buffer_size  );
              }
              else {
                                                                        
                std::string s(comms->mailBox[n].request.reqNewConnection.output_end_point);
                md = new ConnectionMetaData( m_CSendpoint->end_point.c_str(),
                                                s.c_str(),  1, 
                                                comms->mailBox[n].request.reqNewConnection.buffer_size );
              }

              // Create the new circuit
              c = createCircuit( circuit_id, md, NULL,NULL,0);
            }
            catch ( ... ) {

              delete md;
              delete c;

              // Clear our mailbox
              comms->mailBox[n].error_code = -1;
              comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

              // Clear the remote mailbox
              XferFactory* tfactory = 
                XferFactoryManager::getFactoryManager().find( comms->mailBox[n].request.reqNewConnection.output_end_point, NULL );

              // We will copy our copy of their mailbox back to them
              int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
              std::string s(comms->mailBox[n].request.reqNewConnection.output_end_point);
              clearRemoteMailbox( offset, tfactory->getEndPoint( s ));

              throw;
            }

            // Clear our mailbox
            comms->mailBox[n].error_code = 0;
            comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

            // Clear the remote mailbox
            XferFactory* tfactory = 
              XferFactoryManager::getFactoryManager().find( comms->mailBox[n].request.reqNewConnection.output_end_point, NULL );
            if ( ! tfactory ) {
              throw UnsupportedEndpointEx(comms->mailBox[n].request.reqNewConnection.output_end_point);
            }

            // We will copy our copy of their mailbox back to them
            int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
            std::string s(comms->mailBox[n].request.reqNewConnection.output_end_point);
            clearRemoteMailbox( offset, tfactory->getEndPoint( s ) );

            // Hand it back to the listener
            m_newCircuitListener->newCircuitAvailable( c );

          }
          catch( ... ) {
            throw;
          }
        }
        break;


      case DataTransfer::ContainerComms::ReqOutputControlOffset:
        {
#ifndef NDEBUG
          printf("Handling case DataTransfer::ContainerComms::ReqOutputControlOffset:\n");
#endif

          CircuitId circuit_id = comms->mailBox[n].request.reqOutputContOffset.circuitId;
          int port_id = comms->mailBox[n].request.reqOutputContOffset.portId;

          addRemoteEndpoint( comms->mailBox[n].request.reqOutputContOffset.shadow_end_point );

          // Get the circuit 
          Circuit* c = getCircuit( circuit_id );

          OCPI::DataTransport::Port* port = 
            static_cast<OCPI::DataTransport::Port*>(c->getOutputPortSet()->getPortFromOrdinal( port_id ));
          ocpiAssert(port);

          // We will lookup the return addres based upon the mailbox
          SMBResources* res = getEndpointResources(comms->mailBox[n].request.reqOutputContOffset.shadow_end_point) ;
          if ( ! res ) {
#ifndef NDEBUG
            printf("**** INTERNAL programming error !! output shadow port asked for control offset and we dont know its end point !!\n");
#endif
            ocpiAssert(0);
          }

          OCPI::Util::VList offsetv;

          port->getOffsets( comms->mailBox[n].return_offset, offsetv);
          sendOffsets( offsetv, res->sMemServices->endpoint()->end_point);
          port->releaseOffsets( offsetv );

          // Clear our mailbox
          comms->mailBox[n].error_code = 0;
          comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

          // We will copy our copy of their mailbox back to them
          int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
          clearRemoteMailbox( offset, res->sMemServices->endpoint() );

        }
        break;



      case DataTransfer::ContainerComms::ReqShadowRstateOffset:
        {

#ifndef NDEBUG
          printf("Handling case DataTransfer::ContainerComms::ReqShadowRstateOffset:\n");
#endif
          CircuitId circuit_id = comms->mailBox[n].request.reqShadowOffsets.circuitId;
          int port_id = comms->mailBox[n].request.reqShadowOffsets.portId;

          SMBResources* res=    
            getEndpointResources( comms->mailBox[n].request.reqShadowOffsets.url );

          // Get the circuit 
          Circuit* c = getCircuit( circuit_id );

          printf("Return address = %s\n", comms->mailBox[n].request.reqShadowOffsets.url );

          OCPI::DataTransport::Port* port=NULL;
          for ( OCPI::OS::uint32_t y=0; y<c->getInputPortSetCount(); y++ ) {
            PortSet* ps = static_cast<PortSet*>(c->getInputPortSet(y));
            port = static_cast<OCPI::DataTransport::Port*>(ps->getPortFromOrdinal( port_id ));
            if ( port ) {
              break;
            }
          }
          if ( !port ) {
                break;
          }

          OCPI::Util::VList offsetv;
          port->getOffsets( comms->mailBox[n].return_offset, offsetv);
          sendOffsets( offsetv, res->sMemServices->endpoint()->end_point );
          port->releaseOffsets( offsetv );

          // Clear our mailbox
          comms->mailBox[n].error_code = 0;
          comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

          // We will copy our copy of their mailbox back to them
          int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
          clearRemoteMailbox( offset, res->sMemServices->endpoint() );

        }
        break;

      case DataTransfer::ContainerComms::ReqInputOffsets:
        {

#ifndef NDEBUG
          printf("Handling case DataTransfer::ContainerComms::ReqInputOffsets:\n");
#endif
          CircuitId circuit_id = comms->mailBox[n].request.reqInputOffsets.circuitId;
          int port_id = comms->mailBox[n].request.reqInputOffsets.portId;

          SMBResources* res=    
            getEndpointResources( comms->mailBox[n].request.reqInputOffsets.url );

          // Get the circuit 
          Circuit* c = getCircuit( circuit_id );


          OCPI::DataTransport::Port* port=NULL;
          for ( OCPI::OS::uint32_t y=0; y<c->getInputPortSetCount(); y++ ) {
            PortSet* ps = static_cast<PortSet*>(c->getInputPortSet(y));
            port = static_cast<OCPI::DataTransport::Port*>(ps->getPortFromOrdinal( port_id ));
            if ( port ) {
              break;
            }
          }
          if ( !port ) {
                break;
          }

          OCPI::Util::VList offsetv;
          port->getOffsets( comms->mailBox[n].return_offset, offsetv);
          sendOffsets( offsetv, res->sMemServices->endpoint()->end_point );
          port->releaseOffsets( offsetv );

          // Clear our mailbox
          comms->mailBox[n].error_code = 0;
          comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

          // We will copy our copy of their mailbox back to them
          int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
          clearRemoteMailbox( offset, res->sMemServices->endpoint() );

        }
        break;

      case DataTransfer::ContainerComms::NoRequest:
      default:
#ifndef NDEBUG
        printf("Handling case DataTransfer::ContainerComms::Default:\n");
#endif
        //                                ocpiAssert(0);

        break;

      }
    }
  }

  nc--;
}


void  Transport::removeLocalEndpoint(  const char* loc )
{
  for ( unsigned int n=0; n<m_localEndpoints.getElementCount(); n++ ) {
    GEndPoint* gep = (GEndPoint*)(m_localEndpoints.getEntry(n));
    if ( gep->ep == loc ) {
      XferFactoryManager::getFactoryManager().deleteSMBResources(gep->loc);
      m_localEndpoints.remove( gep );
      delete gep;
    }
  }
}

SMBResources* Transport::addRemoteEndpoint( const char* loc )
{
  std::string sloc(loc);
#ifndef NDEBUG
  printf("In TransportGlobal::addRemoteEndpoint, loc = %s\n", loc );
#endif
  
  SMBResources* res = getEndpointResources(loc);
  if ( res ) {
    return res;
  }

  GEndPoint* gep = new GEndPoint;
  gep->ep = loc;
        
  std::string nuls;
  XferFactory* tfactory = 
    XferFactoryManager::getFactoryManager().find( nuls, sloc );
  if ( ! tfactory ) {
    delete gep;
    return NULL;
  }
  gep->loc = tfactory->getEndPoint(sloc);
  gep->loc->local = false;
  gep->res = XferFactoryManager::getFactoryManager().getSMBResources( gep->loc );
  m_remoteEndpoints.insert( gep );
  return gep->res;
}


std::string
Transport::
getEndpointFromProtocol( const char* protocol )
{
  std::string loc(protocol);
  std::string nuls;
  XferFactory* tfactory = 
    XferFactoryManager::getFactoryManager().find( loc, nuls );
  if ( !tfactory ) {
    return NULL;
  }
  std::string sep = tfactory->allocateEndpoint( NULL, NULL );
  return addLocalEndpoint( sep.c_str() ) ->sMemServices->endpoint()->end_point; 

}


SMBResources* 
Transport::
addLocalEndpoint( const char *nfep  )
{

  // finalize it
  std::string loc(nfep);
  std::string nuls;
  XferFactory* tfactory = 
    XferFactoryManager::getFactoryManager().find( loc, nuls );
  if ( !tfactory ) {
    return NULL;
  }
  DataTransfer::EndPoint * lep = tfactory->getEndPoint( loc, true );
  // Force create
  tfactory->getSmemServices( lep );
  const char* ep = lep->end_point.c_str();
  loc = ep;

  SMBResources* res = getEndpointResources( ep );
  if ( res ) {
    return res;
  }

#ifndef NDEBUG
  printf("******** REQ EP = %s as local\n", ep );
#endif

  GEndPoint* gep = new GEndPoint;
  gep->ep = ep;
  gep->loc = tfactory->getEndPoint(loc);
  if ( gep->loc->maxCount >= MAX_ENDPOINTS ) {
    delete gep;
    throw OCPI::Util::EmbeddedException( MAX_ENDPOINT_COUNT_EXCEEDED, loc.c_str() );
  }
  gep->loc->local = true;
  try {
    gep->res = XferFactoryManager::getFactoryManager().createSMBResources(gep->loc);
  }
  catch( ... ) {
    delete gep;
    throw;
  }
  loc = gep->ep = gep->loc->end_point;
  gep->loc->setEndpoint( loc );

#ifndef NDEBUG
  printf("******** ADDING EP = %s as local\n", loc.c_str() );
#endif

  m_localEndpoints.insert( gep );
  return gep->res;
}



bool Transport::isLocalEndpoint( const char* loc )
{
  for ( OCPI::OS::uint32_t n=0; n<m_localEndpoints.getElementCount(); n++ ) {
#ifndef NDEBUG
    printf("isLocalEndpoint:: Comparing (%s) with (%s) \n", loc, 
           ((GEndPoint*)m_localEndpoints.getEntry(n))->ep.c_str()  );
#endif
    if ( strcmp( loc, ((GEndPoint*)m_localEndpoints.getEntry(n))->ep.c_str() ) == 0 ) {
#ifndef NDEBUG
      printf("isLocalEndpoint:: is local\n"  );
#endif

      return true;
    }
  }
  return false;
}



SMBResources* Transport::getEndpointResources(const char* ep)
{
  unsigned int n;
  for ( n=0; n<m_remoteEndpoints.getElementCount(); n++ ) {
    if ( static_cast<GEndPoint*>(m_remoteEndpoints[n])->res->sMemServices->endpoint()->end_point == ep ) {
      return static_cast<GEndPoint*>(m_remoteEndpoints[n])->res;
    }
  }
  for ( n=0; n<m_localEndpoints.getElementCount(); n++ ) {
    if ( static_cast<GEndPoint*>(m_localEndpoints[n])->res->sMemServices->endpoint()->end_point == ep ) {
      return static_cast<GEndPoint*>(m_localEndpoints[n])->res;
    }
  }
  return NULL;
}
