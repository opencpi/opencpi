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

#include "OcpiUtilEzxml.h"
#include "OcpiUtilMemory.h"

namespace OCPI
{
  namespace Util
  {
    namespace OE = OCPI::Util::EzXml;

    Memory::Memory()
      : m_nBytes (0), m_xml (NULL)
    {
      // Empty
    }

    const char *Memory::parse(ezxml_t x)
    {
      const char *err;
      if ((err = OE::checkAttrs(x, "name", "size", NULL)))
	return err;
      const char *name = ezxml_cattr ( x, "name" );
      if (!name)
	return "missing name attribute of memory element";
      if ((err = OE::getNumber(x, "size", &m_nBytes, NULL, 0)))
	return err;
      if (m_nBytes == 0)
	return "size attribute of memory element missing or zero";
      m_name = name;
      m_xml = x;
      return NULL;
    }
  }
}

