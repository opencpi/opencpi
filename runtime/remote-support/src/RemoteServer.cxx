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

// This file is in this directory because it is not part of the remote container
// driver, which is an optional driver for accessing remote containers.
// This class is used by ocpiserve_main.cxx, which is in this directory
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
#include "XferManager.h"

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
    Server(OL::Library &l, OS::ServerSocket &svrSock, std::string &discoveryInfo,
	   std::vector<bool> &needsBridging, std::string &error) :
      OU::Client(svrSock, error),
      m_library(l), m_downloading(false), m_downloaded(false), m_rx(NULL),
      m_lx(NULL), m_local(NULL), m_discoveryInfo(discoveryInfo), m_needsBridging(needsBridging) {
    }
    // Used both in the destructor and in appShutDown
    void Server::
    clear() {
      ezxml_free(m_rx);
      ezxml_free(m_lx);
      for (unsigned n = 0; n < m_containerApps.size(); n++)
	delete m_containerApps[n];
    }
    Server::
    ~Server() {
      clear();
      OC::Manager::cleanForContext(this);
    }

    bool Server::
    receive(bool &eof, std::string &error) {
      DataTransfer::XferManager::getFactoryManager().setEndPointContext(this);
      if (m_downloading)
	return download(error);
      if (OX::receiveXml(fd(), m_rx, m_buf, eof, error))
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
      else if (!strcasecmp(tag, "appshutdown"))
	return appShutDown(error);
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
	length -= (size_t)nr;
	ssize_t nw;
	for (char *cp = &m_downloadBuf[0]; nr; nr -= nw, cp += nw)
	  if ((nw = ::write(wfd, cp, (size_t)nr)) <= 0)
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
      for (ezxml_t ax = ezxml_child(m_lx, "artifact"); ax; ax = ezxml_cnext(ax), n++)
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
	  OU::format(fileName, "%s/%s=%s",
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
      // note: this info is in the in/out element on launch, but in the connection element for
      // updates
      const char *info = ezxml_cattr(px, "ipi");
      if (!info)
	info = ezxml_cattr(px, "iui");
      if (info)
	OU::decodeDescriptor(info, p.m_initial);
      ezxml_t x;
      if ((x = ezxml_cchild(px, "params")) && (err = p.m_params.addXml(x)))
	return err;
      if ((x = ezxml_cchild(px, "port"))) {
	OU::Port *mp = new OU::Port(x);
	if ((err = mp->parse()) ||
	    (err = mp->postParse()))
	  return err;
	p.m_metaPort = mp;
      }
      return NULL;
    }
    // Second pass after both sides are parsed
    const char *Server::
    doSide2(OC::Launcher::Port &p, OC::Launcher::Port &other) {
      // Force a bridge port if this container needs transport bridging
      // and the connection is between local and remote
      if (p.m_member && m_needsBridging[p.m_member->m_container->ordinal()] && !other.m_member) {
	// We have a remote connection and we need transport bridging
	// The transport specified in the connection will be the bridged transport to talk
	// remotely.  But we need the local port to use its native transport.
	p.m_transportBridged = true;
	if (other.m_scale == 0)
	  other.m_scale = 1; // force bridging even if its 1-to-1
      }
      return NULL;
    }

    static void
    updateConnection(OC::Launcher::Connection &c, ezxml_t cx) {
      const char *info;
      if (c.m_out.m_launcher) {
	if ((info = ezxml_cattr(cx, "ipi")))
	  OU::decodeDescriptor(info, c.m_in.m_initial);
	else if ((info = ezxml_cattr(cx, "fpi")))
	  OU::decodeDescriptor(info, c.m_in.m_final);
      } else if (c.m_in.m_launcher) {
	if ((info = ezxml_cattr(cx, "iui")))
	  OU::decodeDescriptor(info, c.m_out.m_initial);
	else if ((info = ezxml_cattr(cx, "fui")))
	  OU::decodeDescriptor(info, c.m_out.m_final);
      }
    }

    bool Server::
    doConnection(ezxml_t cx, OC::Launcher::Connection &c, std::string &error) {
      c.m_transport.transport = ezxml_cattr(cx, "transport");
      c.m_transport.id = ezxml_cattr(cx, "id");
      const char *err;
      size_t roleIn, roleOut, optionsIn, optionsOut;

      if ((err = OX::getNumber(cx, "roleIn", &roleIn, NULL, 0)) ||
	  (err = OX::getNumber(cx, "roleOut", &roleOut, NULL, 0)) ||
	  (err = OX::getNumber(cx, "optionsIn", &optionsIn, NULL, 0)) ||
	  (err = OX::getNumber(cx, "optionsOut", &optionsOut, NULL, 0)) ||
	  (err = OX::getNumber(cx, "bufferSize", &c.m_bufferSize, NULL, 0)) ||
	  (err = doSide(cx, c.m_in, "in")) ||
	  (err = doSide(cx, c.m_out, "out")) ||
	  (err = doSide2(c.m_in, c.m_out)) ||
	  (err = doSide2(c.m_out, c.m_in)))
	return OU::eformat(error, "Error processing connection values for launch: %s", err);
      c.m_transport.roleIn = OCPI_UTRUNCATE(OR::PortRole, roleIn);
      c.m_transport.roleOut = OCPI_UTRUNCATE(OR::PortRole, roleOut);
      c.m_transport.optionsIn = OCPI_UTRUNCATE(uint32_t, optionsIn);
      c.m_transport.optionsOut = OCPI_UTRUNCATE(uint32_t, optionsOut);
      updateConnection(c, cx);
#if 0
      ezxml_t px;
      if ((px = ezxml_cchild(cx, "paramsin")))
	c.m_in.m_params.addXml(px);
      if ((px = ezxml_cchild(cx, "paramsout")))
	c.m_out.m_params.addXml(px);
#endif
      return false;
    }

    // Do the real launch after artifacts are available
    // This might happen in "launch" if all the artifacts are already present.
    bool Server::
    doLaunch(std::string &error) {

      m_containers.resize(OX::countChildren(m_lx, "container"), 0);
      m_containerApps.resize(m_containers.size(), 0);

      unsigned n = 0;
      for (ezxml_t cx = ezxml_cchild(m_lx, "container"); cx; cx = ezxml_cnext(cx), n++) {
	const char *name = ezxml_cattr(cx, "name");
	assert(name);
	assert(!m_containers[n]);
	ocpiInfo("Server using container %s for client", name);
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
      for (ezxml_t cx = ezxml_cchild(m_lx, "crew"); cx; cx = ezxml_cnext(cx), cr++, cxp++) {
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
      for (ezxml_t ix = ezxml_cchild(m_lx, "member"); ix; ix = ezxml_cnext(ix), i++) {
	std::string inst, impl, slaves;
	size_t artN, contN, crewN;
	const char *err;
	if ((err = OX::getRequiredString(ix, i->m_name, "name")) ||
	    (err = OX::getRequiredString(ix, impl, "worker")) ||
	    (err = OX::getBoolean(ix, "done", &i->m_doneInstance)) ||
	    (err = OX::getNumber(ix, "container", &contN, NULL, 0, false, true)) ||
	    (err = OX::getNumber(ix, "artifact", &artN, NULL, 0, false, true)) ||
	    (err = OX::getNumber(ix, "crew", &crewN, NULL, 0, false, true)) ||
	    (err = OX::getNumber(ix, "member", &i->m_member, NULL, 0, false, false))) {
	  error = err;
	  return true;
	}
	OX::getOptionalString(ix, inst, "static");
	OX::getOptionalString(ix, slaves, "slaves");
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
	if (slaves.length()) {
	  for (OU::TokenIter li(slaves.c_str()); li.token(); li.next()) {
	    size_t slave;
	    ocpiCheck(sscanf(li.token(), "%zu", &slave) == 1);
	    assert(slave < m_members.size());
	    i->m_slaves.push_back(&m_members[n]);
	    i->m_slaves.back()->m_hasMaster = true;
	  }
	  i->m_slaveWorkers.resize(i->m_slaves.size());
	}
	i->m_crew = &m_crews[crewN];
	if (crewsXml[crewN]) {
	  // we are the first seen member of the crew - we can parse the property values since we
	  // know the impl now
	  unsigned *u = &i->m_crew->m_propOrdinals[0];
	  OU::Value *v = &i->m_crew->m_propValues[0];
	  for (ezxml_t px = crewsXml[crewN]; px; px = ezxml_cnext(px), u++, v++) {
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
	    *u = OCPI_UTRUNCATE(unsigned, ord);
	  }
	  crewsXml[crewN] = NULL;
	}
      }
      m_connections.resize(OX::countChildren(m_lx, "connection"));
      OC::Launcher::Connection *c = &m_connections[0];
      for (ezxml_t cx = ezxml_cchild(m_lx, "connection"); cx; cx = ezxml_cnext(cx), c++)
	if (doConnection(cx, *c, error))
	  return true;
      m_response
	 = m_local->launch(m_members, m_connections) ? "<launching>" : "<launching done='1'>";
      // Whether we are done or not, we need to send any initial connection info to the other side.
      c = &m_connections[0];
      for (unsigned nn = 0; nn < m_connections.size(); nn++, c++) {
	if (c->m_in.m_launcher == m_local && c->m_in.m_initial.length()) {
	  OU::formatAdd(m_response, "  <connection id='%u'", nn);
	  OU::encodeDescriptor("ipi", c->m_in.m_initial, m_response);
	  m_response += "/>\n";
	  c->m_in.m_initial.clear();
	}
	// Output ports may have initial info if they received IPI in this launch.
	if (c->m_out.m_launcher == m_local && c->m_out.m_initial.length()) {
	  OU::formatAdd(m_response, "  <connection id='%u'", nn);
	  OU::encodeDescriptor("iui", c->m_out.m_initial, m_response);
	  m_response += "/>\n";
	  c->m_out.m_initial.clear();
	}
      }
      return OX::sendXml(fd(), m_response, "responding from server after initial launch", error);
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
      for (ezxml_t ax = ezxml_cchild(m_lx, "artifact"); ax; ax = ezxml_cnext(ax), n++) {
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
	return OX::sendXml(fd(), m_response, "responding from server", error);
      }
      return doLaunch(error);
    }
    // After initial launch, and after any downloading
    bool Server::
    update(std::string &error) {
      // 1. If we were downloading, then this "update" is just doing the real launch
      ocpiDebug("Launch update request.  %s", m_downloaded ? "We downloaded" : "We had no downloading to do");
      if (m_downloaded) {
	m_downloaded = false;
	return doLaunch(error);
      }
      // 2. We take any connection updates from the wire, and prepare then
      //    for the local launcher to chew on
      ocpiDebug("Launch downloads complete.  Processing Connections");
      for (ezxml_t cx = ezxml_cchild(m_rx, "connection"); cx; cx = ezxml_cnext(cx)) {
	const char *err;
	size_t n;
	if ((err = OX::getNumber(cx, "id", &n, NULL, 0, false, true)) ||
	    n >= m_connections.size())
	  return OU::eformat(error, "Bad connection id: %s", err);
	updateConnection(m_connections[n], cx);
      }
      // 3. Give the local launcher a chance to deal with connection info and produce more
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
	    OU::formatAdd(m_response, "  <connection id='%u'", n);
	    OU::encodeDescriptor("fpi", c->m_in.m_final, m_response);
	    m_response += "/>\n";
	    c->m_in.m_final.clear();
	  }
	} else if (c->m_out.m_launcher) {
	  // local input remote output
	  if (c->m_out.m_initial.length()) {
	    OU::formatAdd(m_response, "  <connection id='%u'", n);
	    OU::encodeDescriptor("iui", c->m_out.m_initial, m_response);
	    m_response += "/>\n";
	    c->m_out.m_initial.clear();
	  } else if (c->m_out.m_final.length()) {
	    OU::formatAdd(m_response, "  <connection id='%u'", n);
	    OU::encodeDescriptor("fui", c->m_out.m_final, m_response);
	    m_response += "/>\n";
	    c->m_out.m_final.clear();
	  }
	}
      ocpiDebug("Response prepared.  m_response is: %s", m_response.c_str());
      return OX::sendXml(fd(), m_response, "responding from server", error);
    }
    bool Server::
    control(std::string &error) {
      const char *err;
      size_t inst, n;
      bool get, set, op, wait, hex, unreadableOK, getState = ezxml_cattr(m_rx, "getstate") != NULL;

      if ((err = OX::getNumber(m_rx, "id",   &inst, NULL, 0, false, true)) ||
	  (err = OX::getNumber(m_rx, "get",  &n,    &get, 0, false)) ||
	  (err = OX::getNumber(m_rx, "set",  &n,    &set, 0, false)) ||
	  (err = OX::getNumber(m_rx, "op",   &n,    &op,  0, false)) ||
	  (err = OX::getNumber(m_rx, "wait", &n,    &wait,  0, false)) ||
	  (err = OX::getBoolean(m_rx, "hex", &hex)) ||
	  (err = OX::getBoolean(m_rx, "unreadable_ok", &unreadableOK)) ||
	  inst >= m_members.size() || !m_members[inst].m_worker ||
	  ((get || set) && n >= m_members[inst].m_worker->nProperties()))
	return OU::eformat(error, "Control message error: %s", err);
      m_response = "<control>";
      OC::Worker &w = *m_members[inst].m_worker;
      try {
	if (get || set) {
	  OU::Property &p = w.properties()[n];
	  size_t offset, dimension;
	  if ((err = OX::getNumber(m_rx, "offset", &offset)) ||
	      (err = OX::getNumber(m_rx, "dimension", &dimension)))
	    return OU::eformat(error, "Get/set property control message error: %s", err);
	  OU::Member *m = &p;
	  for (const char *path = ezxml_cattr(m_rx, "path");
	       path && path[0] && path[1]; path += 2) {
	    size_t ordinal;
	    ocpiCheck(sscanf(path, "%2zx", &ordinal) == 1);
	    if (m->m_baseType == OA::OCPI_Struct) {
	      if (ordinal >= m->m_nMembers)
		return OU::eformat(error, "Get/set struct member index error");
	      m = &m->m_members[ordinal];
	    } else if (m->m_baseType == OA::OCPI_Type) {
	      if (ordinal != 0)
		return OU::eformat(error, "Get/set typedef index error");
	      m = m->m_type;
	    }
	  }
	  if (get) {
	    OA::PropertyAttributes a;

	    w.getProperty(p, m_response, *m, offset, dimension,
			  OA::PropertyOptionList({ hex ? OA::HEX : OA::NONE, OA::APPEND,
				                   unreadableOK ? OA::UNREADABLE_OK : OA::NONE}), &a);
	  } else
	    w.setProperty(p, ezxml_txt(m_rx), *m, offset, dimension);
	} else if (op)
	  w.controlOp((OU::Worker::ControlOperation)n);
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
      return OX::sendXml(fd(), m_response, "responding from server", error);
    }
    // Clear out all the state and resources so that this server can be (serially) reused for another app.
    bool Server::
    appShutDown(std::string &error) {
      // Release resources like/in-common-with the destructor
      clear();
      // Clear state not relevant to destruction
      m_downloading = m_downloaded = false;
      m_rx = m_lx = NULL;
      m_launchBuf.clear();
      m_artifacts.clear();
      m_containers.clear();
      m_containerApps.clear();
      m_crews.clear();
      m_members.clear();
      m_connections.clear();
      OU::format(m_response, "<appshutdown>\n");
      return OX::sendXml(fd(), m_response, "responding with discovery from server", error);
    }
    bool Server::
    discover(std::string &error) {
      OU::format(m_response, "<discovery>\n%s", m_discoveryInfo.c_str());
      return OX::sendXml(fd(), m_response, "responding with discovery from server", error);
    }
#if 0
    bool Server:: // static
    fillDiscoveryInfo(char *cp, size_t &length, std::string &error) {
      OA::Container *ac;
      *cp = '\0'; // in case there are NO containers
      for (unsigned n = 0; (ac = OA::ContainerManager::get(n)); n++) {
	OC::Container &c = *static_cast<OC::Container *>(ac);
	std::string info;
	OU::format(info, "%s|%s|%s|%s|%s|%s|%c|", c.name().c_str(), c.model().c_str(), c.os().c_str(),
		   c.osVersion().c_str(), c.arch().c_str(), c.platform().c_str(),
		   c.dynamic() ? '1' : '0');
	bool hasSockets = false;
	for (unsigned nn = 0;  nn < c.transports().size(); nn++) {
	  const OC::Transport &t = c.transports()[nn];
	  OU::formatAdd(info, "%s,%s,%u,%u,0x%x,0x%x|",
			t.transport.c_str(), t.id.c_str()[0] ? t.id.c_str() : " ", t.roleIn,
			t.roleOut, t.optionsIn, t.optionsOut);
	  if (t.transport == "ocpi-socket-rdma")
	    hasSockets = true;
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
#endif
  }
}
