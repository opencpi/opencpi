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
 *   This file contains the Interface for the Cpi transport server class.
 *
 * Author: John F. Miller
 *
 * Date: 3/1/05
 *
 */

#ifndef CPI_Transport_Server_H_
#define CPI_Transport_Server_H_

#include <CpiMessageCircuit.h>
#include <CpiThreadHook.h>
#include <CpiList.h>
#include <CpiCircuit.h>
#include <DtIntEventHandler.h>
#include <CpiTransport.h>

namespace CPI {
  namespace OS {
    class Mutex;
  }
}

namespace DataTransfer {
  struct EndPoint;
}


namespace CPI {

  namespace DataTransport {

    class NewCircuitListener;
    class TransportControl;


    /**********************************
     * This is the class that is used to implement a server event callback handler
     **********************************/
    class ServerEventHandler
    {
    public:

      /**********************************
       *  This method gets called when a new circuit is available
       **********************************/        
      virtual void newMessageCircuitAvailable( MessageCircuit* new_circuit ) = 0;

      /**********************************
       *  This method gets called when data is available on a circuit
       **********************************/        
      virtual void dataAvailable( MessageCircuit* circuit ) = 0;


      /**********************************
       * This method gets called when an error gets generated
       *********************************/
      virtual void error( CPI::Util::EmbeddedException& ex ) = 0;

      virtual ~ServerEventHandler(){};

    };



    /**********************************
     * This class is used in conjuction with the CPI transport to create a server listening endpoint
     **********************************/
    class Server : public ThreadHook
    {

    public:
      friend class NewCircuitListener;
      friend class TransportControl;


      /**********************************
       *  Constructors
       **********************************/
      Server( std::string& end_point,     // In - endpoint
              ServerEventHandler* cb     // In - Server event handler
              );

      /**********************************
       *  Dispatch control, this method needs to be called periodically to provide 
       * the client class with houskeeping time.
       **********************************/
      void dispatch(DataTransfer::EventManager* event_manager=NULL);

      /**********************************
       *  Remove a message circuit
       **********************************/
      void remove( MessageCircuit* circuit );

      /**********************************
       *  Destructor
       **********************************/
      virtual ~Server();

      // Our exception monitor class
      CPI::Util::ExceptionMonitor m_exceptionMonitor;

      // Message circuits
      CPI::Util::VList m_circuits;

    protected:

      // our event handler
      ServerEventHandler* m_event_handler;

      // Thread control flag
      bool runThreadFlag;

      // Cpi transport 
      CPI::DataTransport::Transport* m_transport;

      // Our location
      DataTransfer::EndPoint* m_end_point;

      // Thread safe control
      CPI::OS::Mutex* m_mutex;

      // Transport controller
      TransportControl* m_tpc;

    };

    /**********************************
     ****
     * inline declarations
     ****
     *********************************/

  }
}

#endif

