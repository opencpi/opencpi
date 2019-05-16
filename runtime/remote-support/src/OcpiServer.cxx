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

#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include "OcpiOsFileSystem.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "OcpiServer.h"

namespace OE = OCPI::OS::Ether;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;

namespace OCPI {
  namespace Util {

    Server::
    Server(bool verbose, bool discoverable, bool loopback, bool onlyloopback,
           uint16_t port, const char *label, const char *addrFile, std::string &error) :
      m_serverType(label), m_verbose(verbose), m_disc(NULL), m_maxFd(-1),
      m_sleepUsecs(1000000), m_maxCallers(0), m_countCallers(0) {
      if (!error.empty())
        return;
      FD_ZERO(&m_alwaysSet);
      if (m_verbose)
        fprintf(stderr,
                "Discovery options:  discoverable: %u, loopback: %u, onlyloopback: "
                "%u\n", discoverable, loopback, onlyloopback);
      if (discoverable) {
        OE::Interface udpIf("udp", error);
        if (error.length())
          return;
        m_disc = new OE::Socket(udpIf, ocpi_slave, NULL, 17171, error);
        if (error.length())
          return;
        OE::IfScanner ifs(error);
        if (error.length())
          return;
        addFd(m_disc->fd(), true);
        OE::Interface eif;
        std::string loopName;
        OE::Address udp(true, 17171);
        ocpiInfo("Listening on all network interfaces to be discovered as %s %s server",
                 strchr("aeiou", m_serverType[0]) ? "an" : "a", m_serverType.c_str());
        while (ifs.getNext(eif, error, NULL) && error.empty()) {
          if (eif.up && eif.connected && eif.ipAddr.addrInAddr()) {
            ocpiDebug("Interface \"%s\"(%u) up and connected and has IP address %s.",
                      eif.name.c_str(), eif.index, eif.ipAddr.prettyInAddr());
            if (eif.loopback) {
              loopName = eif.ipAddr.prettyInAddr();
              if (!loopback && !onlyloopback)
                continue;
            } else if (onlyloopback)
              continue;
            else if (m_name.empty())
              m_name = eif.ipAddr.prettyInAddr();
            OE::Socket *s = new OE::Socket(eif, ocpi_device, &udp, 0, error);
            if (!error.empty()) {
              delete s;
              return;
            }
            ocpiDebug("Creating discovery socket %p for responses on %s(%u) %u",
                      s, eif.name.c_str(), eif.index, s->ifIndex());
            m_discSockets.push_back(s);
            addFd(s->fd(), true);
          }
        }
        if (m_discSockets.empty())
          error = "no network interfaces found";
        else if (m_name.empty())
          m_name = loopName;
      }
      if (!error.empty())
        return;
      m_server.bind(port, false, false, onlyloopback);
      port = m_server.getPortNo();
      OE::Address a;
      m_server.getAddr(a);
      addFd(m_server.fd(), true);
      // Note that m_name has the IP address of the FIRST interface we found...
      if (m_name.empty())
        m_name = a.prettyInAddr();
      OU::formatAdd(m_name, ":%u", port);
      if (verbose) {
        if (discoverable) {
          fprintf(stderr,
                  "%c%s server %s at %s, discoverable at UDP: %s\n"
                  "  Using UDP discovery response addresses:\n",
                  toupper(m_serverType[0]), m_serverType.c_str() + 1,
                  m_name.c_str(), a.pretty(), m_disc->ifAddr().pretty());
          for (DiscSocketsIter ci = m_discSockets.begin(); ci != m_discSockets.end(); ci++)
            fprintf(stderr, "    %s\n", (*ci)->ifAddr().pretty());
        } else
          fprintf(stderr, "%c%s server at %s\n",
                  toupper(m_serverType[0]), m_serverType.c_str() + 1, a.pretty());
      }
      FILE *safp = NULL;
      if (addrFile && (safp = fopen(addrFile, "w")) == NULL) {
        OU::format(error, "Can't open file \"%s\" for writing server addresses", addrFile);
        return;
      }
      if (verbose || addrFile) {
        OE::IfScanner ifs(error);
        if (error.length())
          return;
        OE::Interface eif;
        size_t len = 0;
        if (verbose) {
          for (unsigned n = 0; ifs.getNext(eif, error, NULL) && error.empty(); n++)
            if (eif.up && eif.connected && eif.ipAddr.addrInAddr() &&
                eif.name.length() > len)
              len = eif.name.length();
          ifs.reset();
          fprintf(stderr, "  Available TCP server addresses are:\n");
        }
        for (unsigned n = 0; ifs.getNext(eif, error, NULL) && error.empty(); n++)
          if (eif.up && eif.connected && eif.ipAddr.addrInAddr() &&
              ((eif.loopback && (onlyloopback || loopback)) ||
               (!eif.loopback && !onlyloopback))) {
            const char *pretty = eif.ipAddr.pretty();
            std::string addr;
            OU::format(addr, "%.*s:%u", (int)(strchr(pretty, ':') - pretty), pretty,
                       a.addrPort());
            if (verbose)
              fprintf(stderr, "    On interface %s: %*s%s\n", eif.name.c_str(),
                      (int)(len - eif.name.length()), "", addr.c_str());
            if (addrFile)
              fprintf(safp, "%s\n", addr.c_str());
          }
        if (verbose) {
          if (addrFile)
            fprintf(stderr, "Server IP addresses stored in file \"%s\", one per line.\n",
                    addrFile);
#if 0
          fprintf(stderr,
                  "Artifacts stored/cached in the directory \"%s\"; which will be %s on exit.\n",
                  m_library.libName().c_str(), m_remove ? "removed" : "retained");
          fprintf(stderr, "Containers offered to clients are:\n");
#endif
        }
        fflush(stderr); //why?
        if (safp)
          fclose(safp);
      }
    }
    Server::~Server() {
      delete m_disc;
      while (!m_discSockets.empty()) {
        delete m_discSockets.front();
        m_discSockets.pop_front();
      }
    }
    void Server::addFd(int fd, bool always) {
      if (fd > m_maxFd)
        m_maxFd = fd;
      if (always)
        FD_SET(fd, &m_alwaysSet);
    }
    bool Server::run(std::string &error) {
      while (!doit(error))
        ;
      ocpiBad("%c%s server stopped on error: %s",
              toupper(m_serverType[0]), m_serverType.c_str() + 1, error.c_str());
      shutdown();
      return !error.empty();
    }
    // Server is doing what it does
    bool Server::doit(std::string &error) {
      fd_set fds[1];
      *fds = m_alwaysSet;
      struct timeval timeout[1];
      timeout[0].tv_sec = m_sleepUsecs / 1000000;
      timeout[0].tv_usec = m_sleepUsecs % 1000000;
      errno = 0;
      switch (select(m_maxFd + 1, fds, NULL, NULL, timeout)) {
      case 0:
        ocpiDebug("select timeout");
        return false;
      case -1:
        if (errno == EINTR)
          return false;
        OU::format(error, "Select failed: %s %u", strerror(errno), errno);
        return true;
      default:
        ;
      }
      ocpiDebug("Select returned a real fd %" PRIx64 " server %d",
                *(uint64_t *)fds, m_server.fd());
      if (m_disc && FD_ISSET(m_disc->fd(), fds) && receiveDisc(error))
        return true;
      for (DiscSocketsIter dsi = m_discSockets.begin(); dsi != m_discSockets.end(); dsi++)
        if (FD_ISSET((*dsi)->fd(), fds) && receiveDiscSocket(**dsi, error))
          return true;
      if (FD_ISSET(m_server.fd(), fds) && receiveServer(error))
        return true;
      bool eof;
      for (ClientsIter ci = m_clients.begin(); ci != m_clients.end();) {
        if (FD_ISSET((*ci)->fd(), fds) && (*ci)->receive(eof, error)) {
	  if (eof) {
	    if (m_verbose)
             fprintf(stderr, "Client \"%s\" has disconnected.\n", (*ci)->client());
	    ocpiInfo("Client \"%s\" has disconnected.", (*ci)->client());
	  } else {
	    if (m_verbose)
              fprintf(stderr, "Shutting down client \"%s\" due to error: %s\n",
                      (*ci)->client(), error.c_str());
	    ocpiInfo("Shutting down client \"%s\" due to error: %s",
		     (*ci)->client(), error.c_str());
          }
          error.clear();
          Client *c = *ci;
          ClientsIter tmp = ci;
          ci++;
          m_clients.erase(tmp);
          FD_CLR(c->fd(), &m_alwaysSet);
          delete c;
          ++m_countCallers;
          if (m_maxCallers && m_maxCallers <= m_countCallers) {
            ocpiInfo("Max number of callers reached (%lu vs. max %lu)", m_countCallers, m_maxCallers);
            return true;
          }
        } else {
          ci++;
        }
      } // iterating m_clients
      return false;
    }
    bool Server::
    receiveDisc(std::string &error) {
      OE::Packet rFrame;
      size_t length;
      OE::Address from;
      unsigned index = 0;
      if (m_disc->receive(rFrame, length, 0, from, error, &index)) {
        ocpiDebug("Received to %s %s server discovery request from %s, length %zu, index %u",
                  m_name.c_str(), m_serverType.c_str(), from.pretty(), length, index);
        if (index) {
          OE::Socket *s = NULL;
          for (DiscSocketsIter ci = m_discSockets.begin(); ci != m_discSockets.end(); ci++)
            if ((*ci)->ifIndex() == index) {
              s = *ci;
              break;
            }
          if (!s) {
            ocpiInfo("received discovery from unsupported interface index %u, perhaps loopback not enabled?", index);
            return false;
          }
          char
            *start = (char *)rFrame.payload,
            *cp = start;
          strcpy(cp, m_name.c_str());
          cp += m_name.length();
          *cp++ = '\n';
          if (m_discoveryInfo.length() + 1 > (size_t)(OE::MaxPacketSize - (cp - start))) {
            OU::format(error, "Too many containers, discovery buffer would overflow");
            return true;
          }
          strcpy(cp, m_discoveryInfo.c_str());
          length = strlen(start) + 1;
          ocpiLog(9, "%c%s server %s discovery returns: \n%s---end of discovery",
                  toupper(m_serverType[0]), m_serverType.c_str() + 1, m_name.c_str(), start);
          return !s->send(rFrame, length, from, 0, NULL, error);
        } else
          error = "No interface index for receiving discovery datagrams";
      }
      return true;
    }
    bool Server::
    receiveDiscSocket(OE::Socket &, std::string &error) {
      error = "Unexpected packet received on per-interface UDP socket";
      return true;
    }
    bool Server::
    receiveServer(std::string &error) {
      ocpiDebug("Received connection request: creating a new client");
      Client *c = newClient(m_server, error);
#if 0
      OR::Server *c =
        new OR::Server(library(), m_server, m_discoveryInfo, m_needsBridging, error);
#endif
      if (error.length()) {
        delete c;
        return true;
      }
      m_clients.push_back(c);
      addFd(c->fd(), true);
      if (m_verbose)
        fprintf(stderr, "New client is \"%s\".\n", c->client());
      return false;
    }
    unsigned long Server::
    set_maxCallers(const unsigned long new_max_callers) {
        auto old = m_maxCallers;
        m_maxCallers = new_max_callers;
        return old;
    }
    Client::Client(OS::ServerSocket &serverSocket, std::string &/*error*/) {
      serverSocket.accept(m_socket);
      std::string host;
      uint16_t port;
      m_socket.getPeerName(host, port);
      OU::format(m_client, "%s:%u", host.c_str(), port);
    }
    Client::~Client() {}
  }
}
