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


#include <deque>
#include <OcpiOsIovec.h>
#include <OcpiThread.h>
#include <OcpiUtilAutoMutex.h>
#include <DtDriver.h>
#include <DtTransferInterface.h>
#include <DtSharedMemoryInterface.h>

#define FRAME_SEQ_MASK 0xff


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

  struct DatagramTransaction;
  struct DatagramFrameHeader {
    uint16_t destId;
    uint16_t srcId;
    uint16_t frameSeq;
    uint16_t ACKStart;
    uint8_t  ACKCount;
    uint8_t  flags;
  };    
  #define FRAME_FLAG_HAS_MESSAGES 1

  static const int MAX_MSGS = 10;  // FIXME can be calulated
  struct Frame {
    uint64_t             send_time;
    uint16_t             msg_start, msg_count;
    bool                 is_free;
    int                  resends;
    DatagramFrameHeader  frameHdr;
    unsigned             iovlen;
    // This is defined in POSIX 1003..1g for those datagram systems that can
    // take advantage of that.
    struct OCPI::OS::IOVec iov[MAX_MSGS+1];
    DatagramTransaction *transaction;
    EndPoint *           endpoint; // where is this frame going to
    void release();
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
  class DatagramSocket : public OCPI::Util::Thread
  {
  public:  
  DatagramSocket( DatagramSmemServices*  lsmem)
    :m_lsmem(lsmem),m_error(false),m_run(true),m_joined(false) {}

      bool error(){return m_error;}
      virtual ~DatagramSocket();
      virtual void send(Frame &frame) = 0;
      // return bytes read and offset in buffer to use.  Returning zero is timeout
      virtual unsigned receive(uint8_t *buf, unsigned &offset) = 0;
      virtual uint16_t maxPayloadSize()=0;  // Maximum message size, total bytes
      virtual void start() = 0;
      inline void stop() { m_run = false; }
      // Since we are stopped+joined when our smem is stopped, as well as destruction
      inline void join() {
	if (!m_joined) {
	  OCPI::Util::Thread::join();
	  m_joined = true;
	}
      }
      void run();
  protected:
      DatagramSmemServices* m_lsmem;
      bool                  m_error;
      bool                  m_run;
      bool                  m_joined;
  };


  class DatagramXferFactory : public DataTransfer::XferFactory {
    friend class DatagramSmemServices;
  private:
    std::vector<DatagramSmemServices*> m_smems;

  public:
    DatagramXferFactory(const char *name)
      throw ();
    virtual ~DatagramXferFactory();//      throw ();
    virtual EndPoint*  createEndPoint(std::string& endpoint, bool local) =0;
    virtual  std::string allocateEndpoint(const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes)=0;
    virtual const char* getProtocol()=0;
    virtual DatagramXferServices *createXferServices(DatagramSmemServices*source,
						     DatagramSmemServices*target) = 0;
    virtual DatagramSmemServices *createSmemServices(EndPoint *ep) = 0;
    virtual DatagramSocket *createSocket(DatagramSmemServices *) = 0;
    SmemServices *getSmemServices(EndPoint* ep);
    XferServices* getXferServices(SmemServices* source, SmemServices* target);
  };

  class DatagramSmemServices : public SmemServices, public FrameMonitor
    {
    public:
      DatagramSmemServices (DatagramXferFactory * p, EndPoint* ep)
	: SmemServices(p, ep),FrameMonitor(this),m_socket(NULL),m_xferServices(32)
      {
	m_ep = ep;
	m_mem = new char[ep->size];
	memset( m_mem, 0, ep->size );
	if (ep->local)
	  m_socket = p->createSocket(this);
      };
      OCPI::OS::int32_t attach (EndPoint* loc){ ( void ) loc; return 0;};
      OCPI::OS::int32_t detach (){return 0;}
      void* map (uint32_t offset, uint32_t/* size */)
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
      inline void send(Frame &frame) { m_socket->send(frame); }
      void start() {
	FrameMonitor::start();
	if (m_socket)
	  m_socket->start();
      }
      void stop() {
	FrameMonitor::stop();
	if (m_socket) {
	  m_socket->stop();
	  m_socket->join();
	}
      }
    private:
      DatagramSocket * m_socket;
      FrameMonitor   * m_frameMonitor;
      EndPoint* m_ep;
      char* m_mem;
      std::vector< DatagramXferServices * > m_xferServices;
    };

  struct TxTemplate {
    TxTemplate(){}
    ~TxTemplate(){}
    void config(   SmemServices * src, SmemServices * dst );
    SmemServices * ssmem;
    SmemServices * dsmem;
  };


  class DatagramXferRequest;
  // Although the XferRequest represents a single contiguous RDMA request, it may be split up into
  // many datagram messages.
  struct DatagramTransaction { 
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
    ~DatagramTransaction(){
      ///FIXME: *********** free up the frames involved with this transaction      
    }
    // nMessages is the estimated number of messages EXCLUSIVE of the flag transfer
    void init( uint32_t nMessages);
    void add(uint8_t * src, uint64_t dst_offset, uint32_t length);
    void fini(uint32_t flag, uint32_t dst);

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
  /**********************************
   * This is the Programmed I/O transfer request class
   *********************************/
  class DatagramXferRequest : public XferRequest, private DatagramTransaction
    {
    public:
      virtual DatagramXferServices &parent() = 0;
    DatagramXferRequest(XF_template temp)
      : XferRequest(temp), m_tested4Complete(0)
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
    protected:
	OCPI::OS::uint32_t                        m_srcoffset;        // The source memory offset
	OCPI::OS::uint32_t                        m_dstoffset;        // The destination memory offset
	OCPI::OS::uint32_t                        m_length;                // The length of the request in bytes
	int                                       m_tested4Complete;
    };


  class DropPktMonitor;
  class DatagramXferServices : public XferServices, public OCPI::Util::SelfMutex
    {

      // So the destructor can invoke "remove"
      friend class DatagramXferRequest;

    public:
      DatagramXferServices(SmemServices* source, SmemServices* target);
      virtual ~DatagramXferServices ();
      virtual uint16_t maxPayloadSize()=0;  // Maximum message size, total bytes

      void addFrameAck( DatagramFrameHeader * hdr );
      void ack( unsigned count, unsigned start );
      Frame *  nextFreeFrame();
      Frame &  getFrame(  unsigned & bytes_left  );
      void  releaseFrame ( unsigned seq );
      void post( Frame & t );

      void processFrame( DatagramFrameHeader * frame );
      void checkAcks( uint64_t time, uint64_t timeout );
      void sendAcks( uint64_t time_now, uint64_t timeout );

      // Source SMB services pointer
      DatagramSmemServices* m_sourceSmb;

      // Target SMB services pointer
      DatagramSmemServices* m_targetSmb;

    private:
      // Create tranfer services template
      void createTemplate (DatagramSmemServices* p1, DatagramSmemServices* p2);
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
      uint32_t                 m_frameSeq;
      std::vector<FrameRecord> m_frameSeqRecord;
      std::vector<MsgTransactionRecord> m_msgTransactionRecord;
      uint64_t m_last_ack_send;
      unsigned m_frames_in_play;
    };
}
#endif
