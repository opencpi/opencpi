//#define DEBUG_TxRx_Datagram 1

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
 *   This file contains the interface for the Ocpi Datagram transfer driver.
 *
 *  John Miller -  5-24-12
 *  Initial version
 *
 */

#include <DtSharedMemoryInternal.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtExceptions.h>
#include <OcpiThread.h>
#include <DtDataGramXfer.h>
#include <fasttime.h>



namespace DataTransfer {

  using namespace OCPI::Util;
  using namespace OCPI::OS;
  const char *datagramsocket = "datagram-socket"; // name passed to inherited template class
  static const unsigned MAX_TRANSACTION_HISTORY = 512;  // Max records per source
  static const unsigned MAX_FRAME_HISTORY = 0xff; 

  DatagramXferServices::
  DatagramXferServices(SmemServices* source, SmemServices* target)
    : ConnectionBase<DatagramXferFactory,DatagramXferServices,DatagramXferRequest>(source,target),
      m_freeFrames(FRAME_SEQ_MASK+1),m_frameSeq(1),m_frameSeqRecord(MAX_FRAME_HISTORY+1),
      m_msgTransactionRecord(MAX_TRANSACTION_HISTORY)
  {
    createTemplate( source, target);
  }


  extern DatagramTransmisionLayerDriver * getUDPDriver();
  DatagramXferFactory::DatagramXferFactory()
    throw ()
    : m_txDriver(NULL)
  {
    ocpiDebug("In DatagramXferFactory::DatagramXferFactory()");
    m_txDriver = getUDPDriver();
  }

  // Destructor
  DatagramXferFactory::~DatagramXferFactory()
    throw ()
  {

  }

  // This method is used to allocate a transfer compatible SMB
  SmemServices* DatagramXferFactory::getSmemServices(EndPoint* loc )
  {
    if ( loc->smem ) {
      return loc->smem;
    }
    return m_txDriver->getSmemServices( this, loc );
  }

  XferServices* DatagramXferFactory::getXferServices(SmemServices* source, SmemServices* target)
  {
    DatagramSmemServices * dsource = static_cast<DatagramSmemServices*>(source);
    DatagramXferServices *dgs = new DatagramXferServices(source, target);
    dsource->addXfer( dgs );
    return dgs;
  }

  XferRequest* DatagramXferServices::createXferRequest()
  {
    return new DatagramXferRequest( *this );
  }


  std::string DatagramXferFactory::
  allocateEndpoint(const OCPI::Util::PValue* p, unsigned mailBox, unsigned maxMailBoxes)
  {
    OCPI::Util::SelfAutoMutex guard (this);
    return m_txDriver->allocateEndpoint(p,parent().getSMBSize(),mailBox,maxMailBoxes);
  }

  EndPoint* DatagramXferFactory::
  createEndPoint(std::string& endpoint, bool local) {
    return m_txDriver->createEndPoint(endpoint, local);
  }

  void DatagramXferRequest::modify( uint32_t  new_offsets[] , uint32_t  old_offsets[]  )
  {
    ocpiAssert(!"modify not inplemented");
  }

  // DatagramXferRequest destructor implementation
  DatagramXferRequest::~DatagramXferRequest ()
  {
    // Empty
  }

  // Create tranfer services template
  void DatagramXferServices::createTemplate (SmemServices* p1, SmemServices* p2)
  {
    m_sourceSmb = p1;
    m_targetSmb = p2;
    m_txTemplate.config( m_sourceSmb, m_targetSmb );
  }

  // Create a transfer request
  XferRequest* DatagramXferRequest::copy (uint32_t srcoffs, 
					  uint32_t dstoffs, 
					  uint32_t nbytes, 
					  XferRequest::Flags flags
					  )
  {

#ifdef DEBUG_TxRx_Datagram
    printf("\n\n *** COPY to %d, len = %d\n", dstoffs, nbytes );
#endif

    if (flags & XferRequest::FlagTransfer) {
      if ( m_transaction.m_messages.size() == 0 ) {
	m_transaction.init( 1, &parent().m_txTemplate );
	uint8_t *  cur_src = static_cast<uint8_t *>( parent().m_txTemplate.ssmem->map(srcoffs,0) );
	m_transaction.add( this, cur_src, dstoffs, 4, 1 );
      }
      m_transaction.fini( srcoffs, dstoffs,  &parent().m_txTemplate );
      return this;
    }
    
    // For each tranfer at this level we have a header and payload.  We may need to break it up into
    // multiple messages.  Each message gets it own header so each one is self contained and can be
    // acted upon by the receiver without being dependent on previous messages (which could get lost).
    // We ask the underlying transmission layer for the max msg size.
#define MAX_Datagram_DATA_SIZE(x) (x - (sizeof(DatagramMsgHeader) + sizeof(DatagramFrameHeader) + 8))

    unsigned maxpl = parent().parent().maxPayloadSize();
    int nPackets = (nbytes / MAX_Datagram_DATA_SIZE(maxpl) );
    nPackets +=  (nbytes % MAX_Datagram_DATA_SIZE(maxpl)) ? 1 : 0;
    if (flags & XferRequest::DataTransfer) {
      m_transaction.init( nPackets, &parent().m_txTemplate );
      m_txTotal = nPackets + 1;
    }
    int32_t  bytes_left = nbytes;
    uint64_t  cur_dst_off = dstoffs;
    uint8_t *  cur_src = static_cast<uint8_t *>( parent().m_txTemplate.ssmem->map(srcoffs,0) );
    for ( int m=0; m<nPackets; m++ ) {
      ocpiAssert(bytes_left >= 0 );
      uint32_t length = bytes_left < (int32_t)MAX_Datagram_DATA_SIZE(maxpl) ? bytes_left : MAX_Datagram_DATA_SIZE(maxpl);	
      m_transaction.add( this, cur_src, cur_dst_off, length, m_txTotal );
      cur_dst_off += length;
      cur_src += length;
      bytes_left -= length;
    }

    return this;
  }

  // Group data transfer requests
  XferRequest & DatagramXferRequest::group (XferRequest*  )
  {
    return *this;
  }

  // Destructor
  DatagramXferServices::
  ~DatagramXferServices ()
  {

  }

  void
  DatagramXferServices::
  post ( Frame & frame ) {
    frame.send_time = fasttime_getticks();
    static_cast<DatagramSmemServices*>(m_sourceSmb)->socket().socket().sendmsg( &frame.msg, 0 );
  }

  void
  DatagramXferRequest::
  queFrame( Frame & frame ) {

      
#ifdef DEBUG_TxRx_Datagram
    if ( frame.frameHdr.ACKCount ) {
      printf("Responding to frames, count = %d, start = %d\n", frame.frameHdr.ACKCount,
	     frame.frameHdr.ACKStart );
    }
#endif

    myParent().post( frame );

  }


  Frame * 
  DatagramXferServices::  
  nextFreeFrame( )  {
    OCPI::Util::SelfAutoMutex guard ( this );
    int seq = m_frameSeq++;
    unsigned  mseq = seq & FRAME_SEQ_MASK;
    ocpiAssert( mseq < m_freeFrames.size() );
    Frame & f = m_freeFrames[mseq];
    ocpiAssert( f.is_free );
    f.frameHdr.frameSeq = seq;

    //printf("**** Our Frame Seq = %d\n", f.frameHdr.frameSeq );


    f.is_free = false;
    return &f;
  }


  // Here are frames that we sent that are being ACK'ed
  void 
  DatagramXferServices::
  ack(  unsigned count, unsigned start ) {
    OCPI::Util::SelfAutoMutex guard ( this );
    for ( unsigned n=start; n<(start+count); n++ ) {
      //      printf("ACKING frame id = %d\n", n);
      releaseFrame ( n );
    }
  }


  // This is the list of ACK's that we have to send
  void 
  DatagramXferServices::
  addFrameAck( DatagramFrameHeader * hdr ) {
    OCPI::Util::SelfAutoMutex guard ( this );
    m_acks.push_back( hdr->frameSeq );

    //#define ACK_NOW
#ifdef ACK_NOW
    int bytes_left;
    Frame & frame = getFrame( bytes_left );
    frame.frameHdr.ackOnly = 1;
    post( frame );
#endif

  }

  void 
  DatagramXferServices::
  releaseFrame ( unsigned seq ) {	
    unsigned mseq = seq & FRAME_SEQ_MASK;

    // This can occur if we are too agressive with a retry and end up getting back Two ACK's 
    // for the re-transmitted frame.
    if ( m_freeFrames[mseq].is_free ) {
      return;
    }
    m_freeFrames[mseq].is_free = true;
    m_freeFrames[mseq].resends = 0;
    m_freeFrames[mseq].release();
  }

  void 
  DatagramXferServices::
  sendAcks( uint64_t time_now, uint64_t timeout )
  {
    OCPI::Util::SelfAutoMutex guard ( this );
    return;

    if ( ( time_now - m_last_ack_send ) > timeout ) {
      int bytes_left;
      Frame & frame = getFrame( bytes_left );
      frame.frameHdr.flags = 1;
      post( frame );
    }
  }


  Frame & 
  DatagramXferServices::
  getFrame(  int & bytes_left  )
  {
    OCPI::Util::SelfAutoMutex guard ( this );

    Frame & frame = *nextFreeFrame(); 

    bytes_left = (int) parent().txDriver().maxPayloadSize();

    frame.frameHdr.destId = m_targetSmb->endpoint()->mailbox;
    frame.frameHdr.srcId =  m_sourceSmb->endpoint()->mailbox;

    // We will piggyback any pending acks here
    frame.frameHdr.ACKCount = 0;
    if ( m_acks.size() ) {
      m_last_ack_send = fasttime_getticks();
      frame.frameHdr.ACKStart = m_acks[0];
      frame.frameHdr.ACKCount++;
      m_acks.pop_front();
      for ( unsigned  y=0; y<m_acks.size(); y++ ) {
	if ( m_acks[y]  == (unsigned)(frame.frameHdr.ACKStart+1)) {
	  m_acks.pop_front();
	  y = 0;
	  frame.frameHdr.ACKCount++;
	  continue;
	}
	else {
	  break;
	}
      }
    }

    // Here we have to keep track of the number of bytes that we are injecting into the payload
    frame.sock_adr.sin_family = 2;	
    //    frame.sock_adr.sin_port = htons( ( (DatagramSmemServices*)t.m_txTemplate->dsmem)->getEndPoint()->getId()  );
    frame.sock_adr.sin_port = htons( m_targetSmb->endpoint()->getId() );
    //    inet_aton( ((DatagramSmemServices*)t.m_txTemplate->dsmem)->getEndPoint()->getAddress() , &frame.sock_adr.sin_addr);	
    inet_aton( m_targetSmb->endpoint()->getAddress() , &frame.sock_adr.sin_addr);	
    frame.msg.msg_name = &frame.sock_adr;	
    frame.msg.msg_namelen = sizeof( frame.sock_adr );
    frame.msg.msg_control = 0;
    frame.msg.msg_controllen = 0;
    frame.msg.msg_flags = 0;    
    frame.msg.msg_iov = &frame.iov[0];

    // This is a two byte pad for compatibility with the 14 byte ethernet header.
    frame.msg.msg_iovlen = 0;
    frame.iov[frame.msg.msg_iovlen].iov_base = (caddr_t) &frame.frameHdr;
    frame.iov[frame.msg.msg_iovlen].iov_len = 2;
    frame.msg.msg_iovlen++;
    frame.iov[frame.msg.msg_iovlen].iov_base = (caddr_t) &frame.frameHdr;
    frame.iov[frame.msg.msg_iovlen].iov_len = sizeof(frame.frameHdr);	
    bytes_left -= ((int)frame.iov[0].iov_len + 2);
    frame.msg.msg_iovlen++;
		
    return frame;
  }


  void 
  DatagramXferRequest::
  post( DatagramTransaction & t )
  {
    int bytes_left;
    unsigned msg = 0;

    while ( msg < t.msgCount() ) {

      // calculate the next message size
      if ( (int)(sizeof(DatagramMsgHeader) + t.hdrPtr(msg)->dataLen) > bytes_left ) {
	bytes_left = 0;
      }

      Frame & frame = myParent().getFrame( bytes_left );
      frame.frameHdr.flags = 0;
      frame.transaction = &t;
      frame.msg_count =0;
      frame.msg_start = msg;	  

      // Stuff as many messages into the frame as we can
      while ( (bytes_left > 0) && (msg<t.msgCount()) ) {

	if ( bytes_left < (int)( sizeof(DatagramMsgHeader) + t.hdrPtr(msg)->dataLen + 7 ) ) {
	  // Need a new frame
	  t.hdrPtr(msg-1)->nextMsg = false;
	  break;
	}

	frame.iov[frame.msg.msg_iovlen].iov_base = (caddr_t) t.hdrPtr(msg);
	frame.iov[frame.msg.msg_iovlen].iov_len = sizeof(DatagramMsgHeader);
	frame.msg.msg_iovlen++;
	frame.iov[frame.msg.msg_iovlen].iov_base = (caddr_t) t.srcPtr(msg);
	frame.iov[frame.msg.msg_iovlen].iov_len = t.hdrPtr(msg)->dataLen;
	// Adjust the aligment so that the next header is on a 8 byte boundary
	frame.iov[frame.msg.msg_iovlen].iov_len = (frame.iov[frame.msg.msg_iovlen].iov_len + 7) & ~7; 
	frame.msg.msg_iovlen++;
	bytes_left = bytes_left - (frame.iov[frame.msg.msg_iovlen-2].iov_len +
				   frame.iov[frame.msg.msg_iovlen-1].iov_len);
	t.hdrPtr(msg)->nextMsg = true;
	msg++;
	frame.msg_count++;
      }
      t.hdrPtr(msg-1)->nextMsg = false;
      queFrame( frame );	  
    }	
  }


  void 
  DatagramXferRequest::
  post()
  {
    m_transaction.m_nMessagesRx = 0;

    // #define TRACE_Datagram_XFERS  
#ifdef TRACE_Datagram_XFERS
    ocpiDebug("Datagram: copying %d bytes from 0x%llx to 0x%llx", transfer->nbytes,transfer->src_off,transfer->dst_off);
    ocpiDebug("source wrd 1 = %d", src1[0] );
#endif
    post( m_transaction );

  }

  volatile static uint32_t g_txId;
  static uint32_t getNextId()
  {
    g_txId++;
    return g_txId;
  }

  void 
  DatagramXferRequest::DatagramTransaction::
  init( uint32_t nMsgs, TxTemplate * temp  ) {
    ocpiAssert( ! m_init );
    m_nMessagesTx = 0;
    m_messages.reserve( nMsgs + 1 );
    m_txTemplate = temp;
    m_init = true;
    m_tid = getNextId();
  }

  void 
  DatagramXferRequest::DatagramTransaction::
  fini( uint32_t src, uint32_t dst, TxTemplate * temp ) {
    std::vector<Message>::iterator it;
    m_txTemplate=temp;
    uint32_t* sptr =(uint32_t*)m_txTemplate->ssmem->map(src, 4);	  
    for ( it=m_messages.begin(); it!=m_messages.end(); it++ ) {
      (*it).hdr.flagAddr = dst;
      (*it).hdr.flagValue = *sptr;
    }
  }

  
  void 
  DatagramXferRequest::DatagramTransaction::
  add(DatagramXferRequest* ,  uint8_t * src, uint64_t dst_offset, uint32_t length, uint32_t tx_total )
  {
    if ( m_nMessagesTx>= m_messages.size() ) {
      m_messages.reserve( m_messages.size() + 10 );
    }

#ifdef PKT_DEBUG
    struct sockaddr_in * in = (struct sockaddr_in *)&sad;
    int df = ((struct sockaddr_in *)&sad)->sin_family;
    int port = ntohs ( ((struct sockaddr_in *)&sad)->sin_port );
    char * a  = inet_ntoa ( ((struct sockaddr_in *)&sad)->sin_addr );
    struct in_addr  adrr  =  ((struct sockaddr_in *)&sad)->sin_addr;
#endif

    m_messages.push_back( m_messages[m_nMessagesTx] );
    m_messages[m_nMessagesTx].hdr.type = DataTransfer::DatagramMsgHeader::DATA;
    m_messages[m_nMessagesTx].hdr.numMsgsInTransaction = tx_total;
    m_messages[m_nMessagesTx].hdr.msgSequence = m_nMessagesTx;
    m_messages[m_nMessagesTx].hdr.dataAddr = (uint32_t)dst_offset;
    m_messages[m_nMessagesTx].hdr.dataLen = length;
    m_messages[m_nMessagesTx].src_adr = src;
    m_messages[m_nMessagesTx].hdr.transactionId = m_tid; 
    m_nMessagesTx++;


  }


  
  DataTransfer::XferRequest::CompletionStatus 
  DatagramXferRequest::getStatus()
  { 
    // We only need to check the completion status of the flag transfer since it is gated on the 
    // completion status of the data and meta-data transfers
    if ( ! m_transaction.complete() ) {
      return DataTransfer::XferRequest::Pending;
    }
    else {
      return DataTransfer::XferRequest::CompleteSuccess;
    }
  }

  
  DatagramSocket::
  ~DatagramSocket()
  {

  }

  DatagramSmemServices::
  ~DatagramSmemServices ()
  {
    delete [] m_mem;
    if ( m_socket ) {
      delete m_socket;
    }
    stop();
    join();
  }


    void 
    FrameMonitor::
    run() {
      const uint64_t timeout = 200 * 1000 * 1000;  // timeout in mSec
      uint64_t time_now;

      while ( m_loop ) {
	
	// If we have not received an ACK after (timeout) send the frame again.
	std::vector< DatagramXferServices * > & xfer_services =  m_lsmem->getAllXferServices();
	for ( unsigned n=0; n<xfer_services.size(); n++ ) {
	  if ( xfer_services[n] != NULL ) {
	    time_now = fasttime_getticks();
	    xfer_services[n]->checkAcks(time_now, timeout);
	    OCPI::OS::sleep(2);
	    time_now = fasttime_getticks();
	    xfer_services[n]->sendAcks( time_now, timeout/3 );
	    OCPI::OS::sleep(2);
	  }
	}
	
	OCPI::OS::sleep(2);
      }

    };



  void 
  DatagramXferServices::    
  checkAcks( uint64_t time, uint64_t time_out )
  {    
    OCPI::Util::SelfAutoMutex guard ( this );
    for ( unsigned n=0; n<m_freeFrames.size(); n++ ) {
      if ( ! m_freeFrames[n].is_free ) {
	if ( ( time - m_freeFrames[n].send_time ) > time_out ) {

	  // We are multi-threaded and this can happen if a post occurs after this
	  // method id called
	  if ( time > m_freeFrames[n].send_time ) {
	    
	    m_freeFrames[n].resends++;
	    if ( m_freeFrames[n].resends < 4 ) {
	      printf("***** &&&&& Found a dropped frame, re-posting \n");
	      post( m_freeFrames[n] );
	    }
	    else {
	      ocpiAssert(!"Exceeded resend limit !!\n");
	    }
	  }
	}
      }
    }
  }

  void 
  DatagramXferServices::  
  processFrame( DatagramFrameHeader * header )
  {
    OCPI::Util::SelfAutoMutex guard ( this );
    DatagramMsgHeader *   msg;
    uint8_t *             data;	

    // It is possible for the sender to duplicate frames by being too agressive with re-retries
    // Dont process dups. 
    if (  m_frameSeqRecord[ header->frameSeq & MAX_FRAME_HISTORY ].id == header->frameSeq ) {
      
      printf("max history = %d, SID = %d, fqr size = %d mask=%d, seq = %d this = %p \n", 
	     MAX_FRAME_HISTORY, header->srcId,  m_frameSeqRecord.size(), 
	     header->frameSeq&MAX_FRAME_HISTORY, header->frameSeq, this
	     );

      if (  ! m_frameSeqRecord[ header->frameSeq & MAX_FRAME_HISTORY ].acked  ) {
	ocpiAssert( !"programming error, cant have dup without ACK ");
      }	    

      printf("********  Found a duplicate frame, Ignoring it !! \n");
      // Need to ACK the dup anyway
      m_frameSeqRecord[ header->frameSeq & MAX_FRAME_HISTORY ].acked = true;
      addFrameAck( header );
      return;

    }
    else {
      m_frameSeqRecord[ header->frameSeq & MAX_FRAME_HISTORY ].id = header->frameSeq;
    }

    // This frame contains ACK responses	       
    if ( header->ACKCount ) {
      ack(  header->ACKCount, header->ACKStart );
    }
    if ( header->flags ) return;


    // For our response
    m_frameSeqRecord[ header->frameSeq & MAX_FRAME_HISTORY ].acked = true;
    addFrameAck( header );


    msg = reinterpret_cast<DatagramMsgHeader*>( &header[1] );	    
    do {

      MsgTransactionRecord & fr =
	m_msgTransactionRecord[ (msg->transactionId & MAX_TRANSACTION_HISTORY) ];
      if ( ! fr.in_use ) {
	fr.in_use = true;
	fr.msgsProcessed = 0;
	//	frames_in_play++;
	//	ocpiAssert( frames_in_play < MAX_TRANSACTION_HISTORY );
      }
      fr.transactionId = msg->transactionId;
      fr.numMsgsInTransaction = msg->numMsgsInTransaction;	      

      if ( msg->dataLen ) {
	data = reinterpret_cast<uint8_t*>(&msg[1]);
      }

      ocpiDebug("Msg info -->  addr=%d len=%d tid=%d",
		msg->dataAddr, msg->dataLen, msg->transactionId );
      
      switch ( msg->type ) {

      case DataTransfer::DatagramMsgHeader::DATA:
      case DataTransfer::DatagramMsgHeader::METADATA:
	{

	  char* dptr =(char*)m_sourceSmb->map(msg->dataAddr, msg->dataLen);	  
	  if ( msg->dataLen == 4 ) {

#ifdef DEBUG_TxRx_Datagram
	    printf(" FLAG -> Writing %d bytes to offset %d, tid=%d\n",  msg->dataLen, msg->dataAddr, msg->transactionId );
	    int * v = (int*)&msg[1];
	    printf("replacing flag value %d with %d\n", (int)*dptr, (int)*v);
#endif
	  }
	  else {
	    memcpy(dptr, data, msg->dataLen);
	  }

	}
	break;

	// Not yet handled
      case DataTransfer::DatagramMsgHeader::DISCONNECT:
	break;	    
	ocpiAssert(! "Unhandled Datagram message type");
      }

      fr.msgsProcessed++;
      // Close the transaction if needed
      if ( fr.msgsProcessed == fr.numMsgsInTransaction ) {
	char* dptr =(char*)m_sourceSmb->map(msg->flagAddr, 4);	  

#ifdef DEBUG_TxRx_Datagram
	printf("**** Finalizing transaction %d, addr = %d, value = %d, old value=%d\n", 
	       msg->transactionId, msg->flagAddr,msg->flagValue, *dptr);
#endif

	memcpy(dptr, &msg->flagValue, 4);
	fr.in_use = false;
	//	frames_in_play--;
      }

      if  ( msg->nextMsg ) {

#ifdef DEBUG_TxRx_Datagram
	printf("Got a multi message frame !!\n");
#endif

	uint8_t * d = (uint8_t*)&msg[1];
	int len = (msg->dataLen +7) & ~7;
	d += len;
	msg = reinterpret_cast<DatagramMsgHeader*>(d);
      }
      else {
	msg = NULL;
      }

    } while ( msg );	    

  }


  void 
  DatagramSmemServices::
  addXfer( DatagramXferServices * s )
  {
    if (s->m_targetSmb->endpoint()->mailbox >= m_xferServices.size() ) {
      m_xferServices.resize( s->m_targetSmb->endpoint()->mailbox + 8 );
    }
    m_xferServices[ s->m_targetSmb->endpoint()->mailbox ] = s;
  }
  
  void 
  TxTemplate::
  config( SmemServices * src, SmemServices * dst )
  {
    ssmem = src;
    dsmem = dst;
  }

}
