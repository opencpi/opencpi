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
 *   This file contains the Interface for the Ocpi transport message class.
 *   This class is a full duplex circuit used to send and receive unformatted
 *   messages.
 *
 * Author: John F. Miller
 *
 * Date: 3/2/05
 *
 */

#ifndef OCPI_Transport_Message_Circuit_H_
#define OCPI_Transport_Message_Circuit_H_

#include <OcpiOsTimer.h>
#include <OcpiUtilProtocol.h>
#include <OcpiTransport.h>

namespace OCPI {

  namespace DataTransport {

    /**********************************
     * This class is used in conjuction with the OCPI transport to create a full duplex
     * message circuit.
     **********************************/
    class MessageCircuit
    {
      static const uint32_t defaultBufferSize = 4096;
    public:
                  
                  
      /**********************************
       *  Constructors
       **********************************/
      // This constructor is used on the server side when circuits are created
      // in response to mailbox messages from the client side.
      MessageCircuit(OCPI::DataTransport::Transport &transport,
		     //		     OCPI::OS::Mutex &mutex,
                     OCPI::DataTransport::Circuit &send,
                     OCPI::DataTransport::Circuit &rcv);

      // This constructori is used on the client side to connect with the remote
      // endpoint.
      MessageCircuit(OCPI::DataTransport::Transport &transport,
		     //		     OCPI::OS::Mutex &mutex,
		     const char *localEndpoint,
		     const char *remoteEndpoint,
		     uint32_t bufferSize = defaultBufferSize,
		     const char *protocol = NULL,
		     OCPI::OS::Timer *timer = NULL);


      /**********************************
       *  Destructor
       **********************************/
      ~MessageCircuit();
        
      /**********************************
       *  Send a message
       **********************************/
      OCPI::DataTransport::BufferUserFacet*
	getNextEmptyOutputBuffer(void *&data, size_t &length, OCPI::OS::Timer *timer = NULL);
      void sendOutputBuffer( OCPI::DataTransport::BufferUserFacet* msg, size_t length, uint8_t opcode);

      // Indicates whether data is available for input - a "peek"
      
      //bool messageAvailable();

        
      /**********************************
       *  Get a message
       **********************************/
      // bool messageAvailable(); // optional
      OCPI::DataTransport::BufferUserFacet*
	getNextFullInputBuffer(void *&data, size_t &length, uint8_t &opcode,
			       OCPI::OS::Timer *timer = NULL);
      void releaseInputBuffer( OCPI::DataTransport::BufferUserFacet* msg );
      void dispatch(DataTransfer::EventManager* eh = NULL);


      /**********************************
       *  Get the individual circuits
       **********************************/
      // OCPI::DataTransport::Circuit* getSendCircuit();
      // OCPI::DataTransport::Circuit* getRcvCircuit();

      const char *localEndpoint() const;
      const char *remoteEndpoint() const;
      // This is only for the server side.
      // It passes ownership of the char ARRAY to the caller.
      inline char *getProtocol() const {
	return m_rcv_port->getCircuit()->getProtocol();
      }
    private:
      Circuit & makeCircuit(DataTransfer::EndPoint &from, DataTransfer::EndPoint &to, bool send,
			    const char *protocol, OCPI::OS::Timer *timer);

      // Member data at initialization
      OCPI::DataTransport::Transport& m_transport;
      uint32_t m_bufferSize;
      //      OCPI::OS::Mutex &m_mutex;

      // Member data set at/during construction
      OCPI::DataTransport::Port* m_rcv_port;
      OCPI::DataTransport::Port* m_send_port;
    };
  }
}


#endif

