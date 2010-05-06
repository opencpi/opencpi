/*
 * Abstact:
 *        This file contains the implementation for a  PPP shared memory class
 *  implementation.
 *
 * Author: John Miller
 *
 * Date: 2/19/08
 *
 */

#include <CpiPPPSMemServices.h>
#include <CpiUtilAutoMutex.h>
#include <DtExceptions.h>

using namespace DataTransfer;

void PPPInit()
{
  printf("About to call rose_pmem_init()" );
  if (rose_pmem_init() != ROSE_SUCCESS) 
    {
      printf("rose_pmem_init failed\n");
      exit(0);
    }
}

CPI::OS::Mutex PPPSmemServices::m_threadSafeMutex;

// Create the service
void PPPSmemServices::create (EndPoint* loc, CPI::OS::uint32_t size)
{
  if ( m_init ) {
    return;
  }
  CPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 


  m_init = true;
  m_last_offset = 0;
  m_virt_addr = 0;
  if ( size == 0 ) {
    size = 0x1000000;
  }
  m_size = size;

  PPPEndPoint* pep = dynamic_cast<PPPEndPoint*>(loc);

#ifndef WIN32

  if ( loc->local ) {

#ifndef NDEBUG
    printf("SMEM is local !!\n");
#endif

    m_location->mem_info.info.accessBytes = size;
    m_location->mem_info.info.accessOffset = 0;
                
#ifndef NDEBUG
    printf("SMEM name = %s, m_handle = %p \n", m_location->ep_name.c_str(), &m_handle );
#endif

    RoseSizeType actual_n_bytes;

    printf("About to call rose_pmem_alloc()\n");
    int rc = rose_pmem_alloc ( &m_handle, &m_location->mem_info, 
                               &actual_n_bytes, NULL );
    printf("Done call rose_pmem_alloc()\n");
                        
#ifndef NDEBUG                        
    printf("Asked for %d bytes, got %d bytes\n", size, actual_n_bytes );
#endif
    
    if ( rc != ROSE_SUCCESS ) 
      {
        printf ( "rose_pmem_alloc() failed, ec = %d\n", rc);
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_pmem_alloc"  );
      }

    printf("About to call rose_pmem_map()\n");
    rc = rose_pmem_map ( m_handle,
                         ( RoseVirtAddrType* ) &m_virt_addr,
                         0,
                         m_location->size);
    printf("Done call rose_pmem_map()\n");
      
    if ( rc != ROSE_SUCCESS ) 
      {
        printf ( "rose_pmem_map() failed, ec = %d\n", rc );
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_pmem_map"  );
      }
    pep->paddr = m_location->mem_info.info.accessOffset;

#ifndef NDEBUG
    printf("The physical address of the PPP mapped memory is %lld\n", pep->paddr);
#endif

    char buf[80];
    CPI::OS::uint64_t off =  m_location->mem_info.info.accessOffset;
    if ( pep->protocol == "cpi-ppp-dma" ) {
      snprintf( buf, 80, "cpi-ppp-dma://%d.%lld:%d.%d.%d",pep->target_id,off,size,pep->mailbox,pep->maxCount);
    }
    else {
      snprintf( buf, 80, "cpi-ppp-pio://%d.%lld:%d.%d.%d",pep->target_id,off,size,pep->mailbox,pep->maxCount);
    }
    loc->end_point = buf;
                
    // Create our endpoint

    printf("About to call rose_create_endpoint()\n");
    rc = rose_create_endpoint(pep->paddr, &pep->ep_info);
    if ( rc != ROSE_SUCCESS ) 
      {
        printf ( "rose_create_endpointfailed, rc: %d\n", rc );
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_create_endpoint"  );
      }
    printf("Done call rose_create_endpoint()\n");

    /* adjust the tid if necessary */
    for(int i = 0; i < MAX_ROSE_BUS; i++) 
      {
        if (pep->ep_info.RoseEndpoint[i].bus_type == ROSE_BUS_RAPIDIO) {
          pep->ep_info.RoseEndpoint[i].word1 = pep->target_id;
        }
      }
  }
  else {
                
    unsigned long paddr = pep->paddr;
#ifndef NDEBUG
    printf("SMEM is remote !!, target id = %d, paddr = %lu, size = %u\n", 
           pep->target_id, paddr, size);
#endif

    printf("About to call rose_pio_map_rio() tid = %d, paddr = %ld, size = %d \n",
           pep->target_id,
           paddr,
           size
           );

    unsigned int rc = rose_pio_map_rio ( pep->target_id, paddr,
                                         size,
                                         0,
                                         &m_rio_handle );
    printf("Done call rose_pio_map_rio()\n");

    if ( rc != ROSE_SUCCESS ) 
      {
        printf ( "rose_pio_map_rio 1 failed, rc: 0x%x\n", rc );
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_pio_map_rio"  );
      }
    m_virt_addr = (RoseVirtAddrType) m_rio_handle.l_vaddr;
    m_map.push_back( Map(m_rio_handle,0,size,m_virt_addr) );
                
    // In case this endpoint will be used for DMA, we must also create
    // a PPP Endpoint object


    rc = rose_create_endpoint(pep->paddr, &pep->ep_info);
    if ( rc != ROSE_SUCCESS ) 
      {
        printf ( "rose_create_endpointfailed, rc: 0x%x\n", rc );
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_create_endpoint"  );
      }

    /* adjust the tid if necessary */
    for(int i = 0; i < MAX_ROSE_BUS; i++) 
      {
        if (pep->ep_info.RoseEndpoint[i].bus_type == ROSE_BUS_RAPIDIO) {
          pep->ep_info.RoseEndpoint[i].word1 = pep->target_id;
        }
      }

  }
#else
  m_virt_addr = new int[size];
#endif

}


void* PPPSmemServices::getHandle ()
{
#ifndef WIN32
  return (void*)&m_handle;
#else
  return NULL;
#endif
}

// Close shared memory object.
void PPPSmemServices::close ()
{

#ifndef NDEBUG
  printf("In PPPSmemServices::close ()\n");
#endif

}

// Attach to an existing shared memory object by name.
CPI::OS::int32_t PPPSmemServices::attach (EndPoint*)
{
  return 0;
}

// Detach from shared memory object
CPI::OS::int32_t PPPSmemServices::detach ()
{
  // NULL function
  return 0;
}

// Map a view of the shared memory area at some offset/size and return the virtual address.
void* PPPSmemServices::map (CPI::OS::uint64_t offset, CPI::OS::uint32_t size )
{
  CPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  if ( ! m_init  ) {
    create(m_location, m_location->size);
  }
  RoseUint32Type tid;
  rose_get_rio_base_device_id(&tid);
  if ( m_location->local || (tid == m_location->target_id ) ) {        
    return (char*)m_virt_addr + offset;
  }

  



        
        
  // Determine if the request is already satisfied
  std::vector<Map>::iterator it;
  for ( it = m_map.begin(); it != m_map.end(); it ++ ) {
    if ( (offset >= (*it).offset) && ( (offset+size) < ((*it).offset+(*it).size))) {
                
#ifndef NDEBUG
      printf("&*&* Returning address %p\n", (char*)((*it).vaddr) + offset );
#endif
      if ( (*it).offset != 0 ) {
        return (char*)((*it).vaddr) + (offset - (*it).offset) + 1024*16;
      }
      else {
#ifndef NDEBUG                        
        printf("Returning untouched vaddr\n");
#endif
        return (char*)((*it).vaddr);
      }
    }
  }
        
  // Force a large map so we dont have to create a bunch of small ones
  size = 1024*32;
        
  // Anticipate future map requests to lower data
  RosePhysAddrType moffset = offset - 1024*16;
        
#ifndef WIN32

  RosePhysAddrType paddr = moffset + m_location->paddr;
        
#ifndef NDEBUG
  printf("paddr = %lld\n", paddr );
#endif
        
  RoseMapHandle   rio_handle;
  try {
                
    int rc = rose_pio_map_rio ( m_location->target_id, paddr,
                                size,
                                0,
                                &rio_handle );
    if ( rc )
      {
        printf ( "rose_pio_map_rio 2 failed, rc: %d\n", rc );
        throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_pio_map_rio"  );
      }
    m_map.push_back( Map(rio_handle, offset,size,rio_handle.l_vaddr) );
  }
  catch( ... ) {
    printf("Got an unknown exception from rose_pio_map_rio()\n");
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "rose_pio_map_rio" );
  }
#endif

  return (CPI::OS::uint8_t*)rio_handle.l_vaddr + 1024*16;
        
}


//  Externalize an offset into this SMB so that it is "mappable" from outside 
CPI::OS::uint32_t PPPSmemServices::externalizeOffset( CPI::OS::uint32_t offset )
{
  return offset;
}


// Unmap the current mapped view.
CPI::OS::int32_t PPPSmemServices::unMap ()
{
  if ( ! m_location->local ) {
#ifndef WIN32
    ( void ) rose_pio_unmap_rio ( &m_rio_handle );
#endif
  }
  return 0;
}

// Enable mapping
void* PPPSmemServices::enable ()
{
  return m_virt_addr;
}

// Disable mapping
CPI::OS::int32_t PPPSmemServices::disable ()
{
  return 0;
}

PPPSmemServices::~PPPSmemServices ()
{

  int rc;

#ifndef NDEBUG
  printf("IN PPPSmemServices::~PPPSmemServices ()\n");
#endif
        
        
  std::vector<Map>::iterator it;
  for ( it=m_map.begin(); it != m_map.end(); it++ ) {
    rc = rose_pio_unmap_rio ( &(*it).rio_handle );
    if ( rc )  {
      printf ( "rose_pio_unmap_rio failed, rc: 0x%x\n", rc );
    }
  }
  m_map.clear();
        
  if ( m_location->local ) {
    rc = rose_pmem_unmap ( m_handle, m_virt_addr, m_location->size );
    if ( rc ) {
      printf ( "rose_pmem_unmap failed, rc: 0x%x\n", rc );
    }        
    rc = rose_pmem_free ( m_handle ); //, m_virt_addr, ROSE_BUS_LOCAL );
    if ( rc ) {
      printf ( "rose_pmem_free failed, rc: 0x%x\n", rc );
    }        
  }

}


// Sets smem location data based upon the specified endpoint
CPI::OS::int32_t PPPEndPoint::setEndpoint( std::string& ep )
{
  EndPoint::setEndpoint(ep);

  unsigned long n,i=0;
  long start=0;
  char sname[80];
  for ( n=0; n<ep.length(); n++ ) {
    if ( (start<2) && (ep[n] == '/') ) {
      start++;
    }
    else if ( (start == 2) && (ep[n] == ':') ) {
      break;
    }
    else if ( start == 2 ) {
      sname[i++] = ep[n];
    }
  }
  sname[i] = 0;
  n=0;
  char tid[32];
  char pa[32];
  while ( sname[n]!=0 ) {
    if ( sname[n] == '.' ) {
      break;
    }
    tid[n] = sname[n];
    n++;
  }
  tid[n] = 0;
  i=0;
  while (sname[n++] != 0 )pa[i++]=sname[n];
  ep_name = sname;
        
  target_id = atoi(tid);
  paddr = atoi(pa);
      
#ifndef WIN32
  mem_info.info.accessOffset = 0;
  mem_info.info.accessBytes = size;
  mem_info.access.bridgeType = ROSE_BUS_LOCAL;
  mem_info.access.bridge.local.space = ROSE_LOCAL_RAW;
  mem_info.access.bridge.local.sp.raw.accessType = ROSE_ACCESS_D32;
#endif

  return 0;
}

PPPEndPoint::~PPPEndPoint()
{
        
        
}


