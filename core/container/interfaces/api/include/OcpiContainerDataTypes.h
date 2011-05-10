
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
 * Abstact:
 *   This file contains the interface for the OCPI Node Manager Container.  
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_NM_CONTAINER_DATA_TYPES_H
#define OCPI_NM_CONTAINER_DATA_TYPES_H

#include <OcpiRDTInterface.h>
#include <OcpiOsDataTypes.h>
#include <OcpiList.h>
#include <OcpiPValue.h>
#include <string>

namespace OCPI {

  namespace Container {

    // The Application class is used as a context for applications to group and manage workers.
    //    class Application;
    //    class Worker;

    // The Connection cookie is used to manage the lifecycle of a port connection created
    // by an application.
    typedef void* ConnectionCookie;

    
    struct PortConnectionDesc;

    // Connection id
    typedef OCPI::OS::int64_t ConnectionId;

    // Worker id
    //    typedef uint64_t *WorkerId;

    // Port id
    typedef OCPI::OS::int64_t PortId;

    // Port
    typedef OCPI::OS::int64_t PortDesc;

  }

}

#endif

