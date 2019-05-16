/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Abstract:
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
#include <list>
#include <OcpiOsTimer.h>
#include <OcpiOsMutex.h>
#include <OcpiParentChild.h>
#include <OcpiTimeEmit.h>
#include "XferException.h"
#include <DtIntEventHandler.h>
#include "XferEndPoint.h"
#include <OcpiConnectionMetaData.h>
#include <OcpiCircuit.h>
#include <OcpiTransportConstants.h>
#include <OcpiTransportGlobal.h>



// Forward references
namespace OCPI {
  namespace Util {
    class VList;
  }
}

namespace DataTransfer {
  class EndPoint;
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
                bool uses_mailboxes, OCPI::Time::Emit * parent );
      Transport(OCPI::DataTransport::TransportGlobal* tpg,
                bool uses_mailboxes );

      /**********************************
       * Destructor
       *********************************/
      virtual ~Transport();      

      void cleanForContext(void *context);
      DataTransfer::EndPoint &addRemoteEndPoint( const char* ep );
      bool                        isLocalEndpoint(const DataTransfer::EndPoint &ep) const;
      DataTransfer::EndPoint* getEndpoint(const char* ep, bool local);
      // void                        removeLocalEndpoint(  const char* ep );
      DataTransfer::EndPoint &getLocalCompatibleEndpoint(const char *ep, bool exclusive = false);
      //      DataTransfer::EndPoint &getLocalEndpointFromProtocol(const char *ep);
      DataTransfer::EndPoint &getLocalEndpoint(const char *ep);


      /**********************************
       * Creates a new circuit within a connection based upon the source
       * Port set and destibnation ports set(s)
       *********************************/
      // ports in the connection are used.
      Circuit * createCircuit(CircuitId cid, // when zero, allocate one
                              ConnectionMetaData* connection,                
                              PortOrdinal src_ports[]=NULL,        
                              PortOrdinal dest_ports[]=NULL,        
                              uint32_t flags = 0,
			      const char *protocol = NULL,
			      OCPI::OS::Timer *timer = 0); 

      // ports in the connection are used.
      //Circuit * createCircuit( DataTransfer::EndPoint *ep );

      // Initialize descriptor from endpoint info
      static void fillDescriptorFromEndPoint(DataTransfer::EndPoint &ep,
					     OCPI::RDT::Descriptors &desc);
      // Use this one when you know there is only one input port
      Port * createInputPort(OCPI::RDT::Descriptors& desc,
			     const OCPI::Util::PValue *params = NULL);
      // Use this one when you know there is only one output port
      // And the input port is remote
      Port * createOutputPort(OCPI::RDT::Descriptors& outputDesc,
			      const OCPI::RDT::Descriptors& inputDesc );
      // Use this when you are connecting the new outport to 
      // a local input port.
      Port * createOutputPort(OCPI::RDT::Descriptors& outputDesc,
			      Port& inputPort );


      /**********************************
       * Deletes a circuit
       *********************************/
      //      void deleteCircuit( CircuitId circuit );        
      void deleteCircuit( Circuit* circuit );        

      /**********************************
       * Retrieves the requested circuit
       *********************************/
      Circuit* getCircuit( CircuitId circuit_id );
      size_t getCircuitCount();

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

    protected:

      /**********************************
       * This method gets the Node-wide mutex used to lock our mailbox
       * on the specified endpoint for mailbox communication
       *********************************/
      OCPI::OS::Mutex* getMailBoxLock( const char* mbid );

      /**********************************
       * Clear remote mailbox
       *********************************/
      void clearRemoteMailbox(size_t offset, DataTransfer::EndPoint* loc );

      /**********************************
       * Send remote port our offset information
       *********************************/
      void sendOffsets( OCPI::Util::VList& offsets, DataTransfer::EndPoint &ep,
			size_t extraSize = 0, DtOsDataTypes::Offset extraFrom = 0,
			DtOsDataTypes::Offset extraTo = 0);

      /**********************************
       * Request a new connection
       *********************************/
      void requestNewConnection( Circuit* circuit, bool send, const char *protocol, OCPI::OS::Timer *timer);

      // DataTransfer::SMBResources* getEndpointResourcesFromMailbox(OCPI::OS::uint32_t idx);

      

    private:

      DataTransfer::SMBResources *         m_defEndpoint; // FIXME: check lifecycle

      void init();
      

      /**********************************
       * Our mailbox handler
       *********************************/
      void checkMailBoxes();

      // List of interprocess mutex's that we use to lock mailboxes
      OCPI::Util::VList m_mailbox_locks;

      // mailbox support
      bool                               m_uses_mailboxes;

      // These are the endpoints we own. the local list is initialized with 
      // an allocated endpoint for each driver
      DataTransfer::EndPoints m_localEndpoints, m_remoteEndpoints;

      // List of circuits
      std::list<OCPI::DataTransport::Circuit*> m_circuits;
      typedef std::list<OCPI::DataTransport::Circuit*>::iterator CircuitsIter;

      // New circuit listener
      NewCircuitRequestListener* m_newCircuitListener;

      // Our lock
      OCPI::OS::Mutex &m_mutex;

      // used to name circuits
      OCPI::OS::int32_t m_nextCircuitId;
      DataTransfer::EndPoint*  m_CSendpoint;
      ContainerComms *m_CScomms;

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


