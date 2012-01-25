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
		     OCPI::OS::Mutex &mutex,
                     OCPI::DataTransport::Circuit &send,
                     OCPI::DataTransport::Circuit &rcv);

      // This constructori is used on the client side to connect with the remote
      // endpoint.
      MessageCircuit(OCPI::DataTransport::Transport &transport,
		     OCPI::OS::Mutex &mutex,
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
	getNextEmptyOutputBuffer(void *&data, uint32_t &length, OCPI::OS::Timer *timer = NULL);
      void sendOutputBuffer( OCPI::DataTransport::BufferUserFacet* msg, unsigned int length, uint8_t opcode);

      // Indicates whether data is available for input - a "peek"
      
      bool messageAvailable();

        
      /**********************************
       *  Get a message
       **********************************/
      // bool messageAvailable(); // optional
      OCPI::DataTransport::BufferUserFacet*
	getNextFullInputBuffer(void *&data, uint32_t &length, uint8_t &opcode,
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
      Circuit & makeCircuit(const std::string &from, const std::string &to, bool send,
			    const char *protocol, OCPI::OS::Timer *timer);

      // Member data at initialization
      OCPI::DataTransport::Transport& m_transport;
      uint32_t m_bufferSize;
      OCPI::OS::Mutex &m_mutex;

      // Member data set at/during construction
      OCPI::DataTransport::Port* m_rcv_port;
      OCPI::DataTransport::Port* m_send_port;
    };
  }
}


#endif

