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
  <localMemory name="mem0" size="1024"/>
  <localMemory name="mem1" size="2048"/>
*/

#include <iostream>
#include "OcpiUtilEzxml.h"
#include "ContainerMemory.h"

namespace OX = OCPI::Util::EzXml;

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

      const char *err = OX::getNumber(x, "size", &n_bytes, &found, 0, false, false);
      if (err) {
        std::cout << "Error on size value: " << err << std::endl;
      }
      //      n_bytes = OCPI::Container::getAttrNum ( x, "size", true, &found );

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

