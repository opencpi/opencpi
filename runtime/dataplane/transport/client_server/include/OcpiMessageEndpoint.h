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

// A messaging endpoint for communication.
// This class essentially binds the addressing aspect (DataTransfer::Endpoint), with the
// runtime aspect (OCPI::DataTransport::Transport) to support a multiplicity of circuits,
// which com into existence client-style, where the creation of a message circuit is initiated locally,
// or server-style, where the creation of a circuit is initiated remotely.

#ifndef OCPI_Message_Endpoint_H_
#define OCPI_Message_Endpoint_H_

#include <map>
#include <set>
#include <string>
#include <queue>
#include <OcpiOsTimer.h>
#include <OcpiUtilSelfMutex.h>
#include <OcpiTransport.h>
#include <OcpiMessageCircuit.h>
#include "XferEndPoint.h"

namespace OCPI {
  namespace DataTransport {
  
    class MessageEndpoint : private NewCircuitRequestListener, virtual protected OCPI::Util::SelfMutex {
    public:
      MessageEndpoint(const char *endpoint);
      ~MessageEndpoint();

      // Get or create one.
      // If we supply an endpoint it is the "other side" endpoint and thus we must
      // get a "compatible" endpoint.
      static MessageEndpoint &getMessageEndpoint(const char *endpoint);

      // Clean them all up: FIXME: make this happen in global shutdown.
      static void destroyMessageEndpoints();

      // Connect this endpoint to a remote one, creating a message circuit
      static MessageCircuit &connect(const char *server_endpoint, 
				     unsigned bufferSize = 4096,
				     const char *protocol = NULL,
				     OCPI::OS::Timer *timer = NULL);
      
      inline const char *endpoint() const { return m_endpoint->name().c_str(); }

      MessageCircuit *accept(OCPI::OS::Timer *timer);
      // FIXME: resolve the static method vs this one better.
      MessageCircuit &connectTo(const char *server_endpoint, unsigned bufferSize = 4096,
				const char *protocol = NULL, OCPI::OS::Timer *timer = NULL);
    private:
      bool canSupport(const char *endpoint) const;
      inline void dispatch(DataTransfer::EventManager* eh = NULL) {
	OCPI::Util::SelfAutoMutex guard (this);
	m_transport->dispatch( eh );
      }
      void newCircuitAvailable(Circuit* circuit);

      // Set for convenient removal
      typedef std::set<MessageEndpoint *> MessageEndpoints;
      static MessageEndpoints s_messageEndpoints;
      TransportGlobal &m_transportGlobal;
      Transport *m_transport;             // a pointer for resource management during construction
      DataTransfer::EndPoint *m_endpoint; // ditto
      // These are just bookkeepping for the destructor
      typedef std::set<MessageCircuit *> MessageCircuits;
      MessageCircuits m_circuits;

      // These are the pending half-circuits for the server side to
      // marry into message circuits. Fifo since we always want to 
      // look at the oldest for a match.
      // We could also use multimap if we had a good circuit id to match them.
      typedef std::list<Circuit *> HalfCircuits;
      HalfCircuits m_halfCircuits;
    };
  }
}

#endif

