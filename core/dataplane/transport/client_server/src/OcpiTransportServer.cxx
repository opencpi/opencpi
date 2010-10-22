
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


#include <OcpiTransportServer.h>
#include <OcpiTransport.h>
#include <OcpiThread.h>
#include <OcpiList.h>
#include <DtTransferInternal.h>
#include <OcpiOsMutex.h>
#include <OcpiOsMisc.h>
#include <DtIntEventHandler.h>
#include "OcpiUtilAutoMutex.h"

using namespace DataTransfer;
using namespace OCPI::DataTransport;
using namespace OCPI::Util;
using namespace OCPI::OS;
using namespace DataTransport::Interface;

// our callback class
namespace OCPI {
  namespace DataTransport {

    class NewCircuitListener : public NewCircuitRequestListener
    {
    public:

      NewCircuitListener( Server* server, ServerEventHandler* cb, Transport* transport )
        :m_server(server),m_cb(cb),m_transport(transport),m_halfMCircuit(0){}


      void newCircuitAvailable( Circuit* circuit )
      {

#ifndef NDEBUG
        printf("In NewCircuitListener::newCircuitAvailable, got a new circuit \n");
#endif

        // We will block here until the circuit is ready
        while ( 1 ) {

          if ( circuit->ready() ) {
            circuit->initializeDataTransfers();
            break;
          }
          else {
            circuit->updateConnection( NULL, 0 );
          }

          // We need to make sure to give the transport time to perform
          // any houskeeping that it needs to do.
          m_transport->dispatch();

          OCPI::OS::sleep(0);
        }


        // If we have a complete circuit, hand it back to the application
        if ( m_halfMCircuit.getElementCount() == 0  ) {
          m_halfMCircuit.insert( circuit );
        }
        else {
          Circuit *c1;
          for ( unsigned int n=0; n<m_halfMCircuit.getElementCount(); n++ ) {
            c1 = static_cast<Circuit*>(m_halfMCircuit.getEntry(n));

            // Determine if these are a pair
            if ( circuit->getOutputPortSet()->getPortFromIndex(0)->getMetaData()->real_location_string ==
                 c1->getInputPortSet(0)->getPortFromIndex(0)->getMetaData()->real_location_string  ) {

              // Remove it from the list
              m_halfMCircuit.remove( c1 );

              // Tell the application 
              MessageCircuit* mc = new MessageCircuit( m_transport, c1, circuit);
              m_server->m_circuits.push_back( mc );
              m_cb->newMessageCircuitAvailable( mc );
              break;

            }
          }
        }
      }


      /**********************************
       * This method gets called when an error gets generated
       *********************************/
      void error( OCPI::Util::EmbeddedException& ex )
      {
#ifndef NDEBUG
        printf("Got an exception, (%d %s)\n", ex.getErrorCode(), ex.getAuxInfo() );
#endif
        m_cb->error( ex );
      }

    private:
      Server*                  m_server;
      ServerEventHandler* m_cb;
      Transport*          m_transport;
      VList                  m_halfMCircuit;  

    };


#ifdef CONTAINER_MULTI_THREADED
    class TransportControl : public OCPI::Util::Thread
#else
    class TransportControl
#endif
    {
    public:
      friend class Server;

      TransportControl( Server* server, ServerEventHandler* cb,  
                        OCPI::DataTransport::Transport* transport, Mutex* sem)
        : m_server(server),m_cb(cb),m_transport(transport),m_mutex(sem)
      {
        m_listener = new NewCircuitListener(server, m_cb, m_transport);
        transport->setNewCircuitRequestListener( m_listener );
      }

      ~TransportControl()
      {
        delete m_listener;
      }

      void run() {

        m_mutex->lock ();
        while( m_server->runThreadFlag ) {
          bool someCircuitHadData = false;

          m_transport->dispatch();

          // See if any of the circuits have data
          for ( OCPI::OS::uint32_t n=0; n<m_server->m_circuits.getElementCount(); n++ ) {
            if ( static_cast<MessageCircuit*>(m_server->m_circuits[n])->messageAvailable() ) {
              someCircuitHadData = true;
              m_cb->dataAvailable( static_cast<MessageCircuit*>(m_server->m_circuits[n]) );
            }
          }

          m_mutex->unlock();

          /*
           * If there is work to do, keep spinning.
           * Otherwise, be Mr. Nice Guy and yield.
           */

          if (!someCircuitHadData) {
            OCPI::OS::sleep(100);
          }

          m_mutex->lock();
        }

        m_mutex->unlock ();
      }

    private:
      Server*                    m_server;
      ServerEventHandler*   m_cb;
      Transport*            m_transport;
      Mutex*                m_mutex;
      NewCircuitListener*   m_listener;
    };
  }
}





/**********************************
 *  Remove a message circuit
 **********************************/
void Server::remove( MessageCircuit* circuit )
{
#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
#endif
  m_circuits.remove( circuit );
  delete circuit;

#ifdef CONTAINER_MULTI_THREADED
  m_mutex->unlock();
#endif

}


/**********************************
 *  Constructors
 **********************************/
Server::Server( 
                std::string& end_point,     // In - endpoint
                ServerEventHandler* cb     // In - Server event handler
                )
  :m_circuits(0),m_event_handler(cb),runThreadFlag(true)
{
  OCPI::DataTransport::TransportGlobal *tpg = new OCPI::DataTransport::TransportGlobal(0,(char**)0);
  m_transport = new OCPI::DataTransport::Transport(tpg,true);

  // Add this endpoint to our known endpoints
  SMBResources* res;
  try {
    res = m_transport->addLocalEndpoint( end_point.c_str() );
  }
  catch( OCPI::Util::ExceptionMonitor& ) {
    throw;
  }
        
#ifdef CONTAINER_MULTI_THREADED
  m_mutex = new Mutex(true);
#endif

  // Get our endpoint
  m_end_point = res->sMemServices->getEndPoint();
  m_transport->setListeningEndpoint( m_end_point );

  // Create the dispatch thread
  m_tpc = new TransportControl( this, cb, m_transport,m_mutex);
#ifdef CONTAINER_MULTI_THREADED
  m_tpc->start();
#endif

}


/**********************************
 *  Dispatch control
 **********************************/
void Server::dispatch(DataTransfer::EventManager* event_manager)
{
#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
#endif

  m_transport->dispatch(event_manager);

#ifdef CONTAINER_MULTI_THREADED
  m_mutex->unlock();
#endif
}


/**********************************
 *  Destructor
 **********************************/
Server::~Server()
{
  // Shut down the thread
#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
#endif

  runThreadFlag = false;

#ifdef CONTAINER_MULTI_THREADED
  m_mutex->unlock();
#endif


#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
#endif


#ifdef CONTAINER_MULTI_THREADED
  m_tpc->join();
#endif

  // Delete all of the circuits
  for ( OCPI::OS::uint32_t n=0; n<m_circuits.getElementCount(); n++ ) {
    MessageCircuit* c = static_cast<MessageCircuit*>(m_circuits[n]);
    delete c;
  }

#ifdef CONTAINER_MULTI_THREADED
  m_mutex->unlock();
  delete m_mutex;
#endif


  delete m_tpc;
}

