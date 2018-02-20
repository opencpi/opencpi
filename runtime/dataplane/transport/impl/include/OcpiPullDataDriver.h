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

#ifndef OCPI_PULL_DATA_DRIVER_H_
#define OCPI_PULL_DATA_DRIVER_H_

#include <OcpiOsDataTypes.h>

namespace OCPI {
  namespace DataTransport {

    class OcpiPort;

    struct PullDataInfo {
      volatile OCPI::OS::uint8_t*   src_buffer;
      volatile OCPI::OS::uint64_t*  src_metaData;
      volatile OCPI::OS::uint32_t*  src_flag;
      volatile OCPI::OS::uint32_t*  empty_flag;
      OCPI::RDT::Descriptors        pdesc;
    };

    class PullDataDriver
    {
    public:
      PullDataDriver( PullDataInfo* pd )
        :pull_data_info(pd){}
        virtual bool checkBufferEmpty( OCPI::OS::uint8_t* buffer_data, OCPI::OS::uint32_t tlen, 
                                       OCPI::OS::uint64_t& metaData )=0;
        virtual  ~PullDataDriver(){};
    protected:
      PullDataInfo* pull_data_info;
    };
  }
}

#endif
