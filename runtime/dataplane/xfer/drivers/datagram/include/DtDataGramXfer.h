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
#include "OcpiOsTimer.h"
#include "OcpiThread.h"
#include "OcpiUtilSelfMutex.h"
#include "XferDriver.h"
#include "XferEndPoint.h"
#include "XferFactory.h"
#include "XferServices.h"

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
class DGEndPoint : public DataTransfer::EndPoint, protected virtual OCPI::Util::SelfMutex {
  friend class Socket;
  friend class SmemServices;
  friend class XferServices;
  // These are the connections that have this as the local side, indexed by the remote side.
  std::vector<XferServices *> m_xferServices;
protected:
  DGEndPoint(DataTransfer::XferFactory &a_factory, const char *eps, const char *other, bool a_local,
	     size_t a_size, const OCPI::Util::PValue *params)
    : DataTransfer::EndPoint(a_factory, eps, other, a_local, a_size, params), m_xferServices(32) {}
  ~DGEndPoint() {}
  void stop(); // stop all the underlying threads
  void addXfer(XferServices &s, MailBox remote);
  void delXfer(MailBox remote);
  size_t xferServicesSize() {
    OCPI::Util::SelfAutoMutex guard(this);
    return m_xferServices.size(); // will only grow
  }
  XferServices *xferServices(size_t destId) {
    OCPI::Util::SelfAutoMutex guard(this);
    ocpiAssert(destId < m_xferServices.size());
    return m_xferServices[destId];
  }
};

static const int MAX_MSGS = 10;  // FIXME can be calulated
struct Frame {
  OCPI::OS::Time       send_time;
  uint16_t             msg_start, msg_count;
  bool                 is_free;
  unsigned             resends;
  FrameHeader          frameHdr;
  uint16_t             valgrind_pad[3];
  unsigned             iovlen;
  size_t               bytes_left;
  // This is defined in POSIX 1003..1g for those datagram systems that can
  // take advantage of that.
  struct OCPI::OS::IOVec iov[MAX_MSGS+1];
  Transaction         *transaction;
  Frame() : is_free(true), valgrind_pad{} {}
  void prepare(uint16_t seq, size_t payload);
  void release();
};

class SmemServices;
class XferServices;
class Socket : public OCPI::Util::Thread {
  DGEndPoint   &m_lep;
  bool          m_run;
  //  bool          m_joined;
public:
  Socket(DGEndPoint &lep) : m_lep(lep), m_run(true) {} //, m_joined(false) {}
  virtual ~Socket();
  virtual void send(Frame &frame, DGEndPoint &destEp) = 0;
  // return bytes read and offset in buffer to use.  Returning zero is timeout
  virtual size_t receive(uint8_t *buf, size_t &offset) = 0;
  virtual uint16_t maxPayloadSize()=0;  // Maximum message size, total bytes
  virtual void start() = 0;
  inline void stop() { m_run = false; }
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
class SmemServices : public DataTransfer::SmemServices, public OCPI::Util::Thread,
		     public virtual OCPI::Util::SelfMutex {
  DGEndPoint &m_ep;
  Socket     *m_socket;
  char       *m_mem;
  bool        m_loop;
public:
  SmemServices(DGEndPoint& ep)
    : DataTransfer::SmemServices(ep), m_ep(ep), m_socket(NULL), m_mem(NULL), m_loop(true) {
    if (ep.local()) {
      m_mem = new char[ep.size()];
      memset(m_mem, 0, ep.size());
      m_socket = &static_cast<XferFactory *>(&ep.factory())->createSocket(ep);
      m_socket->start();
      start();
    }
  };
  virtual ~SmemServices();
  OCPI::OS::int32_t attach(EndPoint* loc){ ( void ) loc; return 0;};
  OCPI::OS::int32_t detach(){ return 0;}
  void* map (DtOsDataTypes::Offset offset, size_t/* size */) {
    return &m_mem[offset];
  }
  OCPI::OS::int32_t unMap() { return 0;}
  inline void send(Frame &frame, DGEndPoint &dest) { m_socket->send(frame, dest); }
  void run();
  void stop() {
    m_loop = false;
    join();
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

class XferServices : public DataTransfer::XferServices, virtual public OCPI::Util::SelfMutex {
  // So the destructor can invoke "remove"
  friend class XferRequest;

public:
  XferServices(XferFactory &driver, EndPoint &source, EndPoint &target);
  virtual ~XferServices ();
  virtual uint16_t maxPayloadSize()=0;  // Maximum message size, total bytes

  //Frame &nextFreeFrame();
  Frame &getFrame();
  void setAcks(Frame &f);
  void post(Frame &f);
  void processFrame(FrameHeader *frame);
  void checkAcks(OCPI::OS::Time time, OCPI::OS::Time timeout);
  void sendAcks(OCPI::OS::Time time_now, OCPI::OS::Time timeout);

private:
  DGEndPoint   &m_lep, &m_rep; // local and remote
  struct FrameRecord {
    bool     acked;
    uint32_t seq;
    FrameRecord() : acked(false), seq(0) {}
  };

  struct MsgTransactionRecord {
    uint32_t   transactionId;
    uint16_t   numMsgsInTransaction;
    uint16_t   msgsProcessed;
    bool       in_use;
    MsgTransactionRecord() : in_use(false) {}
  };

  std::vector<Frame>       m_freeFrames;
  std::deque<uint16_t>     m_acks;
  uint16_t                 m_frameSeq;
  std::vector<FrameRecord> m_frameSeqRecord;
  std::vector<MsgTransactionRecord> m_msgTransactionRecord;
  OCPI::OS::Time m_last_ack_send;
  unsigned m_transactions_in_play;
};
  }
}
#endif
