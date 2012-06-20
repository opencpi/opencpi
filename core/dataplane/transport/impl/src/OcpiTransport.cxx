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

#include <algorithm>
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
using namespace OCPI::OS;
using namespace DataTransfer;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;

static uint32_t         g_nextCircuitId=0;

OCPI::Util::VList   OCPI::DataTransport::Transport::m_cached_transfers;
OCPI::Util::VList   OCPI::DataTransport::Transport::active_transfers;

struct TransferDesc_ {
  XferRequest*  xfer;
  EndPoint* loc;
  int offset;
};
typedef struct TransferDesc_ TransferDesc;

// FIXME have recursive mutex with default constructor
// Constructors
OCPI::DataTransport::Transport::
Transport( TransportGlobal* tpg, bool uses_mailboxes, OCPI::Time::Emit * parent  )
  : OCPI::Time::Emit(parent, "Transport"), m_defEndpoint(NULL),
    m_uses_mailboxes(uses_mailboxes), m_mutex(*new OCPI::OS::Mutex(true)),
    m_nextCircuitId(0), m_CSendpoint(NULL), m_transportGlobal(tpg)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  init();
}


OCPI::DataTransport::Transport::
Transport( TransportGlobal* tpg, bool uses_mailboxes )
  : OCPI::Time::Emit("Transport"), m_defEndpoint(NULL),
    m_uses_mailboxes(uses_mailboxes), m_mutex(*new OCPI::OS::Mutex(true)),
    m_nextCircuitId(0), m_CSendpoint(NULL), m_transportGlobal(tpg)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  init();
}



void 
OCPI::DataTransport::Transport::
init() {
  if ( g_nextCircuitId == 0 ) {
    srand( time(NULL) );
    g_nextCircuitId = getpid() + rand();
  }

  m_nextCircuitId += ++g_nextCircuitId;

  // ask the xfer factory manager to allocate a set of endpoints for us, 
  // for all possible protocols
  XferFactoryManager::getFactoryManager().allocateSupportedEndpoints(m_localEndpoints);
#ifndef NDEBUG
    for (EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++)
      ocpiInfo("Transport %p initially got endpoint %s", this, (*i)->end_point.c_str());
#endif
}

// This is called when we get a remote endpoint string and need a local endpoint
// to talk to it.
DataTransfer::EndPoint &OCPI::DataTransport::Transport::
getLocalCompatibleEndpoint(const char *remote, bool exclusive) {
  std::string protocol;
  EndPoint::getProtocolFromString(remote, protocol);
  XferFactory* tfactory = XferFactoryManager::getFactoryManager().find(protocol);
  if (!tfactory)
    throw UnsupportedEndpointEx(remote);
  DataTransfer::EndPoint *lep = NULL;
  if (strchr(remote, ':')) {
    char *cs = strdup(remote);
    uint32_t mailBox, maxMb, size;
    EndPoint::getResourceValuesFromString(remote, cs, &mailBox, &maxMb, &size);
    free(cs);
    for (EndPointsIter i = m_localEndpoints.begin();
	 lep == NULL && i != m_localEndpoints.end(); i++) {
      DataTransfer::EndPoint *ep = *i;
      if (ep->protocol == protocol && ep->maxCount == maxMb && ep->mailbox != mailBox)
	lep = ep;
    }
    if (!lep) {
      lep = tfactory->addCompatibleEndPoint(mailBox, maxMb);
      m_localEndpoints.insert(lep);
    }
  } else {
    for (EndPointsIter i = m_localEndpoints.begin(); lep == NULL && i != m_localEndpoints.end(); i++)
      if ((*i)->protocol == protocol)
	lep = *i;
    ocpiAssert(lep); // if we found a factory, it is supported.
  }
  lep->finalize();
  return *lep;
}

// This is called when we need a local endpoint for a protocol.
DataTransfer::EndPoint &OCPI::DataTransport::Transport::
getLocalEndpointFromProtocol(const char *protocol) {
  if (!protocol || !protocol[0])
    protocol = "ocpi-smb-pio";  // FIXME: some global constant
  for (EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++)
    if ((*i)->protocol == protocol) {
      (*i)->finalize();
      return **i;
    }
  throw UnsupportedEndpointEx(protocol);
}
// This is called when we have an explicit local endpoint that specifies something
// not available in the default allocated one for the protocol.

DataTransfer::EndPoint &OCPI::DataTransport::Transport::
getLocalEndpoint(const char *endpoint) {
  std::string protocol;
  EndPoint::getProtocolFromString(endpoint, protocol);
  XferFactory* tfactory = XferFactoryManager::getFactoryManager().find(protocol);
  if (!tfactory)
    throw UnsupportedEndpointEx(endpoint);
  EndPoint *ep = NULL;
  for (EndPointsIter i = m_localEndpoints.begin();  !ep && i != m_localEndpoints.end(); i++)
    if ((*i)->end_point == endpoint)
      ep = *i;
  if (!ep)
    ep = tfactory->getEndPoint(endpoint, true, true); // force creation
  ep->finalize();
  return *ep;
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
    //    mutex->unlock();
    delete mutex;
  }
};


OCPI::DataTransport::Transport::~Transport()
{
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
  Circuit * c = firstChild();
  while ( c ) {
    Circuit * next_c =  c->nextChild();
    c->release();
    c = next_c;
  }

  while (!m_localEndpoints.empty()) {
    EndPointsIter i = m_localEndpoints.begin();
    ocpiInfo("Transport %p removing local ep %p %s", this, *i, (*i)->end_point.c_str());
    (*i)->release();
    m_localEndpoints.erase(i);
  }
  while (!m_remoteEndpoints.empty()) {
    EndPointsIter i = m_remoteEndpoints.begin();
    ocpiInfo("Transport %p removing remote ep %p %s", this, *i, (*i)->end_point.c_str());
    (*i)->release();
    m_remoteEndpoints.erase(i);
  }
  }
  delete &m_mutex;
}


void 
OCPI::DataTransport::Transport::
setNewCircuitRequestListener( NewCircuitRequestListener* listener )
{
  m_newCircuitListener = listener;
}



Mutex* OCPI::DataTransport::Transport::getMailBoxLock( const char* mbid )
{

  OCPI::OS::uint32_t hash = OU::Misc::hashCode( mbid );
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
requestNewConnection( Circuit* circuit, bool send, const char *protocol, OS::Timer *timer)
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
    addRemoteEndPoint( server_loc->c_str() );
  }

  ocpiDebug("requestNewConnection: c: %s s: %s", client_loc->c_str(), server_loc->c_str());
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
  bool openCircuit = circuit->m_openCircuit;
  circuit->m_openCircuit = true;
  while ( ! xmb.mailBoxAvailable(s_res) ) {
    dispatch();
    OCPI::OS::sleep(0);
    if (timer && timer->expired()) {
      mutex->unlock();
      throw OCPI::Util::EmbeddedException("Server Not Responding");
    }
  }
  circuit->m_openCircuit = openCircuit;

  ocpiDebug("Client is making a request to server to establish new connection.");
   
  DataTransfer::ContainerComms::MailBox* mb = xmb.getMailBox( s_res );
  mb->request.reqNewConnection.type = DataTransfer::ContainerComms::ReqNewConnection;
  mb->request.reqNewConnection.circuitId = circuit->getCircuitId();
  mb->request.reqNewConnection.buffer_size = circuit->getOutputPortSet()->getBufferLength();
  mb->request.reqNewConnection.send = send ? 1 : 0;
  strcpy(mb->request.reqNewConnection.output_end_point, m_CSendpoint->end_point.c_str() );
  if (protocol) {
    // If we have protocol info, we will asking the server for a place to put it.
    // We first must allocate space on our side of the transfer, and copy the protocol data
    // into our local smb buffer.  Then later we can transfer it to the server side when we
    // find out where the server's protocol buffer is.  We will do that when the server asks us
    // for our output flow control offsets.
    uint64_t protocolOffset;
    uint32_t protocolSize = strlen(protocol) + 1;
    mb->request.reqNewConnection.protocol_size = protocolSize;
    if (s_res->sMemResourceMgr->alloc(protocolSize, 0, &protocolOffset))
      throw OCPI::Util::EmbeddedException(NO_MORE_BUFFER_AVAILABLE, "for protocol info exchange");
    void *myProtocolBuffer = s_res->sMemServices->map(protocolOffset, protocolSize);
    memcpy(myProtocolBuffer, protocol, protocolSize);
    s_res->sMemServices->unMap();    
    circuit->setProtocolInfo(protocolSize, protocolOffset);
  } else
    mb->request.reqNewConnection.protocol_size = 0;
   
  // For now, this request does not require a return
  mb->return_offset = -1;
  mb->return_size = 0;
  mb->returnMailboxId = client_location->mailbox;

  ocpiDebug("reqNewConnection: mb %p 0x%x type %d", mb, circuit->getCircuitId(),
	    mb->request.reqNewConnection.type);
 
  xmb.makeRequest( s_res, t_res );

  // We will lock here a prescibed amount of time to see if we are successful
  while ( ! xmb.mailBoxAvailable(s_res) ) {
    if (timer && timer->expired()) {
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
              OCPI::OS::uint32_t flags,
	      const char *protocol,
	      OS::Timer *timer
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
  ocpiDebug("New circuit created and registered: id %x flags %x", circuit->getCircuitId(), flags);

  // We may need to make a new connection request
  if ( flags & NewConnectionFlag ) {
    try {
      requestNewConnection( circuit, (flags & SendCircuitFlag) != 0, protocol, timer );
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
              uint32_t flags,
	      const char *protocol,
	      OS::Timer *timer
              )
{
  ( void ) id;
  CircuitId cid;
  cid = this->m_nextCircuitId++;
  return createCircuit( cid, connection, src_ports, dest_ports, flags, protocol, timer);
}


#if 0

// ports in the connection are used.
Circuit * 
OCPI::DataTransport::Transport::
createCircuit( DataTransfer::EndPoint *ep, OCPI::RDT::Descriptors& desc )
{
  return createCircuit(NULL, new ConnectionMetaData( ep, desc ));
}
#endif
// Create an output given a descriptor from a remote input port
// Also returning a flowcontrol descriptor to give to that remote port
OCPI::DataTransport::Port * 
OCPI::DataTransport::Transport::
createOutputPort(OCPI::RDT::Descriptors& outputDesc,
		 const OCPI::RDT::Descriptors& inputDesc )
{
  // Before creating the output port, we need to 
  // create a local endpoint that is compatible with the remote.
  // It will throw an exception if it isn't workable
  EndPoint &oep = getLocalCompatibleEndpoint(inputDesc.desc.oob.oep);
  strcpy(outputDesc.desc.oob.oep, oep.end_point.c_str());
  // Ensure that the input port endpoint is registered
  EndPoint &iep = addRemoteEndPoint(inputDesc.desc.oob.oep);
  if (outputDesc.desc.dataBufferSize > inputDesc.desc.dataBufferSize)
    outputDesc.desc.dataBufferSize = inputDesc.desc.dataBufferSize;

  Circuit *c = createCircuit(NULL, new ConnectionMetaData( oep, outputDesc ));
  c->addInputPort(iep, inputDesc, oep);

  Port *p = c->getOutputPort();
  p->getPortDescriptor(outputDesc, &inputDesc);
  return p;
}
// Create an output port given an existing input port.
OCPI::DataTransport::Port * 
OCPI::DataTransport::Transport::
createOutputPort(OCPI::RDT::Descriptors& outputDesc,
		 OCPI::DataTransport::Port &inputPort )
{
  // With an inside connection, the endpoints are the same
  strcpy(outputDesc.desc.oob.oep, inputPort.getEndpoint()->end_point.c_str());
  if (outputDesc.desc.dataBufferSize > inputPort.getMetaData()->m_descriptor.desc.dataBufferSize) {
    ocpiDebug("Forcing output buffer size to %u from input size %u on local connection",
	   inputPort.getMetaData()->m_descriptor.desc.dataBufferSize,
	   outputDesc.desc.dataBufferSize);	   
    outputDesc.desc.dataBufferSize = inputPort.getMetaData()->m_descriptor.desc.dataBufferSize;
  }

  inputPort.getCircuit()->finalize(outputDesc.desc.oob.oep);

  Port *p = inputPort.getCircuit()->getOutputPort();
  p->getPortDescriptor(outputDesc, NULL);
  return p;
}

OCPI::DataTransport::Port * 
OCPI::DataTransport::Transport::
createInputPort( Circuit * &circuit,  OCPI::RDT::Descriptors& desc, const OU::PValue *params )
{
  // First, process params to establish the right endpoint
  DataTransfer::EndPoint *ep;
  const char *endpoint = NULL, *protocol = NULL;
  if (OU::findString(params, "endpoint", endpoint))
    ep = &getLocalEndpoint(endpoint); // caller is specific, potentiall specifying QoS etc.
  else {
    if (!OU::findString(params, "protocol", protocol) &&
	!OU::findString(params, "transport", protocol) &&
	(protocol = getenv("OCPI_DEFAULT_PROTOCOL")))
      ocpiDebug("Forcing protocol = %s because OCPI_DEFAULT_PROTOCOL set in environment", protocol);
    ep = &getLocalEndpointFromProtocol(protocol);
  }
  strcpy(desc.desc.oob.oep, ep->end_point.c_str());
  
  int ord=-1;
  OCPI::DataTransport::Port * dtPort=NULL;  
  
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
    dtPort = circuit->addPort( new OCPI::DataTransport::PortMetaData( ord, *ep, desc, psmd) );
    circuit->updatePort( dtPort );
  }
  else {

    // Create the port meta-data
    ConnectionMetaData* cmd = new ConnectionMetaData(NULL,
						     ep,
						     desc.desc.nBuffers,
						     desc.desc.dataBufferSize);
    ord = 1;
    circuit = createCircuit(NULL ,cmd);

    OCPI::DataTransport::PortSet* ps = circuit->getInputPortSet(0);
    dtPort = ps->getPortFromOrdinal(ord);    
  }

  return dtPort;
}

OCPI::DataTransport::Port * 
OCPI::DataTransport::Transport::
createInputPort(OCPI::RDT::Descriptors& desc, const OU::PValue *params )
{
  Circuit *circuit = 0;
  Port *port = createInputPort(circuit, desc, params );
  circuit->attach(); // FIXME: why wouldn't port creation do the attach?
  // Merge port descriptor info between what was passed in and what is determined here.
  port->getPortDescriptor(desc, NULL);
 // Make sure the dtport's descriptor is consistent
  //port->getMetaData()->m_descriptor = desc;
  return port;
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
  OCPI::DataTransport::Circuit* circuit = getCircuit( circuit_ord );
  if (circuit)
    deleteCircuit(circuit);
}

void
OCPI::DataTransport::Transport::
deleteCircuit(Circuit *circuit)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

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
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

  // move data from queue if possible
  std::vector<OCPI::DataTransport::Circuit*>::iterator cit;
  for ( cit=m_circuits.begin(); cit!=m_circuits.end(); cit++) {
    if ( (*cit) == NULL ) continue;
    if ( (*cit)->ready() ) {
      // (*cit)->initializeDataTransfers();
      (*cit)->checkQueuedTransfers();
    }
  }

  // handle mailbox requests
  if ( m_uses_mailboxes )
    checkMailBoxs();
  //  OCPI::OS::sleep(1);

}


void OCPI::DataTransport::Transport::clearRemoteMailbox( OCPI::OS::uint32_t offset, EndPoint* loc )
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 
  TransferDesc* td=NULL;

#ifdef DEBUG_L2
  ocpiDebug("Clearing remote mailbox address = %s, offset = 0x%x", loc->end_point, offset );
#endif

  for ( OCPI::OS::uint32_t m=0; m<m_cached_transfers.getElementCount(); m++ ) {
    TransferDesc* tmp_td = static_cast<TransferDesc*>(m_cached_transfers[m]);
      if ( (tmp_td->loc->end_point == loc->end_point) && ((OCPI::OS::uint32_t)tmp_td->offset == offset ) ) {
      td = tmp_td;
      while ( td->xfer->getStatus() ) {
#ifdef DEBUG_L2
        ocpiDebug("Request to clear the remote mailbox has not yet completed");
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

void OCPI::DataTransport::Transport::
sendOffsets( OCPI::Util::VList& offsets, EndPoint *remote_ep, 
	     uint32_t extraSize, uint64_t extraFrom, uint64_t extraTo)
{
  OCPI::Util::AutoMutex guard ( m_mutex, true ); 

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
  /* Attempt to get or make a transfer template */
  XferServices* ptemplate = 
    XferFactoryManager::getFactoryManager().getService( m_CSendpoint, 
							remote_ep );
  if ( ! ptemplate ) {
    ocpiAssert(0);
  }

#ifdef DEBUG_L2
  ocpiDebug("In OCPI::DataTransport::Transport::sendOffsets, sending %d OCPI::OS::int32_ts", offsets.size() );
#endif

  XferRequest* ptransfer = ptemplate->createXferRequest();

  // We do the extra transfer first so that the other side will have the protocol when it
  // sees that the output offsets have been copied.
  if (extraSize)
    ptransfer->copy (extraFrom, extraTo, extraSize, XferRequest::None);

  for ( OCPI::OS::uint32_t y=0; y<offsets.getElementCount(); y++) {

    OCPI::DataTransport::Port::ToFrom* tf = 
      static_cast<OCPI::DataTransport::Port::ToFrom*>(offsets[y]);

#ifdef DEBUG_L3
    ocpiDebug("Adding copy to transfer list, 0x%x to 0x%x", tf->from_offset,tf->to_offset );
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

/**********************************
 * Our mailbox handler
 *********************************/
static volatile int nc=0;
void OCPI::DataTransport::Transport::checkMailBoxs()
{
  nc++;

  // Ignore our request slot
  DataTransfer::ContainerComms* comms =   m_CSendpoint->resources->m_comms;

  // See if we have any comms requests
  unsigned nMailBoxes = m_CSendpoint->maxCount;
  for ( OCPI::OS::uint32_t n=0; n<nMailBoxes; n++ ) {

    if ( (n != m_CSendpoint->mailbox ) && (comms->mailBox[n].request.reqBasic.type != 0) ) {

      ocpiDebug("Got a mailbox request from %d, req = %d", n, 
             comms->mailBox[n].request.reqBasic.type);

      switch ( comms->mailBox[n].request.reqBasic.type ) {

      case DataTransfer::ContainerComms::ReqUpdateCircuit:
        {


          CircuitId circuit_id = comms->mailBox[n].request.reqUpdateCircuit.receiverCircuitId;
          ocpiDebug("Handling case DataTransfer::ContainerComms::ReqUpdateCircuit: 0x%x\n", circuit_id);
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
	  CircuitId circuit_id = comms->mailBox[n].request.reqNewConnection.circuitId;
          ocpiDebug("Handling case DataTransfer::ContainerComms::ReqNewConnection: 0x%x", circuit_id);
          try {

            // If we dont have a new circuit listener installed, ignore the request
            if ( ! m_newCircuitListener ) {
              nc--;
              return;
            }

            ConnectionMetaData* md=NULL;
            Circuit* c=NULL;
            EndPoint &sep =
	      addRemoteEndPoint(comms->mailBox[n].request.reqNewConnection.output_end_point);
            try {

              // send flag indicates that the client is requesting a circuit to send data to me
              if ( comms->mailBox[n].request.reqNewConnection.send ) {

                std::string s(comms->mailBox[n].request.reqNewConnection.output_end_point);
                md = new ConnectionMetaData( &sep, m_CSendpoint, 1, 
                                             comms->mailBox[n].request.reqNewConnection.buffer_size  );
              }
              else {
                                                                        
                std::string s(comms->mailBox[n].request.reqNewConnection.output_end_point);
                md = new ConnectionMetaData( m_CSendpoint,
					     &sep,  1, 
					     comms->mailBox[n].request.reqNewConnection.buffer_size );
              }

              // Create the new circuit on request from the other side (client)
	      // If the client has protocol info for us, allocate local smb space for it, so it can
	      // copy it to me when I tell it where to put it in the request for output control offsets;
	      uint64_t protocolOffset = 0;
	      uint32_t protocolSize = comms->mailBox[n].request.reqNewConnection.protocol_size;
	      if (protocolSize) {
		// Allocate local space
		if (m_CSendpoint->resources->sMemResourceMgr->alloc(protocolSize, 0,  &protocolOffset))
		  throw OCPI::Util::EmbeddedException(NO_MORE_BUFFER_AVAILABLE, "for protocol info exchange");
		// map in local space
		
	      }
              c = createCircuit(circuit_id, md);
	      c->setProtocolInfo(protocolSize, protocolOffset);
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
          CircuitId circuit_id = comms->mailBox[n].request.reqOutputContOffset.circuitId;
          ocpiDebug("Handling case DataTransfer::ContainerComms::ReqOutputControlOffset: 0x%x", circuit_id);

          int port_id = comms->mailBox[n].request.reqOutputContOffset.portId;

          addRemoteEndPoint( comms->mailBox[n].request.reqOutputContOffset.shadow_end_point );

          // Get the circuit 
          Circuit* c = getCircuit( circuit_id );
	  if (!c) {
	    ocpiBad("Unknown circuit %x", circuit_id);
	    ocpiAssert(0);
	  }
          OCPI::DataTransport::Port* port = 
            static_cast<OCPI::DataTransport::Port*>(c->getOutputPortSet()->getPortFromOrdinal( port_id ));
          ocpiAssert(port);

          // We will lookup the return addres based upon the mailbox
          EndPoint* ep = getEndpoint(comms->mailBox[n].request.reqOutputContOffset.shadow_end_point) ;
          if ( !ep ) {
            ocpiBad("**** INTERNAL programming error !! output shadow port asked for control offset and we dont know its end point !!\n");
            ocpiAssert(0);
          }

	  uint32_t protocolSize = 0;
	  uint64_t protocolOffset;
	  if (comms->mailBox[n].request.reqOutputContOffset.protocol_offset) {
	    // The server side is telling us where to put the protocol info, based on our telling
	    // it, in the reqnewconnection, how big it is.
	    c->getProtocolInfo(protocolSize, protocolOffset);
	    ocpiAssert(protocolSize != 0);
	  }

          OCPI::Util::VList offsetv;

          port->getOffsets( comms->mailBox[n].return_offset, offsetv);
	  
	  sendOffsets( offsetv, ep,
		       protocolSize, protocolOffset,
		       comms->mailBox[n].request.reqOutputContOffset.protocol_offset);
          port->releaseOffsets( offsetv );
	  if (protocolSize)
	    m_CSendpoint->resources->sMemResourceMgr->free(protocolOffset, protocolSize);

          // Clear our mailbox
          comms->mailBox[n].error_code = 0;
          comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

          // We will copy our copy of their mailbox back to them
          int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
          clearRemoteMailbox( offset, ep);

        }
        break;



      case DataTransfer::ContainerComms::ReqShadowRstateOffset:
        {

          CircuitId circuit_id = comms->mailBox[n].request.reqShadowOffsets.circuitId;

          ocpiDebug("Handling case DataTransfer::ContainerComms::ReqShadowRstateOffset: 0x%x", circuit_id);
          int port_id = comms->mailBox[n].request.reqShadowOffsets.portId;

          EndPoint *ep = getEndpoint( comms->mailBox[n].request.reqShadowOffsets.url );

          // Get the circuit 
          Circuit* c = getCircuit( circuit_id );

          ocpiDebug("Return address = %s", comms->mailBox[n].request.reqShadowOffsets.url );

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
          sendOffsets( offsetv, ep );
          port->releaseOffsets( offsetv );

          // Clear our mailbox
          comms->mailBox[n].error_code = 0;
          comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

          // We will copy our copy of their mailbox back to them
          int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
          clearRemoteMailbox( offset, ep);

        }
        break;

      case DataTransfer::ContainerComms::ReqInputOffsets:
        {

          CircuitId circuit_id = comms->mailBox[n].request.reqInputOffsets.circuitId;
          ocpiDebug("Handling case DataTransfer::ContainerComms::ReqInputOffsets: 0x%x", circuit_id);
          int port_id = comms->mailBox[n].request.reqInputOffsets.portId;

          EndPoint* ep = getEndpoint( comms->mailBox[n].request.reqInputOffsets.url );

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
          sendOffsets( offsetv, ep );
          port->releaseOffsets( offsetv );

          // Clear our mailbox
          comms->mailBox[n].error_code = 0;
          comms->mailBox[n].request.reqBasic.type = DataTransfer::ContainerComms::NoRequest;

          // We will copy our copy of their mailbox back to them
          int offset = sizeof(UpAndRunningMarker)+sizeof(ContainerComms::MailBox)*n;
          clearRemoteMailbox( offset, ep );

        }
        break;

      case DataTransfer::ContainerComms::NoRequest:
      default:
        ocpiDebug("Handling case DataTransfer::ContainerComms::Default:");
        //                                ocpiAssert(0);

        break;

      }
    }
  }

  nc--;
}

EndPoint& Transport::
addRemoteEndPoint( const char* loc )
{
  std::string sloc(loc);
  ocpiDebug("In Transport::addRemoteEndPoint, loc = %s", loc );
  
  EndPoint *ep = getEndpoint(loc);
  if (ep)
    return *ep;
  XferFactory* tfactory = 
    XferFactoryManager::getFactoryManager().find(loc);
  if (!tfactory)
    throw UnsupportedEndpointEx(loc);
  ep = tfactory->getEndPoint(loc, false);
  if (!ep->resources)
    XferFactoryManager::getFactoryManager().createSMBResources(ep);
  m_remoteEndpoints.insert(ep);
  return *ep;
}

bool Transport::isLocalEndpoint( const char* loc )
{
  for (EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++) {
    if ((*i)->end_point == loc) {
      ocpiDebug("isLocalEndpoint:: '%s' is local", loc  );
      return true;
    }
  }
  return false;
}


EndPoint * Transport::
getEndpoint(const char* ep)
{
  for (EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++)
    if ((*i)->end_point == ep)
      return *i;
  for (EndPointsIter i = m_remoteEndpoints.begin(); i != m_remoteEndpoints.end(); i++)
    if ((*i)->end_point == ep)
      return *i;
  return NULL;
}
