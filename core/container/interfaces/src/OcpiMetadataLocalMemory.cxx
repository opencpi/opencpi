
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

/*
  <localMemory name="mem0" size="1024"/>
  <localMemory name="mem1" size="2048"/>
*/

#include "OcpiMetadataLocalMemory.h"

#include "OcpiOsAssert.h"
#include "OcpiUtilEzxml.h"
#include "OcpiContainerMisc.h"
#include "OcpiMetadataWorker.h"

#include <iostream>

namespace OCPI
{
  namespace Metadata
  {
    LocalMemory::LocalMemory ()
      : n_bytes ( 0 ),
        name ( ),
        myXml ( 0 )
    {
      // Empty
    }

    LocalMemory::~LocalMemory ()
    {
      // Empty
    }

    bool LocalMemory::decode ( ezxml_t x )
    {
      myXml = x;

      if ( !ezxml_cattr ( x, "name" ) )
      {
        std::cout << "Missing local memory name attribute" << std::endl;
        return true;
      }

      name = ezxml_cattr ( x, "name" );

      bool found;

      n_bytes = OCPI::Container::getAttrNum ( x, "size", true, &found );

      if ( !found )
      {
        std::cout << "Missing local memory size attribute for " << name << std::endl;
        return true;
      }

      std::cout << "Local memory " << name << " has size = " << n_bytes << std::endl;

      return false;
    }
  }
}

