#include <inttypes.h>
#include <utime.h>
#include <unistd.h>
#include <cerrno>
#include "OcpiOsFileSystem.h"
#include "OcpiOsEther.h"
#include "Container.h"
#include "ContainerManager.h"
#include "ContainerLauncher.h"
#include "RemoteLauncher.h"
#include "RemoteServer.h"

namespace OX = OCPI::Util::EzXml;
namespace OC = OCPI::Container;
namespace OL = OCPI::Library;
namespace OU = OCPI::Util;
namespace OE = OCPI::OS::Ether;
namespace OA = OCPI::API;
namespace OR = OCPI::RDT;
namespace OCPI {
  namespace Remote {

    Server::
    Server(OL::Library &l, OS::ServerSocket &svrSock, std::string &/*error*/)
      : m_library(l), m_downloading(false), m_downloaded(false), m_rx(NULL), m_lx(NULL),
	m_local(NULL) {
      svrSock.accept(m_socket);
      std::string host;
      uint16_t port;
      m_socket.getPeerName(host, port);
      OU::format(m_client, "%s:%u", host.c_str(), port);
    }
    Server::
    ~Server() {
      ezxml_free(m_rx);
      ezxml_free(m_lx);
      for (unsigned n = 0; n < m_containerApps.size(); n++)
	delete m_containerApps[n];
    }

    bool Server::
    receive(bool &eof, std::string &error) {
      if (m_downloading)
	return download(error);
      if (Launcher::receiveXml(fd(), m_rx, m_buf, eof, error))
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
      else if (!strcasecmp(tag, "discover"))
	return discover(error);
      return OU::eformat(error, "bad request tag: \"%s\"", tag);
    }
    const char *Server::
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

    bool Server::
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
	  bool isDir;
	  if (OS::FileSystem::exists(m_library.libName(), &isDir)) {
	    if (!isDir)
	      return OU::eformat(error,
				 "Artifact directory: \"%s\" exists but is not a directory",
				 m_library.libName().c_str());
	  } else
	    try {
	      OS::FileSystem::mkdir(m_library.libName());
	    } catch (...) {
	      return OU::eformat(error, "Cannot create artifact directoryL \"%s\"", 
				 m_library.libName().c_str());
	    }
	  OU::format(fileName, "%s/%s:%s",
		     m_library.libName().c_str(), uuid.c_str(), artName);
	  ocpiInfo("Downloading artifact \"%s\" to \"%s\".  Length is %" PRIu64 ".",
		   name.c_str(), fileName.c_str(), length);
	  int wfd = open(fileName.c_str(), O_CREAT | O_WRONLY, 0777);
	  if (wfd < 0)
	    return OU::eformat(error, "Can't open artifact file \"%s\" for writing: %s (%d)",
			       fileName.c_str(), strerror(errno), errno);
	  err = downloadFile(wfd, length);
	  close(wfd);
	  if (err) {
	    unlink(fileName.c_str());
	    return OU::eformat(error, "Can't download artifact file \"%s\" when %s: %s (%d)",
			       fileName.c_str(), err, strerror(errno), errno);
	  }
	  // FIXME: error checking to prevent "the big throw"?
	  OL::Artifact &a =
	    *(m_artifacts[n] = m_library.addArtifact(fileName.c_str()));
	  struct utimbuf times = {time(0), a.mtime() };
	  utime(fileName.c_str(), &times);
	  // double check uuid.  setmtime?
	}
      return false;
    }

    const char *Server::
    doSide(ezxml_t cx, OC::Launcher::Port &p, const char *type) {
      ezxml_t px = ezxml_cchild(cx, type);
      ocpiAssert(px);
      size_t member;
      bool hasMember;
      p.m_url = ezxml_cattr(px, "url");
      p.m_name = ezxml_cattr(px, "name");
      const char *err;
      if ((err = OX::getNumber(px, "scale", &p.m_scale, NULL, 1)) ||
	  (err = OX::getNumber(px, "index", &p.m_index, NULL, 0)) ||
	  (err = OX::getNumber(px, "member", &member, &hasMember)))
	return err;
      if (hasMember) {
	assert(member < m_members.size());
	p.m_launcher = m_local;
	p.m_member = &m_members[member];
      }
      ezxml_t x;
      if ((x = ezxml_cchild(px, "params")) && (err = p.m_params.addXml(x)))
	return err;
      if ((x = ezxml_cchild(px, "port"))) {
	OU::Port *mp = new OU::Port(x);
	if ((err = mp->parse()))
	  return err;
	p.m_metaPort = mp;
      }
      return NULL;
    }

    bool Server::
    doConnection(ezxml_t cx, OC::Launcher::Connection &c, std::string &error) {
      c.m_transport.transport = ezxml_cattr(cx, "transport");
      c.m_transport.id = ezxml_cattr(cx, "id");
      const char *err;
      size_t roleIn, roleOut;
      if ((err = OX::getNumber(cx, "roleIn", &roleIn, NULL, 0)) ||
	  (err = OX::getNumber(cx, "roleOut", &roleOut, NULL, 0)) ||
	  (err = OX::getNumber(cx, "bufferSize", &c.m_bufferSize, NULL, 0)) ||
	  (err = doSide(cx, c.m_in, "in")) ||
	  (err = doSide(cx, c.m_out, "out")))
	return OU::eformat(error, "Error processing connection values for launch: %s", err);
      c.m_transport.roleIn = OCPI_UTRUNCATE(OR::PortRole, roleIn);
      c.m_transport.roleOut = OCPI_UTRUNCATE(OR::PortRole, roleOut);
      ezxml_t px;
      if ((px = ezxml_cchild(cx, "paramsin")))
	c.m_in.m_params.addXml(px);
      if ((px = ezxml_cchild(cx, "paramsout")))
	c.m_out.m_params.addXml(px);
      return false;
    }

    // Do the real launch after artifacts are available
    // This might happen in "launch" if all the artifacts are already present.
    bool Server::
    doLaunch(std::string &error) {

      containers.resize(OX::countChildren(m_lx, "container"),0);
      containerApps.resize(containers.size(),0);

      unsigned n = 0;
      for (ezxml_t cx = ezxml_cchild(m_lx, "container"); cx; cx = ezxml_next(cx), n++) {
	const char *name = ezxml_cattr(cx, "name");
	assert(name);
	assert(!m_containers[n]);
	m_containers[n] = OC::Manager::find(name);
	assert(m_containers[n]);
	assert(!m_containerApps[n]);
	OC::Launcher *l = &m_containers[n]->launcher();
	assert(!m_local || m_local == l);
	m_local = l;
	m_containerApps[n] = static_cast<OC::Application*>(m_containers[n]->createApplication());
      }
      std::vector<ezxml_t> crewsXml; // hold onto the xml until we are parsing members
      m_crews.resize(OX::countChildren(m_lx, "crew"));
      crewsXml.resize(m_crews.size());
      ezxml_t *cxp = &crewsXml[0];
      OC::Launcher::Crew *cr = &m_crews[0];
      for (ezxml_t cx = ezxml_cchild(m_lx, "crew"); cx; cx = ezxml_next(cx), cr++, cxp++) {
	const char *err;
	if ((err = OX::getNumber(cx, "size", &cr->m_size, NULL, 0, false, true))) {
	  error = err;
	  return true;
	}
	cr->m_propValues.resize(OX::countChildren(cx, "property"));
	cr->m_propOrdinals.resize(cr->m_propValues.size());
	*cxp = ezxml_cchild(cx, "property");
      }
      m_members.resize(OX::countChildren(m_lx, "member"));
      OC::Launcher::Member *i = &m_members[0];
      for (ezxml_t ix = ezxml_cchild(m_lx, "member"); ix; ix = ezxml_next(ix), i++) {
	std::string inst, impl;
	size_t artN, contN, slave, crewN;
	bool slaveFound;
	const char *err;
	if ((err = OX::getRequiredString(ix, i->m_name, "name")) ||
	    (err = OX::getRequiredString(ix, impl, "worker")) ||
	    (err = OX::getBoolean(ix, "done", &i->m_doneInstance)) ||
	    (err = OX::getNumber(ix, "slave", &slave, &slaveFound)) ||
	    (err = OX::getNumber(ix, "container", &contN, NULL, 0, false, true)) ||
	    (err = OX::getNumber(ix, "artifact", &artN, NULL, 0, false, true)) ||
	    (err = OX::getNumber(ix, "crew", &crewN, NULL, 0, false, true)) ||
	    (err = OX::getNumber(ix, "member", &i->m_member, NULL, 0, false, false))) {
	  error = err;
	  return true;
	}
	OX::getOptionalString(ix, inst, "static");
	assert(contN < m_containers.size() && m_containers[contN]);
	assert(artN < m_artifacts.size() && m_artifacts[artN]);
	i->m_container = m_containers[contN];
	i->m_containerApp = m_containerApps[contN];
	OL::Artifact &art = *m_artifacts[artN];
	// FIXME: this could easily be indexed if OL::Artifact had an impl index
	if (!(i->m_impl = art.findImplementation(impl.c_str(),
						inst.length() ? inst.c_str() : NULL))) {
	  OU::format(error, "Can't find worker \"%s\"%s%s in artifact \"%s\"",
		     impl.c_str(), inst.length() ? " with instance " : "",
		     inst.length() ? inst.c_str() : "", art.name().c_str());
	}
	if (slaveFound) {
	  assert(slave < m_members.size());
	  i->m_slave = &m_members[slave];
	  i->m_slave->m_hasMaster = true;
	}
	i->m_crew = &m_crews[crewN];
	if (crewsXml[crewN]) {
	  // we are the first seen member of the crew - we can parse the property values since we
	  // know the impl now
	  unsigned *u = &i->m_crew->m_propOrdinals[0];
	  OU::Value *v = &i->m_crew->m_propValues[0];
	  for (ezxml_t px = crewsXml[crewN]; px; px = ezxml_next(px), u++, v++) {
	    size_t ord;
	    if ((err = OX::getNumber(px, "n", &ord)))
	      OU::eformat(error, "Error processing instances for launch: %s", err);
	    *u = OCPI_UTRUNCATE(unsigned, ord);
	    const char *val = ezxml_cattr(px, "v");
	    assert(ord <= i->m_impl->m_metadataImpl.nProperties());
	    OU::Property &p = i->m_impl->m_metadataImpl.properties()[ord];
	    assert(!p.m_isParameter);
	    v->setType(p);
	    if ((err = v->parse(val)))
	      return OU::eformat(error, "Error processing property values for launch: %s", err);
	  }
	  crewsXml[crewN] = NULL;
	}
      }
      m_connections.resize(OX::countChildren(m_lx, "connection"));
      OC::Launcher::Connection *c = &m_connections[0];
      for (ezxml_t cx = ezxml_cchild(m_lx, "connection"); cx; cx = ezxml_next(cx), c++)
	if (doConnection(cx, *c, error))
	  return true;
      m_response
	 = m_local->launch(m_members, m_connections) ? "<launching>" : "<launching done='1'>";
      // We know that the only thing that can happen at launch time is to
      // get initial provider info from input ports
      c = &m_connections[0];
      for (unsigned n = 0; n < m_connections.size(); n++, c++)
	if (c->m_in.m_launcher == m_local && c->m_out.m_launcher != m_local) {
	  OU::formatAdd(m_response, "  <connection id='%u' ipi='", n);
	  Launcher::encodeDescriptor(c->m_in.m_initial, m_response);
	  m_response += "'/>\n";
	}
      return Launcher::sendXml(fd(), m_response, "responding from server after initial launch", error);
    }

    bool Server::
    launch(std::string &error) {
      assert(!m_downloading);
      m_lx = m_rx; // save for using later after download
      m_rx = NULL;
      m_launchBuf.swap(m_buf);
      m_response = "<launching>\n";
      m_artifacts.resize(OX::countChildren(m_lx, "artifact"), NULL); 
      size_t n = 0;
      for (ezxml_t ax = ezxml_cchild(m_lx, "artifact"); ax; ax = ezxml_next(ax), n++) {
	const char *uuid = ezxml_cattr(ax, "uuid");
	assert(uuid);
	if (!(m_artifacts[n] = m_library.findArtifact(uuid))) {
	  // We need to request the artifact to be downloaded.
	  m_downloading = true;
	  OU::formatAdd(m_response, "  <artifact id='%zu'/>\n", n);
	}
      }
      if (m_downloading) {
	// Send response to initial launch that is only a request for downloading artifacts.
	// And remember that we downloaded so that the next request does instances.
	m_downloaded = true;
	return Launcher::sendXml(fd(), m_response, "responding from server", error);
      }
      return doLaunch(error);
    }
    // After initial launch, and after any downloading
    bool Server::
    update(std::string &error) {
      // 1. If we were downloading, then this "update" is just doing the real launch
      ocpiDebug("Launch update request.  %s", m_downloaded ? "We downloaded" : "We had no downloading to do");
      if (m_downloaded) {
	m_downloaded = true;
	return doLaunch(error);
      }
      // 2. We take any connection updates from the wire, and prepare then
      //    for the local launcher to chew on
      ocpiDebug("Launch downloads complete.  Processing Connections");
      for (ezxml_t cx = ezxml_cchild(m_rx, "connection"); cx; cx = ezxml_next(cx)) {
	const char *err;
	size_t n;
	if ((err = OX::getNumber(cx, "id", &n, NULL, 0, false, true)) ||
	    n >= m_connections.size())
	  return OU::eformat(error, "Bad connection id: %s", err);
	OC::Launcher::Connection &c = m_connections[n];
	const char *info;
	if (c.m_out.m_launcher) {
	  if ((info = ezxml_cattr(cx, "ipi")))
	    Launcher::decodeDescriptor(info, c.m_in.m_initial);
	  else if ((info = ezxml_cattr(cx, "fpi")))
	    Launcher::decodeDescriptor(info, c.m_in.m_final);
	} else if (c.m_in.m_launcher) {
	  if ((info = ezxml_cattr(cx, "iui")))
	    Launcher::decodeDescriptor(info, c.m_out.m_initial);
	  else if ((info = ezxml_cattr(cx, "fui")))
	    Launcher::decodeDescriptor(info, c.m_out.m_final);
	}
      }
      // 3. Give the local launcher a chance to deal with connection info and produce mode
      ocpiDebug("Connections processed.  Entering local launcher work function.");
      m_response =
	m_local->work(m_members, m_connections) ? "<launching>" : "<launching done='1'>";
      // 4. Take whatever the local launcher produced, and send it back
      ocpiDebug("Local launcher returned.  m_response is: %s", m_response.c_str());
      OC::Launcher::Connection *c = &m_connections[0];
      for (unsigned n = 0; n < m_connections.size(); n++, c++)
	if (c->m_in.m_launcher) {
	  // local input remote output
	  if (c->m_in.m_final.length()) {
	    OU::formatAdd(m_response, "  <connection id='%u' fpi='", n);
	    Launcher::encodeDescriptor(c->m_in.m_final, m_response);
	    m_response += "'/>\n";
	    c->m_in.m_final.clear();
	  }
	} else if (c->m_out.m_launcher) {
	  // local input remote output
	  if (c->m_out.m_initial.length()) {
	    OU::formatAdd(m_response, "  <connection id='%u' iui='", n);
	    Launcher::encodeDescriptor(c->m_out.m_initial, m_response);
	    m_response += "'/>\n";
	    c->m_out.m_initial.clear();
	  } else if (c->m_out.m_final.length()) {
	    OU::formatAdd(m_response, "  <connection id='%u' fui='", n);
	    Launcher::encodeDescriptor(c->m_out.m_final, m_response);
	    m_response += "'/>\n";
	    c->m_out.m_final.clear();
	  }
	}
      ocpiDebug("Response prepared.  m_response is: %s", m_response.c_str());
      return Launcher::sendXml(fd(), m_response, "responding from server", error);
    }
    bool Server::
    control(std::string &error) {
      const char *err;
      size_t inst, n;
      bool get, set, op, wait, hex, getState = ezxml_cattr(m_rx, "getstate") != NULL;
      
      if ((err = OX::getNumber(m_rx, "id",   &inst, NULL, 0, false, true)) ||
	  (err = OX::getNumber(m_rx, "get",  &n,    &get, 0, false)) ||
	  (err = OX::getNumber(m_rx, "set",  &n,    &set, 0, false)) ||
	  (err = OX::getNumber(m_rx, "op",   &n,    &op,  0, false)) ||
	  (err = OX::getNumber(m_rx, "wait", &n,    &wait,  0, false)) ||
	  (err = OX::getBoolean(m_rx, "hex", &hex)) ||
	  inst >= m_members.size() || !m_members[inst].m_worker ||
	  (get || set) && n >= m_members[inst].m_worker->nProperties())
	return OU::eformat(error, "Control message error: %s", err);
      m_response = "<control>";
      OC::Worker &w = *m_members[inst].m_worker;

      try {
	if (get || set) {
	  OU::Property &p = w.properties()[n];
	  if (get)
	    w.getPropertyValue(p, m_response, hex, true);
	  else
	    w.setPropertyValue(p, m_response);
	} else if (op) {

	  printf("******* &&&&&& Got a control op !!, op = %d\n", op );
	  w.controlOp((OU::Worker::ControlOperation)n);
	}
	else if (wait)
	  if (n) {
	    OS::Timer t(OCPI_UTRUNCATE(uint32_t, n), 0);
	    if (w.wait(&t))
	      m_response = "<control timeout='1'>";
	  } else
	    w.wait();
	else if (getState)
	  OU::format(m_response, "<control state='%u'>", w.getControlState());
	else
	  throw OU::Error("Illegal remote control operation");
      } catch (const std::string &e) {
	error = e;
	return true;
      } catch (...) {
	error = "Unknown Exception";
	return true;
      }
      return Launcher::sendXml(fd(), m_response, "responding from server", error);
    }
    bool Server::
    discover(std::string &error) {
      // Use a buffer the same size as a datagram payload.
      size_t length = OE::MaxPacketSize;
      char buf[OE::MaxPacketSize];
      if (fillDiscoveryInfo(buf, length, error))
	return true;
      OU::format(m_response, "<discovery>\n%s", buf);
      return Launcher::sendXml(fd(), m_response, "responding with discovery from server",
			       error);
    }
    bool Server::
    fillDiscoveryInfo(char *cp, size_t &length, std::string &error) {
      OA::Container *ac;
      *cp = '\0'; // in case there are NO containers
      for (unsigned n = 0; (ac = OA::ContainerManager::get(n)); n++) {
	OC::Container &c = *static_cast<OC::Container *>(ac);
	std::string info;
	OU::format(info, "%s|%s|%s|%s|%s|", c.name().c_str(), c.model().c_str(), c.os().c_str(),
		   c.osVersion().c_str(), c.platform().c_str());
	for (unsigned n = 0;  n < c.transports().size(); n++) {
	  const OC::Transport &t = c.transports()[n];
	  OU::formatAdd(info, "%s,%s,%u,%u,0x%x,0x%x|",
			t.transport.c_str(), t.id.c_str(), t.roleIn, t.roleOut, t.optionsIn,
			t.optionsOut);
	}
	info += '\n';
	if (info.length() >= length) {
	  OU::format(error, "Too many containers, discovery buffer would overflow");
	  return true;
	}
	strcpy(cp, info.c_str());
	cp += info.length();
	length -= info.length();
      }
      length--; // account for the null char of the last line
      return false;
    }
  }
}
