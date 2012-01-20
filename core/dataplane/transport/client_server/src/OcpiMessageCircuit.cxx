
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


#include <OcpiOsMisc.h>
#include <DtTransferInternal.h>
#include <OcpiMessageCircuit.h>
#include <OcpiBuffer.h>
#include <OcpiOsAssert.h>
#include <OcpiPort.h>

namespace OS = OCPI::OS;
namespace OCPI {
  namespace DataTransport {
/**********************************
 * Constructor
 **********************************/
MessageCircuit::MessageCircuit(
                               Transport* transport,
                               Circuit* send,                // In - send circuit
                               Circuit* rcv,                // In - recieve circuit
			       OS::Mutex *mutex
                               )
  : m_standalone(false), m_transport(transport),
    m_full_buffer(NULL), m_mutex(mutex)
{
  m_rcv_port = rcv->getInputPortSet(0)->getPort(0);
  m_send_port = send->getOutputPortSet()->getPort(0);
  ocpiAssert(!m_send_port->isShadow());
  m_localEndpoint = m_rcv_port->getMetaData()->real_location_string;
  m_remoteEndpoint =rcv->getOutputPortSet()->getPort(0)->getMetaData()->real_location_string;
}


MessageCircuit::~MessageCircuit()
{
  m_transport->deleteCircuit(m_rcv_port->getCircuit());
  m_transport->deleteCircuit(m_send_port->getCircuit());
  if (m_standalone) {
    // delete m_transport; FIXME: this causes a problem with a mailbox lock not being locked on destruction...
    delete m_mutex;
  }
}

const char *MessageCircuit::localEndpoint() const {
  return m_rcv_port->getMetaData()-> real_location_string.c_str();
}
const char *MessageCircuit::remoteEndpoint() const {
  return m_rcv_port->getCircuit()->getOutputPortSet()->getPort(0)->
    getMetaData()->real_location_string.c_str();
}
BufferUserFacet* MessageCircuit::getNextOutputBuffer(void *&data, uint32_t &length,
						     OS::Timer *timer)
{
  m_transport->dispatch();
  if (timer) {
    BufferUserFacet* b;
    while (!(b = m_send_port->getNextEmptyOutputBuffer(data, length))) {
      if (timer->expired())
	return 0;
      m_transport->dispatch();
      OS::sleep(0);
    }
    return b;
  } else
    return m_send_port->getNextEmptyOutputBuffer(data, length);
}




/**********************************
 *  Send a message
 **********************************/
void MessageCircuit::sendBuffer( BufferUserFacet* buffer, unsigned int length )
{
  return m_send_port->sendOutputBuffer(buffer, length, 0);
}

/**********************************
 *  Determines if a message is available 
 *
 *  returns the number of messages.
 **********************************/
bool MessageCircuit::messageAvailable()
{
  if ( m_full_buffer ) {
    return true;
  }
  
  m_full_buffer = m_rcv_port->getNextFullInputBuffer();
  return m_full_buffer ? true : false;
}

/**********************************
 *  Get a message
 **********************************/
BufferUserFacet* MessageCircuit::getNextInputBuffer(void *&data, uint32_t &length,
						    OS::Timer *timer)
{
  BufferUserFacet *r_buf=NULL;

  if ( m_full_buffer ) {
    r_buf = m_full_buffer;
    m_full_buffer = NULL;
    data = (void*)r_buf->getBuffer();
    length = r_buf->getDataLength();
    return r_buf;
  }
  uint32_t opcode;
  if (timer) {
    while (!(r_buf = m_rcv_port->getNextFullInputBuffer(data, length, opcode))) {
      if (timer->expired())
	return 0;
      m_transport->dispatch();
      OS::sleep(0);
    }
  } else
    r_buf = m_rcv_port->getNextFullInputBuffer(data, length, opcode);

  static bool one_time_warning = 0;
  if ( m_rcv_port->getCircuit()->getStatus() == Circuit::Disconnecting ) {
    if ( ! one_time_warning ) {
      printf("WARNING: Circuit is disconnecting\n");
      one_time_warning = 1;
    }
  }
  return m_full_buffer = r_buf;
}

void MessageCircuit::freeBuffer( BufferUserFacet* msg )
{
  ocpiAssert(msg == m_full_buffer);
  m_full_buffer = NULL;
  m_rcv_port->inputAvailable( static_cast<Buffer*>(msg) );
}

void MessageCircuit::dispatch(DataTransfer::EventManager* eh)
{
#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
  m_transport->dispatch();
  m_mutex->unlock();
#else
  m_transport->dispatch( eh );
#endif
}

// Alternative constructor that directly connects as "client".
// Since the "connect" method below has a timeout, it can't be part of the
// constructor
MessageCircuit::MessageCircuit(const char *local_ep_or_protocol, uint32_t bufferSize)
  :m_standalone(true),
   m_transport(new Transport(new TransportGlobal(0, (char**)0), true)),
   m_rcv_port(NULL), m_send_port(NULL), m_full_buffer(NULL), m_mutex(new OS::Mutex),
   m_bufferSize(bufferSize), m_localEndpoint(local_ep_or_protocol ? local_ep_or_protocol : "")
{
  
}

// Complete one of the two circuits
Circuit &
MessageCircuit::makeCircuit(const std::string &from, const std::string &to, bool send) {
  Circuit &c =
    *m_transport->createCircuit( "", new ConnectionMetaData(from.c_str(), to.c_str(), 1, m_bufferSize),
				 NULL, NULL,
				 NewConnectionFlag | (send ? SendCircuitFlag : RcvCircuitFlag),
				 m_timer);
  while (!c.ready()) {
    c.updateConnection( NULL, 0 );
    m_transport->dispatch();
    OS::sleep(1);
    if (m_timer && m_timer->expired()) {
      delete &c;
      throw OCPI::Util::Error("Timeout (> %us %uns) on %s side of connection '%s'->'%s'",
			      m_timer->getElapsed().seconds(), m_timer->getElapsed().nanoseconds(),
			      send ? "send" : "receive", from.c_str(), to.c_str());
    }
  }
  c.initializeDataTransfers();
#ifndef NDEBUG
  printf("Client side %s circuit is ready\n", send ? "send" : "receive");
#endif
  return c;
}

bool MessageCircuit::
connect(const char *server_end_point, OS::Timer *timer) {
  m_timer = timer;
  DataTransfer::EndPoint *ep;
  if (m_localEndpoint.empty()) {
    // This will always reuse the local endpoint that is compatible with the other side,
    // and never add one.  For each supported protocol a default one is always create, although
    // not finalized (resources allocated).  This will finalize it
    ep = &m_transport->getLocalCompatibleEndpoint(server_end_point);
    m_localEndpoint = ep->end_point;
  } else
    ep = m_transport->addLocalEndpoint(m_localEndpoint.c_str())->sMemServices->endpoint();
  //  m_end_point = res->sMemServices->endpoint();
  m_transport->setListeningEndpoint(ep);
  m_remoteEndpoint = server_end_point;

  Circuit &send = makeCircuit(m_localEndpoint, m_remoteEndpoint, true);
  try {
    Circuit &rcv = makeCircuit(m_remoteEndpoint, m_localEndpoint, false);
    m_rcv_port = rcv.getInputPortSet(0)->getPort(0);
    m_send_port = send.getOutputPortSet()->getPort(0);
  } catch (...) {
    m_transport->deleteCircuit(&send);
    return true;
  }
  return false;
}

MessageCircuit::MessageCircuit(Transport &transport,
			       OS::Mutex &mutex,
			       const char *localEndpoint,
			       const char *remoteEndpoint,
			       uint32_t bufferSize,
			       OS::Timer *timer)
  : m_standalone(false), m_transport(&transport),
    m_rcv_port(NULL), m_send_port(NULL),
    m_full_buffer(NULL), m_mutex(&mutex), m_bufferSize(bufferSize),
    m_localEndpoint(localEndpoint), m_remoteEndpoint(remoteEndpoint)
{
  m_timer = timer;
  Circuit &send = makeCircuit(m_localEndpoint, m_remoteEndpoint, true);
  try {
      Circuit &rcv = makeCircuit(m_remoteEndpoint, m_localEndpoint, false);
      m_rcv_port = rcv.getInputPortSet(0)->getPort(0);
      m_send_port = send.getOutputPortSet()->getPort(0);
  } catch(...) {
    m_transport->deleteCircuit(&send);
    throw;
  }
}

  }
}
