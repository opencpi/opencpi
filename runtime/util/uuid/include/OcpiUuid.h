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

#ifndef _OCPI_UUID_H_
#define _OCPI_UUID_H_

#include <stdint.h>
#include <string.h>

namespace OCPI {
  namespace Util {
    struct Uuid { uint8_t uuid[16]; };
    struct UuidString { char uuid[37];};
    void uuid2string(Uuid &, UuidString &);
    bool string2uuid(UuidString &, Uuid &); // true on bad format
    void generateUuid(Uuid &);
    // This is a comparison object for use in STL classes
    struct UuidComp {
      inline bool operator() (const Uuid lhs, const Uuid rhs) const {
	return memcmp(lhs.uuid, rhs.uuid, sizeof(Uuid)) < 0;
      }
    };
  }
}

#endif
