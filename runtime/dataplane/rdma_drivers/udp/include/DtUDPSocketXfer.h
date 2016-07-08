#define UDP_PAYLOAD_SIZE 512

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
 *   This file contains the implementation for the programed I/O transfer class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 2/2010
 *    Revision Detail: Created
 *
 */

#ifndef DataTransfer_UDPTransfer_H_
#define DataTransfer_UDPTransfer_H_


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <OcpiOsDataTypes.h>
#include <DtDriver.h>
#include <DtTransferInterface.h>
#include <DtSharedMemoryInterface.h>
#include <OcpiOsSocket.h>
#include <OcpiOsServerSocket.h>
#include <xfer_internal.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>



namespace DataTransfer {

  /**********************************
   * This is our GPP shared memory location implementation.  This class
   * relies shared memory implementations that support named resources.
   * Although this class allows for a hostname in the address, it is
   * currently not being used.
   *
   *   Address format:  IPAddr:Port:
   *********************************/
  class  UDPEndPoint : public EndPoint 
  {
  public:

    virtual ~UDPEndPoint();
    UDPEndPoint( std::string& ep, bool a_local, OCPI::OS::uint32_t a_size=0)
      : EndPoint(ep, a_size, a_local) { parse(ep);}

        // Sets smem location data based upon the specified endpoint
        OCPI::OS::int32_t parse( std::string& ep );

	SmemServices &createSmemServices();
        std::string ipAddress;
        uint16_t  portNum;
  };




  /**********************************
   * Each transfer implementation must implement a factory class.  This factory
   * implementation creates a named resource compatible SMB and a programmed I/O
   * based transfer driver.
   *********************************/
  //  class ClientUDPSocketT;
  class UDPSocketXferFactory;
  class UDPSocketDevice : public DataTransfer::DeviceBase<UDPSocketXferFactory,UDPSocketDevice> {
      UDPSocketDevice(const char *a_name)
	: DataTransfer::DeviceBase<UDPSocketXferFactory,UDPSocketDevice>(a_name, *this) {}
  };
  class UDPSocketXferServices;
  extern const char *udpsocket;
  class UDPSocketXferFactory : public DriverBase<UDPSocketXferFactory, UDPSocketDevice,UDPSocketXferServices,udpsocket> {

  public:

    // Default constructor
    UDPSocketXferFactory()
      throw ();

    // Destructor
    virtual ~UDPSocketXferFactory()
      throw ();

    // Get our protocol string
    const char* getProtocol();


    /***************************************
     *  This method is used to create a transfer service object
     ***************************************/
    XferServices* getXferServices(SmemServices* source, SmemServices* target);


    /***************************************
     *  Set (unparse, snprintf) the endpoint string
     ***************************************/
    static void setEndpointString(std::string &str, const char *ipAddr, unsigned port,
				  size_t size, uint16_t mbox, uint16_t maxCount);
    /***************************************
     *  This method is used to dynamically allocate
     *  an endpoint for an application running on "this"
     *  node.
     ***************************************/
    std::string allocateEndpoint(const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes, size_t size);

  protected:
    EndPoint* createEndPoint(std::string& endpoint, bool local = false);
    
  };

  class UDPSmemServices;
  struct TxTemplate {
    TxTemplate(){}
    ~TxTemplate(){}
    void config(   UDPSmemServices * src,   UDPSmemServices * dst );
    UDPSmemServices * ssmem;
    UDPSmemServices * dsmem;
  };



  /**********************************
   * This is the Programmed I/O transfer request class
   *********************************/
  class UDPSocketXferServices;
  class UDPSocketXferRequest : public TransferBase<UDPSocketXferServices,UDPSocketXferRequest>
  {

    // Public methods available to clients
  public:

    struct UDPSocketDataHeader {
      enum MsgType {
	DMAWrite,
	DMARead,
	Ack,
	MutiAck

      };
      uint16_t   type;
      DtOsDataTypes::Offset offset;
      uint32_t   length;
      uint64_t   transaction_cookie;
      uint16_t   transaction_length;
      uint32_t   transaction_sequence;
      
    };

    UDPSocketXferRequest( UDPSocketXferServices &a_parent )
      : TransferBase<UDPSocketXferServices,UDPSocketXferRequest>(a_parent, *this),
      m_tested4Complete(0)
      //      m_thandle(0) 
	{
	  m_TxPkt[0].m_id = 0;
	  m_TxPkt[1].m_id = 1;
	  m_TxPkt[2].m_id = 2;
	}

    void ackReceived( UDPSocketDataHeader * header );

    // Queue data transfer request
    void post();

    // Get Information about a Data Transfer Request
    DataTransfer::XferRequest::CompletionStatus getStatus();

    void checkDrops();

    // Destructor - implementation at end of file
    virtual ~UDPSocketXferRequest ();

    XferRequest & group( XferRequest* lhs );
    void modify(DtOsDataTypes::Offset new_offsets[], DtOsDataTypes::Offset old_offsets[] );

    XferRequest* copy (DtOsDataTypes::Offset srcoff, 
		       DtOsDataTypes::Offset dstoff, 
		       size_t nbytes, 
		       XferRequest::Flags flags
		       );

    // Data members accessible from this/derived class
  private:
    size_t     m_txTotal;   // Total number of datagrams that make up this transaction - flag transfer

  
    struct TxPacket {

      struct ACKTxData {
	UDPSocketDataHeader hdr;
	uint32_t            seq;      // Tx sequence            
	msghdr              msg;
	struct iovec        iov[3];
	bool                ack;
	int                 retry_count;
	ACKTxData():seq(0),ack(false),retry_count(0){}
      };

      TxPacket() 
	: m_init(false), m_nAcksTx(0), m_nAcksRx(0),m_id(0){}
      ~TxPacket(){}
      void init(size_t nPackets, TxTemplate * temp  );
      void add( UDPSocketXferRequest* rqst, uint8_t * src, DtOsDataTypes::Offset, size_t length, size_t tx_total );

      bool            m_init;
      uint32_t        m_nAcksTx;
      uint32_t        m_nAcksRx;
      std::vector<ACKTxData>   m_acks;
      OCPI::OS::ServerSocket * m_socket;
      TxTemplate      *m_txTemplate;
      uint32_t        m_id;
      sockaddr_in     m_adr;

      inline bool init() {return m_init;}
      unsigned pktCount(){return m_nAcksTx;}

      void retry() {
	for (unsigned int n=0; n<m_nAcksTx; n++ ) {
	  if ( ! m_acks[n].ack ) {
	    // If we are a flag transfer, we really need to be carefull with
	    // a re-transmission due to race conditions.  So we will throttle 
	    // the re-tries
	    m_acks[n].retry_count++;
	    if ( m_id == 2 ) {  
	      if ( (m_acks[n].retry_count%2) != 0 ) {
		return;
	      }
	    }
	    (void)m_socket->sendmsg( &m_acks[n].msg, 0 );
	  }
	}
      }


      int64_t sendMsg( unsigned int msgId ) {

#ifdef DEBUG_TxRx_UDP
	int port = m_socket->getPortNo();
	printf("Sending data thru port %d, len = %d \n", port, m_acks[msgId].hdr.length );
#endif

	ocpiAssert( msgId < m_nAcksTx );
	m_acks[msgId].ack = false;	
	return m_socket->sendmsg( &m_acks[msgId].msg, 0 );
      }

      inline bool complete()
      {
	return (m_nAcksRx == m_nAcksTx);
      };
    
    };

  protected:
    void action_socket_transfer(TxPacket *);
    OCPI::OS::uint32_t                        m_srcoffset;        // The source memory offset
    OCPI::OS::uint32_t                        m_dstoffset;        // The destination memory offset
    OCPI::OS::uint32_t                        m_length;                // The length of the request in bytes
    int                                       m_tested4Complete;
    TxPacket                                  m_TxPkt[3];
  };



  // UDPSocketXferServices specializes for MCOE-capable platforms
  class UDPSmemServices;

  class DropPktMonitor;
  class UDPSocketXferServices : public ConnectionBase<UDPSocketXferFactory,UDPSocketXferServices,UDPSocketXferRequest>
    {

      // So the destructor can invoke "remove"
      friend class UDPSocketXferRequest;

    public:

      UDPSocketXferServices(SmemServices* source, SmemServices* target);

	/*
	 * Create tranfer request object
	 */
	XferRequest* createXferRequest();


	// Destructor
	virtual ~UDPSocketXferServices ();

    protected:

	// Source SMB services pointer
	UDPSmemServices* m_sourceSmb;

	// Target SMB services pointer
	UDPSmemServices* m_targetSmb;

	// Create tranfer services template
	void createTemplate (SmemServices* p1, SmemServices* p2);

    private:
	DropPktMonitor * m_dpMonitor;
	TxTemplate         m_txTemplate;

    };




  /**********************************
   ****
   * inline declarations
   ****
   *********************************/

  // inline methods for UDPSocketXferFactory
  inline const char* UDPSocketXferFactory::getProtocol(){return "ocpi-udp-rdma";}




}
#endif
