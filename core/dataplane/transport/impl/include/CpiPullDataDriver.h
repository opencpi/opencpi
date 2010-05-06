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

#ifndef CPI_PULL_DATA_DRIVER_H_
#define CPI_PULL_DATA_DRIVER_H_

#include <CpiOsDataTypes.h>

namespace CPI {
  namespace DataTransport {

    class CpiPort;

    struct PullDataInfo {
      volatile CPI::OS::uint8_t*   src_buffer;
      volatile CPI::OS::uint64_t*  src_metaData;
      volatile CPI::OS::uint32_t*  src_flag;
      volatile CPI::OS::uint32_t*  empty_flag;
      CPI::RDT::Descriptors        pdesc;
    };

    class PullDataDriver
    {
    public:
      PullDataDriver( PullDataInfo* pd )
        :pull_data_info(pd){}
        virtual bool checkBufferEmpty( CPI::OS::uint8_t* buffer_data, CPI::OS::uint32_t tlen, 
                                       CPI::OS::uint64_t& metaData )=0;
        virtual  ~PullDataDriver(){};
    protected:
      PullDataInfo* pull_data_info;
    };
  }
}

#endif
