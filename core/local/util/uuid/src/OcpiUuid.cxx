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
