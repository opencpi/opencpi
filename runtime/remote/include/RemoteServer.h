/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <string>

#include "OcpiOsServerSocket.h"
#include "OcpiLibraryManager.h"
#include "ContainerLauncher.h"
// This file implements the container server, which "serves up" containers on the system
// the server is running on.  The server uses a local launcher.
// The source code for the server is in the application directory since this directory
// is the "remote container driver", and the server is not part of that.
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
      typedef std::vector<OCPI::Container::Launcher::Crew> Crews;
      std::vector<OCPI::Container::Container *> m_containers;
      std::vector<OCPI::Container::Application *> m_containerApps;
      Crews m_crews;
      OCPI::Container::Launcher::Members m_members;
      OCPI::Container::Launcher::Connections m_connections;
      std::string &m_discoveryInfo;       // what to tell clients about our containers, etc.
      std::vector<bool> &m_needsBridging; // per container, does it need bridging to sockets
    public:
      Server(OCPI::Library::Library &l, OCPI::OS::ServerSocket &svrSock,
	     std::string &discoveryInfo, std::vector<bool> &needsBridging, std::string &error);
      ~Server();
      bool receive(bool &eof, std::string &error);
      inline int fd() const { return m_socket.fd(); }
      const OCPI::OS::Socket &socket() const { return m_socket; }
      const char *client() const { return m_client.c_str(); }
      // fill discovery into (container list) into buf, including null termination.
      // length argument is the actual buffer size, and it is decremented with what is
      // put into the buffer, NOT including the NULL termination that is also put in,
      // just like snprintf
      static bool
	fillDiscoveryInfo(char *buf, size_t &length, std::string &error);
    private:
      const char
	*downloadFile(int wfd, uint64_t length),
	*doSide(ezxml_t cx, OCPI::Container::Launcher::Port &p, const char *type),
        *doSide2(OCPI::Container::Launcher::Port &p,
		 OCPI::Container::Launcher::Port &other);
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
