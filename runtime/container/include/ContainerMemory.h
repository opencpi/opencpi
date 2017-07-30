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

#ifndef CONTAINER_MEMORY_H
#define CONTAINER_MEMORY_H

#include <string>
#include "ezxml.h"

namespace OCPI
{
  namespace Metadata
  {
    class LocalMemory
    {
      private:
        friend class Worker;
        bool decode ( ezxml_t x );

      public:
        explicit LocalMemory ();
        virtual ~LocalMemory ();

        size_t n_bytes;
        std::string name;
        ezxml_t myXml;
    };
  }
}

#endif
