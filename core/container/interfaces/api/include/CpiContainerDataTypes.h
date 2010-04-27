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
 *   This file contains the interface for the CPI Node Manager Container.  
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef CPI_NM_CONTAINER_DATA_TYPES_H
#define CPI_NM_CONTAINER_DATA_TYPES_H

#include <CpiRDTInterface.h>
#include <CpiOsDataTypes.h>
#include <CpiList.h>
#include <CpiPValue.h>
#include <string>

namespace CPI {

  namespace Container {

    // The Application class is used as a context for applications to group and manage workers.
    class Application;
    class Worker;

    // The Connection cookie is used to manage the lifecycle of a port connection created
    // by an application.
    typedef void* ConnectionCookie;

    
    struct PortConnectionDesc;

    // Connection id
    typedef CPI::OS::int64_t ConnectionId;

    // Worker id
    typedef Worker* WorkerId;

    // Port id
    typedef CPI::OS::int64_t PortId;

    // Port
    typedef CPI::OS::int64_t PortDesc;

  }

}

#endif

