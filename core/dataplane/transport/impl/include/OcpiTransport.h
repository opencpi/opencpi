
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
 *   This file contains the Implementation for the OCPI transport.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_Transport_H_
#define OCPI_DataTransport_Transport_H_

#include <vector>
#include <OcpiOsTimer.h>
#include <OcpiConnectionMetaData.h>
#include <OcpiCircuit.h>
#include <OcpiTransportConstants.h>
#include <OcpiOsMutex.h>
#include <DtExceptions.h>
#include <OcpiTransportGlobal.h>
#include <DtIntEventHandler.h>
#include <OcpiParentChild.h>


// Forward references
namespace OCPI {
  namespace Util {
    class VList;
  }
}

namespace DataTransfer {
  struct EndPoint;
  struct SMBResources;
}

namespace DtI = ::DataTransport::Interface;

namespace OCPI {

  namespace DataTransport {
    
    class Circuit;

    // New circuit request listener
    class NewCircuitRequestListener {

    public:

      /**********************************
       * This method gets called when a new circuit is available and ready for use
       *********************************/
      virtual void newCircuitAvailable( OCPI::DataTransport::Circuit* new_circuit ) = 0;

      /**********************************
       * This method gets called when an error gets generated
       *********************************/
      //      virtual void error( OCPI::Util::EmbeddedException& ex ) = 0;

      virtual ~NewCircuitRequestListener(){}

    };
    
    /**********************************
     * Constant definitions
     *********************************/
    const OCPI::OS::uint32_t NewConnectionFlag = 0x01;
    const OCPI::OS::uint32_t SendCircuitFlag   = 0x02;
    const OCPI::OS::uint32_t RcvCircuitFlag    = 0x04;

    class Transport : public OCPI::Util::Parent<Circuit> , public OCPI::Time::Emit

    {
    public:

      friend class Circuit;

      /**********************************
       * Constructors
       *********************************/
      Transport(OCPI::DataTransport::TransportGlobal* tpg,
                bool uses_mailboxes );

      /**********************************
       * Destructor
       *********************************/
      virtual ~Transport();      


      /**********************************
       * Is an endpoint local
       *********************************/
      DataTransfer::SMBResources* addLocalEndpointFromProtocol( const char* protocol );      
      DataTransfer::SMBResources* addLocalEndpoint( const char* ep, bool compatibleWith = false );
      DataTransfer::SMBResources* findLocalCompatibleEndpoint( const char* ep );
      DataTransfer::SMBResources* addRemoteEndpoint( const char* ep );
      bool                        isLocalEndpoint( const char* ep );
      DataTransfer::SMBResources* getEndpointResources(const char* ep);
      void                        removeLocalEndpoint(  const char* ep );
      DataTransfer::EndPoint &getLocalCompatibleEndpoint(const char *ep);


      /**********************************
       * Creates a new circuit within a connection based upon the source
       * Port set and destibnation ports set(s)
       *********************************/
      Circuit * createCircuit( 
                              const char* id,                        
                              ConnectionMetaData* connection,                
                              PortOrdinal src_ports[]=NULL,        
                              PortOrdinal dest_ports[]=NULL,   
                              uint32_t flags = 0,
			      OCPI::OS::Timer *timer = 0
                              );                                        

      // ports in the connection are used.
      Circuit * createCircuit( 
                              CircuitId& cid,                     
                              ConnectionMetaData* connection,                
                              PortOrdinal src_ports[]=NULL,        
                              PortOrdinal dest_ports[]=NULL,        
                              uint32_t flags = 0,
			      OCPI::OS::Timer *timer = 0); 

      // ports in the connection are used.
      Circuit * createCircuit( OCPI::RDT::Descriptors& sPort );


      Port * createInputPort( Circuit * &c,  OCPI::RDT::Descriptors& desc, const OCPI::Util::PValue *);
      // Use this one when you know there is only one input port
      Port * createInputPort(OCPI::RDT::Descriptors& desc, const OCPI::Util::PValue *);
      // Use this one when you know there is only one output port
      // And the input port is remote
      Port * createOutputPort(OCPI::RDT::Descriptors& outputDesc,
			      const OCPI::RDT::Descriptors& inputDesc);
      // Use this when you are connecting the new outport to 
      // a local input port.
      Port * createOutputPort(OCPI::RDT::Descriptors& outputDesc,
			      Port& inputPort);


      /**********************************
       * Deletes a circuit
       *********************************/
      void deleteCircuit( CircuitId circuit );        
      void deleteCircuit( Circuit* circuit );        

      /**********************************
       * Retrieves the requested circuit
       *********************************/
      Circuit* getCircuit( CircuitId& circuit_id );
      OCPI::OS::uint32_t getCircuitCount();

      /**********************************
       * General house keeping 
       *********************************/
      void dispatch(DataTransfer::EventManager* event_manager=NULL);
      //      std::vector<std::string> getListOfSupportedEndpoints();

      /**********************************
       * Set the callback listener for new circuit requests on this transport
       *********************************/
      void setNewCircuitRequestListener( NewCircuitRequestListener* listener );


      /**********************************
       * Does this transport support mailboxes?
       *********************************/
      inline bool supportsMailboxes(){return m_uses_mailboxes;}
      void setListeningEndpoint( DataTransfer::EndPoint* ep) throw() {m_CSendpoint=ep;}

      /**********************************
       * Does this (local, finalized) endpoint support talking to this other endpoint?
       *********************************/
      static bool canSupport(DataTransfer::EndPoint &ep, const char *endpoint);

    protected:

      /**********************************
       * This method gets the Node-wide mutex used to lock our mailbox
       * on the specified endpoint for mailbox communication
       *********************************/
      OCPI::OS::Mutex* getMailBoxLock( const char* mbid );

      /**********************************
       * Clear remote mailbox
       *********************************/
      void clearRemoteMailbox( OCPI::OS::uint32_t offset, DataTransfer::EndPoint* loc );

      /**********************************
       * Send remote port our offset information
       *********************************/
      void sendOffsets( OCPI::Util::VList& offsets, std::string& t_ep );

      /**********************************
       * Request a new connection
       *********************************/
      void requestNewConnection( Circuit* circuit, bool send, OCPI::OS::Timer *timer);

      DataTransfer::SMBResources* getEndpointResourcesFromMailbox(OCPI::OS::uint32_t idx);

      

    private:

      DataTransfer::SMBResources *         m_defEndpoint; // FIXME: check lifecycle
      std::vector<std::string>             m_endpoints;
      //      std::vector<DataTransfer::EndPoint*> m_finalized_endpoints;
      

      /**********************************
       * Our mailbox handler
       *********************************/
      void checkMailBoxs();

      // List of interprocess mutex's that we use to lock mailboxes
      OCPI::Util::VList m_mailbox_locks;

      // mailbox support
      bool                               m_uses_mailboxes;

      OCPI::Util::VList   m_localEndpoints;
      OCPI::Util::VList   m_remoteEndpoints;

      // List of circuits
      std::vector<OCPI::DataTransport::Circuit*> m_circuits;

      // New circuit listener
      NewCircuitRequestListener* m_newCircuitListener;

      // Our lock
      OCPI::OS::Mutex &m_mutex;

      // used to name circuits
      OCPI::OS::int32_t m_nextCircuitId;
      DataTransfer::EndPoint*  m_CSendpoint;

      // Cached transfer list
      static OCPI::Util::VList  m_cached_transfers;
      static OCPI::Util::VList   active_transfers;

    public:
      // Our transport global class
      OCPI::DataTransport::TransportGlobal*            m_transportGlobal;


    };

  }
}


#endif


