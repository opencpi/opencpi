
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
