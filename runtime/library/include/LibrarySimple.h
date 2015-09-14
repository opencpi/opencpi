#ifndef LIBRARY_SIMPLE_H
#define LIBRARY_SIMPLE_H

#include "OcpiLibraryManager.h"

namespace OCPI {
  namespace Library {
    namespace Simple {
      // This particular library needs to be explicitly accessed by the library manager.
      OCPI::Library::Driver &getDriver();
    }
  }
}
#endif
