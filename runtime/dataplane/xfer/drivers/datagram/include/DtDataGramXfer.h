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

#ifndef DataTransfer_DATAGRAMTransfer_H_
#define DataTransfer_DATAGRAMTransfer_H_


#include <deque>
#include <vector>
#include "OcpiOsIovec.h"
#include "OcpiThread.h"
#include "OcpiUtilSelfMutex.h"
#include "XferDriver.h"
#include "XferEndPoint.h"
#include "XferFactory.h"
#include "XferServices.h"

#define FRAME_SEQ_MASK 0xff

// These declaration are shared among various datagram drivers
namespace DataTransfer {
  namespace Datagram {

struct MsgHeader {
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

struct Transaction;
struct FrameHeader {
  uint16_t destId;
  uint16_t srcId;
  uint16_t frameSeq;
  uint16_t ACKStart;
  uint8_t  ACKCount;
  uint8_t  flags;
};
#define FRAME_FLAG_HAS_MESSAGES 1

// Not an official base class, just a convenience mix-in
class Socket;
class XferServices;
class DGEndPoint : public OCPI::Util::Thread, protected virtual OCPI::Util::SelfMutex {
  friend class Socket;
  // These are the templates that have this as the local size, indexed by the remote side.
  std::vector<XferServices *> m_xferServices;
  bool m_loop;
protected:
  DGEndPoint() : m_xferServices(32), m_loop(true) {}
  ~DGEndPoint() {}
  void addXfer(XferServices &s);
  XferServices *xferServices(unsigned destId) {
    ocpiAssert(destId < m_xferServices.size() );
    ocpiAssert(m_xferServices[destId] );
    return m_xferServices[destId];
  }
  void run();
  void stop() { m_loop = false; join(); }
};

static const int MAX_MSGS = 10;  // FIXME can be calulated
struct Frame {
  uint64_t             send_time;
  uint16_t             msg_start, msg_count;
  bool                 is_free;
  int                  resends;
  FrameHeader          frameHdr;
  unsigned             iovlen;
  // This is defined in POSIX 1003..1g for those datagram systems that can
  // take advantage of that.
  struct OCPI::OS::IOVec iov[MAX_MSGS+1];
  Transaction         *transaction;
  EndPoint            *endpoint; // where is this frame going to
  void release();
  Frame():is_free(true),resends(0),transaction(0){}
};

class SmemServices;
class XferServices;
class Socket : public OCPI::Util::Thread {
  DGEndPoint   &m_lep;
  bool          m_run;
  bool          m_joined;
public:
  Socket(DGEndPoint &lep) : m_lep(lep), m_run(true), m_joined(false) {}
  virtual ~Socket();
  virtual void send(Frame &frame) = 0;
  // return bytes read and offset in buffer to use.  Returning zero is timeout
  virtual size_t receive(uint8_t *buf, size_t &offset) = 0;
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
};

class XferFactory : public DataTransfer::XferFactory {
  friend class SmemServices;
public:
  XferFactory(const char *name)
    throw ();
  virtual ~XferFactory();
  virtual Socket &createSocket(EndPoint &lep) = 0;
};

// active thread does background retransmissions
class SmemServices : public DataTransfer::SmemServices, public virtual OCPI::Util::SelfMutex {
  Socket *m_socket;
  char   *m_mem;
public:
  SmemServices(EndPoint& ep)
    : DataTransfer::SmemServices(ep), m_socket(NULL), m_mem(NULL) {
    if (ep.local()) {
      m_mem = new char[ep.size()];
      memset(m_mem, 0, ep.size());
      m_socket = &static_cast<XferFactory *>(&ep.factory())->createSocket(ep);
      m_socket->start();
    }
  };
  virtual ~SmemServices ();
  OCPI::OS::int32_t attach(EndPoint* loc){ ( void ) loc; return 0;};
  OCPI::OS::int32_t detach(){ return 0;}
  void* map (DtOsDataTypes::Offset offset, size_t/* size */) {
    return &m_mem[offset];
  }
  OCPI::OS::int32_t unMap() { return 0;}
  //  Socket *&socketServer() { return m_socket;}
  inline void send(Frame &frame) { m_socket->send(frame); }
  void start() {
    if (m_socket)
      m_socket->start();
  }
  void stop() {
    if (m_socket) {
      m_socket->stop();
      m_socket->join();
    }
  }
};

class XferRequest;
// Although the XferRequest represents a single contiguous RDMA request, it may be split up into
// many datagram messages.
struct Transaction {
  struct Message {
    MsgHeader hdr;
    void     *src_adr;
    bool      ack;
    Message():ack(false){}
  };
  bool                m_init;
  unsigned            m_nMessagesTx;
  unsigned            m_nMessagesRx;
  uint32_t            m_tid;
  Flag                *m_localFlagAddr;
  std::vector<Message>   m_messages;
  inline bool init() {return m_init;}
  Transaction()
  : m_init(false), m_nMessagesTx(0), m_nMessagesRx(0), m_tid(0), m_localFlagAddr(NULL) {}
  ~Transaction(){
    ///FIXME: *********** free up the frames involved with this transaction
  }
  // nMessages is the estimated number of messages EXCLUSIVE of the flag transfer
  void init(size_t nMessages);
  void add(uint8_t * src, DtOsDataTypes::Offset dst_offset, size_t length);

  inline bool complete() {
    return (m_nMessagesRx == m_nMessagesTx);
  };

  inline void ACK(unsigned message) {
    m_messages[message].ack = true;
    m_nMessagesRx++;
  }
};

class XferRequest : public DataTransfer::XferRequest, private Transaction {
#if 0
  DtOsDataTypes::Offset         m_srcoffset;        // The source memory offset
  DtOsDataTypes::Offset         m_dstoffset;        // The destination memory offset
  size_t                        m_length;           // The length of the request in bytes
  int                           m_tested4Complete;
#endif
public:
  virtual XferServices &parent() = 0;
  XferRequest(XFTemplate *temp)
    : DataTransfer::XferRequest(temp) { //, m_tested4Complete(0) {
  }
  virtual ~XferRequest ();
  void post();
  DataTransfer::XferRequest::CompletionStatus getStatus();
  DataTransfer::XferRequest &group(DataTransfer::XferRequest* lhs);
  void modify(DtOsDataTypes::Offset new_offsets[], DtOsDataTypes::Offset old_offsets[]);
  XferRequest* copy(DtOsDataTypes::Offset srcoff,
		    DtOsDataTypes::Offset dstoff,
		    size_t nbytes,
		    XferRequest::Flags flags);
};

class DropPktMonitor;
class XferServices : public DataTransfer::XferServices, public OCPI::Util::SelfMutex {
  // So the destructor can invoke "remove"
  friend class XferRequest;

public:
  XferServices(XferFactory &driver, EndPoint &source, EndPoint &target);
  virtual ~XferServices ();
  virtual uint16_t maxPayloadSize()=0;  // Maximum message size, total bytes

  void addFrameAck(FrameHeader *hdr);
  void ack(unsigned count, unsigned start);
  Frame *nextFreeFrame();
  Frame &getFrame(size_t &bytes_left);
  void releaseFrame(unsigned seq);
  void post(Frame &t);
  void processFrame(FrameHeader *frame);
  void checkAcks(uint64_t time, uint64_t timeout);
  void sendAcks(uint64_t time_now, uint64_t timeout);

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

  std::vector<Frame>       m_freeFrames;
  std::deque<uint16_t>     m_acks;
  uint16_t                 m_frameSeq;
  std::vector<FrameRecord> m_frameSeqRecord;
  std::vector<MsgTransactionRecord> m_msgTransactionRecord;
  uint64_t m_last_ack_send;
  unsigned m_frames_in_play;
};
  }
}
#endif
