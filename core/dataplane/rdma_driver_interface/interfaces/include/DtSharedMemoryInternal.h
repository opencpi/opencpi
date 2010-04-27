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
 *   This file contains the common classes used in the SMB package
 *
 * Author: John F. Miller
 *
 * Date: 1/20/05
 *
 */

#ifndef CPI_DataTransfer_SharedMemoryInternal_H_
#define CPI_DataTransfer_SharedMemoryInternal_H_

#include <DtSharedMemoryInterface.h>

namespace DataTransfer {

  // Platform dependent global that creates an instance
  SmemServices* CreateSmemServices ( EndPoint* loc );

  class ResourceServices
  {
  public:
    // Create a local resource pool
    //	Arguments:
    //		size	- size of resource pool in bytes.
    //	Returns:
    //		Returns 0 if transfer has completed, non-zero otherwise
    //	Throws:
    //		DtException for all other exception conditions
    virtual CPI::OS::int32_t createLocal (CPI::OS::uint32_t size) = 0;

    // Allocate from pool
    //	Arguments:
    //		nbytes		- size of block in bytes.
    //		alignment	- desired alignment of block
    //		addr_p		- receives address of allocated block
    //	Returns:
    //		Returns 0 if transfer has completed, non-zero otherwise
    //	Throws:
    //		DtException for all other exception conditions
    virtual CPI::OS::int32_t alloc (
				    CPI::OS::uint32_t nbytes, 
				    CPI::OS::uint32_t alignment, 
				    CPI::OS::uint64_t* addr_p) = 0;

    // Free back to pool
    //	Arguments:
    //		addr		- address of block to free
    //		nbytes		- size of block in bytes.
    //	Returns:
    //		Returns 0 if transfer has completed, non-zero otherwise
    //	Throws:
    //		DtException for all other exception conditions
    virtual CPI::OS::int32_t free (
				   CPI::OS::uint32_t addr, 
				   CPI::OS::uint32_t nbytes) = 0;

    // Destroy resource pool
    //	Arguments:
    //	Returns:
    //		Returns 0 if transfer has completed, non-zero otherwise
    //	Throws:
    //		DtException for all other exception conditions
    virtual CPI::OS::int32_t destroy () = 0;

    // Destructor
    virtual ~ResourceServices () {};

  };

  // Platform dependent global that creates an instance
  ResourceServices* CreateResourceServices ();

}

#endif
