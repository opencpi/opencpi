
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
 *   This file contains the implementation for the base class for SMB transfers.
 *
 * Author: John F. Miller
 *
 * Date: 7/20/04
 *
 */

#include <DtSharedMemoryInternal.h>
#include <DtPCIPioXfer.h>
#include <DtPioXfer.h>
//#include <OcpiPCISMemServices.h>
#include <xfer_if.h>
#include "OcpiOsAssert.h"
#include <OcpiList.h>
#include <OcpiUtilHash.h>
#include <OcpiOsMutex.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiOsMisc.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <OcpiOsAssert.h>
#include <OcpiPValue.h>


#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <time.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


namespace DataTransfer {
using namespace OCPI::Util;
using namespace OCPI::OS;


// Used to register with the data transfer system;
static VList g_locations;


const char *pci = "pci"; // name passed to inherited template class
PCIPIOXferFactory::PCIPIOXferFactory()
  throw ()
{
  ocpiDebug("In PCIPIOXferFactory::PCIPIOXferFactory()");
}

// Destructor
PCIPIOXferFactory::~PCIPIOXferFactory()
  throw ()
{
  //  clearCache();
}


#if 0
/***************************************
 *  This method is used to flush any cached items in the factoy
 ***************************************/
void PCIPIOXferFactory::clearCache()
{
  g_locations.destroyList();
}

// Get the location via the endpoint
EndPoint* PCIPIOXferFactory::getEndPoint( std::string& end_point, bool local)
{ 
  PCIEndPoint *loc;
  for ( OCPI::OS::uint32_t n=0; n<g_locations.getElementCount(); n++ ) {
    loc = static_cast<PCIEndPoint*>(g_locations.getEntry(n));
    if ( end_point == loc->end_point ) {
      return loc;
    }
  }

  loc = new PCIEndPoint(end_point, local);
        
  // This is a test case to make sure that a factory can modify the endpoint
  // string and system can handle it !!

  g_locations.insert( loc );
  return loc;
}

void PCIPIOXferFactory::releaseEndPoint( EndPoint* loc )
{
   ( void ) loc;

#ifndef NDEBUG
  printf("void PCIPIOXferFactory::releaseLocation( EndPoint* loc ), NOT YET IMPLEMENTED !!\n");
#endif

}
#endif

  EndPoint* PCIPIOXferFactory::
  createEndPoint(std::string& endpoint, bool local) {
    return new PCIEndPoint(endpoint, local);
  }


// This method is used to allocate a transfer compatible SMB
SmemServices* PCIPIOXferFactory::getSmemServices( EndPoint* loc )
{
  if ( loc->smem ) {
    return loc->smem;
  }
  return new PCISmemServices( this, loc);
}



/***************************************
 *  This method is used to create a transfer service object
 ***************************************/
XferServices* PCIPIOXferFactory::getXferServices(SmemServices* source, SmemServices* target)
{
  return new PIOXferServices(source, target);
}




/***************************************
 *  This method is used to dynamically allocate
 *  an endpoint for an application running on "this"
 *  node.
 ***************************************/
static OCPI::OS::int32_t pid;
std::string PCIPIOXferFactory::
allocateEndpoint(const OCPI::Util::PValue*, unsigned mailBox, unsigned maxMailBoxes)
{
  std::string ep;
  OCPI::Util::SelfAutoMutex guard (this); 

  pid++;

  unsigned int size = m_SMBSize;

  char tep[128];
  pid = getpid();
  int bus_id = 0;
  snprintf(tep,128,"ocpi-pci-pio:%d.0:%d.%d.%d",bus_id, size, mailBox, maxMailBoxes);
  ep = tep;
  return ep;
}







void PCIInit()
{

}

// Create the service
void PCISmemServices::create (EndPoint* loc, OCPI::OS::uint32_t size)
{
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex,
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

    static const char *dma = getenv("OCPI_DMA_MEMORY");
    OCPI::OS::uint64_t base_adr;
    if (dma) {
      unsigned sizeM;
      ocpiCheck(sscanf(dma, "%uM$0x%llx", &sizeM,
		       (unsigned long long *) &base_adr) == 2);
      size = (unsigned long long)sizeM * 1024 * 1024;
      fprintf(stderr, "DMA Memory:  %uM at 0x%llx\n", sizeM,
	      (unsigned long long)base_adr);
    }
    else {
      ocpiCheck(!"OCPI_DMA_MEMORY not found in the environment\n");
    }

    uint32_t offset = 1024*1024*128;
    size -= offset;
    base_adr+=offset;
    char buf[128];
    snprintf(buf, 128,
             "ocpi-pci-pio:%s.%lld:%lld.2.10", "0", (unsigned long long)base_adr,
             (unsigned long long)size);
    loc->end_point = buf;

    if ( m_fd == -1 ) {
      if ( ( m_fd = open("/dev/mem", O_RDWR|O_SYNC )) < 0 ) {
        throw OCPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant open /dev/mem"  );
      }
    }

    printf("mmap mapping base = %" PRIu64 "with size = %d\n", base_adr, size );

    m_vaddr =  (uint8_t*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED,
                              m_fd, base_adr);

    if ( m_vaddr == MAP_FAILED )
    {
      throw OCPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "mmap() of DMA memory failed."  );
    } 

  }
  else {

    m_vaddr = NULL;

#ifndef NDEBUG
    printf("About to open shm\n");
#endif

    if ( m_fd == -1 ) {
      if ( (m_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0) {
        throw OCPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant open /dev/mem"  );
      }
    }

#ifndef NDEBUG
    printf("*******************  fd = %d *************************\n", m_fd);
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
OCPI::OS::int32_t PCISmemServices::attach (EndPoint*)
{
  return 0;
}

// Detach from shared memory object
OCPI::OS::int32_t PCISmemServices::detach ()
{
  // NULL function
  return 0;
}


// Map a view of the shared memory area at some offset/size and return the virtual address.
void* PCISmemServices::map (OCPI::OS::uint64_t offset, OCPI::OS::uint32_t size )
{  
  ( void ) size;
  OCPI::Util::AutoMutex guard ( m_threadSafeMutex,
                               true ); 

  if ( ! m_init  ) {
    create(m_location, m_location->size);
  }
        
  if ( m_location->local ) {        
#ifndef NDEBUG
    printf("**************** Returning local vaddr = %p\n", (uint8_t*)m_vaddr + offset);
#endif
    return (char*)m_vaddr + offset;
  }


  // We will create a map per offset until we change the OCPI endpoint to handle segments
  // We just need to make sure we dont run out of maps.
  printf("shm size = %" PRIu64 ", bus_offset = %" PRIu64 ", offset = %" PRIu64 ", fd = %d\n", 
         m_location->map_size, 
         m_location->bus_offset,
         offset, 
          m_fd );
  if (m_vaddr == NULL ) {
    m_vaddr = mmap(NULL, m_location->map_size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    m_fd,  m_location->bus_offset );
    printf("vaddr = %p\n", m_vaddr );
  }

  if ( (m_vaddr == NULL) || (m_vaddr == (void*)-1 )) {
    printf("mmap failed to map to offset %llu\n", (long long)offset );
    throw OCPI::Util::EmbeddedException (  RESOURCE_EXCEPTION, "cant map offset"  );
  }
  return (OCPI::OS::uint8_t*)m_vaddr+offset;
}

// Unmap the current mapped view.
OCPI::OS::int32_t PCISmemServices::unMap ()
{
  return 0;
}

// Enable mapping
void* PCISmemServices::enable ()
{
  return m_vaddr;
}

// Disable mapping
OCPI::OS::int32_t PCISmemServices::disable ()
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
OCPI::OS::int32_t PCIEndPoint::parse( std::string& ep )
{

  printf("Scaning %s\n", ep.c_str() );
  if (sscanf(ep.c_str(), "ocpi-pci-pio:%x.%" SCNu64 ":%" SCNu64 ".3.10", 
                   &bus_id,
                   &bus_offset,
                   &map_size) != 3)
    throw OCPI::Util::EmbeddedException("Remote endpoint description wrong: ");
  
#ifndef NDEBUG
  printf("bus_id = %d, offset = 0x%" PRIx64 ", size = 0x%" PRIx64 "\n", 
          bus_id, bus_offset, map_size );
#endif
   
  return 0;
}

PCIEndPoint::~PCIEndPoint(){}

  RegisterTransferDriver<PCIPIOXferFactory> pciDriver;
}
