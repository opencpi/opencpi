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
 *   This file contains the Implementation for the CPI transport.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport_Transport_H_
#define CPI_DataTransport_Transport_H_

#include <vector>
#include <CpiConnectionMetaData.h>
#include <CpiCircuit.h>
#include <CpiTransportConstants.h>
#include <CpiOsMutex.h>
#include <DtExceptions.h>
#include <CpiTransportGlobal.h>
#include <DtIntEventHandler.h>
#include <CpiParentChild.h>


// Forward references
namespace CPI {
  namespace Util {
    class VList;
  }
}

namespace DataTransfer {
  struct EndPoint;
  struct SMBResources;
}

namespace DtI = ::DataTransport::Interface;

namespace CPI {

  namespace DataTransport {
    
    class Circuit;

    // New circuit request listener
    class NewCircuitRequestListener {

    public:

      /**********************************
       * This method gets called when a new circuit is available and ready for use
       *********************************/
      virtual void newCircuitAvailable( CPI::DataTransport::Circuit* new_circuit ) = 0;

      /**********************************
       * This method gets called when an error gets generated
       *********************************/
      virtual void error( CPI::Util::EmbeddedException& ex ) = 0;

      virtual ~NewCircuitRequestListener(){}

    };
    
    /**********************************
     * Constant definitions
     *********************************/
    const CPI::OS::uint32_t NewConnectionFlag = 0x01;
    const CPI::OS::uint32_t SendCircuitFlag   = 0x02;
    const CPI::OS::uint32_t RcvCircuitFlag    = 0x04;

    class Transport : public CPI::Util::Parent<Circuit> , public CPI::Time::Emit

    {

    public:

      friend class Circuit;

      /**********************************
       * Constructors
       *********************************/
      Transport(CPI::DataTransport::TransportGlobal* tpg,
		bool uses_mailboxes );

      /**********************************
       * Destructor
       *********************************/
      virtual ~Transport();      


      /**********************************
       * Is an endpoint local
       *********************************/
      std::string getEndpointFromProtocol( const char* protocol );      
      DataTransfer::SMBResources* addLocalEndpoint( const char* ep );
      DataTransfer::SMBResources* addRemoteEndpoint( const char* ep );
      bool                        isLocalEndpoint( const char* ep );
      DataTransfer::SMBResources* getEndpointResources(const char* ep);
      void                        removeLocalEndpoint(  const char* ep );
      std::string& getDefaultEndPoint();


      /**********************************
       * Creates a new circuit within a connection based upon the source
       * Port set and destibnation ports set(s)
       *********************************/
      Circuit * createCircuit( 
			      const char* id,			
			      ConnectionMetaData* connection,		
			      PortOrdinal src_ports[]=NULL,	
			      PortOrdinal dest_ports[]=NULL,   
			      CPI::OS::uint32_t flags=0
			      );					

      // ports in the connection are used.
      Circuit * createCircuit( 
			      CircuitId& cid,		     
			      ConnectionMetaData* connection,		
			      PortOrdinal src_ports[]=NULL,	
			      PortOrdinal dest_ports[]=NULL,	
			      CPI::OS::uint32_t flags=0 ); 

      // ports in the connection are used.
      Circuit * createCircuit( CPI::RDT::Descriptors& sPort );


      Port * createInputPort( Circuit * &c,  CPI::RDT::Descriptors& desc);


      /**********************************
       * Deletes a circuit
       *********************************/
      void deleteCircuit( CircuitId circuit );	

      /**********************************
       * Retrieves the requested circuit
       *********************************/
      Circuit* getCircuit( CircuitId& circuit_id );
      CPI::OS::uint32_t getCircuitCount();

      /**********************************
       * General house keeping 
       *********************************/
      void dispatch(DataTransfer::EventManager* event_manager=NULL);
      std::string getLocalCompatibleEndpoint( const char* ep );
      std::vector<std::string> getListOfSupportedEndpoints();

      /**********************************
       * Set the callback listener for new circuit requests on this transport
       *********************************/
      void setNewCircuitRequestListener( NewCircuitRequestListener* listener );


      /**********************************
       * Does this transport support mailboxes?
       *********************************/
      inline bool supportsMailboxes(){return m_uses_mailboxes;}
      void setListeningEndpoint( DataTransfer::EndPoint* ep){m_CSendpoint=ep;}

    protected:

      /**********************************
       * This method gets the Node-wide mutex used to lock our mailbox
       * on the specified endpoint for mailbox communication
       *********************************/
      CPI::OS::Mutex* getMailBoxLock( const char* mbid );

      /**********************************
       * Clear remote mailbox
       *********************************/
      void clearRemoteMailbox( CPI::OS::uint32_t offset, DataTransfer::EndPoint* loc );

      /**********************************
       * Send remote port our offset information
       *********************************/
      void sendOffsets( CPI::Util::VList& offsets, std::string& t_ep );

      /**********************************
       * Request a new connection
       *********************************/
      void requestNewConnection( Circuit* circuit, bool send);

      DataTransfer::SMBResources* getEndpointResourcesFromMailbox(CPI::OS::uint32_t idx);

      

    private:

      std::vector<std::string>             m_endpoints;
      std::string                          m_defEndpoint;
      std::vector<DataTransfer::EndPoint*> m_finalized_endpoints;
      

      /**********************************
       * Our mailbox handler
       *********************************/
      void checkMailBoxs();

      // List of interprocess mutex's that we use to lock mailboxes
      CPI::Util::VList m_mailbox_locks;

      // mailbox support
      bool                               m_uses_mailboxes;

      CPI::Util::VList   m_localEndpoints;
      CPI::Util::VList   m_remoteEndpoints;

      // List of circuits
      std::vector<CPI::DataTransport::Circuit*> m_circuits;

      // New circuit listener
      NewCircuitRequestListener* m_newCircuitListener;

      // Our lock
      static CPI::OS::Mutex m_mutex;

      // used to name circuits
      CPI::OS::int32_t m_nextCircuitId;
      DataTransfer::EndPoint*  m_CSendpoint;

      // Cached transfer list
      static CPI::Util::VList  m_cached_transfers;
      static CPI::Util::VList   active_transfers;

    public:
      // Our transport global class
      CPI::DataTransport::TransportGlobal*            m_transportGlobal;


    };

  }
}


#endif


