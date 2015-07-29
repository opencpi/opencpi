#if 0
#include <vector>

#include "ContainerLauncher.h"
#endif

#include <string>
#include <list>
#include "OcpiOsMisc.h"
#include "OcpiOsEther.h"
#include "OcpiOsServerSocket.h"
#include "OcpiLibraryManager.h"
#include "RemoteServer.h"

// This file implements the container server, which "serves up" containers on the system
// the server is running on.  The server uses a local launcher.
namespace OCPI {
  namespace Application {
    class Server {
      OCPI::Library::Library &m_library;
      std::string m_name;
      bool m_verbose;
      bool m_remove;
      // This UDP endpoint is where we get "discovered" and we only only receive, never transmit
      // We receive multicast UDP discovery probes, but respond on the interface-specific
      // endpoints in m_discSockets
      OCPI::OS::Ether::Socket *m_disc;
      // These endpoints is what we use for responding to discovery probes
      // We respond with our server TCP address
      typedef std::list<OCPI::OS::Ether::Socket *> DiscSockets;
      typedef DiscSockets::iterator DiscSocketsIter;
      DiscSockets m_discSockets;
      // This is our TCP server socket on which to establish per-client connections
      OCPI::OS::ServerSocket m_server;
      // These clients are TCP clients that are using us as a container server
      typedef std::list<OCPI::Remote::Server *> Clients;
      typedef Clients::iterator ClientsIter;
      Clients m_clients;
      int m_maxFd;
      fd_set m_alwaysSet;
      unsigned m_sleepUsecs;
      std::string m_directory;
    public:
      Server(bool verbose, bool discoverable, OCPI::Library::Library &lib, uint16_t port,
	     bool remove, std::string &error);
      ~Server();
      bool run(std::string &error);
    protected:
      OCPI::Library::Library &library() { return m_library; }
    private:
      bool doit(std::string &error);
      void shutdown() {};
      void addFd(int fd, bool always);
      bool receiveDisc(std::string &error);
      bool receiveServer(std::string &error);
      bool receiveDiscSocket(OCPI::OS::Ether::Socket &c, std::string &error);
    };
  }
}
