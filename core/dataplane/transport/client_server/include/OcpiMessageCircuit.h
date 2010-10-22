
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

#include <OcpiTransport.h>
#include <OcpiCircuit.h>
#include <OcpiUtilException.h>

namespace OCPI {

  namespace DataTransport {

    /**********************************
     * This class is used in conjuction with the OCPI transport to create a full duplex
     * message circuit.
     **********************************/
    class MessageCircuit
    {
        
    public:
                  
                  
      /**********************************
       *  Constructors
       **********************************/
      MessageCircuit(
                     OCPI::DataTransport::Transport* transport,
                     OCPI::DataTransport::Circuit* send,        // In - send circuit
                     OCPI::DataTransport::Circuit* rcv        // In - recieve circuit
                     );


      /**********************************
       *  Destructor
       **********************************/
      ~MessageCircuit();
        
      /**********************************
       *  Send a message
       **********************************/
      OCPI::DataTransport::Buffer* getSendMessageBuffer();
      void sendMessage( OCPI::DataTransport::Buffer* msg, unsigned int length );


      /**********************************
       *  Determines if a message is available 
       *
       *  returns the number of messages.
       **********************************/
      bool messageAvailable();

        
      /**********************************
       *  Get a message
       **********************************/
      OCPI::DataTransport::Buffer* getNextMessage();
      void freeMessage( OCPI::DataTransport::Buffer* msg );


      /**********************************
       *  Get the individual circuits
       **********************************/
      OCPI::DataTransport::Circuit* getSendCircuit();
      OCPI::DataTransport::Circuit* getRcvCircuit();
      OCPI::DataTransport::Transport* m_transport;

    protected:

      // our circuits
      OCPI::DataTransport::Circuit* m_send;
      OCPI::DataTransport::Circuit* m_rcv;
      OCPI::DataTransport::Port* m_rcv_port;
      OCPI::DataTransport::Port* m_send_port;
      OCPI::DataTransport::Buffer* m_full_buffer;
                  
                  
    };
          
          
    
    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

    /**********************************
     *  Get the individual circuits
     **********************************/
    inline OCPI::DataTransport::Circuit* MessageCircuit::getSendCircuit(){return m_send;}
    inline OCPI::DataTransport::Circuit* MessageCircuit::getRcvCircuit(){return m_rcv;}
 
          
  }
  
}


#endif

