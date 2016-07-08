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
 *   This file contains the interface for the Ocpi UDP transfer driver.
 *
 *  John Miller -  5-24-12
 *  Initial version
 *
 */
#include <inttypes.h>
#include <DtSharedMemoryInternal.h>
#include <xfer_if.h>
#include <xfer_internal.h>
#include <OcpiList.h>
#include <OcpiUtilHash.h>
#include <OcpiUtilMisc.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtExceptions.h>
#include <OcpiThread.h>
#include <DtUDPSocketXfer.h>

namespace DataTransfer {

  using namespace OCPI::Util;
  using namespace OCPI::OS;
  namespace DDT = DtOsDataTypes;
  class UDPSocketServerT;

  // Shared memory services.  
  class UDPSmemServices : public SmemServices
  {
  public:
    UDPSmemServices (EndPoint& ep)
      :SmemServices(ep),m_socketServerT(NULL)
    {
      //      m_ep = static_cast<UDPEndPoint*>(ep);
      m_mem = new char[ep.size];
      memset( m_mem, 0, ep.size );
    };
    int32_t attach(EndPoint* loc){ ( void ) loc; return 0;};
    int32_t detach(){return 0;}
    void* map(DtOsDataTypes::Offset offset, size_t/* size */)
    {
      return &m_mem[offset];
    }
    int32_t unMap (){return 0;}
    UDPEndPoint* getEndPoint (){return static_cast<UDPEndPoint*>(&m_endpoint);}
    virtual ~UDPSmemServices ();

  public: 
    UDPSocketServerT * m_socketServerT;

  private:
    //    UDPEndPoint* m_ep;
    char* m_mem;
  };

  struct UDPSocketStartupParams {
    UDPSocketStartupParams ( )
      : lsmem ( 0 )
    { }
    SmemServices*  lsmem;
  };


  class DropPktMonitor : public Thread
  {
  public:
    DropPktMonitor( UDPSocketXferServices * xfs )
      :m_xfs(xfs),m_loop(true){}
    void run() {
      while ( m_loop ) {

	UDPSocketXferRequest * req = m_xfs->firstChild();
	while ( req ) {
	  req->checkDrops();
	  req = req->nextChild();
	};
	OCPI::OS::sleep(10);
      }
    };
    void stop(){m_loop=false;}

  private:
    UDPSocketXferServices * m_xfs;
    bool m_loop;
  };


  UDPSocketXferServices::
  UDPSocketXferServices(SmemServices *a_source, SmemServices *a_target)
    : ConnectionBase<UDPSocketXferFactory,UDPSocketXferServices,UDPSocketXferRequest>(*this,
										      a_source,
										      a_target)
  {
    m_dpMonitor = new DropPktMonitor(this);
    m_dpMonitor->start();
    createTemplate(a_source, a_target);
  }



  class ServerUDPSocketHandler : public Thread
  {
  public:
    ServerUDPSocketHandler( OCPI::OS::ServerSocket & socket, UDPSocketStartupParams & sp  )
      : m_run(true), m_startupParms(sp), m_server(socket) {}

    virtual ~ServerUDPSocketHandler()
    {
      m_server.close();
      ocpiDebug("In ~ServerUDPSocketHandler()");
    }

#define RX_BUFFER_SIZE (1024*8)
    void run() {

      unsigned int    bytes_left = 0;
      UDPSocketXferRequest::UDPSocketDataHeader * header;
      bool in_header = true;

      try {
	while ( m_run ) {
	  char   buf[RX_BUFFER_SIZE];
	  struct sockaddr sad;
	  size_t size = sizeof( struct sockaddr);
	  size_t n = m_server.recvfrom( buf,RX_BUFFER_SIZE, 0, (char*)&sad, &size);
	  if ( n == 0 ) {
	    ocpiInfo("Got a socket EOF, terminating connection");
	    break;
	  }

#ifdef DEBUG_TxRx_UDP
	  // All DEBUG
	  int port = ntohs ( ((struct sockaddr_in *)&sad)->sin_port );
	  char * a  = inet_ntoa ( ((struct sockaddr_in *)&sad)->sin_addr );
	  printf(" Recved %zu bytes of data on port %d from addr %s port %d\n", n , m_socket.getPortNo(), 
		 a, port );
#endif

	  header = (UDPSocketXferRequest::UDPSocketDataHeader *)buf;

#ifdef DEBUG_TxRx_UDP
	  printf("Message type = %d\n", header->type );
#endif


	  // MORE DEBUG
	  ocpiDebug("Got %zu bytes data on server socket !! %u %d %" DTOSDATATYPES_OFFSET_PRIu " %" PRIu32,
		    n, bytes_left, in_header, header->offset, header->length);

	  switch ( header->type ) {

	  case DataTransfer::UDPSocketXferRequest::UDPSocketDataHeader::Ack:
	    {
	      UDPSocketXferRequest * xfer = reinterpret_cast<UDPSocketXferRequest*>(header->transaction_cookie);
	      xfer->ackReceived( header );
	    }
	    break;

	  case DataTransfer::UDPSocketXferRequest::UDPSocketDataHeader::DMAWrite:
	    {


      // This causes a packet drop for testing
#define DROP_PACKET
#ifdef DROP_PACKET
      const char* env = getenv("OCPI_UDP_DROP_PKTS");
      if ( env != NULL ) {
	static int dropit=1;
	static int dt = 300000;
	static int m = 25357;
	if ( dt && (((++dropit)%m)==0) ) {
	  //	  printf("\n\n\n DROP A PACKET FOR TESTING \n\n\n");
	  dt--;
	  m = 5000 + rand()%10000;
	  continue;
	}
      }
#endif



	      char* dptr =(char*)m_startupParms.lsmem->map(header->offset, header->length);	  

#ifdef DEBUG_TxRx_UDP
	      printf(" Writing %d bytes to offset %lld\n",  header->length, (long long )header->offset );
	      if ( header->length == 4 ) {
		int * v = (int*)&header[1];
		printf("replacing flag value %d with %d\n", (int)*dptr, (int)*v);
	      }
#endif
	      memcpy(dptr, &header[1], header->length);

	      // Send back the ACK now
	      header->type = DataTransfer::UDPSocketXferRequest::UDPSocketDataHeader::Ack;
	      m_server.sendto( (const char*)header, sizeof(UDPSocketXferRequest::UDPSocketDataHeader), 0, (char*)&sad, size );
	    }
	    break;


	    // Not yet handled
	  case DataTransfer::UDPSocketXferRequest::UDPSocketDataHeader::DMARead:
	  case DataTransfer::UDPSocketXferRequest::UDPSocketDataHeader::MutiAck:
	    break;	    
	    ocpiAssert("Unhandled UDP message type"==0);
	  }

	}
      } catch (std::string &s) {
	ocpiBad("Exception in socket background thread: %s", s.c_str());
      } catch (...) {
	ocpiBad("Unknown exception in socket background thread");
      }
      m_server.close();


    }

    void stop(){m_run=false;}

  private:
    bool   m_run;
    UDPSocketStartupParams   m_startupParms;
    OCPI::OS::ServerSocket & m_server;
  };


  class UDPSocketServerT : public OCPI::Util::Thread
  {
  public:  
    UDPSocketServerT( UDPSocketStartupParams& sp )
      :m_startupParms(sp),m_stop(false),m_started(false),m_error(false){}
    ~UDPSocketServerT(){}
    //    OCPI::OS::Socket m_socket;
    OCPI::OS::ServerSocket & socket() { return m_server;}    

    void run() {
      UDPEndPoint *sep = static_cast<UDPEndPoint*>(m_startupParms.lsmem->endpoint());
   
      try {
	m_server.bind(sep->portNum, false, true);
      }
      catch( std::string & err ) {
	m_error=true;
	ocpiBad("UDPSocket bind error. %s", err.c_str() );
	ocpiAssert("Unable to bind to socket"==0);
	return;
      }
      catch( ... ) {
	m_error=true;
	ocpiAssert("Unable to bind to socket"==0);
	return;
      }
      if (sep->portNum == 0) {
	// We now know the real port, so we need to change the endpoint string.
	sep->portNum = m_server.getPortNo();
	UDPSocketXferFactory::setEndpointString(sep->end_point, sep->ipAddress.c_str(),
						sep->portNum, sep->size, sep->mailbox,
						sep->maxCount);
      }
      m_started = true;
      ServerUDPSocketHandler * ssh = new ServerUDPSocketHandler(m_server, m_startupParms);
      m_sockets.push_back( ssh );
      ssh->start();

      while ( ! m_stop ) OCPI::OS::sleep( 2000 );
      m_server.close();
    }
    void stop(){m_stop=true;}
    void btr(){while (!m_started)OCPI::OS::sleep(10);}
    bool error(){return m_error;}


  private:
    OCPI::OS::ServerSocket         m_server;
    std::vector<ServerUDPSocketHandler*>  m_sockets;
    UDPSocketStartupParams   m_startupParms;
    bool                  m_stop;
    bool                  m_started;
    bool                  m_error;
  };

  const char *udpsocket = "udpsocket"; // name passed to inherited template class
  UDPSocketXferFactory::UDPSocketXferFactory()
    throw ()
  {

    ocpiDebug("In PIOXferFactory::PIOXferFactory()");
    // Empty
  }

  // Destructor
  UDPSocketXferFactory::~UDPSocketXferFactory()
    throw ()
  {

  }


  // This method is used to allocate a transfer compatible SMB
  SmemServices& UDPEndPoint::createSmemServices()
  {
    UDPSmemServices * smem = new UDPSmemServices(*this);

    if (local ) {
      // Create our listener socket thread so that we can respond to incoming
      // requests  
      UDPSocketStartupParams sp;
      sp.lsmem = smem;
      smem->m_socketServerT = new UDPSocketServerT( sp );
      smem->m_socketServerT->start();
      smem->m_socketServerT->btr();  
    }
    return *smem;
  }


  /***************************************
   *  This method is used to create a transfer service object
   ***************************************/
  XferServices* UDPSocketXferFactory::getXferServices(SmemServices* source, SmemServices* target)
  {
    return new UDPSocketXferServices(source, target);
  }

  XferRequest* UDPSocketXferServices::createXferRequest()
  {
    return new UDPSocketXferRequest( *this );
  }


  /***************************************
   *  This method is used to dynamically allocate
   *  an endpoint for an application running on "this"
   *  node.
   ***************************************/

  static std::string sep;

  // This is static
  void UDPSocketXferFactory::
  setEndpointString(std::string &ep, const char *ipAddr, unsigned port,
		    size_t size, uint16_t mbox, uint16_t maxCount)
  {
    char tep[128];

    snprintf(tep, 128, "ocpi-udp-rdma:%s;%u:%zu.%" PRIu16 ".%" PRIu16,
	     ipAddr, port, size, mbox, maxCount);
    ep = tep;
  }
  std::string UDPSocketXferFactory::
  allocateEndpoint(const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes,
		   size_t size = 0)
  {
    OCPI::Util::SelfAutoMutex guard (this);

    std::string ep;
    char ip_addr[128];

    const char* env = getenv("OCPI_TRANSFER_IP_ADDR");
    if( !env || (env[0] == 0)) {
      ocpiDebug("Set ""OCPI_TRANSFER_IP_ADDR"" environment variable to set socket IP address");
      gethostname(ip_addr,128);
    }
    else {
      strcpy(ip_addr,env);
    }

    int port;
    const char* penv = getenv("OCPI_TRANSFER_PORT");
    if( !penv || (penv[0] == 0)) {
      ocpiDebug("Set ""OCPI_TRANSFER_PORT"" environment variable to set socket IP address");
      port = 0;
    }
    else {
      static int m_port = 0;
      if ( m_port == 0 ) {
	m_port = atoi(penv);
      }
      port = m_port++;
    }

    const char* mb = getenv("OCPI_MAILBOX");    
    if ( mb ) {
      mailBox = (uint16_t)atoi(mb);
    }

    setEndpointString(ep, ip_addr, port, size ? size : parent().getSMBSize(), mailBox,
		      maxMailBoxes);
    sep = ep;
    return ep;
  }

  EndPoint* UDPSocketXferFactory::
  createEndPoint(std::string& endpoint, bool local) {
    return new UDPEndPoint(endpoint, local);
  }

  // Sets smem location data based upon the specified endpoint
  OCPI::OS::int32_t 
  UDPEndPoint::
  parse( std::string& ep )
  {
    char ipaddr[80];
    int rv = sscanf(ep.c_str(), "ocpi-udp-rdma:%[^;];%hu:", ipaddr, &portNum);
    if (rv != 2) {
      fprintf( stderr, "UDPEndPoint  ERROR: Bad socket endpoint format (%s)\n", ep.c_str() );
      throw DataTransfer::DataTransferEx( UNSUPPORTED_ENDPOINT, ep.c_str() );	  
    }
    ipAddress = ipaddr;  
    return 0;
  }

  UDPEndPoint::
  ~UDPEndPoint()
  {
    // Empty
  }

  void UDPSocketXferRequest::modify(DtOsDataTypes::Offset */*new_offsets[]*/,
				    DtOsDataTypes::Offset */*old_offsets[]*/ )
  {
    ocpiAssert("modify not inplemented"==0);
  }

  // UDPSocketXferRequest destructor implementation
  UDPSocketXferRequest::~UDPSocketXferRequest ()
  {
    // Empty
  }

  // Create tranfer services template
  void UDPSocketXferServices::createTemplate (SmemServices* p1, SmemServices* p2)
  {
    m_sourceSmb = static_cast<UDPSmemServices*>(p1);
    m_targetSmb = static_cast<UDPSmemServices*>(p2);
    m_txTemplate.config( m_sourceSmb, m_targetSmb );
  }

  // Create a transfer request
  XferRequest* UDPSocketXferRequest::copy (DtOsDataTypes::Offset srcoffs, 
					   DtOsDataTypes::Offset dstoffs, 
					   size_t nbytes, 
					   XferRequest::Flags flags
					   )
  {

#ifdef DEBUG_TxRx_UDP
    printf("\n\n *** COPY to %" OCPI_UTIL_RESADDR_PRIx ", len = %zu\n", dstoffs, nbytes );
#endif

    TxPacket * txPkt;
    if (flags & XferRequest::DataTransfer) {
      txPkt = &m_TxPkt[0];
    }    
    else if (flags & XferRequest::FlagTransfer) {
      txPkt = &m_TxPkt[2];
    }
    else {
      txPkt = &m_TxPkt[1];
    }

    
    // For each tranfer at this level we have a header and payload.  We may need to break it up into
    // multiple messages.  Each message gets it own header so each one is self contained and can be
    // acted upon by the receiver without being dependent on previous messages (which could get lost).
#define MAX_UDP_PAYLOAD_SIZE 1500
#define MAX_UDP_DATA_SIZE (MAX_UDP_PAYLOAD_SIZE - sizeof(UDPSocketDataHeader))

    size_t nPackets = (uint16_t)(nbytes / MAX_UDP_DATA_SIZE );
    nPackets +=  (nbytes % MAX_UDP_DATA_SIZE) ? 1 : 0;
    if (flags & XferRequest::DataTransfer) {
      m_txTotal = nPackets + 1;
    }
    size_t  bytes_left = nbytes;
    DtOsDataTypes::Offset  cur_dst_off = dstoffs;

    uint8_t *  cur_src = static_cast<uint8_t *>( parent().m_txTemplate.ssmem->map(srcoffs,0) );

    txPkt->init( nPackets, &parent().m_txTemplate );
    for (size_t m = 0; m < nPackets; m++) {
      ocpiAssert(bytes_left > 0);
      size_t length = bytes_left < (int32_t)MAX_UDP_DATA_SIZE ? bytes_left : MAX_UDP_DATA_SIZE;	
      txPkt->add( this, cur_src, cur_dst_off, length, m_txTotal );
      cur_dst_off += OCPI_UTRUNCATE(DDT::Offset, length);
      cur_src += length;
      bytes_left -= length;
    }

    return this;
  }

  // Group data transfer requests
  XferRequest & UDPSocketXferRequest::group (XferRequest*  )
  {
    return *this;
  }

  // Destructor
  UDPSocketXferServices::
  ~UDPSocketXferServices ()
  {
    m_dpMonitor->stop();
    m_dpMonitor->join();    
  }

  UDPSmemServices::
  ~UDPSmemServices ()
  {
    delete [] m_mem;
    if ( m_socketServerT ) {
      m_socketServerT->stop();
      m_socketServerT->join();
      delete m_socketServerT;
    }
  }

  void 
  UDPSocketXferRequest::
  ackReceived( UDPSocketDataHeader * header )
  {
    uint32_t idx= header->transaction_sequence >> 16;
    unsigned int id =  header->transaction_sequence & 0xffff;
    ocpiAssert( idx < 3 );


    TxPacket * pkt = &m_TxPkt[idx];
    ocpiAssert( id < pkt->m_nAcksTx );
    pkt->m_acks[id].retry_count = 0;

    // This can happen when there is a significant stall on the network or
    // on the remote endpoint.
    if ( pkt->m_acks[id].ack == true ) return;

    pkt->m_acks[id].ack = true;

    // total number of packets Acknowledged
    pkt->m_nAcksRx++;

    ocpiAssert( pkt->m_nAcksRx <= pkt->m_nAcksTx );

    // If we have received all of our acks, send the flag
    if ( (idx<2)  && m_TxPkt[0].complete() && m_TxPkt[1].complete() ) {
      action_socket_transfer( &m_TxPkt[2] );
    }

  }

  void 
  UDPSocketXferRequest::
  post()
  {
    m_TxPkt[2].m_nAcksRx = 0;

    if ( m_TxPkt[0].m_nAcksTx == 0 ) {

      // Flag only transfer "Consume"
      action_socket_transfer( &m_TxPkt[2] );      
    }
    else {
      m_TxPkt[0].m_nAcksRx = m_TxPkt[1].m_nAcksRx = 0;

      // Since this is not a reliable protocol, we can only send data, we
      // will send the flag when we get ACK's back for all packets for 
      // this transaction.
      action_socket_transfer( &m_TxPkt[0] );
      action_socket_transfer( &m_TxPkt[1] );

    }

  }

  void UDPSocketXferRequest::
  action_socket_transfer(TxPacket * pkt )
  {

    // #define TRACE_UDP_XFERS  
#ifdef TRACE_UDP_XFERS
    ocpiDebug("UDPSocket: copying %d bytes from 0x%llx to 0x%llx", transfer->nbytes,transfer->src_off,transfer->dst_off);
    ocpiDebug("source wrd 1 = %d", src1[0] );
#endif

    for ( unsigned int n=0; n<pkt->pktCount(); n++ ) {
      long long nb=0;  

      nb = pkt->sendMsg( n );
      if ( nb == 0 ) {
	throw EmbeddedException("UDPSocket write error during data transfer operation\n");
      }
    };
  }


  void 
  UDPSocketXferRequest::TxPacket::
  init(size_t /*nPackets*/, TxTemplate * temp  ) {
    m_nAcksTx = 0;
    m_acks.reserve( 20 + 1 );
    m_socket = &temp->ssmem->m_socketServerT->socket();
    m_txTemplate = temp;
    m_adr.sin_family = 2;
    m_adr.sin_port = htons( m_txTemplate->dsmem->getEndPoint()->portNum  );
    UDPEndPoint *dep = static_cast<UDPEndPoint *>(m_txTemplate->dsmem->getEndPoint());
    inet_aton(dep->ipAddress.c_str(), &m_adr.sin_addr);
    m_init = true;
  }
  
  void 
  UDPSocketXferRequest::TxPacket::
  add(UDPSocketXferRequest * parent,  uint8_t * src, DtOsDataTypes::Offset dst_offset, size_t length, size_t tx_total )
  {
    if ( m_nAcksTx>= m_acks.size() ) {
      m_acks.reserve( m_acks.size() + 10 );
    }

#ifdef PKT_DEBUG
    struct sockaddr_in * in = (struct sockaddr_in *)&sad;
    int df = ((struct sockaddr_in *)&sad)->sin_family;
    int port = ntohs ( ((struct sockaddr_in *)&sad)->sin_port );
    char * a  = inet_ntoa ( ((struct sockaddr_in *)&sad)->sin_addr );
    struct in_addr  adrr  =  ((struct sockaddr_in *)&sad)->sin_addr;
#endif
    //    ACKTxData ack;
    //    m_acks.push_back( ack );

    m_acks[m_nAcksTx].msg.msg_name = &m_adr;
    m_acks[m_nAcksTx].msg.msg_namelen = sizeof( m_adr );	
    m_acks[m_nAcksTx].msg.msg_control = 0;
    m_acks[m_nAcksTx].msg.msg_controllen = 0;
    m_acks[m_nAcksTx].msg.msg_flags = 0;    
    m_acks[m_nAcksTx].hdr.type = DataTransfer::UDPSocketXferRequest::UDPSocketDataHeader::DMAWrite;
    m_acks[m_nAcksTx].hdr.length = OCPI_UTRUNCATE(uint32_t,length);
    m_acks[m_nAcksTx].hdr.offset = (uint32_t)dst_offset;
    m_acks[m_nAcksTx].hdr.transaction_sequence  = (m_id<<16) | m_nAcksTx;
    m_acks[m_nAcksTx].hdr.transaction_cookie  = (uint64_t)parent;
    m_acks[m_nAcksTx].hdr.transaction_length  = OCPI_UTRUNCATE(uint16_t, tx_total);
    m_acks[m_nAcksTx].iov[0].iov_base = (caddr_t) &m_acks[m_nAcksTx].hdr;
    m_acks[m_nAcksTx].iov[0].iov_len = sizeof(UDPSocketDataHeader);
    m_acks[m_nAcksTx].iov[1].iov_base = (caddr_t) &src[0];
    m_acks[m_nAcksTx].iov[1].iov_len = length;    
    m_acks[m_nAcksTx].msg.msg_iov = &m_acks[m_nAcksTx].iov[0];
    m_acks[m_nAcksTx].msg.msg_iovlen = 2;
    m_nAcksTx++;
  }

  
  DataTransfer::XferRequest::CompletionStatus 
  UDPSocketXferRequest::getStatus()
  { 
    // We only need to check the completion status of the flag transfer since it is gated on the 
    // completion status of the data and meta-data transfers
    if ( ! m_TxPkt[2].complete() ) {
      return DataTransfer::XferRequest::Pending;
    }
    else {
      return DataTransfer::XferRequest::CompleteSuccess;
    }
  }
  
  void
  UDPSocketXferRequest::checkDrops()
  { 
    static const int start_to_worry = 25;

    // We only need to check the completion status of the flag transfer since it is gated on the 
    // completion status of the data and meta-data transfers
    if ( ! m_TxPkt[2].complete() ) {
      m_tested4Complete++;

      if ( m_tested4Complete > start_to_worry ) {

	static int pdc=0;
	pdc++;
	printf("   **** Found a drop packet, drop count = %d!! \n", pdc );

	if ( ! m_TxPkt[0].complete() ) {
	  printf("Dropped a data pkt\n");
	  m_TxPkt[0].retry();	  
	}
	else if ( ! m_TxPkt[1].complete() ) {
	  printf("Dropped a meta data pkt\n");
	  m_TxPkt[1].retry();	  
	}
	// Looks like the data made it but the flag didnt.
	else {
	  // This could lead to a race condition.  If we re-send the flag too soon and the target
	  // actually received the first one, it will appear that it has a new buffer.  So lets make
	  // sure that our worry time is sufficient.

	  printf("Dropped a flag pkt\n");
	  m_TxPkt[2].retry();	  
	}
	m_tested4Complete=0;
      }
    }
    else {
      m_tested4Complete = 0;
    }
  }
  
  void 
  TxTemplate::
  config(   UDPSmemServices * src,   UDPSmemServices * dst )
  {
    ssmem = src;
    dsmem = dst;
  }


#define UDP_RDMA_SUPPORT
#ifdef UDP_RDMA_SUPPORT
  // Used to register with the data transfer system;
  RegisterTransferDriver<UDPSocketXferFactory> UDPSocketDriver;
#endif

}
