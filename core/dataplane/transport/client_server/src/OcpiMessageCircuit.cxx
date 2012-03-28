
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
#include <OcpiOsAssert.h>
#include <OcpiMessageCircuit.h>

namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OCPI {
  namespace DataTransport {

    // Server side creation from underlying circuits
    MessageCircuit::
    MessageCircuit(Transport &transport,
		   OS::Mutex &mutex,
		   Circuit &send,
		   Circuit &rcv)
      : m_transport(transport),	m_bufferSize(0), m_mutex(mutex), 
	m_rcv_port(rcv.getInputPortSet(0)->getPort(0)),
	m_send_port(send.getOutputPortSet()->getPort(0))
    {
      ocpiAssert(!m_send_port->isShadow());
    }

    // Client side creation - providing optional protocol string info
    MessageCircuit::
    MessageCircuit(Transport &transport,
		   OS::Mutex &mutex,
		   const char *localEndpoint,
		   const char *remoteEndpoint,
		   uint32_t bufferSize,
		   const char *protocol,
		   OS::Timer *timer)
      : m_transport(transport),	m_bufferSize(bufferSize ? bufferSize : defaultBufferSize), m_mutex(mutex), 
	m_rcv_port(NULL), m_send_port(NULL)
    {
      Circuit &send = makeCircuit(localEndpoint, remoteEndpoint, true, protocol, timer);
      try {
	Circuit &rcv = makeCircuit(remoteEndpoint, localEndpoint, false, NULL, timer);
	m_rcv_port = rcv.getInputPortSet(0)->getPort(0);
	m_send_port = send.getOutputPortSet()->getPort(0);
      } catch(...) {
	m_transport.deleteCircuit(&send);
	throw;
      }
    }

    MessageCircuit::
    ~MessageCircuit()
    {
      m_transport.deleteCircuit(m_rcv_port->getCircuit());
      m_transport.deleteCircuit(m_send_port->getCircuit());
    }

    // Complete one of the two circuits
    Circuit & MessageCircuit::
    makeCircuit(const std::string &from, const std::string &to, bool send,
		const char *protocol, OS::Timer *timer) {
      Circuit &c =
	*m_transport.createCircuit( "", new ConnectionMetaData(from.c_str(), to.c_str(), 1, m_bufferSize),
				    NULL, NULL,
				    NewConnectionFlag | (send ? SendCircuitFlag : RcvCircuitFlag),
				    protocol, timer);
      while (!c.ready()) {
	m_transport.dispatch();
	OS::sleep(1);
	if (timer && timer->expired()) {
	  delete &c;
	  throw OCPI::Util::Error("Timeout (> %us %uns) on %s side of connection '%s'->'%s'",
				  timer->getElapsed().seconds(), timer->getElapsed().nanoseconds(),
				  send ? "send" : "receive", from.c_str(), to.c_str());
	}
      }
      // c.initializeDataTransfers();
      ocpiDebug("Client side %s circuit is ready", send ? "send" : "receive");
      return c;
    }

    const char *MessageCircuit::
    localEndpoint() const {
      return m_rcv_port->getMetaData()-> real_location_string.c_str();
    }
    const char *MessageCircuit::
    remoteEndpoint() const {
      return m_rcv_port->getCircuit()->getOutputPortSet()->getPort(0)->
	getMetaData()->real_location_string.c_str();
    }

    BufferUserFacet* MessageCircuit::
    getNextEmptyOutputBuffer(void *&data, uint32_t &length, OS::Timer *timer)
    {
      m_transport.dispatch();
      if (timer) {
	BufferUserFacet* b;
	while (!(b = m_send_port->getNextEmptyOutputBuffer(data, length))) {
	  if (timer->expired())
	    return 0;
	  m_transport.dispatch();
	  OS::sleep(0);
	}
	return b;
      } else
	return m_send_port->getNextEmptyOutputBuffer(data, length);
    }

    void MessageCircuit::
    sendOutputBuffer( BufferUserFacet* buffer, unsigned int length, uint8_t opcode )
    {
      return m_send_port->sendOutputBuffer(buffer, length, opcode);
    }

    /**********************************
     *  Determines if a message is available 
     *
     *  returns the number of messages.
     **********************************/
    //    bool MessageCircuit::messageAvailable()
    //    {
    //      return m_rcv_port->hasFullInputBuffer();
    //    }

    BufferUserFacet* MessageCircuit::
    getNextFullInputBuffer(void *&data, uint32_t &length, uint8_t &opcode, OS::Timer *timer)
    {
      BufferUserFacet *r_buf = NULL;

      if (timer) {
	while (!(r_buf = m_rcv_port->getNextFullInputBuffer(data, length, opcode))) {
	  if (timer->expired())
	    return 0;
	  m_transport.dispatch();
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
      return r_buf;
    }

    void MessageCircuit::
    releaseInputBuffer( BufferUserFacet* msg )
    {
      m_rcv_port->releaseInputBuffer(msg);
    }

    void MessageCircuit::
    dispatch(DataTransfer::EventManager* eh)
    {
#ifdef CONTAINER_MULTI_THREADED
      m_mutex.lock();
      m_transport.dispatch();
      m_mutex.unlock();
#else
      m_transport.dispatch( eh );
#endif
    }

#if 0
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
#endif

#if 0
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
#endif


  }
}
