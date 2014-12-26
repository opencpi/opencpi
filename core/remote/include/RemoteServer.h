#include <vector>
#include <string>

#include "OcpiOsServerSocket.h"
#include "OcpiLibraryManager.h"
#include "ContainerLauncher.h"
// This file implements the container server, which "serves up" containers on the system
// the server is running on.  The server uses a local launcher.
namespace OCPI {
  namespace Remote {
    class Server {
      OCPI::Library::Library &m_library;
      OCPI::OS::Socket m_socket;
      bool m_downloading;                     // true when downloading artifacts
      bool m_downloaded;                      // true if any were actually downloaded
      std::vector<char> m_buf;                // xml message buffer
      ezxml_t m_rx;                           // xml message xml
      std::vector<char> m_launchBuf;          // saved launch message to back up the XML
      ezxml_t m_lx;                           // saved initial launch XML
      std::vector<OCPI::Library::Artifact*> m_artifacts; // in order of launch request
      std::string m_response;                 // xml response to send to client
      std::string m_client;
      std::vector<char> m_downloadBuf;
      OCPI::Container::Launcher *m_local;
      // These two are what the underlying local launcher needs
      OCPI::Container::Launcher::Instances m_instances;
      OCPI::Container::Launcher::Connections m_connections;
    public:
      Server(OCPI::Library::Library &l, OCPI::OS::ServerSocket &svrSock, std::string &error);
      ~Server();
      bool receive(bool &eof, std::string &error);
      inline int fd() const { return m_socket.fd(); }
      const OCPI::OS::Socket &socket() const { return m_socket; }
      const char *client() const { return m_client.c_str(); }
      // fill discovery into (container list) into buf, including null termination.
      // length argument is the actual buffer size, and it is decremented with what is
      // put into the buffer, NOT including the NULL termination that is also put in,
      // just like snprintf
      static bool fillDiscoveryInfo(char *buf, size_t &length, std::string &error);
    private:
      const char *downloadFile(int wfd, uint64_t length);
      bool
	download(std::string &error),
	launch(std::string &error),
	update(std::string &error),
	control(std::string &error),
	discover(std::string &error),
	doConnection(ezxml_t cx, OCPI::Container::Launcher::Connection &c, std::string &error),
	doLaunch(std::string &error);
    };
  }
}
