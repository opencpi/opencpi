/*
 * Abstact:
 *   This file contains the implementation for the PPP based DMA transfers.
 *
 * 06/23/09
 * Integrated and tested modify method.  
 *
 * 06/01/09
 * Added modify method.
 *
 * 07/20/08 John Miller
 * Initial Version
 *
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef PORT_COMPLETE
#include <DtSharedMemoryInternal.h>
#include <CpiUtilHash.h>
#endif

#include <CpiList.h>

#include <DtPPP_DMAXfer.h>
#include <CpiPPPSMemServices.h>
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <DtPPPEventHandler.h>
#include <DtExceptions.h>

using namespace DataTransfer;
using namespace CPI::Util;
using namespace CPI::OS;

static DataTransfer::PPPEventHandlerFactory* ppp_eh;
CPI::Util::VList PPPDMAXferServices::m_map;
CPI::Util::VList PPPDMAXferFactory::g_locations;

// Used to register with the data transfer system;
PPPDMAXferFactory *g_pppDmaFactory = new PPPDMAXferFactory;

PPPDMAXferFactory::PPPDMAXferFactory()
  throw ()
  : XferFactory("PPP DMA transfer driver")
{

  printf("In PPPDMAXferFactory::PPPDMAXferFactory()\n");

  try {
    ppp_eh = new DataTransfer::PPPEventHandlerFactory();
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "no more memory"  );
  }
  DataTransfer::EventManager::registerEventHandlerFactory( ppp_eh );
}

// Destructor
PPPDMAXferFactory::~PPPDMAXferFactory()
  throw()
{
  clearCache();
}


/***************************************
 *  This method is used to flush any cached items in the factoy
 ***************************************/
void PPPDMAXferFactory::clearCache()
{

#ifndef NDEBUG	
  printf("PPPDMAXferFactory() Destroying all global data !!\n");
#endif

  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    PPPEndPoint* loc = static_cast<PPPEndPoint*>(g_locations.getEntry(n));
    delete loc;
  }
  g_locations.destroyList();
}


// Get the location via the endpoint
EndPoint* PPPDMAXferFactory::getEndPoint( std::string& end_point)
{ 
  PPPEndPoint *loc;
  for ( CPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<PPPEndPoint*>(g_locations.getEntry(n));
    if ( end_point == loc->end_point ) {
      return loc;
    }
  }
#ifndef NDEBUG
  printf("Creating new location for %s\n", end_point.c_str() );
#endif
  try {
    loc = new PPPEndPoint(end_point);	
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "no more memory"  );
  }
  g_locations.insert( loc );
  return loc;
}

void PPPDMAXferFactory::releaseEndPoint( EndPoint* loc )
{
  // deprecated method
}


// This method is used to allocate a transfer compatible SMB
SmemServices* PPPDMAXferFactory::createSmemServices(EndPoint* loc )
{
  DataTransfer::PPPSmemServices* ss=NULL;
  try {
    ss = new DataTransfer::PPPSmemServices((EndPoint*)loc);
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "no more memory"  );
  }
  return ss;
}



/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* PPPDMAXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  XferServices* xs=NULL;
  try {
    xs = new PPPDMAXferServices(source, target);
  }
  catch( std::bad_alloc ) {
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "no more memory"  );
  }
  return xs;
}

//"cpi-ppp-dma://1.1:900000.18.20";
static int mailbox=9;
std::string PPPDMAXferFactory::allocateEndpoint(CPI::OS::uint32_t *size )
{
  std::string ep;

  RoseUint32Type target_id;
  rose_get_rio_base_device_id(&target_id);

#ifdef USE_ENV_FOR_MAILBOX
  if ( mailbox == -1 ) {
    const char* env = getenv("CPI_TRANSFER_MAILBOX");
    if( !env || (env[0] == 0)) {
      CPI_THROWNULL( DataTransferEx(PROPERTY_NOT_SET, "CPI_TRANSFER_MAILBOX" ) ) ;
    }
    mailbox = atoi(env);
    pid++;
  }
#endif

  char tep[128];
  snprintf(tep,128,"cpi-ppp-dma://%d.%d:%d.%d.20",target_id,0,*size, target_id);
  ep = tep;

  mailbox++;

  return ep;

}


PPPDMAXferRequest::PPPDMAXferRequest()
  :m_index(0),m_init(false)
{
}

void PPPDMAXferRequest::modify( CPI::OS::uint32_t new_offsets[], CPI::OS::uint32_t old_offsets[] )
{
#ifdef PORT_COMPLETE

#ifndef _WRS_KERNEL 
  int rc;
  if ( ! m_init ) {
    rc  = rose_dma_iovec_alloc( &m_iovec, &m_iovec_handle, 0 );
    if ( rc != ROSE_SUCCESS ) {
#ifndef NDEBUG
      printf("PPPDMAXferRequest::modify(), error allocating transfer, error = %d\n", rc );
#endif
      throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_dma_iovec_alloc"  );
    }
    m_init = true;
  }

  // CPI protocol sends the data first
  old_offsets[0] = m_iovec.iov[0].src_offset;
  rc = rose_dma_iovec_modify_src_offset( &m_iovec_handle,
					 0,
					 &m_iovec.iov[0].src_ep_info,		
					 new_offsets[0],
					 0);
  if ( rc != ROSE_SUCCESS ) {
#ifndef NDEBUG 
    printf("PPPDMAXferRequest::modify() error modifying iovec source offsets, error = %d\n", rc );
#endif
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_dma_iovec_modify_src_offset"  );
  }
#endif

#else
  cpiAssert(0);

#endif
}


int PPPDMAXferRequest::addTransfer( Creator cr, 
				    Flags             flags, 
				    CPI::OS::uint32_t srcoffs, 
				    Shape             *psrcshape, 
				    CPI::OS::uint32_t dstoffs, 
				    Shape             *pdstshape, 
				    CPI::OS::uint32_t length,
				    PPPEndPoint*    sEp,
				    PPPEndPoint*    dEp
				    )
{

#ifndef NDEBUG	
  if ( (m_index > 0) && (flags & FirstTransfer)  ) {
    printf("ERROR (m_index > 0) && (flags & FirstTransfer)\n");
    cpiAssert(0);
  }
  printf("Target event id = %d\n", dEp->event_id );
#endif

  m_creator    = cr;
  m_iovec.iov[m_index].src_ep_info = sEp->ep_info;
  m_iovec.iov[m_index].dst_ep_info = dEp->ep_info;

  // This is a hack since the RDMS doorbell payload is 64 bits, but WCI can only handle 32 bits
  // When the IP gets fixed this needs to be fixed also.  Leave this until we can test BBIO on VxWorks
#ifdef NEED_PROTO_MOD  
  if ( (flags & LastTransfer) && (dEp->target_id > 200) ) {
#endif


#ifdef WAS
    //    if ( (flags & LastTransfer) && ( length == 4 ) ) {
  if ( (flags & LastTransfer) && (dEp->target_id > 2) ) {

      printf("Got a flag transfer of size 4\n");


       m_iovec.iov[m_index].size = 4;
       m_iovec.iov[m_index].src_offset = srcoffs+4;
  }
  else {
    m_iovec.iov[m_index].size = length;    
    m_iovec.iov[m_index].src_offset = srcoffs;
  }
#else
    m_iovec.iov[m_index].size = length;    
    m_iovec.iov[m_index].src_offset = srcoffs;
#endif

  m_iovec.iov[m_index].flag = ROSE_DMA_IOV_DATA_WITH_OFFSET;
  m_iovec.iov[m_index].dst_offset = dstoffs;
  m_iovec.iov_count = ++m_index;


#ifdef NEED_PROTO_MOD  
  if ( (flags & LastTransfer) && (dEp->target_id <= 200) && (dEp->event_id < 0xfff) ) {
  if ( (flags & LastTransfer) && (dEp->target_id <= 2) && (dEp->event_id < 0xfff) ) {
#endif

    //   if ( (flags & LastTransfer) && (dEp->event_id < 0xfff) ) {

    printf("tid = %d, eid = 0x%x\n", dEp->target_id , dEp->event_id );

    
    if ( (flags & LastTransfer) && ((dEp->event_id<0xfff)&&(dEp->event_id>0)) ) {

    RoseEndpointInfo   mailbox_flag_info;
#ifndef NDEBUG
    printf("Generating mailbox for tid = %d, eid = %d\n", dEp->target_id, dEp->event_id );
#endif
    mailbox_flag_info.RoseEndpoint[0].word1 = dEp->target_id;       /* target id */
    mailbox_flag_info.RoseEndpoint[0].word2 = dEp->event_id;
    mailbox_flag_info.RoseEndpoint[0].word3 = sEp->target_id;         /* mailbox word 1 */
    mailbox_flag_info.RoseEndpoint[0].word4 = dEp->event_id;
    m_iovec.iov[m_index].dst_ep_info =  mailbox_flag_info;
    m_iovec.iov[m_index].flag = ROSE_DMA_IOV_MAILBOX;
    m_iovec.iov_count = ++m_index;
  }
  return m_index;
}

void PPPDMAXferRequest::dump_transfer()
{

  printf("There are %d transfers in this chain\n",m_index);
  for ( int n=0; n<m_index; n++ ) {
    printf("Transfering from offset %lld to offset %lld, size = %d\n",
	   m_iovec.iov[n].src_offset, m_iovec.iov[n].dst_offset,
	   m_iovec.iov[n].size );
  }
}

volatile bool CPI_TRACE_TX=false;
extern "C" {
  int rose_iovec_start( RoseIovecHandle* );
};

void PPPDMAXferRequest::start(Shape* s_shape, Shape* t_shape)
{
  if ( ! m_init ) {
    if ( rose_dma_iovec_alloc( &m_iovec, &m_iovec_handle, 0 ) != ROSE_SUCCESS ) {
#ifndef NDEBUG
      printf("PPPDMAXferRequest::start(), error allocating transfer !!\n");
#endif
      throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_dma_iovec_alloc"  );
    }
    m_init = true;
  }
	
  //#define NDEBUG_STATUS
#ifndef NDEBUG_STATUS
    int count = 0;
    if ( this->getStatus() == 1 ) {
    printf("ERROR!! Attempting to queue a DMA that is NOT complete !!\n");
    dump_transfer();
    while((this->getStatus() == 1) && (count < 10) ) {
    CPI::OS::sleep( 1 );
    count++;
    }
    if ( count < 10 ) {
    printf("Recovered!! DMA now complete, moving on\n");
    }
    }
#endif
	
	
    //#define TRACE_START
#ifdef TRACE_START
  printf("In PPPDMAXferRequest::start() \n");
  dump_transfer();
#endif

  //  CPI_TATL_TRACE( TATL_TRACE_PPP_START_TX );
	
#ifndef _WRS_KERNEL
  int rc;
  if ( (rc=rose_dma_iovec_start( &m_iovec_handle )) != ROSE_SUCCESS ) {
#ifndef NDEBUG
    printf("PPPDMAXferRequest::start(), error staring transfer, error code = %d: Retrying !!\n", rc);
#endif
    if ( (rc=rose_dma_iovec_start( &m_iovec_handle )) != ROSE_SUCCESS ) {
      throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_dma_iovec_start"  );
    }
  }
#else
  int rc;
  if ( (rc=rose_iovec_start( &m_iovec_handle )) != ROSE_SUCCESS ) {
#ifndef NDEBUG
    printf("PPPDMAXferRequest::start(), error = %d starting transfer, retrying!!\n",rc );
#endif
    if ( (rc=rose_iovec_start( &m_iovec_handle )) != ROSE_SUCCESS ) {
      throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_dma_iovec_start"  );
    }
  }

#endif
	
  //  CPI_TATL_TRACE( TATL_TRACE_PPP_RETURN_Q );
	
}


CPI::OS::int32_t PPPDMAXferRequest::getStatus()
{
  int done;
  //  CPI_TATL_TRACE( TATL_TRACE_PPP_START_SCHECK );
  if ( rose_dma_iovec_get_status( &m_iovec_handle, 0, &done) != ROSE_SUCCESS ) {
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_dma_iovec_get_status"  );
  }
  if ( done ) {
    //    CPI_TATL_TRACE( TATL_TRACE_PPP_END_TX );
  }
  return (CPI::OS::int32_t)!done;
}

PPPDMAXferRequest::~PPPDMAXferRequest ()
{

  if ( m_init ) {
    ( void ) rose_dma_iovec_free( &m_iovec_handle );
  }

  try {
    // remove self from the map and release xfer handle.
    PPPDMAXferServices::remove (this);
  }
  catch( ... ) {};
}


// Create tranfer services template
void PPPDMAXferServices::createTemplate (SmemServices* p1, SmemServices* p2)
{
  m_sourceSmb = p1;
  m_targetSmb = p2;
}


// Create a transfer request
XferRequest* PPPDMAXferServices::copy (CPI::OS::uint32_t srcoffs, 
				       CPI::OS::uint32_t dstoffs, 
				       CPI::OS::uint32_t nbytes, 
				       XferRequest::Flags flags,
				       XferRequest* group_to
				       )
{

  PPPEndPoint* Spep = static_cast<PPPEndPoint*>(m_sourceSmb->getEndPoint());
  PPPEndPoint* Tpep = static_cast<PPPEndPoint*>(m_targetSmb->getEndPoint());

  PPPDMAXferRequest* pXferReq;
  if ( group_to == NULL ) {
    try {
      pXferReq =   new PPPDMAXferRequest();
    }
    catch( std::bad_alloc ) {
      throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "no more memory"  );
    }
  }
  else {
    pXferReq = static_cast<PPPDMAXferRequest*>(group_to);
  }

#ifdef DEBUG_DONE_FLAG
  if ( flags & PPPDMAXferRequest::LastTransfer) {
    printf("DEBUG: about to map source offset\n");
    CPI::OS::uint64_t *vad = (CPI::OS::uint64_t *)m_sourceSmb->map( srcoffs, 512 );
    printf("Flag value = %lld\n", *vad);
    m_sourceSmb->unMap();
  }
#endif

  pXferReq->addTransfer( XferRequest::Copy, flags, srcoffs, 0, 
			 dstoffs, 0, nbytes, Spep, Tpep  );

  return pXferReq;
}


// Create a 2-dimensional transfer request
XferRequest* PPPDMAXferServices::copy2D (CPI::OS::uint32_t srcoffs, Shape* psrc, 
					 CPI::OS::uint32_t dstoffs, Shape* pdst, XferRequest*)
{
  // Not yet supported
  return NULL;
}


// Group data transfer requests
XferRequest* PPPDMAXferServices::group (XferRequest* preqs[])
{
  cpiAssert( preqs[0] == preqs[1] );
  return preqs[1];
}

// Release a transfer request
void PPPDMAXferServices::release (XferRequest* preq)
{
  // Delete of request insures list removal.
  delete preq;
}


// remove all transfer request instances from the list for "this"
void PPPDMAXferServices::releaseAll ()
{
  for ( CPI::OS::uint32_t n=0; n<m_map.size(); n++ ) {
    PPPDMAXferRequest* req = static_cast<PPPDMAXferRequest*>(m_map[n]);
    delete req;
    m_map.remove(req);
  }

}


// remove a specified transfer request instance from the list
void PPPDMAXferServices::remove (PPPDMAXferRequest* pXferReq )
{
  m_map.remove( pXferReq );
}


// Destructor
PPPDMAXferServices::~PPPDMAXferServices ()
{
  try  {
    // Release all transfer requests
    releaseAll ();
  }
  catch( ... ) {}
}





