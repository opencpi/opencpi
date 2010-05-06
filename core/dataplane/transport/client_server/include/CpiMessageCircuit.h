// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file contains the Interface for the Cpi transport message class.
 *   This class is a full duplex circuit used to send and receive unformatted
 *   messages.
 *
 * Author: John F. Miller
 *
 * Date: 3/2/05
 *
 */

#ifndef CPI_Transport_Message_Circuit_H_
#define CPI_Transport_Message_Circuit_H_

#include <CpiTransport.h>
#include <CpiCircuit.h>
#include <CpiUtilException.h>

namespace CPI {

  namespace DataTransport {

    /**********************************
     * This class is used in conjuction with the CPI transport to create a full duplex
     * message circuit.
     **********************************/
    class MessageCircuit
    {
        
    public:
                  
                  
      /**********************************
       *  Constructors
       **********************************/
      MessageCircuit(
                     CPI::DataTransport::Transport* transport,
                     CPI::DataTransport::Circuit* send,        // In - send circuit
                     CPI::DataTransport::Circuit* rcv        // In - recieve circuit
                     );


      /**********************************
       *  Destructor
       **********************************/
      ~MessageCircuit();
        
      /**********************************
       *  Send a message
       **********************************/
      CPI::DataTransport::Buffer* getSendMessageBuffer();
      void sendMessage( CPI::DataTransport::Buffer* msg, unsigned int length );


      /**********************************
       *  Determines if a message is available 
       *
       *  returns the number of messages.
       **********************************/
      bool messageAvailable();

        
      /**********************************
       *  Get a message
       **********************************/
      CPI::DataTransport::Buffer* getNextMessage();
      void freeMessage( CPI::DataTransport::Buffer* msg );


      /**********************************
       *  Get the individual circuits
       **********************************/
      CPI::DataTransport::Circuit* getSendCircuit();
      CPI::DataTransport::Circuit* getRcvCircuit();
      CPI::DataTransport::Transport* m_transport;

    protected:

      // our circuits
      CPI::DataTransport::Circuit* m_send;
      CPI::DataTransport::Circuit* m_rcv;
      CPI::DataTransport::Port* m_rcv_port;
      CPI::DataTransport::Port* m_send_port;
      CPI::DataTransport::Buffer* m_full_buffer;
                  
                  
    };
          
          
    
    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

    /**********************************
     *  Get the individual circuits
     **********************************/
    inline CPI::DataTransport::Circuit* MessageCircuit::getSendCircuit(){return m_send;}
    inline CPI::DataTransport::Circuit* MessageCircuit::getRcvCircuit(){return m_rcv;}
 
          
  }
  
}


#endif

