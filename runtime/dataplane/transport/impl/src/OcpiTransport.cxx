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

#include <unistd.h>
#include <algorithm>
#include "OcpiOsMutex.h"
#include "OcpiOsMisc.h"
#include "OcpiOsAssert.h"
#include "OcpiList.h"
#include "OcpiUtilHash.h"
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilMisc.h"
#include "DtOsDataTypes.h"
#include "DtHandshakeControl.h"
#include "DtTransferInternal.h"
//#include "OcpiIntTransportExceptions.h"
#include "OcpiTransport.h"
#include "OcpiTransportExceptions.h"
#include "OcpiIntParallelDataDistribution.h"

namespace DT = DataTransfer;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace DDT = DtOsDataTypes;
namespace OCPI {
  namespace DataTransport {
static uint32_t         g_nextCircuitId=0;

OU::VList   Transport::m_cached_transfers;
OU::VList   Transport::active_transfers;

struct TransferDesc_ {
  DT::XferRequest*  xfer;
  DT::EndPoint* loc;
  size_t offset;
};
typedef struct TransferDesc_ TransferDesc;

// FIXME have recursive mutex with default constructor
// Constructors
Transport::
Transport( TransportGlobal* tpg, bool uses_mailboxes, OCPI::Time::Emit * parent  )
  : OCPI::Time::Emit(parent, "Transport"), m_defEndpoint(NULL),
    m_uses_mailboxes(uses_mailboxes), m_mutex(*new OS::Mutex(true)),
    m_nextCircuitId(0), m_CSendpoint(NULL), m_transportGlobal(tpg)
{
  OU::AutoMutex guard ( m_mutex, true ); 
  init();
}

Transport::
Transport( TransportGlobal* tpg, bool uses_mailboxes )
  : OCPI::Time::Emit("Transport"), m_defEndpoint(NULL),
    m_uses_mailboxes(uses_mailboxes), m_mutex(*new OS::Mutex(true)),
    m_nextCircuitId(0), m_CSendpoint(NULL), m_transportGlobal(tpg)
{
  OU::AutoMutex guard ( m_mutex, true ); 
  init();
}

void 
Transport::
init() {
  if ( g_nextCircuitId == 0 ) {
    srand( (unsigned)time(NULL) );
    g_nextCircuitId = getpid() + rand();
  }

  m_nextCircuitId += ++g_nextCircuitId;

  // ask the xfer factory manager to allocate a set of endpoints for us, 
  // for all possible protocols
  //  XferFactoryManager::getFactoryManager().allocateSupportedEndpoints(m_localEndpoints);
#ifndef NDEBUG
  for (DT::EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++)
      ocpiDebug("Transport %p initially got endpoint %s", this, (*i)->end_point.c_str());
#endif
}

// This is called when we get a variably complete remote endpoint string
// and need a local endpoint to talk to it.
DT::EndPoint &Transport::
getLocalCompatibleEndpoint(const char *remote, bool /* exclusive */) {
  if (!remote || !remote[0])
    remote = getenv("OCPI_DEFAULT_TRANSPORT");
  if (!remote)
    remote = "ocpi-smb-pio";
  DT::XferFactory* tfactory = DT::XferFactoryManager::getFactoryManager().find(remote);
  if (!tfactory) {
    std::string rem("ocpi-");
    if (!strcasecmp(remote, "pio"))
      rem += "smb-pio";
    else if (!strcasecmp(remote, "pci"))
      rem += "pci-pio";
    else {
      rem += remote;
      rem += "-rdma";
    }
    if (!(tfactory = DT::XferFactoryManager::getFactoryManager().find(rem)))
      throw OU::Error("No driver found/loaded when looking for data transfer driver for %s/%s",
		      remote, rem.c_str());
    //      throw UnsupportedEndpointEx(remote);
  }
#if 1
  uint16_t mailBox = 0, maxMb = 0;
  const char *after = strchr(remote, ':');
  if (after) {
    after++; // for isCompatible below
    char *cs = strdup(remote);
    size_t size;
    DT::EndPoint::getResourceValuesFromString(remote, cs, &mailBox, &maxMb, &size);
    free(cs);
  }
  DT::EndPoint *lep = NULL;
  for (DT::EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++) {
    DT::EndPoint *ep = *i;
    if (ep->factory == tfactory &&
	(mailBox == 0 || (ep->maxCount == maxMb && ep->mailbox != mailBox)) &&
	ep->isCompatibleLocal(after)) {
      lep = ep;
      break;
    }
  }
  if (!lep) {
    lep = tfactory->addCompatibleLocalEndPoint(after, mailBox, maxMb);
    m_localEndpoints.insert(lep);
  }
#else
  DT::EndPoint *lep = NULL;
  if (strchr(remote, ':')) {
    // remote has details (not just a protool) we need to match up in the details
    // we first parse the generic back end (if there is one).
    char *cs = strdup(remote);
    uint16_t mailBox, maxMb;
    uint32_t size;
    DT::EndPoint::getResourceValuesFromString(remote, cs, &mailBox, &maxMb, &size);
    free(cs);
    // Now we ask the driver to do the real work.
    lep = tfactory->getCompatibleEndPoint(remote, m_localEndpoints, mailBox, maxMb);
  } else {
    // Remote is just a protocol
    for (DT::EndPointsIter i = m_localEndpoints.begin(); lep == NULL && i != m_localEndpoints.end(); i++)
      if ((*i)->protocol == protocol)
	lep = *i;
    if (!lep) {
      // We have no local endpoint for this protocol yet.
      lep = tfactory->addCompatibleEndPoint(0, 0);
      m_localEndpoints.insert(lep);
    }
  }
#endif
  lep->finalize();
  return *lep;
}

// This is called when we have an explicit local endpoint that specifies something
// not available in the default allocated one for the protocol.

DT::EndPoint &Transport::
getLocalEndpoint(const char *endpoint) {
  std::string protocol;
  DT::EndPoint::getProtocolFromString(endpoint, protocol);
  DT::XferFactory* tfactory = DT::XferFactoryManager::getFactoryManager().find(protocol);
  if (!tfactory)
    throw UnsupportedEndpointEx(endpoint);
  DT::EndPoint *ep = NULL;
  for (DT::EndPointsIter i = m_localEndpoints.begin();  !ep && i != m_localEndpoints.end(); i++)
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
  OS::Mutex *mutex;
  uint32_t hash_code;
  ~MailBoxLock()
  {
    //    mutex->unlock();
    delete mutex;
  }
};


Transport::~Transport()
{
  {
  OU::AutoMutex guard ( m_mutex, true ); 
  uint32_t m;

  for ( m=0; m<m_cached_transfers.size(); m++ ) {
    TransferDesc* tmp_td = static_cast<TransferDesc*>(m_cached_transfers[m]);
    delete tmp_td;
  }
  m_cached_transfers.destroyList();

  for ( m=0; m<active_transfers.size();  m++ ) {
    DT::XferRequest* xr = static_cast<DT::XferRequest*>(active_transfers[m]);
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
    DT::EndPointsIter i = m_localEndpoints.begin();
    ocpiDebug("Transport %p removing local ep %p %s", this, *i, (*i)->end_point.c_str());
    (*i)->release();
    m_localEndpoints.erase(i);
  }
  while (!m_remoteEndpoints.empty()) {
    DT::EndPointsIter i = m_remoteEndpoints.begin();
    ocpiDebug("Transport %p removing remote ep %p %s", this, *i, (*i)->end_point.c_str());
    (*i)->release();
    m_remoteEndpoints.erase(i);
  }
  }
  delete &m_mutex;
}


void 
Transport::
setNewCircuitRequestListener( NewCircuitRequestListener* listener )
{
  m_newCircuitListener = listener;
}



OS::Mutex* Transport::
getMailBoxLock( const char* mbid )
{

  uint32_t hash = OU::Misc::hashCode( mbid );
  int32_t len = m_mailbox_locks.getElementCount();
  for ( int n=0; n<len; n++ ) {
    MailBoxLock* mb = static_cast<MailBoxLock*>(m_mailbox_locks.getEntry(n));
    if ( mb->hash_code == hash ) {
      return mb->mutex;
    }
  }

  // We didnt find one, so create one
  MailBoxLock *mbl = new MailBoxLock;
  mbl->hash_code = hash;
  mbl->mutex = new OS::Mutex();
  m_mailbox_locks.insert( mbl );
  return mbl->mutex;
}



void 
Transport::
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
  DT::XferFactory* tfactory = 
    DT::XferFactoryManager::getFactoryManager().find( *client_loc, nuls );
  if ( ! tfactory ) {
    throw UnsupportedEndpointEx(client_loc->c_str());
  }
  DT::EndPoint* client_location = tfactory->getEndPoint( *client_loc );

  OS::Mutex* mutex = getMailBoxLock(server_loc->c_str());
  mutex->lock();

  DT::SMBResources* s_res = 
    DT::XferFactoryManager::getFactoryManager().getSMBResources( *client_loc );
  DT::SMBResources* t_res = 
    DT::XferFactoryManager::getFactoryManager().getSMBResources( *server_loc );

  DT::XferMailBox xmb(client_location->mailbox );
  bool openCircuit = circuit->m_openCircuit;
  circuit->m_openCircuit = true;
  while ( ! xmb.mailBoxAvailable(s_res) ) {
    dispatch();
    OS::sleep(0);
    if (timer && timer->expired()) {
      mutex->unlock();
      throw OU::EmbeddedException("Server Not Responding");
    }
  }
  circuit->m_openCircuit = openCircuit;

  ocpiDebug("Client is making a request to server to establish new connection.");
   
  DT::ContainerComms::MailBox* mb = xmb.getMailBox( s_res );
  mb->request.header.type = DT::ContainerComms::ReqNewConnection;
  mb->request.header.circuitId = circuit->getCircuitId();
  mb->request.reqNewConnection.buffer_size = circuit->getOutputPortSet()->getBufferLength();
  mb->request.reqNewConnection.send = send ? 1 : 0;
  strcpy(mb->request.reqNewConnection.output_end_point, m_CSendpoint->end_point.c_str() );
  if (protocol) {
    // If we have protocol info, we will asking the server for a place to put it.
    // We first must allocate space on our side of the transfer, and copy the protocol data
    // into our local smb buffer.  Then later we can transfer it to the server side when we
    // find out where the server's protocol buffer is.  We will do that when the server asks us
    // for our output flow control offsets.
    OU::ResAddrType protocolOffset;
    size_t protocolSize = strlen(protocol) + 1;
    mb->request.reqNewConnection.protocol_size = (uint32_t)protocolSize;
    if (s_res->sMemResourceMgr->alloc(protocolSize, 0, &protocolOffset))
      throw OU::EmbeddedException(NO_MORE_BUFFER_AVAILABLE, "for protocol info exchange");
    void *myProtocolBuffer = s_res->sMemServices->map(protocolOffset, protocolSize);
    memcpy(myProtocolBuffer, protocol, protocolSize);
    s_res->sMemServices->unMap();    
    circuit->setProtocolInfo(protocolSize, protocolOffset);
  } else
    mb->request.reqNewConnection.protocol_size = 0;
   
  // For now, this request does not require a return
  mb->return_offset = 0; // unused when size is zero
  mb->return_size = 0;
  mb->returnMailboxId = client_location->mailbox;

  ocpiDebug("reqNewConnection: mb %p 0x%x type %d", mb, circuit->getCircuitId(),
	    mb->request.header.type);
 
  xmb.makeRequest( s_res, t_res );

  // We will lock here a prescibed amount of time to see if we are successful
  while ( ! xmb.mailBoxAvailable(s_res) ) {
    if (timer && timer->expired()) {
      mutex->unlock();
      throw OU::EmbeddedException("Server Not Responding");
    }
    dispatch();
    OS::sleep(1);
  }
  if ( mb->error_code ) {
    mutex->unlock();
    throw OU::EmbeddedException("Failed to create client connection");
  }

  mutex->unlock();
}



Circuit *
Transport::
createCircuit(CircuitId   cid,
              ConnectionMetaData* connection,
              PortOrdinal src_ports[],
              PortOrdinal dest_ports[],
              uint32_t flags,
	      const char *protocol,
	      OS::Timer *timer
              )                        
{

  Circuit *circuit;
  if (cid == 0)
    cid = m_nextCircuitId++; // FIXME: make better uniqueness - use endpoint somehow.
  else if ((circuit = getCircuit(cid)))
    deleteCircuit(circuit);
  if ( supportsMailboxes() )
    circuit = new Circuit( this, cid, connection, src_ports, dest_ports, m_mutex );
  else
    // NOTE:: This needs to be specialized for optimization. It is redundant now
    circuit = new Circuit( this, cid, connection, src_ports, dest_ports, m_mutex );

  m_circuits.push_back( circuit );
  ocpiDebug("New circuit created and registered: id %x flags %x", cid, flags);

  // We may need to make a new connection request
  if ( flags & NewConnectionFlag ) {
    try {
      requestNewConnection( circuit, (flags & SendCircuitFlag) != 0, protocol, timer );
    }
    catch( ... ) {
      deleteCircuit(circuit);
      throw;
    }
  }
  return circuit;
}

// There could be a shared sub-structure here...
void
Transport::
fillDescriptorFromEndPoint(DT::EndPoint &ep, OCPI::RDT::Descriptors &desc) {
  strcpy(desc.desc.oob.oep, ep.end_point.c_str());
  desc.desc.oob.address = ep.address;
  desc.desc.oob.size = (uint32_t)ep.size;
  desc.desc.oob.mailBox = ep.mailbox;
  desc.desc.oob.maxCount = ep.maxCount;
}


// Create an output given a descriptor from a remote input port
// Also returning a flowcontrol descriptor to give to that remote port
Port * 
Transport::
createOutputPort(OCPI::RDT::Descriptors& outputDesc, const OCPI::RDT::Descriptors& inputDesc) {
  // Before creating the output port, create a local endpoint compatible with the remote.
  // It will throw an exception if it isn't workable
  DT::EndPoint &oep = getLocalCompatibleEndpoint(inputDesc.desc.oob.oep);
  fillDescriptorFromEndPoint(oep, outputDesc);
  // Ensure that the input port endpoint is registered
  DT::EndPoint &iep = addRemoteEndPoint(inputDesc.desc.oob.oep);
  ocpiAssert(outputDesc.desc.dataBufferSize <= inputDesc.desc.dataBufferSize);
#if 0
    ocpiDebug("createOutputPort: setting buffer size on output from %zu to %zu",
	      (size_t)outputDesc.desc.dataBufferSize, (size_t)inputDesc.desc.dataBufferSize);
    outputDesc.desc.dataBufferSize = inputDesc.desc.dataBufferSize;
#endif
  Circuit *c = createCircuit(0, new ConnectionMetaData(oep, outputDesc));
  c->addInputPort(iep, inputDesc, oep);
  Port *p = c->getOutputPort();
  p->getPortDescriptor(outputDesc, &inputDesc);
  return p;
}
// Create an output port given an existing input port.
Port *Transport::
createOutputPort(OCPI::RDT::Descriptors& outputDesc, Port &inputPort) {
  // With an inside connection, the endpoints are the same
  DT::EndPoint &iep = *inputPort.getEndpoint();
  fillDescriptorFromEndPoint(iep, outputDesc);
  if (outputDesc.desc.dataBufferSize > inputPort.getMetaData()->m_descriptor.desc.dataBufferSize) {
    ocpiDebug("Forcing output buffer size to %zu from input size %zu on local connection",
	      (size_t)inputPort.getMetaData()->m_descriptor.desc.dataBufferSize,
	      (size_t)outputDesc.desc.dataBufferSize);	   
    outputDesc.desc.dataBufferSize = inputPort.getMetaData()->m_descriptor.desc.dataBufferSize;
  }

  inputPort.getCircuit()->finalize(outputDesc.desc.oob.oep);

  Port *p = inputPort.getCircuit()->getOutputPort();
  p->getPortDescriptor(outputDesc, NULL);
  return p;
}

Port *Transport::
createInputPort(OCPI::RDT::Descriptors& desc)
{
  const char *epString = desc.desc.oob.oep;
  DT::EndPoint *ep;
  ep = strchr(desc.desc.oob.oep, ':') ?
    &getLocalEndpoint(epString) : // caller is specific, potentially specifying QoS etc.
    &getLocalCompatibleEndpoint(epString);
  // We may be overwriting the endpoint string, but that should be ok.
  fillDescriptorFromEndPoint(*ep, desc);
  Circuit *circuit = createCircuit(0, new ConnectionMetaData(NULL, ep, desc.desc.nBuffers,
							     desc.desc.dataBufferSize));
  Port *port = circuit->getInputPortSet(0)->getPortFromOrdinal(1);    
  circuit->attach(); // FIXME: why wouldn't port creation do the attach?
  // Merge port descriptor info between what was passed in and what is determined here.
  port->getPortDescriptor(desc, NULL);
  return port;
}

Circuit* 
Transport::
getCircuit(  CircuitId circuit_id )
{
  CircuitsIter cit;
  for ( cit=m_circuits.begin(); cit!=m_circuits.end(); cit++) {
    if ( (*cit)->getCircuitId() == circuit_id ) {
      return (*cit);
    }
  }
  return NULL;
}


size_t
Transport::
getCircuitCount()
{
  return m_circuits.size();
}


#if 0
void 
Transport::
deleteCircuit( CircuitId circuit_ord )
{
  Circuit* circuit = getCircuit( circuit_ord );
  if (circuit)
    deleteCircuit(circuit);
}
#endif
void
Transport::
deleteCircuit(Circuit *circuit)
{
  OU::AutoMutex guard ( m_mutex, true ); 

  CircuitsIter it = std::find(m_circuits.begin(), m_circuits.end(), circuit);
  m_circuits.erase(it);
  delete circuit;
  if ( m_circuits.size() == 0 ) {
    for ( uint32_t m=0; m<m_cached_transfers.getElementCount(); m++ ) {
      TransferDesc* tmp_td = static_cast<TransferDesc*>(m_cached_transfers[m]);
      m_cached_transfers.remove( tmp_td );
      delete tmp_td;
    }
  }
}



/**********************************
 * General house keeping 
 *********************************/
void Transport::dispatch(DT::EventManager*)
{
  OU::AutoMutex guard ( m_mutex, true ); 

  // move data from queue if possible
  CircuitsIter cit;
  for ( cit=m_circuits.begin(); cit!=m_circuits.end(); cit++) {
    if ( (*cit) == NULL ) continue;
    if ( (*cit)->ready() ) {
      // (*cit)->initializeDataTransfers();
      (*cit)->checkQueuedTransfers();
    }
  }

  // handle mailbox requests
  if ( m_uses_mailboxes )
    checkMailBoxes();
  //  OS::sleep(1);

}


void Transport::
clearRemoteMailbox(size_t offset, DT::EndPoint* loc )
{
  assert(loc);
  OU::AutoMutex guard ( m_mutex, true ); 
  TransferDesc* td=NULL;

#ifdef DEBUG_L2
  ocpiDebug("Clearing remote mailbox address = %s, offset = 0x%x", loc->end_point, offset );
#endif

  for ( uint32_t m=0; m<m_cached_transfers.getElementCount(); m++ ) {
    TransferDesc* tmp_td = static_cast<TransferDesc*>(m_cached_transfers[m]);
    if (tmp_td->loc == loc && (uint32_t)tmp_td->offset == offset) {
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
    DT::XferServices* ptemplate = 
      DT::XferFactoryManager::getFactoryManager().getService( m_CSendpoint, 
                                      loc );
    if ( ! ptemplate ) {
      ocpiAssert(0);
    }

    DT::XferRequest* ptransfer = ptemplate->createXferRequest();

    // Create the copy in the template

    ptransfer->copy (
		     OCPI_UTRUNCATE(DDT::Offset, offset + sizeof(DT::ContainerComms::RequestHeader)),
		     OCPI_UTRUNCATE(DDT::Offset, offset + sizeof(DT::ContainerComms::RequestHeader)),
		     sizeof(DT::ContainerComms::MailBox) - sizeof(DT::ContainerComms::RequestHeader),
		     DT::XferRequest::DataTransfer );
                
    ptransfer->copy (
		     OCPI_UTRUNCATE(DDT::Offset, offset),
		     OCPI_UTRUNCATE(DDT::Offset, offset),
		     sizeof(DT::ContainerComms::RequestHeader),
		     DT::XferRequest::FlagTransfer );

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

void Transport::
sendOffsets( OU::VList& offsets, DT::EndPoint &remote_ep, 
	     size_t extraSize, DtOsDataTypes::Offset extraFrom, DtOsDataTypes::Offset extraTo)
{
  OU::AutoMutex guard ( m_mutex, true ); 

 FORSTART:

  for ( uint32_t m=0; m<active_transfers.getElementCount();  m++ ) {
    DT::XferRequest* xr = static_cast<DT::XferRequest*>(active_transfers[m]);
    if ( xr->getStatus() == 0 ) {
      DT::XferRequest* for_delete = xr;
      active_transfers.remove( xr );
      delete for_delete;
      goto FORSTART;
    }
  }
  /* Attempt to get or make a transfer template */
  DT::XferServices* ptemplate = 
    DT::XferFactoryManager::getFactoryManager().getService( m_CSendpoint, 
							&remote_ep );
  if ( ! ptemplate ) {
    ocpiAssert(0);
  }

#ifdef DEBUG_L2
  ocpiDebug("In Transport::sendOffsets, sending %d OS::int32_ts", offsets.size() );
#endif

  DT::XferRequest* ptransfer = ptemplate->createXferRequest();

  // We do the extra transfer first so that the other side will have the protocol when it
  // sees that the output offsets have been copied.
  if (extraSize)
    ptransfer->copy (extraFrom, extraTo, extraSize, DT::XferRequest::None);

  for ( uint32_t y=0; y<offsets.getElementCount(); y++) {

    Port::ToFrom* tf = 
      static_cast<Port::ToFrom*>(offsets[y]);

#ifdef DEBUG_L3
    ocpiDebug("Adding copy to transfer list, 0x%x to 0x%x", tf->from_offset,tf->to_offset );
#endif
    ptransfer->copy (
		     tf->from_offset,
		     tf->to_offset,
		     sizeof(uint32_t),
		     DT::XferRequest::None );

  }
  ptransfer->post();
  active_transfers.push_back( ptransfer );
}

/**********************************
 * Our mailbox handler
 *********************************/
static volatile int nc=0;
void Transport::checkMailBoxes()
{
  nc++;

  // Ignore our request slot
  DT::ContainerComms* comms =   m_CSendpoint->resources.m_comms;

  // See if we have any comms requests
  unsigned nMailBoxes = m_CSendpoint->maxCount;
  for (DDT::MailBox n = 0; n < nMailBoxes; n++) {
    DT::ContainerComms::Request &request = comms->mailBox[n].request;
    DT::ContainerComms::ReqTypeIds type = request.header.type;

    if (n != m_CSendpoint->mailbox && type != 0) {
      size_t offset = sizeof(DT::UpAndRunning) + sizeof(DT::ContainerComms::MailBox) * n;
      Circuit *circuit;
      if (type == DT::ContainerComms::ReqNewConnection)
	circuit = 0; // will be making new circuit using the provided ID
      else if (!(circuit = getCircuit(request.header.circuitId))) {
	ocpiBad("Unknown circuit %x", request.header.circuitId);
	ocpiAssert(0);
      }
      ocpiDebug("Got a mailbox request from %d, req = %d cid = 0x%x", n, 
		request.header.type, request.header.circuitId);
      // Clear our mailbox - it will be set if needed
      comms->mailBox[n].error_code = 0;
      request.header.type = DT::ContainerComms::NoRequest;
      switch (type) {

      case DT::ContainerComms::ReqUpdateCircuit:
	ocpiDebug("Handling case DT::ContainerComms::ReqUpdateCircuit");
	circuit->updateInputs(&request.reqUpdateCircuit);
	clearRemoteMailbox(offset, getEndpoint(request.reqUpdateCircuit.output_end_point, false));
        break;

      case DT::ContainerComms::ReqNewConnection:
	ocpiDebug("Handling case DT::ContainerComms::ReqNewConnection");
	// If we dont have a new circuit listener installed, ignore the request
	if (!m_newCircuitListener)
	  break;
	{
	  ConnectionMetaData* md = NULL;
	  DT::EndPoint &sep = addRemoteEndPoint(request.reqNewConnection.output_end_point);
	  try {
	    // send flag indicates that the client is requesting a circuit to send data to me
	    md = request.reqNewConnection.send ?
	      new ConnectionMetaData(&sep, m_CSendpoint, 1, request.reqNewConnection.buffer_size) :
	      new ConnectionMetaData(m_CSendpoint, &sep,  1, request.reqNewConnection.buffer_size);

	    // Create the new circuit on request from the other side (client)
	    // If the client has protocol info for us, allocate local smb space for it, so it can
	    // copy it to me when I tell it where to put it in the request for output control offsets;
	    OU::ResAddr protocolOffset = 0;
	    size_t protocolSize = request.reqNewConnection.protocol_size;
	    if (protocolSize) {
	      // Allocate local space
	      if (m_CSendpoint->resources.sMemResourceMgr->alloc(protocolSize, 0,  &protocolOffset))
		throw OU::EmbeddedException(NO_MORE_BUFFER_AVAILABLE, "for protocol info exchange");
	      // map in local space
	    }
	    circuit = createCircuit(request.header.circuitId, md);
	    circuit->setProtocolInfo(protocolSize, protocolOffset);
	  }  catch ( ... ) {
	    delete md;
	    delete circuit;

	    comms->mailBox[n].error_code = -1;
	    clearRemoteMailbox(offset, &sep);
	    throw;
	  }
	  // We will copy our copy of their mailbox back to them
	  clearRemoteMailbox(offset, &sep);
	  // Hand it back to the listener
	  m_newCircuitListener->newCircuitAvailable(circuit);
	}
        break;

      case DT::ContainerComms::ReqOutputControlOffset:
        {
          ocpiDebug("Handling case DT::ContainerComms::ReqOutputControlOffset");

          DT::EndPoint &ep = addRemoteEndPoint(request.reqOutputContOffset.shadow_end_point);
          Port* port = circuit->getOutputPortSet()->getPortFromOrdinal(request.reqOutputContOffset.portId);
          ocpiAssert(port);

	  size_t protocolSize = 0;
	  OU::ResAddr protocolOffset = 0; // suppress warning
	  if (request.reqOutputContOffset.protocol_offset) {
	    // The server side is telling us where to put the protocol info, based on our telling
	    // it, in the reqnewconnection, how big it is.
	    circuit->getProtocolInfo(protocolSize, protocolOffset);
	    ocpiAssert(protocolSize != 0);
	  }
	  {
	    OU::VList offsetv;

	    port->getOffsets(comms->mailBox[n].return_offset, offsetv);
	    sendOffsets(offsetv, ep, protocolSize, protocolOffset,
			request.reqOutputContOffset.protocol_offset);
	    port->releaseOffsets(offsetv);
	  }
	  if (protocolSize)
	    m_CSendpoint->resources.sMemResourceMgr->free(protocolOffset, protocolSize);
          clearRemoteMailbox( offset, &ep);
        }
        break;

      case DT::ContainerComms::ReqShadowRstateOffset:
        {
          ocpiDebug("Handling case DT::ContainerComms::ReqShadowRstateOffset");
	  DT::EndPoint *ep = getEndpoint(request.reqShadowOffsets.url, true);
          ocpiDebug("Return address = %s", request.reqShadowOffsets.url );

          Port* port = NULL;
          for (PortOrdinal y = 0; y < circuit->getInputPortSetCount(); y++)
            if ((port = circuit->getInputPortSet(y)->getPortFromOrdinal(request.reqShadowOffsets.portId)))
              break;
          if (port) {
	    OU::VList offsetv;
	    port->getOffsets( comms->mailBox[n].return_offset, offsetv);
	    sendOffsets( offsetv, *ep );
	    port->releaseOffsets( offsetv );
	    clearRemoteMailbox(offset, ep);
	  }
        }
        break;

      case DT::ContainerComms::ReqInputOffsets:
        {
          ocpiDebug("Handling case DT::ContainerComms::ReqInputOffsets");

	  DT::EndPoint* ep = getEndpoint(request.reqInputOffsets.url, true);

          Port* port = NULL;
          for (PortOrdinal y = 0; y < circuit->getInputPortSetCount(); y++)
            if  ((port = circuit->getInputPortSet(y)->getPortFromOrdinal(request.reqInputOffsets.portId)))
              break;
          if (port) {
	    OU::VList offsetv;
	    port->getOffsets( comms->mailBox[n].return_offset, offsetv);
	    sendOffsets( offsetv, *ep );
	    port->releaseOffsets( offsetv );
	    clearRemoteMailbox(offset, ep );
	  }

        }
        break;
      case DT::ContainerComms::NoRequest:
      default:
        ocpiAssert("Handling case DT::ContainerComms::Default:"==0);
        break;

      }
    }
  }
  nc--;
}

DT::EndPoint& Transport::
addRemoteEndPoint( const char* loc )
{
  std::string sloc(loc);
  ocpiDebug("In Transport::addRemoteEndPoint, loc = %s", loc );
  
  DT::EndPoint *ep = getEndpoint(loc, false);
  if (ep)
    return *ep;
  DT::XferFactory* tfactory = 
    DT::XferFactoryManager::getFactoryManager().find(loc);
  if (!tfactory)
    throw UnsupportedEndpointEx(loc);
  ep = tfactory->getEndPoint(loc, false);
  ep->finalize();
  m_remoteEndpoints.insert(ep);
  return *ep;
}

bool Transport::isLocalEndpoint( const char* loc )
{
  for (DT::EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++) {
    if ((*i)->end_point == loc) {
      ocpiDebug("isLocalEndpoint:: '%s' is local", loc  );
      return true;
    }
  }
  return false;
}


DT::EndPoint * Transport::
getEndpoint(const char* ep, bool local)
{
  if (local) {
    for (DT::EndPointsIter i = m_localEndpoints.begin(); i != m_localEndpoints.end(); i++)
      if ((*i)->end_point == ep)
	return *i;
  } else {
    for (DT::EndPointsIter i = m_remoteEndpoints.begin(); i != m_remoteEndpoints.end(); i++)
      if ((*i)->end_point == ep)
	return *i;
  }
  return NULL;
}

}
}
