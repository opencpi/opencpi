#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include "OcpiServer.h"
#include "OcpiLauncher.h"

namespace OE = OCPI::OS::Ether;
namespace OX = OCPI::Util::EzXml;
namespace OA = OCPI::API;
namespace OC = OCPI::Container;
namespace OL = OCPI::Library;
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
      m_name = OCPI::OS::getHostname();
      addFd(m_server.fd(), true);
      OU::formatAdd(m_name, ":%u", port);
#if 0
      if (mkdir(m_directory.c_str(), 0777) != 0 && errno != EEXIST) {
	OU::format(error, "Can't create directory %s to run simulation (%s)",
		   m_directory.c_str(), strerror(errno));
	return;
      }
#endif
      if (verbose)
	if (discoverable) {
	  fprintf(stderr,
		  "Container server at %s (discoverable at %s) using at UDP response addresses: ",
		  m_name.c_str(), m_disc->ifAddr().pretty());
	  for (DiscSocketsIter ci = m_discSockets.begin(); ci != m_discSockets.end(); ci++)
	    fprintf(stderr, "%s%s", ci == m_discSockets.begin() ? "" : ", ",
		    (*ci)->ifAddr().pretty());
	  fprintf(stderr, "\n");
	} else
	  fprintf(stderr, "Container server at %s\n", m_name.c_str());
      fprintf(stderr, "Artifacts are stored and cached in \"%s\" and will be %s on exit.\n",
	      m_library.libName().c_str(), m_remove ? "removed" : "retained");
	  fflush(stderr);
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
      if (FD_ISSET(m_disc->fd(), fds) && receiveDisc(error))
	return true;
      for (DiscSocketsIter dsi = m_discSockets.begin(); dsi != m_discSockets.end(); dsi++)
	if (FD_ISSET((*dsi)->fd(), fds) && receiveDiscSocket(**dsi, error))
	  return true;
      if (FD_ISSET(m_server.fd(), fds) && receiveServer(error))
	return true;
      for (ClientsIter ci = m_clients.begin(); ci != m_clients.end(); ci++)
	if (FD_ISSET((*ci)->fd(), fds) && (*ci)->receive(error))
	  return true;
#if 0
      // Next is to keep sim running by providing more credits
      // We will only enable this fd when there is no response queue
      if (FD_ISSET(m_ack.m_rfd, fds)) {
	printTime("Received ACK indication");
	if (ack(error) || spin(error))
	  return true;
      }
      if (m_xferSrvr && FD_ISSET(m_xferSrvr->fd(), fds)) {
	// Client has connected to our bit file transfer socket
	if ((m_xfd = creat(m_exec.c_str(), 0666)) < 0) {
	  OU::format(m_xferError, "Couldn't create local copy of executable: '%s' (%s)",
		     m_exec.c_str(), strerror(errno));
	} else {
	  m_xferSckt = new OS::Socket();
	  *m_xferSckt = m_xferSrvr->accept();
	  m_xferDone = false;
	  addFd(m_xferSckt->fd(), false);
	}
	delete m_xferSrvr;
	m_xferSrvr = NULL;
      }
      if (m_xferSckt && FD_ISSET(m_xferSckt->fd(), fds) && doXfer(error))
	return true;
#endif
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
    receiveServer(std::string &/*error*/) {
      OS::Socket as = m_server.accept();
      m_clients.push_back(new Client(*this, as));
      addFd(as.fd(), true);
      return false;
    }
    Server::Client::
    Client(Server &s, OS::Socket sock)
      : m_server(s), m_socket(sock), m_downloading(false), m_downloaded(false), m_rx(NULL),
	m_lx(NULL) {
    }
    Server::Client::
    ~Client() {
      ezxml_free(m_rx);
      ezxml_free(m_lx);
    }

    bool Server::Client::
    receive(std::string &error) {
      if (m_downloading)
	return download(error);
      if (receiveXml(fd(), m_rx, m_buf, error))
	return true;
      const char *tag = OX::ezxml_tag(m_rx);
      // The operations are:
      // 1. launch
      // 2. update launch
      // 3. control
      if (!strcasecmp(tag, "launch"))
	return launch(error);
      else if (!strcasecmp(tag, "update"))
	return update(error);
      else if (!strcasecmp(tag, "control"))
	return control(error);
      return OU::eformat(error, "bad request tag: \"%s\"", tag);
    }
    const char *Server::Client::
    downloadFile(int wfd, uint64_t length) {
      do {
	ssize_t nr = ::read(fd(), &m_downloadBuf[0],
			    length > m_downloadBuf.size() ?
			    m_downloadBuf.size() : OCPI_UTRUNCATE(size_t, length));
	if (nr <= 0)
	  return "reading from socket";
	length -= nr;
	ssize_t nw;
	for (char *cp = &m_downloadBuf[0]; nr; nr -= nw, cp += nw)
	  if ((nw = ::write(wfd, cp, nr)) <= 0)
	    return "writing to file";
      } while (length);
      return NULL;
    }

    bool Server::Client::
    download(std::string &error) {
      const char *err;
      size_t n = 0;
      m_downloadBuf.resize(64*1024);
      m_downloading = false;
      for (ezxml_t ax = ezxml_child(m_lx, "artifact"); ax; ax = ezxml_next(ax), n++)
	if (m_artifacts[n] == NULL) {
	  uint64_t length, mtime;
	  std::string uuid, name;
	  if ((err = OX::getNumber64(ax, "length", &length, NULL, 0, false, true)) ||
	      (err = OX::getNumber64(ax, "mtime", &mtime, NULL, 0, false, true)) ||
	      (err = OX::getRequiredString(ax, uuid, "uuid")) ||
	      (err = OX::getRequiredString(ax, name, "name")))
	    break;
	  const char
	    *artName = name.c_str(),
	    *slash = strrchr(artName, '/');
	  if (slash)
	    artName = slash + 1;
	  std::string fileName;
	  OU::format(fileName, "%s/%s:%s",
		     m_server.library().libName().c_str(), uuid.c_str(), artName);
	  ocpiInfo("Downloading artifact \"%s\" to \"%s\".  Length is %" PRIu64 ".",
		   name.c_str(), fileName.c_str(), length);
	  int wfd = open(fileName.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0777);
	  if (wfd < 0)
	    return OU::eformat(error, "Can't open artifact file \"%s\" for writing: %s (%d)",
			       fileName.c_str(), strerror(errno), errno);
	  err = downloadFile(wfd, length);
	  close(wfd);
	  if (err)
	    return OU::eformat(error, "Can't download artifact file \"%s\" when %s: %s (%d)",
			       fileName.c_str(), err, strerror(errno), errno);
	  // FIXME: error checking to prevent "the big throw"?
	  OL::Artifact &a =
	    *(m_artifacts[n] = m_server.library().addArtifact(fileName.c_str()));
	  struct utimbuf times = {time(0), a.mtime() };
	  utime(fileName.c_str(), &times);
	  // double check uuid.  setmtime?
	}
      return false;
    }

    bool Server::Client::
    doInstances(std::string &error) {
      m_instances.resize(OX::countChildren(m_lx, "instance")); 
      Launcher::Instance *i = &m_instances[0];
      for (ezxml_t ix = ezxml_cchild(m_lx, "instance"); ix; ix = ezxml_next(ix), i++) {
	std::string name, inst, slave;
	size_t artN, implN;
	const char *err;
	if ((err = OX::getRequiredString(ix, name, "name")) ||
	    (err = OX::getNumber(ix, "artifact", &artN, NULL, 0, false, true)) ||
	    (err = OX::getNumber(ix, "worker", &implN, NULL, 0, false, true))) {
	  error = err;
	  return true;
	}
	OX::getOptionalString(ix, slave, "slave");
	OX::getOptionalString(ix, inst, "static");
	assert(artN < m_artifacts.size());
	//	assert(implN < m_artifacts[artN]->m_nImplementations);
      }
      return false;
    }

    bool Server::Client::
    launch(std::string &error) {
      assert(!m_downloading);
      uint64_t done64;
      bool doneFound;
      m_lx = m_rx;
      m_rx = NULL;
      m_launchBuf = m_buf;
      const char *err = OX::getNumber64(m_lx, "done", &done64, &doneFound);
      do {
	if (err)
	  break;
	m_artifacts.resize(OX::countChildren(m_lx, "artifact"), NULL); 
	size_t n = 0;
	for (ezxml_t ax = ezxml_cchild(m_lx, "artifact"); ax; ax = ezxml_next(ax), n++) {
	  std::string uuid, id;
	  if ((err = OX::getRequiredString(ax, uuid, "uuid")) ||
	      (err = OX::getRequiredString(ax, id, "id")))
	    break;
	  if (!(m_artifacts[n] = m_server.library().findArtifact(uuid.c_str()))) {
	    if (!m_downloading) {
	      m_response = "<lanching done='0'>\n";
	      m_downloading = true;
	    }
	    // We need to request the artifact to be downloaded.
	    m_downloading = true;
	    OU::formatAdd(m_response, "  <artifact id='%s'/>\n", id.c_str());
	  }
	}
	if (m_downloading) {
	  m_response += "</launching>\n";
	  m_downloaded = true;
	  return sendXml(fd(), m_response, "responding from server", error);
	}

	// 
	// We have all the artifacts.  proceed with the app.
	// Workers
	// Instances
	// Connections
      } while(0);
      if (err)
	return OU::eformat(error, "XML error: %s", err);
      return false;
    }
    // After initial launch
    bool Server::Client::
    update(std::string &error) {
      if (m_downloaded && doInstances(error))
	return true;
      return false;
    }
    bool Server::Client::
    control(std::string &error) {
      return false;
    }
  }
}
