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

//#define USE_EMULATOR

#ifdef NDEBUG
#undef NDEBUG
#endif

/*
 * Abstact:
 *  This file contains the implementation for a  PCI shared memory class
 *  implementation.
 *
 * Author: John Miller
 *
 * Date: 2/19/08
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <time.h>
#include "CpiPCISMemServices.h"
#include "CpiUtilAutoMutex.h"

using namespace DataTransfer;




void PCIInit()
{

}

// Create the service
void PCISmemServices::create (EndPoint* loc, CPI::OS::uint32_t size)
{
  CPI::Util::AutoMutex guard ( m_threadSafeMutex,
			       true ); 

  if ( m_init ) {
    return;
  }
  m_init = true;
  m_location = dynamic_cast<PCIEndPoint*>(loc);
  m_last_offset = 0;
  if ( size == 0 ) {
    size = m_location->map_size;
  }
  m_size = size;

  if ( loc->local ) {

#ifndef NDEBUG
    printf("************************ SMEM is local !!, vaddr = %p\n", m_vaddr);
#endif

#define LOCAL_ONLY
#ifdef LOCAL_ONLY
    m_vaddr = new int[size];
#else

    //    CPI_DMA_MEMORY   env var that specifies the base address of pinned memory
    char *dma_base_str = getenv("CPI_DMA_MEMORY");
    CPI::OS::uint64_t base_adr;
    if ( dma_base_str ) {
      sscanf( dma_base_str, "%dM$0x%lld", &size, &base_adr );
    }


    static uint64_t base = 0x8f700000ll;
    char buf[128];
    snprintf(buf, 128,
	     "cpi-pci-pio://%s.%llx:%llx.1.10", "0", (unsigned long long)base,
	     (unsigned long long)size);
    loc->end_point = buf;

    if ( m_fd == -1 ) {
      if ( ( m_fd = open("/dev/mem", O_RDWR|O_SYNC )) < 0 ) {
	throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant open /dev/mem"  );
      }
    }
    m_vaddr =  (uint8_t*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED,
			      m_fd, base);
#endif


  }
  else {

    m_vaddr = NULL;

#ifdef SIM
    m_vaddr = new int[size];
#else

#ifndef NDEBUG
    printf("About to open shm\n");
#endif

#ifdef USE_EMULATOR
    umask(0);
    if ( m_fd == -1 ) {
      if ( (m_fd = shm_open("myshm", O_RDWR, 0666)) <= 0) {
	printf("errno = %d\n", errno);
	throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant open /dev/shm/myshm"  );
      }
    }
#else
    if ( m_fd == -1 ) {
      if ( (m_fd = open("/dev/mem", O_RDWR)) < 0) {
	throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant open /dev/mem"  );
      }
    }
#endif

#ifndef NDEBUG
    printf("*******************  fd = %d *************************\n", m_fd);
#endif

#endif
  }
}


void* PCISmemServices::getHandle ()
{
  return NULL;
}

// Close shared memory object.
void PCISmemServices::close ()
{
#ifndef NDEBUG
  printf("In PCISmemServices::close ()\n");
#endif

}

// Attach to an existing shared memory object by name.
CPI::OS::int32_t PCISmemServices::attach (EndPoint*)
{
  return 0;
}

// Detach from shared memory object
CPI::OS::int32_t PCISmemServices::detach ()
{
  // NULL function
  return 0;
}


// Map a view of the shared memory area at some offset/size and return the virtual address.
void* PCISmemServices::map (CPI::OS::uint64_t offset, CPI::OS::uint32_t size )
{
  CPI::Util::AutoMutex guard ( m_threadSafeMutex,
			       true ); 

  if ( ! m_init  ) {
    create(m_location, m_location->size);
  }
	
  if ( m_location->local ) {	
#ifndef NDEBUG
    printf("**************** Returning local vaddr = %p\n", m_vaddr );
#endif
    return (char*)m_vaddr + offset;
  }


  // We will create a map per offset until we change the CPI endpoint to handle segments
  // We just need to make sure we dont run out of maps.
  printf("shm size = %d, bus_offset = %llu, offset = %llu, fd = %d\n", 
	 m_location->map_size, (long long)m_location->bus_offset,(long long)offset, m_fd );
  if (m_vaddr == NULL ) {
    m_vaddr = mmap(NULL, m_location->map_size, PROT_READ|PROT_WRITE, MAP_SHARED,
		    m_fd,  m_location->bus_offset );
    printf("vaddr = %p\n", m_vaddr );
  }

  if ( (m_vaddr == NULL) || (m_vaddr == (void*)-1 )) {
    printf("mmap failed to map to offset %llu\n", (long long)offset );
    throw CPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant map offset"  );
  }
  return (CPI::OS::uint8_t*)m_vaddr+offset;
}

// Unmap the current mapped view.
CPI::OS::int32_t PCISmemServices::unMap ()
{
  return 0;
}

// Enable mapping
void* PCISmemServices::enable ()
{
  return m_vaddr;
}

// Disable mapping
CPI::OS::int32_t PCISmemServices::disable ()
{
  return 0;
}

PCISmemServices::~PCISmemServices ()
{

#ifndef NDEBUG
  printf("IN PCISmemServices::~PCISmemServices ()\n");
#endif

}


// Sets smem location data based upon the specified endpoint
CPI::OS::int32_t PCIEndPoint::setEndpoint( std::string& ep )
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
    else if ( start == 2) {
      sname[i++] = ep[n];
    }
  }
  sname[i] = 0;
  n=0;
  char bi[32];
  char pa[32];
  char ms[32];
  while ( sname[n]!=0 ) {
    if ( sname[n] == '.' ) {
      break;
    }
    bi[n] = sname[n];
    n++;
  }
  bi[n] = 0;

  printf("bi(%s)\n", bi );

  n++;
  i=0;
  while ( sname[n]!=0 ) {
    if ( sname[n] == '.' ) {
      break;
    }
    pa[i++] = sname[n];
    n++;
  }
  pa[i] = 0;
  n++;
  printf("pa(%s)\n", pa );

  i=0;
  while ( sname[n]!=0 ) {
    if ( sname[n] == '.' ) {
      break;
    }
    ms[i++] = sname[n];
    n++;
  }
  ms[i] = 0;
  n++;
  printf("ms(%s)\n", ms );

#ifndef NDEBUG
  printf("bus_id = %s, off = %s, size = %s,\n", bi,pa,ms);
#endif
	
  bus_id = atoi(bi);
  bus_offset = strtoul( pa, NULL, 0);
  map_size = atoi(ms);
  
#ifndef NDEBUG
  printf("bus_id = %d, off = %llu, size = %d\n", bus_id, (long long)bus_offset, map_size );
#endif
   
  return 0;
}

PCIEndPoint::~PCIEndPoint(){}

