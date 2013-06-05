
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
 *   This file contains the OCPI circuit implementation.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 1/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_DataTransport_Circuit_H_
#define OCPI_DataTransport_Circuit_H_

#include <OcpiList.h>
#include <OcpiTimeEmit.h>
#include <OcpiCircuit.h>
#include <OcpiConnectionMetaData.h>
#include <OcpiPortSet.h>
#include <OcpiTransportConstants.h>
#include <OcpiParentChild.h>

namespace CU = ::OCPI::Util;
namespace OCPI {

  namespace DataTransport {

    class  PullDataDriver;
    class  Port;

    struct ExternalPortDependencyData {
      char endpoint[80];
      OCPI::RDT::Descriptors  pdd;
    };

    // Unique ordinal for a circuit, with zero being a sentinel
    typedef uint32_t CircuitId;

    class Transport;

    class Circuit : public OCPI::Util::Child<Transport,Circuit>, public OCPI::Util::Parent<PortSet>,
      public OCPI::Time::Emit
    {

    public:
      friend class Port;

      /**********************************
       * Constructors
       *********************************/
      Circuit( Transport* tpg,
               CircuitId  iid, ConnectionMetaData* connection, 
               PortOrdinal sps[], 
               PortOrdinal dpss[]);


      /**********************************
       * Destructor
       *********************************/
      virtual ~Circuit();

      /**********************************
       * Get circuit status
       *********************************/
      enum Status {
        Active,
        Disconnecting,
        Unknown
      };

      /**********************************
       * finalize the circuit.
       *********************************/
      void finalize( const char* endpoint );

      /**********************************
       * Reference count
       *********************************/
      void attach();
      void release();

      /**********************************
       * Determines if a circuit is ready to be initialized
       *********************************/
      bool ready();

      /**********************************
       * Reset a circuit
       *********************************/
      void reset();

      /**********************************
       * Attempts to make the circuit ready
       *********************************/
      bool updateConnection(OCPI::DataTransport::Port*,uint32_t count);

      /**********************************
       * Adds a port to the circuit
       *********************************/
      void addInputPort(DataTransfer::EndPoint &iep,
			const OCPI::RDT::Descriptors& inputDesc,
			DataTransfer::EndPoint &oep);

      /**********************************
       * Adds a port to the circuit
       *********************************/
      OCPI::DataTransport::Port* addPort( PortMetaData* pmd );        

      /**********************************
       * Updates a port in the circuit
       *********************************/
      OCPI::DataTransport::Port* updatePort( OCPI::DataTransport::Port* p );        


      /**********************************
       * Sets the feedback descriptor for this port.
       *********************************/
      void setFlowControlDescriptor( OCPI::DataTransport::Port* p, 
                                     const OCPI::RDT::Descriptors& desc);


      /**********************************
       * Updates our input ports
       *********************************/
      //      bool updateInputs();
      void updateInputs(DataTransfer::ContainerComms::RequestUpdateCircuit*);

      /**********************************
       * Initialize transfers
       *********************************/
      void initializeDataTransfers();


      /**********************************
       * This method is reponsible for cheching its queued request buffers and
       * starting any queued requests if possible.
       *********************************/
      uint32_t checkQueuedTransfers();

      /**********************************
       * This method causes the buffer to be transfered to the
       * inputs if a transfer can take place.  If not, the circuit 
       * manager is responsible for queing the request.
       *
       * This method returns -1 if an error occurs.
       *********************************/
      void startBufferTransfer( Buffer* src_buf );

      /***********************************
       * This method is used to send an input buffer thru an output port with Zero copy, 
       * if possible
       *********************************/
      void sendZcopyInputBuffer( 
                                Port* out_port,
                                Buffer* src_buf,
                                size_t len );


      /***********************************
       * This method braodcasts a buffer to all of the input ports in 
       * in the circuit.
       *********************************/
      void broadcastBuffer( Buffer* src_buf );


      /**********************************
       * Que the transfer for this buffer for later.
       *********************************/
      void queTransfer( Buffer* input_buf, bool prepend=false );


      /**********************************
       * This method is reponsible for determining if a buffer can be 
       * transfered.  If not, the caller is responsible for queuing the
       * transfer.
       *********************************/
      bool canTransferBuffer( 
                             Buffer* input_buf,                                // In - Buffer to be transfered
                             bool queued_transfer=false);                // In - Is this a queued buffer


      /**********************************
       * Get the maximum port ordinal for this circuit
       *********************************/
      PortOrdinal getMaxPortOrd();

      PullDataDriver* createPullDriver( const OCPI::RDT::Descriptors& pdesc);

      /**********************************
       * This method is used to print circuit information for debug
       *********************************/
      void debugDump();


      /**********************************
       * This method is used to determine if a circuit is "open"
       *********************************/
      bool isCircuitOpen();

      /***********************************
       * This method is used to set the relative work load for this instance of the circuit.
       * This is an optional method that can be used by a transport to communicate a realtive
       * "busy" factor back to the output ports for load balancing.  The higher the value,
       * the busier the circuit.
       *********************************/
      virtual void setRelativeLoadFactor( uint32_t load_factor );
      uint32_t getRelativeLoadFactor();

      /**********************************
       * Get/Set circuit id
       *********************************/
      void setCircuitId( CircuitId id );
      CircuitId getCircuitId();

      /**********************************
       * Get the output port set
       *********************************/
      PortSet* getOutputPortSet();
      Port*    getOutputPort(int ord = 0);

      /**********************************
       * Get the transport object
       *********************************/
      inline Transport& getTransport(){return *m_transport;}

      /**********************************
       * Get the number of destination port sets
       *********************************/
      PortOrdinal getInputPortSetCount();


      /**********************************
       * Get the output destination port sets
       *
       * returns: Null terminated list of port sets
       *********************************/
      OCPI::Util::VList& getInputPortSets();

      /**********************************
       * Get the output destination port sets
       *
       * returns: Null terminated list of port sets
       *********************************/
      PortSet* getInputPortSet(uint32_t idx );
      void addPortSet( PortSet* port_set );


      // Get the status of the circuit
      inline Status& getStatus(){return m_status;}


      /**********************************
       * Get the connection meta-data
       *********************************/
      ConnectionMetaData * getConnectionMetaData();


      /**********************************
       * store and retrieve protocol info during connection setup
       *********************************/
      void getProtocolInfo(size_t &size, OCPI::Util::ResAddr &offset) {
	// ocpiAssert(m_protocol != 0 && m_protocol->length() != 0 && m_protocolOffset != 0);
	size = m_protocolSize;
	offset = m_protocolOffset;
      }

      // Estalish where in the local SMB the protocol does or will exist.
      inline void setProtocolInfo(size_t size, OCPI::Util::ResAddrType offset) {
	m_protocolSize = size;
	m_protocolOffset = offset;
	//	if (m_protocol) {
	//        delete m_protocol;
	//	  m_protocol = 0;
	//	}
      }
      inline void setProtocol(char *protocol) {
	ocpiAssert(m_protocol == 0 && m_protocolSize != 0);
	m_protocolSize = 0;
	m_protocol = protocol;
	ocpiDebug("Setting protocol string (id %x p %p): \"%s\"",
		  getCircuitId(), protocol, isprint(*protocol) ? protocol : "unprintable");
      }
      // Retrieve the protocol string and pass ownership to caller as a char ARRAY
      inline char *getProtocol() {
	ocpiDebug("Getprotocol id %x protocol %p", getCircuitId(), m_protocol);
	char *s = m_protocol; m_protocol = NULL; return s;
      }
    protected:

      /***************************************************
       * Completes the circuit once the definition is complete
       ***************************************************/
      void update();


      void QInputToWaitForOutput( 
                                 Port* out_port,
                                 Buffer* input_buf,
                                 size_t len );

      void QInputToOutput( 
                          Port* out_port,
                          Buffer* input_buf,
                          size_t len );

      void checkIOZCopyQ();
 

      /**********************************
       * This method is used to select the controllers that are needed 
       * within this circuit.
       *********************************/
      void createCircuitControllers();

      /**********************************
       * This method is used to get the user port flow control descriptor.
       *********************************/
      //      void
      //        getUserPortFlowControlDescriptor( OCPI::RDT::Descriptors* fb, unsigned int idx );


      /**********************************
       * This method is used to select the transfer template generators that are needed 
       * within this circuit.
       *********************************/
      void createCircuitTemplateGenerators();

      // Transport
      Transport* m_transport;


      /**********************************
       * These methods are used to control the input port sets that are used to 
       * receive data based upon the circuits data distribution type.  If the circuits
       * DD type is parallel, then 
       *********************************/
      uint32_t getQualifiedInputPortSetCount(bool queued=false);
      PortSet* getQualifiedInputPortSet(uint32_t n, bool queued=false );

      // our init flag
      //      static uint32_t m_init;

      // Queued transfer lists
      uint32_t m_maxPortOrd;
      CU::VList m_queuedTransfers[MAX_PCONTRIBS];

      // ZCopy transfer list.  This list is used to Q up zero copy transfers of another
      // ports buffers
      CU::VList m_queuedInputOutputTransfers[MAX_PCONTRIBS];

      // Last input port set processed, (used for sequential distribution at connection level)
      uint32_t m_lastPortSet;

      // Debug stuff
      bool m_fromQ;

      // Templates generated flag
      bool m_templatesGenerated;

      // Last status received
      Status m_status;

      // Circuit ready flag
      bool m_ready;

      // Updated flag
      bool m_updated;

    public: // temporary
      // Circuit is open
      bool m_openCircuit;
    protected:
      // Relative load factor
      uint32_t m_loadFactor;

      // Circuit ordinal
      CircuitId m_circuitId;

      // Output port set
      PortSet* m_outputPs;

      // Input port sets
      OCPI::Util::VList m_inputPs;

      // Our meta data
      ConnectionMetaData* m_metaData;

      // Number of port sets already initialized
      uint32_t m_portsets_init;

      // Reference count
      int m_ref_count;

      // Protocol info goes through a different lifecycle on server and client.

      // On client, it is passed into the DataTransport::createCircuit, which passes it into
      // the circuit constructor, which takes ownership of it.
      // Then transport->createCircuit ALSO, after construction, does requestNewConnection and passes the protocol
      // info into the mailbox exchanges: it allocates local smb space, maps and copies the protocol
      // info into the smb in order to transfer it to the server's smb.  At this time the client
      // circuit is told where in the local smb the info exists, and thus it can delete the original string
      // information passed into the constructor. (in setProtocolInfo).  After copying it into the SMB,
      // the connection request to the server contains the size of this information, so that the server can 
      // allocate space for it in its SMB.

      // On the server side, the receipt of the connection request carries the size of the protocol info.
      // Upon receipt, the server allocates SMB space to receive the protocol info into, and sends this offset
      // back to the client in the request for output flow control offsets.  When the client receives this
      // request, it copies the protocol info from client smb to server smb, and deallocates the smb space
      // holding the protocol info on the client side.  When the server sees that its request for output flow
      // control offsets has been satisfied (and thus the protocol info has also arrived), it creates a protocol
      // string in the circuit, copies the info out of the SMB and deallocates the smb space.
      // Then when the user of the circuit processes the protocol info it deletes it.
      char *m_protocol; // a char array we own
      // Offset in our local endpoint smb where we have put the protoco info for sending to the
      // server side.
      size_t m_protocolSize;
      OCPI::Util::ResAddrType m_protocolOffset;
    };


    /**********************************
     ****
     * inline declarations
     ****
     *********************************/
    inline PortOrdinal Circuit::getMaxPortOrd(){return m_maxPortOrd;}
    inline bool Circuit::isCircuitOpen(){return m_openCircuit;}
    inline void Circuit::setRelativeLoadFactor( uint32_t load_factor ){m_loadFactor = load_factor;}
    inline uint32_t Circuit::getRelativeLoadFactor(){return m_loadFactor;}
    inline void Circuit::setCircuitId( CircuitId id ){m_circuitId = id;}
    inline CircuitId Circuit::getCircuitId(){return m_circuitId;}
    inline PortSet* Circuit::getOutputPortSet() {return m_outputPs;}
    inline PortOrdinal Circuit::getInputPortSetCount() {return m_inputPs.size();}
    inline OCPI::Util::VList& Circuit::getInputPortSets() {return m_inputPs; }
    inline PortSet* Circuit::getInputPortSet(uint32_t idx) {return static_cast<PortSet*>(m_inputPs[idx]);}
    inline ConnectionMetaData * Circuit::getConnectionMetaData() {return m_metaData;}

  }

}

#endif

