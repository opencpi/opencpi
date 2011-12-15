#ifndef OCPI_LIBRARY_API_H
#define OCPI_LIBRARY_API_H
#include <string>

namespace OCPI {
  namespace API {
    // How to express how a worker will be connected in the application
    // it is defined here since it is relevant to connections that are hardwired
    // in artifacts.
    struct Connection {
      const char *port, *otherWorker, *otherPort;
    };
    class LibraryManager {
      // The function that allows a control application to set the library
      // path independent of the environment.  It will supercede any
      // environment setting.
      static void setPath(const char*);
      static std::string getPath(); // return a copy so it can change...
    };
  }
}
#endif
