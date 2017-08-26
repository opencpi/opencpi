
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
 *   This file contains the Interface for the Ocpi transport server class.
 *
 * Author: John F. Miller
 *
 * Date: 3/1/05
 *
 */

#ifndef OCPI_Transport_Server_H_
#define OCPI_Transport_Server_H_

#include <OcpiMessageCircuit.h>
#include <OcpiThreadHook.h>
#include <OcpiList.h>
#include <OcpiCircuit.h>
#include <DtIntEventHandler.h>
#include <OcpiTransport.h>

namespace OCPI {
  namespace OS {
    class Mutex;
  }
}

namespace DataTransfer {
  class EndPoint;
}


namespace OCPI {

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
      virtual void error( OCPI::Util::EmbeddedException& ex ) = 0;

      virtual ~ServerEventHandler(){};

    };



    /**********************************
     * This class is used in conjuction with the OCPI transport to create a server listening endpoint
     **********************************/
    class Server : public ThreadHook
    {

    public:
      friend class NewCircuitListener;
      friend class TransportControl;


      /**********************************
       *  Constructors
       **********************************/
      Server( const char *end_point,     // In - endpoint
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
      OCPI::Util::ExceptionMonitor m_exceptionMonitor;

      // Message circuits
      OCPI::Util::VList m_circuits;

    protected:

      // our event handler
      ServerEventHandler* m_event_handler;

      // Thread control flag
      bool runThreadFlag;

      // Ocpi transport 
      OCPI::DataTransport::Transport* m_transport;

      // Our location
      DataTransfer::EndPoint* m_end_point;

      // Thread safe control
      OCPI::OS::Mutex* m_mutex;

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

