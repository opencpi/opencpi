/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
