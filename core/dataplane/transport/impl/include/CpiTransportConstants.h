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
 *   This file contains the constant definitions for the node manager.
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 7/2004
 *    Revision Detail: Created
 *
 */

#ifndef CPI_DataTransport__Constants_H_
#define CPI_DataTransport__Constants_H_

#include <CpiOsDataTypes.h>

namespace CPI {

namespace DataTransport {

    // Maximum endpoint size
    const CPI::OS::uint32_t MAX_ENDPOINT_SIZE = 160;

    // maximum number of transport source contributers
    const CPI::OS::uint32_t MAX_PCONTRIBS = 32;

    // maximum number of endpoints 
    const CPI::OS::uint32_t MAX_ENDPOINTS = 32;

    // Maximum number of ports per port set
    const CPI::OS::uint32_t MAX_PORTS_PER_SET = 16;

    // Maximum number of circuits per connection
    const CPI::OS::uint32_t MAX_CIRCUITS_PER_CONNECTION = 2;

    // Maximum number of ports per connection
    const CPI::OS::uint32_t MAX_PORT_SETS_PER_CONNECTION = 4;

    // Invalid port id
    const CPI::OS::int32_t InvalidPortId = -1;

    // Maximum number of buffer
    const CPI::OS::uint32_t MAX_BUFFERS = 20;

    // Maximum number of transfers
    const CPI::OS::uint32_t MAX_TRANSFERS = 8;

    // Maximum number of transfers per buffer
    const CPI::OS::uint32_t MAX_TRANSFERS_PER_BUFFER = 4;

    // maximum length for transport name
    const CPI::OS::uint32_t MAX_TRANSPORT_NAME_LEN = 80;

    // Maximum number of factories
    const CPI::OS::uint32_t MAX_FACTORY_LEN = 5;

    // Maximum number of allowed templates
    const CPI::OS::uint32_t MAX_TX_TEMPLATES = 10;


  }

}


#endif


