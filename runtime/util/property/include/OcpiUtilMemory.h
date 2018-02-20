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

#ifndef OCPI_UTIL_MEMORY_H
#define OCPI_UTIL_MEMORY_H

#include <string>
#include "ezxml.h"
#include "OcpiUtilMemory.h"

namespace OCPI
{
  namespace Util
  {
    class Memory
    {
      public:
        size_t    m_nBytes;
        std::string m_name;
        ezxml_t     m_xml;
        Memory ();
        const char *parse(ezxml_t x);
    };
  }
}

#endif
