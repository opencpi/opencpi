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

#include "OcpiOsMisc.h"
#include "OcpiOsAssert.h"
#include "OcpiMessageCircuit.h"
#include "XferEndPoint.h"

namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OCPI {
  namespace DataTransport {

    // Server side creation from underlying circuits
    MessageCircuit::
    MessageCircuit(Transport &transport,
		   //		   OS::Mutex &mutex,
		   Circuit &send,
		   Circuit &rcv)
      : m_transport(transport),	m_bufferSize(0),// m_mutex(mutex), 
	m_rcv_port(rcv.getInputPortSet(0)->getPort(0)),
	m_send_port(send.getOutputPortSet()->getPort(0))
    {
      ocpiAssert(!m_send_port->isShadow());
    }

    // Client side creation - providing optional protocol string info
    MessageCircuit::
    MessageCircuit(Transport &transport,
		   //		   OS::Mutex &mutex,
		   const char *localEndpoint,
		   const char *remoteEndpoint,
		   uint32_t bufferSize,
		   const char *protocol,
		   OS::Timer *timer)
      : m_transport(transport),	m_bufferSize(bufferSize ? bufferSize : defaultBufferSize), // m_mutex(mutex), 
	m_rcv_port(NULL), m_send_port(NULL)
    {
      DataTransfer::EndPoint
	&local = transport.getLocalEndpoint(localEndpoint),
	&remote = transport.addRemoteEndPoint(remoteEndpoint);

      Circuit &send = makeCircuit(local, remote, true, protocol, timer);
      try {
	Circuit &rcv = makeCircuit(remote, local, false, NULL, timer);
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
    makeCircuit(DataTransfer::EndPoint &from, DataTransfer::EndPoint &to, bool send,
		const char *protocol, OS::Timer *timer) {
      Circuit &c =
	*m_transport.createCircuit(0, new ConnectionMetaData(&from, &to, 1, m_bufferSize),
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
				  send ? "send" : "receive", from.name().c_str(),
				  to.name().c_str());
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
    getNextEmptyOutputBuffer(uint8_t *&data, size_t &length, OS::Timer *timer)
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
    sendOutputBuffer( BufferUserFacet* buffer, size_t length, uint8_t opcode )
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
    getNextFullInputBuffer(uint8_t *&data, size_t &length, uint8_t &opcode, OS::Timer *timer)
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
  }
}
