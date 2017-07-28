
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
 * Abstract:
 *   This file contains the common classes used in the SMB package
 *
 * Author: John F. Miller
 *
 * Date: 1/20/05
 *
 */

#ifndef OCPI_DataTransfer_SharedMemoryInternal_H_
#define OCPI_DataTransfer_SharedMemoryInternal_H_

#include "OcpiRes.h"
#include "DtSharedMemoryInterface.h"

namespace DataTransfer {

  // Platform dependent global that creates an instance
  SmemServices& createHostSmemServices (EndPoint& loc );

  class ResourceServices
  {
  public:
    // Create a local resource pool
    //        Arguments:
    //                size        - size of resource pool in bytes.
    //        Returns:
    //                Returns 0 if transfer has completed, non-zero otherwise
    //        Throws:
    //                DtException for all other exception conditions
    virtual int createLocal (size_t size) = 0;

    // Allocate from pool
    //        Arguments:
    //                nbytes                - size of block in bytes.
    //                alignment        - desired alignment of block
    //                addr_p                - receives address of allocated block
    //        Returns:
    //                Returns 0 if transfer has completed, non-zero otherwise
    //        Throws:
    //                DtException for all other exception conditions
    virtual int alloc (
		       size_t nbytes, 
		       unsigned alignment, 
		       OCPI::Util::ResAddrType* addr_p) = 0;

    // Free back to pool
    //        Arguments:
    //                addr                - address of block to free
    //                nbytes                - size of block in bytes.
    //        Returns:
    //                Returns 0 if transfer has completed, non-zero otherwise
    //        Throws:
    //                DtException for all other exception conditions
    virtual int free (
		      OCPI::Util::ResAddrType addr, 
		      size_t nbytes) = 0;

    // Destroy resource pool
    //        Arguments:
    //        Returns:
    //                Returns 0 if transfer has completed, non-zero otherwise
    //        Throws:
    //                DtException for all other exception conditions
    virtual int destroy () = 0;

    // Destructor
    virtual ~ResourceServices () {};

  };

  // Platform dependent global that creates an instance
  ResourceServices* CreateResourceServices ();

}

#endif
