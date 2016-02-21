#ifndef _OCPI_REMOTE_DRIVER_H_
#define _OCPI_REMOTE_DRIVER_H_
namespace OCPI {
  namespace Remote {
    // These are common - they are initialized to zero, but accessible by driver users
    // even when the driver is not loaded.  They are declared in the driver because
    // the driver cannot depend on the non-driver/server code or headers.
    extern bool g_suppressRemoteDiscovery; // not extern so it is common
    extern bool (*g_probeServer)(const char *server, bool verbose, const char **exclude,
			  std::string &error);
  }
}
#endif
