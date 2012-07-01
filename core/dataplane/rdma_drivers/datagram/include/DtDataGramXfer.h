
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

#ifndef DataTransfer_DATAGRAMTransfer_H_
#define DataTransfer_DATAGRAMTransfer_H_


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
#include <OcpiUtilAutoMutex.h>
#include <deque>

// After socket abstraction, remove me
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define FRAME_SEQ_MASK 0xff
#define DATAGRAM_PAYLOAD_SIZE 512


namespace DataTransfer {

  struct DatagramMsgHeader {
    enum MsgType {
      DATA=0,
      METADATA=1,
      FLOWCONTROL=2,
      DISCONNECT=3
    };
    uint32_t   transactionId;
    uint32_t   flagAddr;
    uint32_t   flagValue;
    uint16_t   numMsgsInTransaction;
    uint16_t   msgSequence;
    uint32_t   dataAddr;
    uint16_t   dataLen;
    uint8_t    type;
    uint8_t    nextMsg;      
  };


  // Abstract base class that needs to be inherited by a transmission layer driver.
  class TxTemplate;

  // Virtual class for a transaction object
  class Transaction {
  public:
    virtual unsigned msgCount() = 0;
    virtual void * srcPtr( unsigned  part )=0;
    virtual DatagramMsgHeader * hdrPtr( unsigned part )=0;
    virtual void ACK(int part )=0;            
    TxTemplate   * m_txTemplate;
    virtual ~Transaction(){};
  };

  struct DatagramFrameHeader {
    uint16_t destId;
    uint16_t srcId;
    uint16_t frameSeq;
    uint16_t ACKStart;
    uint8_t  ACKCount;
    uint8_t  flags;
  };    

  static const int MAX_MSGS = 10;  // FIXME can be calulated
  struct Frame {
    uint64_t             send_time;
    uint16_t             msg_start, msg_count;
    bool                 is_free;
    int                  resends;
    DatagramFrameHeader  frameHdr;
    sockaddr_in          sock_adr;
    msghdr               msg;
    struct iovec         iov[MAX_MSGS+1];
    Transaction *        transaction;
    void release() {	  
      // We have to release the individual messages for the transaction here.
      for (unsigned int n=msg_start; n<(msg_count+msg_start); n++ ) {
	ocpiAssert( transaction );
	transaction->ACK( n );
      }
      transaction = 0;
    }
    Frame():is_free(true),resends(0),transaction(0){}
  };



  class DatagramSmemServices;

  class FrameMonitor : public OCPI::Util::Thread,  public OCPI::Util::SelfMutex
    {
    public:
      FrameMonitor( DatagramSmemServices* lsmem )
	:m_loop(true),m_lsmem(lsmem){}

      void run();
      void stop(){m_loop=false;}
    protected:
      bool m_loop;
      DatagramSmemServices*  m_lsmem;
    };



  class DatagramXferServices;
  class DatagramSocket;
  class DatagramSmemServices : public SmemServices, public FrameMonitor
    {
    public:
      DatagramSmemServices (XferFactory * p, EndPoint* ep)
	:SmemServices(p, ep),FrameMonitor(this),m_socket(NULL),m_xferServices(32)
      {
	m_ep = ep;
	m_mem = new char[ep->size];
	memset( m_mem, 0, ep->size );
      };
      OCPI::OS::int32_t attach (EndPoint* loc){ ( void ) loc; return 0;};
      OCPI::OS::int32_t detach (){return 0;}
      void* map (uint64_t offset, uint32_t/* size */)
      {
	return &m_mem[offset];
      }
      OCPI::OS::int32_t unMap (){return 0;}
      EndPoint* getEndPoint (){return m_ep;}
      virtual ~DatagramSmemServices ();
      DatagramSocket *& socketServer(){return m_socket;}
      void addXfer( DatagramXferServices * s );
      DatagramXferServices * xferServices( unsigned destId ) {
	ocpiAssert( destId < m_xferServices.size() );
	ocpiAssert( m_xferServices[destId] );
	return m_xferServices[destId];
      }
      std::vector< DatagramXferServices * > & getAllXferServices(){return  m_xferServices;}
      DatagramSocket & socket() {return *m_socket;}
    private:
      DatagramSocket * m_socket;
      FrameMonitor   * m_frameMonitor;
      EndPoint* m_ep;
      char* m_mem;
      std::vector< DatagramXferServices * > m_xferServices;
    };
  



  /*
  struct DatagramSocketStartupParams {
    DatagramSocketStartupParams ( )
      : lsmem ( 0 )
    { }
    DatagramSmemServices*  lsmem;
  };
  */

  class DatagramTransmisionLayerDriver;

  class DatagramSocket 
  {
  public:  
    DatagramSocket( DatagramSmemServices*  lsmem, DatagramTransmisionLayerDriver * driver )
      :m_lsmem(lsmem),m_error(false),m_driver(driver){}


      OCPI::OS::Socket & socket() {return  m_socket;}    
      virtual void start()=0;
      bool error(){return m_error;}
      virtual ~DatagramSocket();
  protected:
      OCPI::OS::Socket m_socket;
      OCPI::OS::ServerSocket          m_server;
      DatagramSmemServices*          m_lsmem;
      bool                           m_error;
      DatagramTransmisionLayerDriver *               m_driver;
  };


  class DatagramTransmisionLayerDriver {
  public:

    virtual ~DatagramTransmisionLayerDriver(){};
    virtual uint16_t maxPayloadSize()=0;  // Maximum message size, total bytes

    virtual SmemServices * getSmemServices(XferFactory *f, EndPoint* loc )=0;
    virtual EndPoint * getEndPoint()=0;
    virtual EndPoint*  createEndPoint(std::string& endpoint, bool local) =0;
    virtual  std::string allocateEndpoint(const OCPI::Util::PValue*, unsigned smbsize, unsigned mailBox, unsigned maxMailBoxes)=0;
    

    virtual const char* getProtocol()=0;
  };


  class DatagramXferFactory;
  class DatagramDevice : public OCPI::Driver::DeviceBase<DatagramXferFactory,DatagramDevice> {
  };
  class DatagramXferServices;
  extern const char *datagramsocket;

  class DatagramXferFactory : 
  public DriverBase<DatagramXferFactory, DatagramDevice,DatagramXferServices,datagramsocket> {

  private:
    DatagramTransmisionLayerDriver * m_txDriver;

  public:
    DatagramXferFactory()
      throw ();
    virtual ~DatagramXferFactory()
      throw ();
    void setDriver( DatagramTransmisionLayerDriver* d)
    {
      m_txDriver=d;
      printf("Set driver to %p\n", d);
    }
    inline const char* getProtocol(){return m_txDriver->getProtocol();}
    SmemServices* getSmemServices(EndPoint* ep );
    XferServices* getXferServices(SmemServices* source, SmemServices* target);
    std::string allocateEndpoint(const OCPI::Util::PValue*, unsigned mailBox, unsigned maxMailBoxes);
    unsigned maxPayloadSize(){return m_txDriver->maxPayloadSize();}
    DatagramTransmisionLayerDriver & txDriver(){return *m_txDriver;}

  protected:
    EndPoint* createEndPoint(std::string& endpoint, bool local = false);
    
  };

  struct TxTemplate {
    TxTemplate(){}
    ~TxTemplate(){}
    void config(   SmemServices * src, SmemServices * dst );
    SmemServices * ssmem;
    SmemServices * dsmem;
  };


  /**********************************
   * This is the Programmed I/O transfer request class
   *********************************/
  class DatagramXferServices;
  class DatagramXferRequest : public TransferBase<DatagramXferServices,DatagramXferRequest>
    {
    public:

      DatagramXferRequest( DatagramXferServices & parent )
	: TransferBase<DatagramXferServices,DatagramXferRequest>(parent), m_tested4Complete(0)
	{
	
	}

	void post();
	DataTransfer::XferRequest::CompletionStatus getStatus();
	virtual ~DatagramXferRequest ();
	XferRequest & group( XferRequest* lhs );
	void modify( OCPI::OS::uint32_t new_offsets[], OCPI::OS::uint32_t old_offsets[] );
	XferRequest* copy (OCPI::OS::uint32_t srcoff, 
			   OCPI::OS::uint32_t dstoff, 
			   OCPI::OS::uint32_t nbytes, 
			   XferRequest::Flags flags
			   );

    private:
	uint32_t     m_txTotal;   // Total number of datagrams that make up this transaction - flag transfer

	// Although the XferRequest represents a single contiguous RDMA request, it may be split up into
	// many datagram messages.
	struct DatagramTransaction : public Transaction  {

	  struct Message {
	    DatagramMsgHeader   hdr;
	    void *              src_adr;
	    bool                ack;
	    Message():ack(false){}
	  };
	  bool                m_init;
	  uint32_t            m_nMessagesTx;
	  uint32_t            m_nMessagesRx;
	  uint32_t            m_tid;
	  std::vector<Message>   m_messages;
	  inline bool init() {return m_init;}
	  unsigned msgCount(){return m_nMessagesTx;}
	  DatagramTransaction() 
	    : m_init(false), m_nMessagesTx(0), m_nMessagesRx(0) {}
	  ~DatagramTransaction(){}
	  void init( uint32_t nMessages, TxTemplate * temp  );
	  void add( DatagramXferRequest* rqst, uint8_t * src, uint64_t dst_offset, uint32_t length, uint32_t tx_total );
	  void fini( uint32_t src, uint32_t dst, TxTemplate * temp  );

	  inline bool complete() 
	  {
	    return (m_nMessagesRx == m_nMessagesTx);
	  };

	  inline void * srcPtr( unsigned  msg )
	  {
	    ocpiAssert( msg < m_messages.size() );
	    return m_messages[msg].src_adr;
	  }
	  inline DatagramMsgHeader * hdrPtr( unsigned msg ) {
	    ocpiAssert( msg < m_messages.size() );
	    return &m_messages[msg].hdr;
	  }
	  inline void ACK(int message ) {
	    m_messages[message].ack = true;
	    m_nMessagesRx++;
	  }    
	};

	void post( DatagramTransaction & t );
	void queFrame( Frame & frame );
	DatagramXferServices & myParent(){return static_cast<DatagramXferServices&>(parent());}


    protected:
	OCPI::OS::uint32_t                        m_srcoffset;        // The source memory offset
	OCPI::OS::uint32_t                        m_dstoffset;        // The destination memory offset
	OCPI::OS::uint32_t                        m_length;                // The length of the request in bytes
	int                                       m_tested4Complete;
	DatagramTransaction                       m_transaction;
    };


  class DropPktMonitor;
  class DatagramXferServices : public ConnectionBase<DatagramXferFactory,DatagramXferServices,DatagramXferRequest>,
     public OCPI::Util::SelfMutex
    {

      // So the destructor can invoke "remove"
      friend class DatagramXferRequest;

    public:
      DatagramXferServices(SmemServices* source, SmemServices* target);
      XferRequest* createXferRequest();
      virtual ~DatagramXferServices ();


      void addFrameAck( DatagramFrameHeader * hdr );
      void ack( unsigned count, unsigned start );
      Frame *  nextFreeFrame();
      Frame &  getFrame(  int & bytes_left  );
      void  releaseFrame ( unsigned seq );
      void post( Frame & t );

      void processFrame( DatagramFrameHeader * frame );
      void checkAcks( uint64_t time, uint64_t timeout );
      void sendAcks( uint64_t time_now, uint64_t timeout );

      // Source SMB services pointer
      SmemServices* m_sourceSmb;

      // Target SMB services pointer
      SmemServices* m_targetSmb;

      // Create tranfer services template
      void createTemplate (SmemServices* p1, SmemServices* p2);

    private:
      struct FrameRecord {
	bool     acked;
	uint32_t id;
	FrameRecord():acked(false),id(0){}
      };

      struct MsgTransactionRecord {
	uint32_t   transactionId;
	uint16_t   numMsgsInTransaction;
	uint16_t   msgsProcessed;
	bool       in_use;
	MsgTransactionRecord():in_use(false){}
      };

      TxTemplate               m_txTemplate;
      std::vector<Frame>       m_freeFrames;
      std::deque<unsigned>     m_acks;
      uint32_t                  m_frameSeq;
      std::vector<FrameRecord> m_frameSeqRecord;
      std::vector<MsgTransactionRecord> m_msgTransactionRecord;
      uint64_t m_last_ack_send;

    };



}
#endif
