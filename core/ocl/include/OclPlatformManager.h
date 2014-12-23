/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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

#ifndef OCL_PLATFORM_MANAGER_H
#define OCL_PLATFORM_MANAGER_H

#include <string>
#include <vector>
#ifdef _LIBCPP_VERSION
#include <memory>
#define STDTR1 std
#else
#include <tr1/memory>
#define STDTR1 std::tr1
#endif

#include "OcpiPValue.h"

namespace OCPI
{
  namespace OCL
  {
    struct Grid
    {
      explicit Grid ( )
        : dim ( 0 )
      {
        gtid_offset [ 0 ] = 0;
        gtid_offset [ 1 ] = 0;
        gtid [ 0 ] = 0;
        gtid [ 1 ] = 0;
        ltid [ 0 ] = 0;
        ltid [ 1 ] = 0;
      }

      Grid ( size_t gtid0Offset,
             size_t gtid0,
             size_t ltid0 )
      {
        setGrid1d ( gtid0Offset, gtid0, ltid0 );
      }

      Grid ( size_t gtid0Offset,
             size_t gtid1Offset,
             size_t gtid0,
             size_t gtid1,
             size_t ltid0,
             size_t ltid1 )
      {
        setGrid2d ( gtid0Offset,
                    gtid1Offset,
                    gtid0,
                    gtid1,
                    ltid0,
                    ltid1 );
      }

      void setGrid1d ( size_t gtid0Offset,
                       size_t gtid0,
                       size_t ltid0 )
      {
        dim = 1;
        gtid_offset [ 0 ] = gtid0Offset;
        gtid_offset [ 1 ] = 0;
        gtid [ 0 ] = gtid0;
        gtid [ 1 ] = 0;
        ltid [ 0 ] = ltid0;
        ltid [ 1 ] = 0;
      }

      void setGrid2d ( size_t gtid0Offset,
                       size_t gtid1Offset,
                       size_t gtid0,
                       size_t gtid1,
                       size_t ltid0,
                       size_t ltid1 )
       {
        dim = 2;
        gtid_offset [ 0 ] = gtid0Offset;
        gtid_offset [ 0 ] = gtid1Offset;
        gtid [ 0 ] = gtid0;
        gtid [ 1 ] = gtid1;
        ltid [ 0 ] = ltid0;
        ltid [ 1 ] = ltid1;
       }

       size_t dim;
       size_t gtid_offset [ 2 ];
       size_t gtid [ 2 ];
       size_t ltid [ 2 ];
    };

    class DeviceWorker;

    class DeviceContext
    {
      public:
        explicit DeviceContext ( const OCPI::Util::PValue* props = 0 );

        ~DeviceContext ( );

        void loadArtifact ( const std::string& pathToArtifact,
                            const OCPI::Util::PValue* artifactParams );

        void unloadArtifact ( const std::string& pathToArtifact );

        DeviceWorker& loadWorker ( const std::string& entryPoint );

        void unloadWorker ( DeviceWorker& worker );

        bool compile ( const std::string& options,
                       const std::vector<std::string>& sources,
                       const std::string& output_file );

      private:
        class Impl;
        STDTR1::shared_ptr<Impl> d_impl;

        DeviceContext ( const DeviceContext& );
        DeviceContext& operator=( const DeviceContext& );
    };


    class DeviceWorker
    {
      friend class DeviceContext;

      public:
        DeviceWorker ( const std::string& entryPoint );

        ~DeviceWorker ( );

        void run ( const Grid& grid );

        void registerPtr ( void* ptr,
                           size_t n_bytes );

        void unregisterPtr ( void* ptr );

        enum MapFlags
        {
          MAP_TO_READ,
          MAP_TO_WRITE
        };

        void* mapPtr ( void* ptr, MapFlags flag );
        void unmapPtr ( void* ptr );

        enum SyncDirection
        {
          HOST_TO_DEVICE,
          DEVICE_TO_HOST
        };

        void syncPtr ( void* ptr, SyncDirection flag );

        void setKernelArg ( size_t arg_idx, void* ptr );

        void setKernelArg ( size_t arg_idx, size_t sizeof_arg, void* arg );

      private:
        class Impl;
        STDTR1::shared_ptr<Impl> d_impl;
    };

  } // End: namespace OCL

} // End: namespace OCPI

#endif // End: #ifndef INCLUDED_OCPI_OCL_PLATFORM_MANAGER_H

