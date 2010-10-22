
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


#include <OcpiTransportClient.h>
#include <OcpiTransport.h>
#include <OcpiMessageCircuit.h>
#include <DtTransferInternal.h>
#include <OcpiOsMutex.h>
#include <OcpiOsMisc.h>
#include <OcpiThread.h>
#include <OcpiCircuit.h>

using namespace OCPI::DataTransport;
using namespace OCPI::OS;
using namespace DataTransfer;
using namespace DataTransport::Interface;

int Client::m_count=0;
        
/**********************************
 *  Local init
 **********************************/
void Client::init()
{
  if ( !m_init && (m_endpoint_string.length() != 0)  ) {

    SMBResources* res;
    try {
      res = m_transport->addLocalEndpoint( m_endpoint_string.c_str() );
    }
    catch( ... ) {
      throw;
    }
                
    // Get our endpoint
    m_end_point = res->sMemServices->getEndPoint();
    m_transport->setListeningEndpoint( m_end_point );
                
    m_init=true;
  }

}

class CTransportControl : public OCPI::Util::Thread
{
public:
  friend class Client;

  CTransportControl( Client* client, ClientEventHandler* cb,  OCPI::DataTransport::Transport* transport, Mutex* mut)
    : m_client(client),m_cb(cb),m_transport(transport),m_mutex(mut)
  {
  }

  void run() {

    m_mutex->lock ();

    while( m_client->runThreadFlag ) {
      bool someCircuitHadData = false;

      m_transport->dispatch();

      for ( OCPI::OS::uint32_t n=0; n<m_client->m_circuits.getElementCount(); n++ ) {
        if ( static_cast<MessageCircuit*>(m_client->m_circuits[n])->messageAvailable() ) {
          someCircuitHadData = true;
          m_cb->dataAvailable( static_cast<MessageCircuit*>(m_client->m_circuits[n]) );
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
  Client*                            m_client;
  ClientEventHandler*               m_cb;
  OCPI::DataTransport::Transport*    m_transport;
  Mutex*                            m_mutex;

};

        
/**********************************
 *  Constructors
 **********************************/
Client::Client( 
                std::string&                   end_point,    
                int                            buffer_size,  
                ClientEventHandler*            cb     

                )
  : m_circuits(0),runThreadFlag(true),m_event_handler(cb),m_buffer_size(buffer_size),
    m_init(false)
{

  OCPI::DataTransport::TransportGlobal *tpg = new OCPI::DataTransport::TransportGlobal(0,(char**)0);
  m_transport = new OCPI::DataTransport::Transport(tpg,true);

  // If we dont have an endpoint, we need to get one
  if ( end_point.length() ) {
    m_endpoint_string = end_point;
  }
  init();

  // Create our controller
#ifdef CONTAINER_MULTI_THREADED
  m_mutex = new Mutex(true);
#endif

#ifdef CONTAINER_MULTI_THREADED
  m_transport_controller = new CTransportControl(this, cb, m_transport, m_mutex );
#endif

#ifdef CONTAINER_MULTI_THREADED
  m_transport_controller->start();
#endif
        
}


/**********************************
 *  Create a new circuit
 **********************************/
#define CLIENT_DEFAULT_BUFFER_SIZE 1024*10
MessageCircuit* Client::createCircuit( std::string& server_end_point )
{
  OCPI::OS::uint32_t buffer_size = CLIENT_DEFAULT_BUFFER_SIZE;

#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
#endif

  //Make sure we have an endpoint to work with
  if ( m_endpoint_string.length() == 0 ) {
    // Ask the transfer factory to give me an endpoint that we can support
    std::string nuls;
    m_endpoint_string = XferFactoryManager::getFactoryManager().allocateEndpoint(nuls, &buffer_size);
    init();
  }

  MessageCircuit* mc=NULL;
  OCPI::DataTransport::Circuit* send_circuit;
  OCPI::DataTransport::Circuit* rcv_circuit;

  // This class uses whole parrallel / parrallel whole transfers only
  ConnectionMetaData* send_md = 
    new ConnectionMetaData(m_end_point->end_point.c_str(), server_end_point.c_str(), 1, m_buffer_size );

  ConnectionMetaData* rcv_md = 
    new ConnectionMetaData( server_end_point.c_str(), m_end_point->end_point.c_str(), 1, m_buffer_size );

  // Now create the circuit
  char cid[80];
  sprintf( cid, "%s-%d", m_end_point->end_point.c_str(), m_count++);
  try {
    send_circuit = m_transport->createCircuit( cid, send_md, NULL, NULL, NewConnectionFlag | SendCircuitFlag );
  }
  catch( ... ) {
#ifdef CONTAINER_MULTI_THREADED
    m_mutex->unlock();
#endif
    throw;
  }

  // We will block here until the send circuit is ready
  while ( 1 ) {
    if ( send_circuit->ready() ) {
      send_circuit->initializeDataTransfers();
      break;
    }
    else {
      send_circuit->updateConnection( NULL, 0 );
    }
    // We need to make sure to give the transport time to perform
    // any houskeeping that it needs to do.
    m_transport->dispatch();

    OCPI::OS::sleep(0);
  }

  try {
    sprintf( cid, "%s-%d\n", m_end_point->end_point.c_str(), m_count++);
    rcv_circuit = m_transport->createCircuit( cid, rcv_md, NULL, NULL, NewConnectionFlag | RcvCircuitFlag  );
  }
  catch( ... ) {
#ifdef CONTAINER_MULTI_THREADED
    m_mutex->unlock();
#endif
    throw;
  }

  // We will block here until the rcv circuit is ready
  while ( 1 ) {
    if ( rcv_circuit->ready() ) {
      rcv_circuit->initializeDataTransfers();
      break;
    }
    else {
      rcv_circuit->updateConnection( NULL, 0 );
    }
    // We need to make sure to give the transport time to perform
    // any houskeeping that it needs to do.
    m_transport->dispatch();

    OCPI::OS::sleep(0);
  }

  mc = new MessageCircuit( m_transport, send_circuit, rcv_circuit);
  m_circuits.push_back( mc );

#ifdef CONTAINER_MULTI_THREADED
  m_mutex->unlock();
#endif
  return mc;
}


/**********************************
 *  Dispatch control
 **********************************/
void Client::dispatch(DataTransfer::EventManager* eh)
{
#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
  m_transport->dispatch();
  m_mutex->unlock();
#else
  m_transport->dispatch( eh );
#endif
}


/**********************************
 *  Remove a message circuit
 **********************************/
void Client::remove( MessageCircuit* circuit )
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
 *  Destructor
 **********************************/
Client::~Client()
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
  m_transport_controller->join();
#endif


  /*
   * FP Note: Race condition here. The thread might still be blocked in
   * m_mutex->lock(), and will access the m_circuits list as soon as it
   * gets a chance.
   */
#ifdef CONTAINER_MULTI_THREADED
  m_mutex->lock();
#endif

  try {
    // Delete all of the circuits
    for ( OCPI::OS::uint32_t n=0; n<m_circuits.getElementCount(); n++ ) {
      MessageCircuit* c = static_cast<MessageCircuit*>(m_circuits[n]);
      delete c;
    }

#ifdef CONTAINER_MULTI_THREADED
    delete m_transport_controller;
#endif

  }
  catch( ... ){}


#ifdef CONTAINER_MULTI_THREADED
  m_mutex->unlock();
  delete m_mutex;
#endif


}
        
