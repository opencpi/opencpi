#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include "OcpiOsFileSystem.h"
#include "Container.h"
#include "ContainerManager.h"
#include "RemoteLauncher.h"
#include "OcpiServer.h"

namespace OE = OCPI::OS::Ether;
namespace OA = OCPI::API;
namespace OC = OCPI::Container;
namespace OR = OCPI::Remote;
namespace OU = OCPI::Util;
namespace OCPI {
  namespace Application {

    Server::
    Server(bool verbose, bool discoverable, OCPI::Library::Library &library, uint16_t port,
	   bool remove, std::string &error)
      : m_library(library), m_verbose(verbose), m_remove(remove), m_disc(NULL),
	m_maxFd(-1), m_sleepUsecs(1000000) {
      if (!error.empty())
	return;
      FD_ZERO(&m_alwaysSet);
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
	OE::Address udp(true, 17171);
	ocpiInfo("Listening on all network interfaces to be discovered as a container server");
	while (ifs.getNext(eif, error, NULL) && error.empty()) {
	  if (eif.up && eif.connected && eif.ipAddr.addrInAddr()) {
	    ocpiDebug("Interface \"%s\" up and connected and has IP address.",
		      eif.name.c_str());
	    OE::Socket *s = new OE::Socket(eif, ocpi_device, &udp, 0, error);
	    if (!error.empty()) {
	      delete s;
	      return;
	    }
	    m_discSockets.push_back(s);
	    addFd(s->fd(), true);
	  }
	}
	if (m_discSockets.empty())
	  error = "no network interfaces found";
      }
      if (!error.empty())
	return;
      m_server.bind(port);
      port = m_server.getPortNo();
      OE::Address a;
      m_server.getAddr(a);
      m_name = OS::getHostname();
      addFd(m_server.fd(), true);
      OU::formatAdd(m_name, ":%u", port);
      if (verbose) {
	if (discoverable) {
	  fprintf(stderr,
		  "Container server at %s (TCP: %s, discoverable at UDP %s)\n"
		  " using UDP response addresses:  ",
		  m_name.c_str(), a.pretty(), m_disc->ifAddr().pretty());
	  for (DiscSocketsIter ci = m_discSockets.begin(); ci != m_discSockets.end(); ci++)
	    fprintf(stderr, "%s%s", ci == m_discSockets.begin() ? "" : ", ",
		    (*ci)->ifAddr().pretty());
	  fprintf(stderr, "\n");
	} else
	  fprintf(stderr, "Container server at %s (IP: %s)\n", m_name.c_str(), a.pretty());
	if (a.addrInAddr() == 0) {
	  fprintf(stderr, "Available TCP addresses are: ");
	  OE::IfScanner ifs(error);
	  if (error.length())
	    return;
	  OE::Interface eif;
	  for (unsigned n = 0; ifs.getNext(eif, error, NULL) && error.empty(); n++)
	    if (eif.up && eif.connected && eif.ipAddr.addrInAddr()) {
	      const char *pretty = eif.ipAddr.pretty();
	      fprintf(stderr, "%son %s: %*s:%u", n ? ", " : "", 
		      eif.name.c_str(), (int)(strrchr(pretty, ':') - pretty), pretty, a.addrPort());
	    }
	  fprintf(stderr, "\n");
	}
	fprintf(stderr, "Artifacts stored and cached in the directory \"%s\"; will be %s on exit.\n",
		m_library.libName().c_str(), m_remove ? "removed" : "retained");
	fflush(stderr);
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
      ocpiBad("Container server stopped on error: %s", error.c_str());
      shutdown();
      return !error.empty();
    }
    // Container server is launching apps and doing control operations.
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
      ocpiDebug("Select returned a real fd %"PRIx64" server %d", *(uint64_t *)fds, m_server.fd());
      if (FD_ISSET(m_disc->fd(), fds) && receiveDisc(error))
	return true;
      for (DiscSocketsIter dsi = m_discSockets.begin(); dsi != m_discSockets.end(); dsi++)
	if (FD_ISSET((*dsi)->fd(), fds) && receiveDiscSocket(**dsi, error))
	  return true;
      if (FD_ISSET(m_server.fd(), fds) && receiveServer(error))
	return true;
      bool eof;
      for (ClientsIter ci = m_clients.begin(); ci != m_clients.end();)
	if (FD_ISSET((*ci)->fd(), fds) && (*ci)->receive(eof, error)) {
	  std::string host;
	  uint16_t port;
	  (*ci)->socket().getPeerName(host, port);
	  if (m_verbose)
	    fprintf(stderr, "Shutting down client %s:%u due to error: %s\n",
		    host.c_str(), port, error.c_str());
	  ocpiInfo("Shutting down client %s:%u due to error: %s",
		   host.c_str(), port, error.c_str());
	  error.clear();
	  OR::Server *c = *ci;
	  ClientsIter tmp = ci;
	  ci++;
	  m_clients.erase(tmp);
	  FD_CLR(c->fd(), &m_alwaysSet);
	  delete c;
	} else
	  ci++;
      return false;
    }
    bool Server::
    receiveDisc(std::string &error) {
      OE::Packet rFrame;
      size_t length;
      OE::Address from;
      unsigned index = 0;
      if (m_disc->receive(rFrame, length, 0, from, error, &index)) {
	ocpiDebug("Received container server discovery request from %s, length %zu",
		  from.pretty(), length);
	if (index) {
	  OE::Socket *s = NULL;
	  for (DiscSocketsIter ci = m_discSockets.begin(); ci != m_discSockets.end(); ci++)
	    if ((*ci)->ifIndex() == index) {
	      s = *ci;
	      break;
	    }
	  if (!s) {
	    OU::format(error, "can't find a client socket with interface %u", index);
	    return true;
	  }
	  char
	    *start = (char *)rFrame.payload,
	    *cp = start;
	  strcpy(cp, m_name.c_str());
	  cp += m_name.length();
	  *cp++ = '\n';
	  OA::Container *ac;
	  for (unsigned n = 0; (ac = OA::ContainerManager::get(n)); n++) {
	    OC::Container &c = *static_cast<OC::Container *>(ac);
	    size_t
	      left = OE::MaxPacketSize - (cp - start),
	      inserted = snprintf(cp, left, "%s:%s:%s:%s:%s\n",
				  c.name().c_str(), c.model().c_str(), c.os().c_str(),
				  c.osVersion().c_str(), c.platform().c_str());
	      if (inserted+1 > left)
		OU::format(error, "Too many containers, discovery buffer would overflow");
	    cp += inserted;
	  }
	  *cp++ = 0;
	  length = cp - (char*)rFrame.payload;
	  ocpiDebug("Container server discovery returns: \n%s\n---end of discovery",
		    start);
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
      OR::Server *c = new OR::Server(library(), m_server, error);
      if (error.length()) {
	delete c;
	return true;
      }
      m_clients.push_back(c);
      addFd(c->fd(), true);
      return false;
    }
  }
}
