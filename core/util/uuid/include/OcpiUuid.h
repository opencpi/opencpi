#ifndef _OCPI_UUID_H_
#define _OCPI_UUID_H_

#include <stdint.h>

namespace OCPI {
  namespace Util {
    typedef uint8_t Uuid[16];
    typedef char UuidString[37];
    void uuid2string(Uuid &, UuidString &);
    void generateUuid(Uuid &);
  }
}

#endif
