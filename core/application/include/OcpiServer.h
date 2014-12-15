#include "OcpiOsEther.h"
#include "OcpiOsMisc.h"
#include "OcpiOsServerSocket.h"
#include "OcpiLibraryManager.h"
#include "OcpiLauncher.h"
// This file implements the container server, which "serves up" containers on the system
// the server is running on.  The server uses a local launcher.
namespace OCPI {
  namespace Application {
    class Server {
      class Client {
	friend class Server;
	Server &m_server;
	OCPI::OS::Socket m_socket;
	bool m_downloading;                     // true when downloading artifacts
	bool m_downloaded;                      // true if we downloaded any artifacts
	std::vector<char> m_buf;                // xml message buffer
	ezxml_t m_rx;                           // xml message xml
	std::vector<char> m_launchBuf;          // saved launch message to back up the XML
	ezxml_t m_lx;                           // saved initial launch XML
	std::vector<OCPI::Library::Artifact*> m_artifacts; // in order of launch request
	std::string m_response;                 // xml response to send to client
	std::vector<char> m_downloadBuf;
	Launcher::Instances m_instances;
      protected:
	Client(Server &s, OCPI::OS::Socket sock);
	~Client();
	bool receive(std::string &error);
      private:
	inline int fd() const { return m_socket.fd(); }
	const char *downloadFile(int wfd, uint64_t length);
	bool
	  download(std::string &error),
	  launch(std::string &error),
	  update(std::string &error),
	  control(std::string &error),
	  doInstances(std::string &error);
      };
      friend class Client;
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
      typedef std::list<Client *> Clients;
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
