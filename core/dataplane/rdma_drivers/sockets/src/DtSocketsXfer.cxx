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

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <deque>
#include <DtSharedMemoryInternal.h>
#include <DtSocketsXfer.h>
#include <xfer_if.h>
#include <xfer_internal.h>
#include <OcpiList.h>
#include <OcpiUtilHash.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtExceptions.h>
#include <OcpiThread.h>

#include <OcpiOsSocket.h>
#include <OcpiOsServerSocket.h>

namespace DataTransfer {

namespace OU = OCPI::Util;
namespace OS = OCPI::OS;


#define TCP_BUFSIZE_READ 4096

struct SocketDataHeader {
  DtOsDataTypes::Offset offset;
  uint32_t   length;
  uint32_t   count;
};

class SocketServerT;
// Shared memory services.  
class SocketSmemServices : public SmemServices
  {
  public:
    SocketSmemServices (EndPoint& ep)
      :SmemServices(ep),m_socketServerT(NULL)//,m_ep(ep)
    {
      m_mem = new char[ep.size];
      memset( m_mem, 0, ep.size );
    };
    int32_t attach(EndPoint* loc) { ( void ) loc; return 0;};
    int32_t detach() {return 0;}
    void* map(DtOsDataTypes::Offset offset, size_t/* size */)
    {
      return &m_mem[offset];
    }
    int32_t unMap (){return 0;}
    //    EndPoint* getEndPoint (){return m_ep;}
    virtual ~SocketSmemServices ();

  public: 

    SocketServerT * m_socketServerT;


  private:
    //    EndPoint* m_ep;
    char* m_mem;
  };



struct SocketStartupParams {
    SocketStartupParams ( )
    : rsmem ( 0 ),
      lsmem ( 0 )
    { }
    SmemServices*  rsmem;        
    SmemServices*  lsmem;
};


class ClientSocketT
{
public:

  ClientSocketT( SocketStartupParams& sp )
    :m_startupParms(sp)
  {
    SocketEndPoint *rsep = static_cast<SocketEndPoint*>(m_startupParms.rsmem->endpoint());
    m_socket.connect(rsep->ipAddress,rsep->portNum);
    m_socket.linger(false);
  }
  ~ClientSocketT( )
  {
    ocpiDebug("In ~ClientSocketT()");
    m_socket.close();
  }
  OS::Socket& socket(){return m_socket;}

private:
  OS::Socket                m_socket;
  SocketStartupParams            m_startupParms;
};


  class ServerSocketHandler : public OU::Thread
{
public:
  ServerSocketHandler( OS::ServerSocket & server, SocketStartupParams & sp  )
    : m_run(true), m_startupParms(sp) {
    server.accept(m_socket);
    m_socket.linger(true); // we want to give some time for data to the client FIXME timeout param?
    start();
  }

  virtual ~ServerSocketHandler()
  {
    stop();
    join();
    ocpiDebug("In ~ServerSocketHandler()");
  }


  void run() {
    char * current_ptr;
    size_t    bytes_left = 0;
    SocketDataHeader header;
    bool in_header = true;;
    try {
      while ( m_run ) {
	char   buf[TCP_BUFSIZE_READ];
	// FIXME: verify that a timeout is set on this socket.
	size_t n = m_socket.recv( buf, TCP_BUFSIZE_READ, 500);
	if ( n == 0 ) {
	  ocpiInfo("Got a socket EOF, terminating connection");
	  break;
	} else if (n == ~(size_t)0)
	  continue; // allow shutdown to happen
	//	ocpiDebug("Got %lld bytes data on server socket !! %d %d %lld %d", n, bytes_left, in_header,
	//		  header.offset, header.length);
	size_t copy_len;
	for (char *bp = buf; n; n -= copy_len, bp += copy_len) {
	  // We are either filling a header or filling a request based on a header.
	  if (bytes_left == 0) {
	    if (in_header) {
	      current_ptr = (char *)&header;
	      bytes_left = sizeof(header);
	    } else {
	      ocpiDebug("Received Header: %8x: %" PRIx32 " %" PRIx32,
			header.count, header.length, header.offset);
	      current_ptr =(char*)m_startupParms.lsmem->map(header.offset, header.length);
	      bytes_left = header.length;
	    }
	  }
	  copy_len = (n <= bytes_left) ? n : bytes_left;
	  ocpiDebug("Copying socket data to %p, size = %zu, in header %d, left %zu, first %lx",
		    current_ptr, copy_len, in_header, bytes_left, *(unsigned long*)bp);
	  memcpy(current_ptr, bp, copy_len );
	  current_ptr += copy_len;
	  if (!(bytes_left -= copy_len))
	    in_header = !in_header;
	  //ocpiDebug("After copying socket data to %p, size = %u, in header %d, left %d %lld %d",
	  //current_ptr, copy_len, in_header, bytes_left, header.offset, header.length);
	}
      }
    } catch (std::string &s) {
      ocpiBad("Exception in socket background thread: %s", s.c_str());
    } catch (...) {
      ocpiBad("Unknown exception in socket background thread");
    }
    m_socket.close();
  }

  void stop(){m_run=false;}

private:
  bool   m_run;
  SocketStartupParams   m_startupParms;
  OS::Socket m_socket;
};


class SocketServerT : public OCPI::Util::Thread
{
public:  
  SocketServerT( SocketStartupParams& sp )
    :m_startupParms(sp),m_stop(false),m_started(false),m_error(false){}
  ~SocketServerT(){
    while (!m_sockets.empty()) {
      ServerSocketHandler* ssh = m_sockets.front();
      m_sockets.pop_front();
      delete ssh;
    }
  }

  void run() {
    SocketEndPoint *sep = static_cast<SocketEndPoint*>(m_startupParms.lsmem->endpoint());
   
    try {
      m_server.bind(sep->portNum,false);
    }
    catch( std::string & err ) {
      m_error=true;
      ocpiBad("Socket bind error. %s", err.c_str() );
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
      SocketXferFactory::setEndpointString(sep->end_point, sep->ipAddress.c_str(),
					   sep->portNum, sep->size, sep->mailbox,
					   sep->maxCount);
    }
    m_started = true;
    while ( ! m_stop )
      if (m_server.wait(500)) // give a chance to stop every 1/2 second
	m_sockets.push_back(new ServerSocketHandler(m_server, m_startupParms));
    m_server.close();
  }
  void stop(){m_stop=true;}
  void btr(){while (!m_started)OS::sleep(10);}
  bool error(){return m_error;}


private:
  OS::ServerSocket         m_server;
  std::deque<ServerSocketHandler*>  m_sockets;
  SocketStartupParams   m_startupParms;
  bool                  m_stop;
  bool                  m_started;
  bool                  m_error;
};



const char *socket = "socket"; // name passed to inherited template class
SocketXferFactory::SocketXferFactory()
  throw ()
{
  // Empty
}

// Destructor
SocketXferFactory::~SocketXferFactory()
  throw ()
{
  //  clearCache();
}

// This method is used to allocate a transfer compatible SMB
SmemServices& SocketEndPoint::createSmemServices()
{

  SocketSmemServices * smem = new SocketSmemServices(*this);

  if (local) {
    // Create our listener socket thread so that we can respond to incoming
    // requests  
    SocketStartupParams sp;
    sp.lsmem = smem;
    smem->m_socketServerT = new SocketServerT( sp );
    smem->m_socketServerT->start();
    smem->m_socketServerT->btr();  
  }
  return *smem;
}


/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* SocketXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  return new SocketXferServices(source, target);
}

XferRequest* SocketXferServices::createXferRequest()
{
  return new SocketXferRequest( *this, m_xftemplate );
}

static std::string sep;

// This is static
void SocketXferFactory::
setEndpointString(std::string &ep, const char *ipAddr, unsigned port,
		  size_t size, uint16_t mbox, uint16_t maxCount)
{
  char tep[128];
  snprintf(tep, 128, "ocpi-socket-rdma:%s:%u;%zu.%" PRIu16 ".%" PRIu16,
	   ipAddr, port, size, mbox, maxCount);
  ep = tep;
}
std::string SocketXferFactory::
allocateEndpoint(const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes)
{
  OCPI::Util::SelfAutoMutex guard (this);
  std::string ep;
  char ip_addr[128];


//#define USE_LOOPBACK
#ifdef USE_LOOPBACK
  strcpy( ip_addr, "127.0.0.1");
#else

  const char* env = getenv("OCPI_TRANSFER_IP_ADDR");
  if( !env || (env[0] == 0)) {
    ocpiDebug("Set ""OCPI_TRANSFER_IP_ADDR"" environment variable to set socket IP address");
    gethostname(ip_addr,128);
  }
  else {
    strcpy(ip_addr,env);
  }
#endif

  int port;
  const char* penv = getenv("OCPI_TRANSFER_PORT");
  if( !penv || (penv[0] == 0)) {
    ocpiDebug("Set ""OCPI_TRANSFER_PORT"" environment variable to set socket IP address");
    port = 0;//getNextPortNum();
  }
  else {
    static int m_port = 0;
    if ( m_port == 0 ) {
      m_port = atoi(penv);
    }
    port = m_port++;
  }
  setEndpointString(ep, ip_addr, port, parent().getSMBSize(), mailBox, maxMailBoxes);
  sep = ep;
  return ep;
}

  EndPoint* SocketXferFactory::
  createEndPoint(std::string& endpoint, bool local) {
    return new SocketEndPoint(endpoint, local);
  }

int32_t SocketEndPoint::
parse(std::string& ep) {
  // Note that IPv6 addresses may have colons, even though colons are commonly used to
  // separate addresses from ports.  Since there must be a port, it will be after the last
  // colon.  There is also a convention that IPV6 addresses embedded in URLs are in fact
  // enclosed in square brackets, like {--ipv6-addr-with-colons]:port
  // So this scheme will work whether the square bracket convention is used or not
  const char
    *firstcolon = strchr(ep.c_str(), ':'), // after our protocol name
    *lastcolon = strrchr(ep.c_str(), ':');  // before the port
  if (strncasecmp("ocpi-socket-rdma:", ep.c_str(), (firstcolon - ep.c_str()) + 1) ||
      sscanf(lastcolon+1, "%hu;", &portNum) != 1)
    throw OU::Error("Invalid socket endpoint format in \"%s\"", ep.c_str());
  // FIXME: we could do more parsing/checking on the ipaddress
  ipAddress.assign(firstcolon+1, (lastcolon - firstcolon) - 1);
  return 0;
}

SocketEndPoint::
~SocketEndPoint()
{
  // Empty
}

// SocketXferRequest destructor implementation
SocketXferRequest::~SocketXferRequest ()
{
}


// Create tranfer services template
void SocketXferServices::createTemplate (SmemServices* p1, SmemServices* p2)
{

  m_txRequest = NULL;
  m_sourceSmb = static_cast<SocketSmemServices*>(p1);
  m_targetSmb = static_cast<SocketSmemServices*>(p2);

  xfer_create (p1, p2, 0, &m_xftemplate);

  // We need to create a client socket here
  SocketStartupParams ssp;
  ssp.lsmem = p1;
  ssp.rsmem = p2;
  m_clientSocketT = new ClientSocketT( ssp );

}

// Destructor
SocketXferServices::
~SocketXferServices ()
{

  // Invoke destroy without flags.
  xfer_destroy (m_xftemplate, 0);

  if ( m_clientSocketT ) {
    delete m_clientSocketT;
  }
}


SocketSmemServices::
~SocketSmemServices ()
{
  if ( m_socketServerT ) {
    m_socketServerT->stop();
    m_socketServerT->join();
    delete m_socketServerT;
  }
  delete [] m_mem;
}

void SocketXferRequest::
action_transfer(PIO_transfer transfer)
{
  //#define TRACE_PIO_XFERS  
#ifdef TRACE_PIO_XFERS
  ocpiDebug("Socket: copying %d bytes from 0x%llx to 0x%llx", transfer->nbytes,transfer->src_off,transfer->dst_off);
  ocpiDebug("source wrd 1 = %d", src1[0] );
#endif
  SocketDataHeader hdr;
  static uint32_t count = 0xabc00000;
  hdr.length = (uint32_t)transfer->nbytes;
  hdr.offset = transfer->dst_off;
  hdr.count = count++;
  ocpiDebug("Sending IP header %zu %" PRIu32 " %" DTOSDATATYPES_OFFSET_PRIx" %" PRIx32,
	    sizeof(SocketDataHeader), hdr.length, hdr.offset, hdr.count);
  size_t nb;  
  unsigned trys = 10;
  char* cp = (char*)&hdr;
  for (size_t btt = sizeof(SocketDataHeader);
       trys && btt && (nb = parent().m_clientSocketT->socket().send(cp, btt)) != 0;
       btt -= nb, cp += nb, trys--)
    ;
  if ( trys <= 0 )
    throw OU::EmbeddedException("Socket write error during data transfer operation\n");

  ocpiDebug("Sending IP data %zu %x", transfer->nbytes, *(uint32_t*)transfer->src_va);
#ifndef NDEBUG
  OS::sleep( 100 );
#endif

  trys = 10;
  cp = (char *)transfer->src_va;
  for (size_t btt = transfer->nbytes;
       trys && btt && (nb = parent().m_clientSocketT->socket().send(cp, btt)) != 0;
       btt -= nb, cp += nb)
    ;
  if ( trys <= 0 )
    throw OU::EmbeddedException("Socket write error during data transfer operation\n");
}

#define SOCKET_RDMA_SUPPORT
#ifdef SOCKET_RDMA_SUPPORT
// Used to register with the data transfer system;
  RegisterTransferDriver<SocketXferFactory> socketDriver;
#endif

}
