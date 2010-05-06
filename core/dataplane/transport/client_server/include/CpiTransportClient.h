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
 *   This file contains the Interface for the Cpi transport client class.
 *
 * Author: John F. Miller
 *
 * Date: 3/1/05
 *
 */

#ifndef CPI_Transport_Client_H_
#define CPI_Transport_Client_H_

#include <CpiMessageCircuit.h>
#include <CpiThreadHook.h>
#include <CpiList.h>
#include <string>
#include <DtIntEventHandler.h>


namespace CPI {
  namespace DataTransport {
    class Transport;
  }
}

namespace DataTransfer {
  struct EndPoint;
}

namespace CPI {
  namespace OS {
    class Mutex;
  }
}
class CTransportControl;

namespace CPI {

  namespace DataTransport {
  
    /**********************************
     * This is the class that is used to implement a server event callback handler
     **********************************/
    class ClientEventHandler
    {
    public:

      /**********************************
       *  This method gets called when data is available on a circuit
       **********************************/        
      virtual void dataAvailable( MessageCircuit* circuit ) = 0;

      /**********************************
       * This method gets called when an error gets generated
       *********************************/
      virtual void error( CPI::Util::EmbeddedException& ex ) = 0;

      virtual ~ClientEventHandler(){};

    };


    /**********************************
     * This class is used in conjuction with the CPI transport to create a client circuit
     **********************************/
    class Client : public ThreadHook
    {

    public:

      friend class CTransportControl;


      /**********************************
       *  Constructors
       **********************************/
      Client( std::string&     our_end_point, // In - endpoint
              int             buffer_size,       // In - Buffer size in bytes
              ClientEventHandler* cb     // In - Client event handler
              );

      /**********************************
       *  Create a new circuit
       **********************************/
      MessageCircuit* createCircuit( std::string& server_end_point );


      /**********************************
       *  Dispatch control, this method needs to be called periodically to provide 
       * the client class with houskeeping time.
       **********************************/
      void dispatch(DataTransfer::EventManager* eh=NULL);


      /**********************************
       *  Remove a message circuit
       **********************************/
      void remove( MessageCircuit* circuit );


      /**********************************
       *  Destructor
       **********************************/
      virtual ~Client();


      // Our exception monitor class
      CPI::Util::ExceptionMonitor m_exceptionMonitor;

      // Message circuits
      CPI::Util::VList m_circuits;

      // Thread control flag
      bool runThreadFlag;

    protected:


      /**********************************
       *  local init
       **********************************/
      void init();

      // our event handler
      ClientEventHandler* m_event_handler;

      // buffer count
      int m_buffer_size;

      // Cpi transport 
      CPI::DataTransport::Transport* m_transport;

      // Init flag
      bool m_init;

      // Our location
      std::string m_endpoint_string;
      DataTransfer::EndPoint* m_end_point;

      // Total number of connections
      static int m_count;

      // Thread safe control
      CPI::OS::Mutex* m_mutex;

      // Transport controller
      CTransportControl* m_transport_controller;

    };

    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

  }
}

#endif

