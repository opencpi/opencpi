
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

#ifndef OCPI_RES_H
#define OCPI_RES_H
#include <OcpiOsDataTypes.h>
#include <cstdlib>
#include <memory>

namespace OCPI {
  namespace Util {
    // This data type is the offset within an allocation pool.
    // It must be a type with an explicit width (e.g. uint32_t),
    // that does not vary with compiler or architecture.
    // If you change it, change the printf macro also
    typedef uint32_t ResAddr;
#define OCPI_UTIL_RESADDR_PRIx PRIx32

    typedef ResAddr ResAddrType; // backwards compatibility
    struct ResPool;
    class MemBlockMgr
    {
    public:
      MemBlockMgr(ResAddr start, size_t size)
        throw( std::bad_alloc );
      ~MemBlockMgr()
        throw();
      int alloc(size_t nbytes, unsigned alignment, ResAddr& req_addr)
        throw( std::bad_alloc );
      int free(ResAddr  addr )
        throw( std::bad_alloc );


    private:
      ResPool *m_pool;

    };
  }
}
#endif
