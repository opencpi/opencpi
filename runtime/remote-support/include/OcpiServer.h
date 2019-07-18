#ifndef __OCPI_SERVER_H__
#define __OCPI_SERVER_H__
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

#include <string>
#include <list>
#include <vector>
#include "OcpiOsMisc.h"
#include "OcpiOsEther.h"
#include "OcpiOsServerSocket.h"
//#include "OcpiLibraryManager.h"
//#include "RemoteServer.h"

// This file implements the container server, which "serves up" containers on the system
// the server is running on.  The server uses a local launcher.
namespace OCPI {
  namespace Util {
    class Client;
    class Server {
      //      OCPI::Library::Library &m_library;
      std::string m_name;
      std::string m_serverType;
      bool m_verbose;
      std::string m_discoveryInfo;       // what to tell clients about our capabilities, etc.
      //      bool m_remove;
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
      // These clients are TCP clients that are using us as a server
      typedef std::list<Client *> Clients;
      typedef Clients::iterator ClientsIter;
      Clients m_clients;
      int m_maxFd;
      fd_set m_alwaysSet;
      unsigned m_sleepUsecs;
      unsigned long m_maxCallers; // after this many callers, exit
      unsigned long m_countCallers; // how many callers we have had
    public:
      Server(bool verbose, bool discoverable, bool loopback, bool onlyloopback,
	     uint16_t port, const char *label, const char *addrFile, std::string &error);
      ~Server();
      bool run(std::string &error);
      unsigned long set_maxCallers(const unsigned long new_max_callers);
      unsigned long get_maxCallers() { return m_maxCallers; }
      //    protected:
    protected:
      std::string &discoveryInfo() { return m_discoveryInfo; }
      virtual Client *newClient(OCPI::OS::ServerSocket &server, std::string &error) = 0;
    private:
      bool doit(std::string &error);
      void shutdown() {};
      void addFd(int fd, bool always);
      bool receiveDisc(std::string &error);
      bool receiveServer(std::string &error);
      bool receiveDiscSocket(OCPI::OS::Ether::Socket &c, std::string &error);
    };
    // The server side object representing a connected client
    class Client {
      friend class Server;
      OCPI::OS::Socket m_socket;
      std::string m_client;
    protected:
      Client(OCPI::OS::ServerSocket &svrSock, std::string &error);
      virtual ~Client();
      virtual bool receive(bool &eof, std::string &error) = 0;
      const OCPI::OS::Socket &socket() const { return m_socket; }
      inline int fd() const { return m_socket.fd(); }
      const char *client() const { return m_client.c_str(); }
      // xml message buffers?
    };
  }
}
#endif
