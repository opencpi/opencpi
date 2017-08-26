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

#ifndef OCPI_DataTransport__Constants_H_
#define OCPI_DataTransport__Constants_H_

#include "XferEndPoint.h"

namespace OCPI {

namespace DataTransport {

    // Maximum endpoint size
  //    const uint32_t MAX_ENDPOINT_SIZE = 160;

    // maximum number of transport source contributers
    const uint32_t MAX_PCONTRIBS = DtOsDataTypes::MAX_SYSTEM_SMBS;

    // maximum number of endpoints 
    // const uint32_t MAX_ENDPOINTS = 32;

    // Maximum number of ports per port set
    //    const uint32_t MAX_PORTS_PER_SET = 16;

    // Maximum number of circuits per connection
    //    const uint32_t MAX_CIRCUITS_PER_CONNECTION = 2;

    // Maximum number of ports per connection
    //    const uint32_t MAX_PORT_SETS_PER_CONNECTION = 4;

    // Invalid port id
    //    const int32_t InvalidPortId = -1;

    // Maximum number of buffer
    const uint32_t MAX_BUFFERS = 16;

    // Maximum number of transfers
    const uint32_t MAX_TRANSFERS = 8;

    // Maximum number of transfers per buffer
    const uint32_t MAX_TRANSFERS_PER_BUFFER = 4;

    // maximum length for transport name
    //    const uint32_t MAX_TRANSPORT_NAME_LEN = 80;

    // Maximum number of factories
    //    const uint32_t MAX_FACTORY_LEN = 5;

    // Maximum number of allowed templates
    //    const uint32_t MAX_TX_TEMPLATES = 10;


  }

}


#endif


