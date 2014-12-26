#ifndef _OCPI_REMOTE_DRIVER_H_
#define _OCPI_REMOTE_DRIVER_H_
namespace OCPI {
  namespace Remote {
    extern bool g_suppressRemoteDiscovery;
    extern bool useServer(const char *server, bool verbose, const char **exclude,
			  std::string &error);
  }
}
#endif
