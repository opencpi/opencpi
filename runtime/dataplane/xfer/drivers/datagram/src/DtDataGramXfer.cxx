//#define DEBUG_TxRx_Datagram 1
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
 *   This file contains the interface for the Ocpi Datagram transfer driver.
 *
 *  John Miller -  5-24-12
 *  Initial version
 *
 */

#include "OcpiOsMisc.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "DtDataGramXfer.h"

namespace XF = DataTransfer;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace DataTransfer {
  namespace DDT = DtOsDataTypes;
  namespace Datagram {

const char *datagramsocket = "datagram-socket"; // name passed to inherited template class
static const unsigned
  MAX_TRANSACTION_HISTORY = 512,  // transactions outstanding
  MAX_FRAME_HISTORY = 256,        // max frames for retransmit
  MAX_RESENDS = 10;               // max retries before squawking

XferServices::
XferServices(XferFactory &driver, EndPoint &source, EndPoint &target) :
  XF::XferServices(driver, source, target),
  m_lep(*static_cast<DGEndPoint *>(source.local() ? &source : &target)),
  m_rep(*static_cast<DGEndPoint *>(source.local() ? &target : &source)),
  m_freeFrames(MAX_FRAME_HISTORY), m_frameSeq(1), m_frameSeqRecord(MAX_FRAME_HISTORY),
  m_msgTransactionRecord(MAX_TRANSACTION_HISTORY), m_last_ack_send(OS::Time::now()),
  m_transactions_in_play(0)
{
  assert((source.local() && !target.local()) || (!source.local() && target.local()));
  m_lep.addXfer(*this, m_rep.mailBox());
}
XferServices::
~XferServices() {
  ocpiDebug("DatagramXferServices::~DatagramXferServices entered");
  // Shutdown the local endpoints threads before the endpoints are released since they callback
  // to this object in their threads and we want to be locked
  m_lep.delXfer(m_rep.mailBox());
  // Note that members are destroyed before base classes,
  // so our frames are destroyed, and then our children (xferrequests and transactions)
  // which makes sense because frames refer to transactions
  lock();
}

XferFactory::
XferFactory(const char *a_name) throw ()
  : XF::XferFactory(a_name) {
  ocpiDebug("In DatagramXferFactory::DatagramXferFactory()");
}

XferFactory::
~XferFactory() {
  ocpiDebug("DatagramXferFactory::~DatagramXferFactory entered");
#if 0
  std::vector<SmemServices*>::iterator it;
  for ( it=m_smems.begin(); it!=m_smems.end(); it++ ) {
    (*it)->stop();
    (*it)->join();
  }
  m_smems.clear();
#endif
}

void XferRequest::
modify(DtOsDataTypes::Offset  */*new_offsets[]*/, DtOsDataTypes::Offset  */*old_offsets[]*/) {
  ocpiAssert("modify not inplemented"==0);
}

XferRequest::
~XferRequest() {
}

// Create a transfer request
XferRequest* XferRequest::
copy(DtOsDataTypes::Offset srcoffs, DtOsDataTypes::Offset dstoffs, size_t nbytes, XferRequest::Flags flags) {

#ifdef DEBUG_TxRx_Datagram
  printf("\n\n *** COPY to %d, len = %d\n", dstoffs, nbytes );
#endif

  if (flags & XferRequest::FlagTransfer) {
    if (!init()) {
      init(0);
      add(NULL, 0, 0);
    }
    // Finalize the transaction since we now have the flag transfer information
    // and we know how many messages were actually sent for the transaction,
    // essentially filling in the constant fields
    m_localFlagAddr = (Flag *)parent().from().sMemServices().map(srcoffs, 0);
    Message *m = &m_messages[0];
    for (unsigned n = 0; n < m_nMessagesTx; n++, m++) {
      m->hdr.transactionId = m_tid;
      m->hdr.numMsgsInTransaction = (uint16_t)(m_nMessagesTx == 1 ? 0 : m_nMessagesTx);
      m->hdr.flagAddr = OCPI_UTRUNCATE(uint32_t, dstoffs);
    }
    return this;
  }

  // For each transfer at this level we have a header and payload.  We may need to break it up into
  // multiple "messages".  Each message gets it own header so each one is self contained and can be
  // acted upon by the receiver without being dependent on previous messages (which could get lost).
  // We ask the underlying transmission layer for the max frame payload size.
  size_t maxpl =
    parent().maxPayloadSize() - (sizeof(MsgHeader) + sizeof(FrameHeader) + 8);
  if (!init())
    // conservative estimate, but it still might be exceeded
    init((nbytes + maxpl - 1)/maxpl + 1);
  size_t length;
  for (uint8_t *src = (uint8_t*)parent().from().sMemServices().map(srcoffs,0); nbytes > 0;
       nbytes -= length, src += length, dstoffs += OCPI_UTRUNCATE(DDT::Offset, length)) {
    length = nbytes > maxpl ? maxpl : nbytes;
    add(src, dstoffs, length);
  }
  return this;
}

// Group data transfer requests
XF::XferRequest &XferRequest::
group(XF::XferRequest*xr) {
  return *xr; // FIXME: an error of some type?
}


void XferServices::
post(Frame & frame) {
  frame.send_time = OS::Time::now();
  if (frame.msg_count)
    frame.frameHdr.flags |= FRAME_FLAG_HAS_MESSAGES;
  ocpiLog(9, "SendDG: seq %3u start %3u count %3u flags %u msgcount %2u", frame.frameHdr.frameSeq,
	  frame.frameHdr.ACKStart, frame.frameHdr.ACKCount, frame.frameHdr.flags,
	  frame.msg_count);
  static_cast<SmemServices *>(&m_from.sMemServices())->send(frame, *static_cast<DGEndPoint*>(&m_to));
  // If there is nothing to ack (no messages) in this frame, free it as soon as it is sent.
  // The "send" is required to take it and not queue it (or at least copy it).
  if (!frame.msg_count)
    frame.release();
}

void XferServices::
sendAcks(OS::Time time_now, OS::Time timeout) {
  OU::SelfAutoMutex guard(this);
  if (m_acks.size() && (time_now - m_last_ack_send ) > timeout) {
    ocpiDebug("acks %zu now %" PRIu64 " timeout %" PRIu64 " m_last_ack_send %" PRIu64,
	      m_acks.size(), time_now.bits(), timeout.bits(), m_last_ack_send.bits());
    post(getFrame());
  }
}

void XferServices::
setAcks(Frame &f) {
  // We will piggyback any pending acks here
  f.frameHdr.ACKCount = 0;
  if (m_acks.size()) {
    m_last_ack_send = OS::Time::now();
    uint16_t seq = f.frameHdr.ACKStart = m_acks.front();
    do
      m_acks.pop_front();
    while (++f.frameHdr.ACKCount != UINT8_MAX && m_acks.size() && m_acks.front() == ++seq);
    ocpiDebug("Sending ACKS: start %u count %u", f.frameHdr.ACKStart, f.frameHdr.ACKCount);
  }
}

Frame &XferServices::
getFrame() {
  OCPI::Util::SelfAutoMutex guard(this);
  uint16_t
    fseq = m_frameSeq++,
    mseq = fseq & (MAX_FRAME_HISTORY-1);
  ocpiAssert(mseq < m_freeFrames.size());
  Frame &frame = m_freeFrames[mseq];

  frame.prepare(fseq, maxPayloadSize());
  frame.frameHdr.destId = m_to.mailBox();
  frame.frameHdr.srcId =  m_from.mailBox();
  setAcks(frame);
  return frame;
}

void XferRequest::
post() {
  m_nMessagesRx = 0;
  Message *m = &m_messages[0];
  uint32_t flag = *m_localFlagAddr; // retrieve the possibly dynamic flag value for this message
  for (unsigned nMsgs = 0; nMsgs < m_nMessagesTx; ) {
    Frame &frame = parent().getFrame();
    frame.transaction = this;
    frame.msg_start = OCPI_UTRUNCATE(uint16_t, nMsgs);
    OS::IOVec *iov = &frame.iov[frame.iovlen];
    for (size_t msg_bytes;
	 nMsgs < m_nMessagesTx &&
	   frame.bytes_left >= (msg_bytes = sizeof(MsgHeader) + ((m->hdr.dataLen + 7u) & ~7u));
	 frame.bytes_left -= msg_bytes, m++, nMsgs++, frame.msg_count++) {
      m->hdr.nextMsg = true;
      m->hdr.flagValue = flag;
      *iov++ = { .iov_base = (void*)&m->hdr, .iov_len = sizeof(MsgHeader)};
      *iov++ = { .iov_base = m->src_adr, .iov_len = msg_bytes - sizeof(MsgHeader)};
    }
    frame.iovlen = OCPI_UTRUNCATE(unsigned, OCPI_SIZE_T_DIFF(iov, frame.iov));
    m[-1].hdr.nextMsg = false;
    parent().post(frame);
  }
}

volatile static uint32_t g_txId;
static uint32_t getNextId() {
  g_txId++;
  return g_txId;
}

void Transaction::
init(size_t nMsgs) {
  ocpiAssert( ! m_init );
  m_nMessagesTx = 0;
  m_messages.reserve(nMsgs ? nMsgs : 1);
  m_tid = getNextId();
  m_init = true;
}

// src == NULL means the message only carries the flag and the transaction has no other messages
void Transaction::
add(uint8_t *src, DtOsDataTypes::Offset dst_offset, size_t length) {
  if (m_nMessagesTx >= m_messages.size())
    m_messages.reserve( m_nMessagesTx + 10 );
  m_messages.resize(++m_nMessagesTx);
  Message &m = m_messages.back();

  m.hdr.dataAddr = (uint32_t)dst_offset;
  m.hdr.dataLen = (uint16_t)length;
  m.src_adr = src;

  // This might disappear since we could put disconnect in a frame flag
  m.hdr.type = MsgHeader::DATA;
  // This might disappear unless there is a use-case for its heuristic value
  m.hdr.msgSequence = (uint16_t)m_nMessagesTx;
}

#ifdef PKT_DEBUG
    struct sockaddr_in * in = (struct sockaddr_in *)&sad;
    int df = ((struct sockaddr_in *)&sad)->sin_family;
    int port = ntohs ( ((struct sockaddr_in *)&sad)->sin_port );
    char * a  = inet_ntoa ( ((struct sockaddr_in *)&sad)->sin_addr );
    struct in_addr  adrr  =  ((struct sockaddr_in *)&sad)->sin_addr;
#endif

XF::XferRequest::CompletionStatus
XferRequest::getStatus() {
  // We only need to check the completion status of the flag transfer since it is gated on the
  // completion status of the data and meta-data transfers
  if (! complete())
    return XF::XferRequest::Pending;
  return XF::XferRequest::CompleteSuccess;
}

void Socket::
run() {
  ocpiInfo("ENTERING DG SOCKET THREAD");
  try {
    while ( m_run ) {
      unsigned size =	maxPayloadSize();
      uint8_t buf[size];
      size_t offset;
      size_t n = receive(buf, offset);
      if (n == 0)
	continue; // zero is timeout
      // This causes a frame drop for testing
      //#define DROP_FRAME
#ifdef DROP_FRAME
      const char* env = getenv("OCPI_Datagram_DROP_FRAMES");
      if ( env != NULL )
	{
	  static int dropit=1;
	  static int dt = 300;
	  static int m = 678900;
	  if ( dt && (((++dropit)%m)==0) ) {
	    printf("\n\n\n DROP A PACKET FOR TESTING \n\n\n");
	    dt--;
	    m = 500000 + rand()%10000;
	    continue;
	  }
	}
#endif
      // Get the xfer service that handles this conversation
      FrameHeader *header = reinterpret_cast<FrameHeader*>(&buf[offset + 2]);
      XferServices *xfs = m_lep.xferServices(header->srcId);
      if (xfs)
	xfs->processFrame(header);
    }
    ocpiInfo("EXISING DG SOCKET THREAD");

  }
  catch (std::string &s) {
    ocpiBad("Exception in socket background thread: %s", s.c_str());
  } catch (...) {
    ocpiBad("Unknown exception in socket background thread");
  }
  ocpiInfo("Datagram socket receiver thread exiting");
}

Socket::
~Socket() {
  try {
    stop();
    join();
  }
  catch( ... ) {
  }
}

SmemServices::
~SmemServices () {
  ocpiDebug("DatagramSmemServices::~DatagramSmemServices entered");
  stop();
  delete [] m_mem;
}

// send acknowledgements occasionally
void SmemServices::
run() {
  const OS::Time
    checkTimeout(0, 200 * 1000 * 1000),
    ackTimeout(0, 80 * 1000 * 1000);  // timeout in nSec

  do {
    for (size_t n = 0, max = m_ep.xferServicesSize(); n < max; ++n) {
      OU::SelfAutoMutex epGuard(&m_ep);
      XferServices *xfs = m_ep.xferServices(n);
      if (xfs) {
	OS::Time time_now = OS::Time::now();
	xfs->checkAcks(time_now, checkTimeout);
	OCPI::OS::sleep(2);
	time_now = OS::Time::now();
	xfs->sendAcks(time_now, ackTimeout);
	OCPI::OS::sleep(2);
      }
    }
    OCPI::OS::sleep(2);
  } while (m_loop);
}

void XferServices::
checkAcks(OS::Time time, OS::Time time_out) {
  OCPI::Util::SelfAutoMutex guard(this);
  for (unsigned n = 0; n < m_freeFrames.size(); n++) {
    Frame &f = m_freeFrames[n];
    if (!f.is_free && (time - f.send_time) > time_out && time > f.send_time) {
      ocpiLog(9, "Resending datagram frame %u try %u", f.frameHdr.frameSeq, f.resends);
      if (++f.resends > MAX_RESENDS)
	ocpiBad("Datagram retransmissions exceeds %u for seq %u", MAX_RESENDS, f.frameHdr.frameSeq);
      setAcks(f);
      post(f);
    }
  }
}

void XferServices::
processFrame(FrameHeader *header) {
  FrameRecord &seqRecord = m_frameSeqRecord[header->frameSeq & (MAX_FRAME_HISTORY-1)];
  OCPI::Util::SelfAutoMutex guard(this);

  if (seqRecord.seq == header->frameSeq) {
    // The sender may duplicate frames due to retries. Dont process dups, but ack them.
    ocpiDebug("dup datagram: history = %u, SID = %u, fqr size = %zu seq=0x%x/%u",
	      MAX_FRAME_HISTORY, header->srcId,  m_frameSeqRecord.size(), header->frameSeq,
	      header->frameSeq);
    ocpiAssert(seqRecord.acked);
    ocpiAssert(header->flags & FRAME_FLAG_HAS_MESSAGES);
    m_acks.push_back(header->frameSeq); // retransmnit ack
    return;
  }
  for (unsigned count = header->ACKCount, seq = header->ACKStart; count; --count, ++seq) {
    Frame &f = m_freeFrames[seq & (MAX_FRAME_HISTORY-1)];
    if (!f.is_free && f.frameHdr.frameSeq == seq)
      f.release();
    else
      ocpiDebug("Received ack 0x%x when frame is %s with num 0x%x",
		seq, f.is_free ? "free" : "busy", f.frameHdr.frameSeq);
  }
  if (!(header->flags & FRAME_FLAG_HAS_MESSAGES))
    return;
  seqRecord.acked = true;
  seqRecord.seq = header->frameSeq;
  m_acks.push_back(header->frameSeq);  // ack for the first time

  for (MsgHeader *msg = reinterpret_cast<MsgHeader*>(&header[1]); msg;
       msg = msg->nextMsg ? (MsgHeader *)((uint8_t *)(msg + 1) + ((msg->dataLen + 7) & ~7)) : NULL) {
    MsgTransactionRecord &tr = m_msgTransactionRecord[msg->transactionId & (MAX_TRANSACTION_HISTORY-1)];
    if (!tr.in_use) {
      tr.in_use = true;
      tr.msgsProcessed = 0;
      tr.transactionId = msg->transactionId;
      tr.numMsgsInTransaction = msg->numMsgsInTransaction;
      ocpiAssert(m_transactions_in_play < MAX_TRANSACTION_HISTORY);
      m_transactions_in_play++;
    }
    if (msg->numMsgsInTransaction) { // there is message contents, not just a flag
      memcpy((char*)from().sMemServices().map(msg->dataAddr, msg->dataLen),
	     reinterpret_cast<uint8_t*>(&msg[1]), msg->dataLen);
      tr.msgsProcessed++;
    }
    ocpiDebug("Msg: addr=0x%x len=%u tid=%u seq=%u max %u now=%ufirst=%x", msg->dataAddr,
	      msg->dataLen, msg->transactionId, header->frameSeq, msg->numMsgsInTransaction,
	      tr.msgsProcessed, *(uint32_t*)&msg[1]);
    if (tr.msgsProcessed == tr.numMsgsInTransaction) { // transaction is done
      ocpiDebug("Finalizing datagram transaction %u, addr = 0x%x, value = 0x%x",
		msg->transactionId, msg->flagAddr, msg->flagValue);
      *(uint32_t *)m_from.sMemServices().map(msg->flagAddr, sizeof(uint32_t)) = msg->flagValue;
      tr.in_use = false;
      m_transactions_in_play--;
    }
  }
}

void DGEndPoint::
stop() {
  // A lousy way to navigate back to our threads.
  // FIXME: consolidate the two threads into one with receive timeouts, and put it under
  // the endpoint class (when local).
  static_cast<SmemServices*>(&sMemServices())->stop();
}
void DGEndPoint::
addXfer(XferServices &s, MailBox remote) {
  OCPI::Util::SelfAutoMutex guard(this);
  if (remote >= m_xferServices.size())
    m_xferServices.resize(remote + 8u);
  m_xferServices[remote] = &s;
  ocpiDebug("xfer service %p added with mbox %d", &s, remote);
}

void DGEndPoint::
delXfer(MailBox remote) {
  OCPI::Util::SelfAutoMutex guard(this);
  assert(remote < m_xferServices.size());
  m_xferServices[remote] = NULL;
  ocpiDebug("xfer service removed with mbox %d", remote);
}

void Frame::
prepare(uint16_t seq, size_t payload) {
  assert(is_free);
  is_free = false;
  frameHdr.flags = 0;
  frameHdr.ACKCount = 0;
  frameHdr.ACKStart = 0; // for valgrind...
  frameHdr.frameSeq = seq;
  transaction = NULL;
  resends = 0;
  msg_count = 0;
  msg_start = 0;
  bytes_left = payload;
  // This is a two byte pad for compatibility with the 14 byte ethernet header.
  iov[0].iov_base = (void *)&frameHdr;
  iov[0].iov_len = 2;
  iov[1].iov_base = (void *)&frameHdr;
  iov[1].iov_len = sizeof(frameHdr);
  iovlen = 2;
  bytes_left -=  sizeof(frameHdr) + 2;
}

void Frame::
release() {
  // This can occur if we are too agressive with a retry and end up getting back Two ACK's
  // for the re-transmitted frame.
  if (is_free)
    return;
  // We have to release the individual messages for the transaction here.
  for (unsigned int n=msg_start; n<(msg_count+msg_start); n++ ) {
    ocpiAssert( transaction );
    transaction->ACK( n );
  }
  transaction = 0;
  is_free = true;
  resends = 0;
}
}
}
