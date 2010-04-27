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

#ifndef CPI_PCI_SMEM_SERVICES_H_
#define CPI_PCI_SMEM_SERVICES_H_

#include "DtSharedMemoryInterface.h"
#include <map>
#include <vector>
#include <string>
#include "DtExceptions.h"
#include <CpiOsMutex.h>


namespace DataTransfer {

  /**********************************
   * This is our PCI shared memory location implementation.  The format of the 
   * address is as follows:
   *     "cpi-pci-pio://bid.base.size:300000.1.2"
   *  Where:
   *      bid:  is the PCI bus id
   *      name: is the name of the endpoint
   *********************************/
  class  PCIEndPoint : public EndPoint 
  {
  public:

    // Constructors
    PCIEndPoint(long s=0)
      :EndPoint(s){};
      virtual ~PCIEndPoint();
      PCIEndPoint( std::string& ep, CPI::OS::uint32_t size=0)
	:EndPoint(ep,size){setEndpoint(ep);};

	// Sets smem location data based upon the specified endpoint
	virtual CPI::OS::int32_t setEndpoint( std::string& ep );

	// Get the address from the endpoint
	virtual const char* getAddress(){return p_virt_addr;}
	char*              p_virt_addr;

	int bus_id;
	CPI::OS::uint64_t bus_offset;
	int map_size;

  };


  class PCISmemServices : public DataTransfer::SmemServices
  {
    // Public methods available to clients
  public:

    // Create the service
    void create (EndPoint* loc, CPI::OS::uint32_t size);

    // Close shared memory object.
    void close ();

    // Attach to an existing shared memory object by name.
    CPI::OS::int32_t attach (EndPoint*);

    // Detach from shared memory object
    CPI::OS::int32_t detach ();

    // Map a view of the shared memory area at some offset/size and return the virtual address.
    void* map (CPI::OS::uint64_t offset, CPI::OS::uint32_t size );

    // Unmap the current mapped view.
    CPI::OS::int32_t unMap ();

    // Enable mapping
    void* enable ();

    // Disable mapping
    CPI::OS::int32_t disable ();

    //	GetName - the name of the shared memory object
    const char* getName ()
      {
	return m_location->getAddress();
      }

    //	getEndPoint - the location of the shared area as an enumeration
    EndPoint* getEndPoint ()
      {
	return m_location;
      }

    //	GetHandle - platform dependent opaque handle for current mapping
    void* getHandle ();

    // Ctor/dtor
    PCISmemServices ( EndPoint* loc ) 
      : DataTransfer::SmemServices( loc ), m_init(false),m_fd(-1)
      {
	m_location = dynamic_cast<PCIEndPoint*>(loc);
	create( loc, loc->size);

      }
      PCISmemServices ();
      virtual ~PCISmemServices ();

  protected:

      //	Our thread safe mutex
      CPI::OS::Mutex m_threadSafeMutex;

      // Our location
      PCIEndPoint      *m_location;
      bool              m_init;
      CPI::OS::uint64_t m_last_offset;
      unsigned int      m_size;
      int               m_fd;

      struct Map {
	CPI::OS::uint64_t offset;
	unsigned int size;
	void*        vaddr;
	Map( CPI::OS::uint64_t o,unsigned int s, void* v ):offset(o),size(s),vaddr(v){};
      };
      std::vector<Map> m_map;

      // Our virtual address
      void* m_vaddr;

  };
}

#endif

