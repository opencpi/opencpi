// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * Abstact:
 *   This file contains the implementation for the base class for SMB transfers.
 *
 *  John Miller - 6/15/09
 *  Fixed Coverity issues
 *
 *  John Miller -  7/20/04
 *  Initial version
 *
 */

#include <DtSharedMemoryInternal.h>
#include <DtSocketsXfer.h>
#include <xfer_if.h>
#include <xfer_internal.h>
#include <CpiList.h>
#include <CpiUtilHash.h>
#include <CpiOsMisc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <CpiOsAssert.h>
#include <CpiUtilAutoMutex.h>
#include <DtExceptions.h>
#include <CpiThread.h>

#include <TcpSocket.h>
#include <SocketHandler.h>
#include <ISocketHandler.h>
#include <ListenSocket.h>

using namespace DataTransfer;
using namespace CPI::Util;
using namespace CPI::OS;

CPI::Util::VList SocketXferServices::m_map(0);

#define SOCKET_RDMA_SUPPORT
#ifdef SOCKET_RDMA_SUPPORT
// Used to register with the data transfer system;
SocketXferFactory *g_socketsFactory = new SocketXferFactory;
#endif



namespace DataTransfer {

struct SocketDataHeader {
  uint64_t   offset;
  uint32_t   length;
};


// Shared memory services.  
class SocketSmemServices : public DataTransfer::SmemServices
  {
  public:
    SocketSmemServices (EndPoint* ep)
      :DataTransfer::SmemServices(ep),m_socketServerT(NULL),m_ep(ep)
    {
      m_mem = new char[ep->size];
      memset( m_mem, 0, ep->size );
    };
    CPI::OS::int32_t attach (EndPoint* loc){return 0;};
    CPI::OS::int32_t detach (){return 0;}
    void* map (CPI::OS::uint64_t offset, CPI::OS::uint32_t/* size */)
    {
      return &m_mem[offset];
    }
    CPI::OS::int32_t unMap (){return 0;}
    DataTransfer::EndPoint* getEndPoint (){return m_ep;}
    virtual ~SocketSmemServices ();

  public: 

    SocketServerT * m_socketServerT;
    Socket * m_serverSocket;

  private:
    DataTransfer::EndPoint* m_ep;
    char* m_mem;
  };


// The client socket is used to talk to the associated server and also is 
// required to listen to callbacks for flow control and event notification.
class ClientSocketHandler : public TcpSocket
{
public:

  ClientSocketHandler(ISocketHandler& h )
    :TcpSocket(h)
  {
    //    DisableInputBuffer();  
  }

  ~ClientSocketHandler()
  {
    
  }

  void OnConnect()
  {
    printf("Got a new client connection on client thread ????? !!\n");
  }

  void OnRawData(const char *buf,size_t len) 
  {
    printf("Got %ld bytes of data on client socket ????? \n", len);
  }

private:
        
};



struct SocketStartupParams {
    SmemServices*  rsmem;        
    SmemServices*  lsmem;
};


class ClientSocketT  : public CPI::Util::Thread
{
public:

  ClientSocketT( SocketStartupParams& sp )
    :m_socket(m_sh),m_startupParms(sp),m_stop(false),m_started(false)
  {

  }
  ~ClientSocketT( ){}


  void run() {
    SocketEndPoint *rsep = static_cast<DataTransfer::SocketEndPoint*>(m_startupParms.rsmem->getEndPoint());
    m_socket.Open(rsep->ipAddress,rsep->portNum);
    m_sh.Add(&m_socket);
    m_started = true;
    m_sh.Select(1,0);
    while ( !m_stop )
      {
        m_sh.Select(1,0);
      }
  }
  void stop(){m_stop=true;}
  void btr(){while (!m_started)CPI::OS::sleep(10);}

  ClientSocketHandler& socket(){return m_socket;}  

private:
  SocketHandler         m_sh;
  ClientSocketHandler   m_socket;
  SocketStartupParams   m_startupParms;
  bool                  m_stop;
  bool                  m_started;
};



class ServerSocketHandler : public TcpSocket
{
public:
  ServerSocketHandler(ISocketHandler& h)
    :TcpSocket(h),m_h(h),m_bytes_left(0){}
  ServerSocketHandler(ISocketHandler& h, SocketStartupParams & sp)
    :TcpSocket(h),m_h(h),m_startupParms(sp),m_bytes_left(0){}

  virtual ~ServerSocketHandler(){}

  void OnRead()
  {
    m_bidx=0;
    TcpSocket::OnRead();
    size_t n = ibuf.GetLength();
#ifndef NDEBUG
    printf("Got %ld bytes data on server socket !!\n", n);
#endif
    ibuf.Read(m_buf,n);
    processBuffer( n );
  }


  void processBuffer( unsigned int len )
  {
    // Its a header
    uint64_t off;
    if ( m_bytes_left == 0 ) {
      if ( len >= sizeof(SocketDataHeader) ) {
        SocketDataHeader* hdr = reinterpret_cast<SocketDataHeader*>(&m_buf[m_bidx]);
        m_current_ptr = (char*)m_startupParms.lsmem->map(hdr->offset,hdr->length);
        off = hdr->offset;
        m_bytes_left = hdr->length;
        len -= sizeof(SocketDataHeader);
        m_bidx += sizeof(SocketDataHeader);
      }        
    }

    int copy_len = (len <= m_bytes_left) ? len : m_bytes_left;
#ifndef NDEBUG
    printf("Copying socket data to %lld, size = %d\n", off, copy_len );
#endif
    memcpy( m_current_ptr, &m_buf[m_bidx], copy_len );
    m_bytes_left -= copy_len;
    len -= copy_len;
    m_bidx += copy_len;
    if ( len ) {
      processBuffer( len );
    }
  }

  virtual Socket *Create() 
  { 
    ServerSocketHandler * s = new ServerSocketHandler( m_h, m_startupParms );
    return s;
  }

private:
    ISocketHandler    &  m_h;
  SocketStartupParams   m_startupParms;
  char * m_current_ptr;
  int    m_bytes_left;
  char   m_buf[TCP_BUFSIZE_READ];
  int    m_bidx;

};

class MyListener : public   ListenSocket<ServerSocketHandler> 
{
public:
  MyListener( SocketHandler & h, SocketStartupParams & sp )
    : ListenSocket<ServerSocketHandler>(h),m_startupParms(sp)
  {
    delete m_creator;
    m_creator = new ServerSocketHandler(h,sp);
  }
  private:
    SocketStartupParams   m_startupParms;
};


class SocketServerT : public CPI::Util::Thread
{
public:  
  SocketServerT( SocketStartupParams& sp )
    :m_socket(m_sh,sp),m_startupParms(sp),m_stop(false),m_started(false),m_error(false){}
  ~SocketServerT(){}
  void run() {
    int portNum = static_cast<DataTransfer::SocketEndPoint*>(m_startupParms.lsmem->getEndPoint())->portNum;
   
    try {
      if (m_socket.Bind(portNum,10)) {
        printf("SocketServer: Unable to bind to port %d\n", portNum );
        cpiAssert(!"Failed to bind socket");
      }
    }
    catch( ... ) {
      m_error=true;
      cpiAssert(!"Unable to bind to socket");
      return;
    }
    m_sh.Add(&m_socket);
    m_started = true;
    while ( ! m_stop )  {
      m_sh.Select(1,0);
    }
  }
  void stop(){m_stop=true;}
  void btr(){while (!m_started)CPI::OS::sleep(10);}
  bool error(){return m_error;}
  ListenSocket<ServerSocketHandler> & socket(){return m_socket;}

private:
  SocketHandler         m_sh;
  MyListener            m_socket;
  SocketStartupParams   m_startupParms;
  bool                  m_stop;
  bool                  m_started;
  bool                  m_error;
};


}

SocketXferFactory::SocketXferFactory()
  throw ()
  : XferFactory("Network Socket transfer driver")
{
  printf("In SocketXferFactory::SocketXferFactory()\n");
}




// Destructor
SocketXferFactory::~SocketXferFactory()
  throw ()
{
  clearCache();
}


/***************************************
 *  This method is used to flush any cached items in the factoy
 ***************************************/
void SocketXferFactory::clearCache()
{
  SocketEndPoint *loc;
  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<SocketEndPoint*>(g_locations.getEntry(n));
    delete loc;
  }
  g_locations.destroyList();
}


// Get the location via the endpoint
EndPoint* SocketXferFactory::getEndPoint( std::string& end_point  )
{ 
  CPI::Util::AutoMutex guard ( m_mutex, true ); 

  SocketEndPoint *loc;
  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<SocketEndPoint*>(g_locations.getEntry(n));
    if ( end_point == loc->end_point ) {
      return loc;
    }
  }
  loc = new SocketEndPoint(end_point);
  g_locations.insert( loc );
  return loc;
}

void SocketXferFactory::releaseEndPoint( EndPoint* )
{}


// This method is used to allocate a transfer compatible SMB
SmemServices* SocketXferFactory::createSmemServices(EndPoint* loc )
{
  SocketStartupParams sp;

  sp.lsmem = new SocketSmemServices(loc);
  SocketSmemServices * smem = static_cast<SocketSmemServices*>(sp.lsmem);

  if ( loc->local ) {
    // Create our listener socket thread so that we can respond to incoming
    // requests  
    smem->m_socketServerT = new SocketServerT( sp );
    smem->m_socketServerT->start();
    smem->m_socketServerT->btr();  
    smem->m_serverSocket = & smem->m_socketServerT->socket();
  }
  return sp.lsmem;
}


/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* SocketXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  return new SocketXferServices(source, target);
}


/***************************************
 *  This method is used to dynamically allocate
 *  an endpoint for an application running on "this"
 *  node.
 ***************************************/


static CPI::OS::int32_t mailbox=1;
static CPI::OS::int32_t portNum=40001;
static CPI::OS::int32_t getNextMailBox()
{

  if ( mailbox == 1 ) {
    const char* env = getenv("CPI_TRANSFER_MAILBOX");
    if( !env || (env[0] == 0)) {
      printf("Set ""CPI_TRANSFER_MAILBOX"" environment varible to control mailbox\n");
    }
    else {
      mailbox = atoi(env);
    }
  }

  return mailbox++;
}
static CPI::OS::int32_t getNextPortNum()
{
  return portNum++;
}

static std::string sep;
std::string SocketXferFactory::allocateEndpoint(CPI::OS::uint32_t *size )
{
  CPI::Util::AutoMutex guard ( m_mutex, true ); 
  std::string ep;
  char ip_addr[128];

  // Only one socket based endpoint per process
  if ( sep.length() ) {
    return sep;
  }

  //#define USE_LOOPBACK
#ifdef USE_LOOPBACK
  strcpy( ip_addr, "127.0.0.1");
#else

  const char* env = getenv("CPI_TRANSFER_IP_ADDR");
  if( !env || (env[0] == 0)) {
    printf("Set ""CPI_TRANSFER_IP_ADDR"" environment varible to set socket IP address\n");
    gethostname(ip_addr,128);
  }
  else {
    strcpy(ip_addr,env);
  }
#endif

  int port;
  const char* penv = getenv("CPI_TRANSFER_PORT");
  if( !penv || (penv[0] == 0)) {
    printf("Set ""CPI_TRANSFER_PORT"" environment varible to set socket IP address\n");
    port = getNextPortNum();
  }
  else {
    port = atoi(penv);
  }

  char tep[128];
  snprintf(tep,128,"cpi-socket-rdma://%s;%d:%d.%d.20",ip_addr,port,*size, getNextMailBox());
  sep = ep = tep;  
  return ep;
}




// Sets smem location data based upon the specified endpoint
CPI::OS::int32_t 
SocketEndPoint::
setEndpoint( std::string& ep )
{
  EndPoint::setEndpoint(ep);

  CPI::OS::uint32_t n,i=0;
  CPI::OS::int32_t start=0;
  char ipaddr[80];
  char sportNum[80];
  bool port=false;
  for ( n=0; n<ep.length(); n++ ) {
    if ( (start<2) && (ep[n] == '/') ) {
      start++;
    }
    else if ( (start == 2) && (ep[n] == ':') ) {
      break;
    }
    else if ( start == 2 ) {
      if ( ep[n] == ';' ) {
        port = true;
        ipaddr[i] = 0;
        i = 0;
      }
      if ( ! port ) {
        ipaddr[i++] = ep[n];
      }
      else {
        sportNum[i++] = ep[n];
      }
    }
  }
  sportNum[i] = 0;
  ipAddress = ipaddr;  
  portNum = atoi(&sportNum[1]);
  return 0;
}

SocketEndPoint::
~SocketEndPoint()
{
  // Empty
}



void SocketXferRequest::init (Creator cr, 
                           Flags flags, 
                           CPI::OS::uint32_t srcoffs, 
                           Shape *psrcshape, 
                           CPI::OS::uint32_t dstoffs, 
                           Shape *pdstshape, 
                           CPI::OS::uint32_t length)
{
  m_creator = cr;
  m_flags = flags;
  m_srcoffset = srcoffs;
  m_dstoffset = dstoffs;
  m_length = length;
  m_thandle = 0;
  memset (&m_srcshape, 0, sizeof (m_srcshape));
  if (psrcshape)
    {
      memcpy (&m_srcshape, psrcshape, sizeof (m_srcshape));
    }
  memset (&m_dstshape, 0, sizeof (m_dstshape));
  if (pdstshape)
    {
      memcpy (&m_dstshape, pdstshape, sizeof (m_dstshape));
    }
}


void SocketXferRequest::modify( CPI::OS::uint32_t new_offsets[], CPI::OS::uint32_t old_offsets[] )
{
  int n=0;
  while ( new_offsets[n] ) {
    xfer_modify( m_thandle, &new_offsets[n], &old_offsets[n] );
    n++;
  }
}


// SocketXferRequest destructor implementation
SocketXferRequest::~SocketXferRequest ()
{
  // remove self from the map and release xfer handle.
  SocketXferServices::remove (this);
  if (m_thandle)
    {
      (void)xfer_release (m_thandle, 0);
    }
}


// Create tranfer services template
void SocketXferServices::createTemplate (SmemServices* p1, SmemServices* p2)
{

  m_txRequest = NULL;
  m_sourceSmb = static_cast<DataTransfer::SocketSmemServices*>(p1);
  m_targetSmb = static_cast<DataTransfer::SocketSmemServices*>(p2);

  xfer_create (p1, p2, 0, &m_xftemplate);



  // We need to create a client socket here
  SocketStartupParams ssp;
  ssp.lsmem = p1;
  ssp.rsmem = p2;
  m_clientSocketT = new ClientSocketT( ssp );
  m_clientSocketT->start();
  m_clientSocketT->btr();

}





// Create a transfer request
XferRequest* SocketXferServices::copy (CPI::OS::uint32_t srcoffs, 
                                    CPI::OS::uint32_t dstoffs, 
                                    CPI::OS::uint32_t nbytes, 
                                    XferRequest::Flags flags,
                                    XferRequest*
                                    )
{
  // Create a transfer request instance and save in list
  SocketXferRequest* pXferReq = new SocketXferRequest (this);
  pXferReq->init (XferRequest::Copy, flags, srcoffs, 0, dstoffs, 0, nbytes);
  add (pXferReq);

  // Begin exception block
  CPI::OS::int32_t retVal = 0;
  CPI_TRY
    {
      // map flags
      CPI::OS::int32_t newflags = 0;
      if (flags & XferRequest::FirstTransfer) newflags |= XFER_FIRST;
      if (flags & XferRequest::LastTransfer) newflags |= XFER_LAST;

#ifndef NDEBUG
      printf("\n\n Creating tx request, src = %d, dst = %d, size = %d\n", srcoffs, dstoffs, nbytes );
#endif
      // Invoke original code.
      retVal = xfer_copy (m_xftemplate, srcoffs, dstoffs, nbytes, newflags, &pXferReq->getHandle());
      if (retVal)
        {
          CPI_RETHROW_TO_NEXT_LEVEL(LEVEL1);
        }
    }
  CPI_CATCH_LEVEL( m_exceptionMonitor, LEVEL1 )
    {
      remove (pXferReq);
      delete pXferReq;
      pXferReq = 0;
    }
  return pXferReq;

}


// Create a 2-dimensional transfer request
XferRequest* SocketXferServices::copy2D (CPI::OS::uint32_t srcoffs, Shape* psrc, 
                                      CPI::OS::uint32_t dstoffs, Shape* pdst, XferRequest*)
{
  // Create a transfer request instance and save in list
  SocketXferRequest* pXferReq = new SocketXferRequest (this);
  pXferReq->init (XferRequest::Copy2D, (XferRequest::Flags)0, srcoffs, psrc, dstoffs, pdst, 0);
  add (pXferReq);

  // Begin exception block
  CPI::OS::int32_t retVal = 0;
  try
    {
      // Invoke original code.
      // We simple cast "XferServices::Shape" to "EP_shape" since they must have the
      // exact same definitions. We don't specify any flags (they weren't used in the original).
      //                        retVal = xfer_copy_2d (m_xftemplate, srcoffs, (Shape*)psrc, dstoffs, (Shape*)pdst, 0, &pXferReq->m_thandle);
      if (retVal)
        {
          CPI_RETHROW_TO_NEXT_LEVEL(LEVEL1);
        }
    }
  catch( ... )
    {
      remove (pXferReq);
      delete pXferReq;
      pXferReq = 0;
    }
  return pXferReq;
}


// Group data transfer requests
XferRequest* SocketXferServices::group (XferRequest* preqs[])
{
  // Create a transfer request instance and save in list
  SocketXferRequest* pXferReq = new SocketXferRequest (this);
  pXferReq->init (XferRequest::Group, (XferRequest::Flags)0, 0, 0, 0, 0, 0);
  add (pXferReq);

  // Begin exception handler
  CPI::OS::int32_t retVal = 0;
  XF_transfer* handles = 0;
  try 
    {
      // Make a list of existing XF_transfer from the XferRequest* [] argument.
      int numHandles = 0;
      while (preqs[numHandles]) { numHandles++;}
      handles = new XF_transfer [numHandles + 1] ;
      for (int i = 0; i < numHandles; i++)
        {
          handles[i] = ((SocketXferRequest*)preqs[i])->getHandle();
        }
      handles[numHandles] = 0;

      // Invoke original code.
      retVal = xfer_group (handles, 0, &pXferReq->getHandle());
      if (retVal)
        {
          CPI_RETHROW_TO_NEXT_LEVEL(LEVEL1);
        }
    }
  catch( ... )
    {
      remove (pXferReq);
      delete pXferReq;
      pXferReq = 0;
      delete handles;
    }
  delete[] handles;
  return pXferReq;
}

// Release a transfer request
void SocketXferServices::release (XferRequest* preq)
{
  // Delete of request insures list removal.
  delete preq;
}


// remove all transfer request instances from the list for "this"
void SocketXferServices::releaseAll ()
{
  for ( CPI::OS::uint32_t n=0; n<m_map.size(); n++ ) {
    SocketXferRequest* req = static_cast<SocketXferRequest*>(m_map[n]);
    delete req;
  }
}


// remove a specified transfer request instance from the list
void SocketXferServices::remove (SocketXferRequest* pXferReq )
{
  m_map.remove( pXferReq );
}


// Destructor
SocketXferServices::
~SocketXferServices ()
{
  // Release all transfer requests
  releaseAll ();

  // Invoke destroy without flags.
  xfer_destroy (m_xftemplate, 0);

  if ( m_clientSocketT ) {
    delete m_clientSocketT;
  }
}


SocketSmemServices::
~SocketSmemServices ()
{
  delete [] m_mem;
  if ( m_socketServerT ) {
    m_socketServerT->stop();
    m_socketServerT->join();
    delete m_socketServerT;
  }
}


namespace DataTransfer {
  CPI::OS::int32_t xfer_socket_starti(PIO_transfer pio_transfer, CPI::OS::int32_t, SocketXferRequest* req);
}
void 
SocketXferRequest::
start(Shape* , Shape*)
{
  struct xf_transfer_ *xf_transfer = (struct xf_transfer_ *)m_thandle;  
  CPI::OS::int32_t pio_rc=0;
  if (xf_transfer->first_pio_transfer) {
    pio_rc = xfer_socket_starti(xf_transfer->first_pio_transfer, 0, this);
  }
  if (xf_transfer->pio_transfer) {
    pio_rc = xfer_socket_starti(xf_transfer->pio_transfer, 0, this);
  }
  if (xf_transfer->last_pio_transfer) {
    pio_rc = xfer_socket_starti(xf_transfer->last_pio_transfer, 0, this);
  }
}


namespace DataTransfer {

static void
action_socket_transfer(PIO_transfer transfer,SocketXferRequest* req)
{
  CPI::OS::int32_t nwords = ((transfer->nbytes + 5) / 8) ;
  CPI::OS::int32_t *src1 = (CPI::OS::int32_t *)transfer->src_va;
  CPI::OS::int32_t *dst1 = (CPI::OS::int32_t *)transfer->dst_va;    

  //#define TRACE_PIO_XFERS  
#ifdef TRACE_PIO_XFERS
  printf("Socket: copying %d bytes from 0x%llx to 0x%llx\n", transfer->nbytes,transfer->src_off,transfer->dst_off);
  printf("source wrd 1 = %d\n", src1[0] );
#endif
  
  SocketDataHeader hdr;
  hdr.length = transfer->nbytes;
  hdr.offset = transfer->dst_off;


#ifndef NDEBUG
  printf("Sending IP header\n");
  CPI::OS::sleep( 100 );
#endif
  req->m_xferServices->m_clientSocketT->socket().SendBuf((const char*)&hdr,(size_t)sizeof(SocketDataHeader),1);

#ifndef NDEBUG
  printf("Sending IP data\n");
  CPI::OS::sleep( 100 );
#endif
  req->m_xferServices->m_clientSocketT->socket().SendBuf((const char*)src1,(size_t)transfer->nbytes,1);

  
  


  /*
  for (CPI::OS::int32_t i=0; i < nwords*2; i++) {
    dst1[i] = src1[i];
  }
  */


}


CPI::OS::int32_t
xfer_socket_starti(PIO_transfer pio_transfer, CPI::OS::int32_t, SocketXferRequest* req)
{
  PIO_transfer transfer = pio_transfer;
  do {
    action_socket_transfer(transfer,req);
  } while ((transfer = transfer->next));
  return 0;
}



}
