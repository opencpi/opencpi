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

#include <assert.h>
#include "OcpiUuid.h"

#ifdef __APPLE__

#include <uuid/uuid.h>

namespace OCPI {
  namespace Util {
    void uuid2string(Uuid &uuid, UuidString &s) {
      uuid_unparse_lower((unsigned char *)&uuid,  s);
    }
    void generateUuid(Uuid &uuid) {
      uuid_generate((unsigned char *)&uuid);
    }
  }
}

#else
// Use the OSSP one
#include "../src/uuid.h"
namespace OCPI {
  namespace Util {
    void uuid2string(Uuid &u, UuidString &s) {
      assert(sizeof(Uuid) == UUID_LEN_BIN);
      uuid_t *uuid;
      uuid_create(&uuid);
      uuid_import(uuid, UUID_FMT_BIN, u, sizeof(u));
      char *cp = s;
      size_t size = sizeof(s);
      uuid_export(uuid, UUID_FMT_STR, &cp, &size);
      uuid_destroy(uuid);
    }
    void generateUuid(Uuid &u) {
      assert(sizeof(Uuid) == UUID_LEN_BIN);
      uuid_t *uuid;
      uuid_create(&uuid);
      uuid_make(uuid, UUID_MAKE_V1);
      unsigned char *cp = u;
      size_t size = sizeof(u);
      uuid_export(uuid, UUID_FMT_BIN, &cp, &size);
      uuid_destroy(uuid);
    }
  }
}
#endif
