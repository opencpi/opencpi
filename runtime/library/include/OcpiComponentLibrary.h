#ifndef OcpiComponentLibrary_h
#define OcpiComponentLibrary_h
#include "OcpiLibraryManager.h"

namespace OCPI {
  namespace Library {
    namespace CompLib {
      // This is here as a linker hook so that in this particular case,
      // the library manager can force this driver to be loaded.
      extern const char *component;
    }
  }
}
#endif
