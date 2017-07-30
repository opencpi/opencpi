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
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <cstring>
#include <set>
#include "OcpiOsSocket.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilMisc.h"
#include "Container.h"
#include "ContainerApplication.h"
#include "RemoteLauncher.h"
#include "RemoteServer.h"
namespace OA = OCPI::API;
namespace OC = OCPI::Container;
namespace OX = OCPI::Util::EzXml;
namespace OL = OCPI::Library;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OCPI {
  namespace Remote {

Launcher::
Launcher(OS::Socket &socket)
  : m_fd(socket.fd()), m_sending(false), m_rx(NULL) {
}

Launcher::
~Launcher() {
  ezxml_free(m_rx);
}


void Launcher::
receive() {
  std::string error;
  bool eof;
  if (OX::receiveXml(m_fd, m_rx, m_response, eof, error))
    throw OU::Error("error reading from container server \"%s\": %s",
		    m_name.c_str(), error.c_str());
}

void Launcher::
send() {
  std::string error;
  if (OX::sendXml(m_fd, m_request, "container server", error))
    throw OU::Error("%s", error.c_str());
  m_request.clear();
  m_sending = false;
}


void Launcher::
emitContainer(const OCPI::Container::Container &cont) {
  const char *colon = strchr(cont.name().c_str(), ':');
  assert(colon);
  colon = strchr(colon+1, ':');
  assert(colon);
  OU::formatAdd(m_request, "  <container name='%s'/>\n", colon + 1);
}
// Tell the server that this artifact is needed
// The server will ask for it if it doesn't already have it.
// The name and mtime are fluff - the uuid and length are critical
void Launcher::
emitArtifact(const OCPI::Library::Artifact &art) {
  OU::formatAdd(m_request,
		"  <artifact name='%s' mtime='%" PRIu32 "' uuid='%s' length='%" PRIu64 "'/>\n",
		art.name().c_str(), OCPI_UTRUNCATE(uint32_t,art.mtime()), art.uuid().c_str(),
		art.length());
}
// FIXME: someday allow for instance parameters?
void Launcher::
emitInstance(const char *a_name, unsigned contN, unsigned artN, const Launcher::Instance &i,
	     int slave) {
  OU::formatAdd(m_request,
		"  <instance name='%s' container='%u' artifact='%u' worker='%s'",
		a_name, contN, artN, i.m_impl->m_metadataImpl.specName().c_str());
  if (i.m_impl->m_staticInstance)
    OU::formatAdd(m_request, " static='%s'", ezxml_cattr(i.m_impl->m_staticInstance, "name"));
  if (slave >= 0)
    OU::formatAdd(m_request, " slave='%u'", slave);
  if (i.m_doneInstance)
    m_request += " done='1'";
  if (i.m_propValues.size()) {
    m_request += ">\n";
    const OU::Value *vals = &i.m_propValues[0];
    for (unsigned n = 0; n < i.m_propValues.size(); n++, vals++) {
      OU::formatAdd(m_request,
		    "    <property n='%u' v='", i.m_propOrdinals[n]);
      vals->unparse(m_request, NULL, true); // FIXME: someday an XML unparser for perfect quoting
      m_request += "'/>\n";
    }
    m_request += "  </instance>\n";
  } else
    m_request += "/>\n";
}
void Launcher::
emitPort(const Launcher::Instance &i, const char *port, const OA::PValue *params,
	 const char *which) {
  OU::formatAdd(m_request, "  <%s instance='%p' port='%s'", which, &i, port);
  if (params) {
    while (params->name) {
      OU::formatAdd(m_request, "    <parameter name='%s' value='", params->name);
      params->unparse(m_request, true);
      m_request += "'/>\n";
    }
    OU::formatAdd(m_request, "  </%s>\n", which);
  } else
    m_request += "/>\n";
}
static void 
unparseParams(const OU::PValue *params, std::string &out) {
  for (;params && params->name; params++) {
    OU::formatAdd(out, " %s='", params->name);
    params->unparse(out, true);
    out += '\'';
  }
}
// This connection is one of ours, either internal or external
void Launcher::
emitConnection(const Launcher::Instances &instances, const Launcher::Connection &c) { 
  OU::formatAdd(m_request, "  <connection");
  if (c.m_launchIn == this)
    OU::formatAdd(m_request, " instIn='%u' nameIn='%s'",
		  m_instanceMap[c.m_instIn - &instances[0]], c.m_nameIn);
  if (c.m_launchOut == this)
    OU::formatAdd(m_request, " instOut='%u' nameOut='%s'",
		  m_instanceMap[c.m_instOut - &instances[0]], c.m_nameOut);
  if (c.m_url)
    OU::formatAdd(m_request, "  url='%s'", c.m_url);
  else {
    // put out names for external ports
    if (!c.m_instIn && c.m_nameIn)
      OU::formatAdd(m_request, "  nameIn='%s'", c.m_nameIn);
    if (!c.m_instOut && c.m_nameOut)
      OU::formatAdd(m_request, "  nameOut='%s'", c.m_nameOut);
  }
  if (c.m_paramsIn.list() || c.m_paramsOut.list()) {
    m_request +=">\n";
    if (c.m_paramsIn.list()) {
      m_request += "    <paramsin";
      unparseParams(c.m_paramsIn, m_request);
      m_request += "/>\n";
    }
    if (c.m_paramsOut.list()) {
      m_request += "    <paramsout";
      unparseParams(c.m_paramsOut, m_request);
      m_request += "/>\n";
    }
    m_request += "  </connection>\n";
  } else
    m_request += "/>\n";
}
void Launcher::
emitConnectionUpdate(unsigned connN, const char *iname, std::string &sinfo) {
  if (m_request.empty())
    m_request = "<update>\n";
  OU::formatAdd(m_request, "  <connection id='%u' %s=\"",
		m_connectionMap[connN], iname);
  // This encoding escapes double quotes and allows for embedded null characters
  OU::encodeDescriptor(sinfo, m_request);
  m_request += "\"/>\n";
  sinfo.clear();
}
void Launcher::
loadArtifact(ezxml_t ax) {
  // This is called when receiving a message that is requesting artifact downloads.
  // We simply send the data in band one after the other.
  size_t n;
  bool found;
  const char *err = OX::getNumber(ax, "id", &n, &found);
  if (err || !found || n >= m_artifacts.size())
    throw OU::Error("Bad artifact load request from container server: %s (%u)",
		    err ? err : "", found);
  const OL::Artifact &l = *m_artifacts[n];
  int rfd = open(l.name().c_str(), O_RDONLY);
  if (rfd < 0)
    throw OU::Error("Can't open artifact file \"%s\" for container server: %s (%d)",
		    l.name().c_str(), strerror(errno), errno);
  // FIXME: use locking to prevent it from changing...
  struct stat st;
  if (fstat(rfd, &st) || st.st_mtime != l.mtime() || (uint64_t)st.st_size != l.length())
    throw OU::Error("Artifact \"%s\" has changed since this application was started.",
		    l.name().c_str());
  char buf[64*1024];
  ssize_t nr, nw;
  uint64_t length = l.length();
  while ((nr = read(rfd, buf, sizeof(buf))) > 0) {
    while (nr) {
      if ((size_t)nr > length)
	throw OU::Error("Artifact \"%s\" has grown in size during download?",
			l.name().c_str());
      if ((nw = write(m_fd, buf, nr)) <= 0)
	throw OU::Error("Error sending artifact file \"%s\" to container server: %s (%d)",
			l.name().c_str(), strerror(errno), errno);
      nr -= nw;
      length -= nw;
    }
  }
  if (nr < 0)
    throw OU::Error("Error reading artifact file \"%s\" for container server: %s (%d)",
		    l.name().c_str(), strerror(errno), errno);
}

void Launcher::
updateConnection(ezxml_t cx) {
  size_t n;
  bool found;
  const char *err = OX::getNumber(cx, "id", &n, &found);
  if (err || !found || n >= m_connections.size())
    throw OU::Error("Bad connection update from container server: %s (%u)",
		    err ? err : "", found);
  OC::Launcher::Connection &c = *m_connections[n];
  // We should only get this for connections that we are one side of.
  const char *info;
  if (c.m_launchIn == this) {
    // The remote is an input, so we should be receiving provider info in the update
    if ((info = ezxml_cattr(cx, "ipi")))
      OU::decodeDescriptor(info, c.m_ipi);
    else if ((info = ezxml_cattr(cx, "fpi")))
      OU::decodeDescriptor(info, c.m_fpi);
  } else if (c.m_launchOut == this) {
    // The remote is an input, so we should be receiving user info in the update
    if ((info = ezxml_cattr(cx, "iui")))
      OU::decodeDescriptor(info, c.m_iui);
    else if ((info = ezxml_cattr(cx, "fui")))
      OU::decodeDescriptor(info, c.m_fui);
  }
}

bool Launcher::
launch(Launcher::Instances &instances, Launcher::Connections &connections) {
  // Initialize various maps - they will be truncated later
  m_connections.resize(connections.size());
  m_artifacts.resize(instances.size());
  m_instanceMap.resize(instances.size());
  m_connectionMap.resize(connections.size());
  // A temporary map to keep track of which containers we have emitted
  typedef std::map<OC::Container*, unsigned> Containers;
  typedef Containers::const_iterator ContainersIter;
  Containers containers;
  unsigned nContainers = 0;

  // A temporary map to keep track of which artifacts we have emitted
  typedef std::map<OL::Artifact*, unsigned> Artifacts;
  //  typedef Artifacts::value_type ArtifactsPair;
  typedef Artifacts::const_iterator ArtifactsIter;
  Artifacts artifacts;
  unsigned nArtifacts = 0;

 // Create map from global instance num to remote instance num
  Launcher::Instance *i = &instances[0];
  unsigned nRemote = 0;
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this)
      m_instanceMap[n] = nRemote++;
  // Loop for all instances, emiting instances, artifacts and containers as we see them
  m_request = "<launch>\n";
  i = &instances[0];
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this) {
      unsigned contN;
      ContainersIter ci = containers.find(i->m_container);
      if (ci == containers.end()) {
	emitContainer(*i->m_container);
	containers[i->m_container] = contN = nContainers++;
      } else
	contN = ci->second;
      OL::Artifact &a = i->m_impl->m_artifact;
      unsigned artN;
      ArtifactsIter arti = artifacts.find(&a);
      if (arti == artifacts.end()) {
	emitArtifact(a);    // name, mtime, uuid, length, ptr
	artifacts[&a] = artN = nArtifacts++;
	m_artifacts[artN] = &a;
      } else
	artN = arti->second;
      emitInstance(i->m_name.c_str(), contN, artN, *i,
		   i->m_slave && &i->m_slave->m_container->launcher() == this ?
		   (int)(m_instanceMap[i->m_slave - &instances[0]]) : -1);
    }
  m_artifacts.resize(nArtifacts);
  unsigned nConnections = 0;
  Launcher::Connection *c = &connections[0];
  for (unsigned n = 0; n < connections.size(); n++, c++) {
    c->prepare(); // make sure transport parameters are on both sides
    if (c->m_launchIn == this || c->m_launchOut == this) {
      if (c->m_launchIn != c->m_launchOut) {
	// If the connection is off of the remote server and no
	// transport is specified, force sockets on the input side
	const char *endpoint = NULL, *transport = NULL;
	if (!OU::findString(c->m_paramsIn, "endpoint", endpoint) &&
	    !OU::findString(c->m_paramsIn, "transport", transport))
	  c->m_paramsIn.add("transport", "socket");
      }
      emitConnection(instances, *c);
      m_connectionMap[n] = nConnections;
      m_connections[nConnections++] = c;
    }
  }
  m_connections.resize(nConnections);
  send();
  return m_more = true; // more to do
}
// Here we process a response from launch request.
// We get here because we "asked for more".
// We in fact ping-pong between receiving responses and sending requests
bool Launcher::
work(Launcher::Instances &instances, Launcher::Connections &connections) {
  m_more = false;
  if (m_sending) {
    m_request = "<update>";
    // Time to send request for more stuff.  The only reason after the initial send,
    // is to advance the state of the connections.  We expect that the caller
    // has advanced the connection since the last receive.
    Launcher::Instance *i = &instances[0];
    for (unsigned n = 0; n < instances.size(); n++, i++)
      if (!i->m_worker) {
	m_more = true;
	break;
      }
    Launcher::Connection *c = &connections[0];
    for (unsigned n = 0; n < connections.size(); n++, c++)
      if (c->m_launchIn == this) {
	if (c->m_launchOut && c->m_launchOut != this) {
	  // We are input.  See what there is to do:
	  if (c->m_iui.length())
	    emitConnectionUpdate(n, "iui", c->m_iui), m_more = true;
	  else if (c->m_fui.length())
	    emitConnectionUpdate(n, "fui", c->m_fui), m_more = true;
	}
      } else if (c->m_launchOut == this && c->m_launchIn && c->m_launchIn != this) {
	  // We are output.  See what there is to do:
	  if (c->m_ipi.length())
	    emitConnectionUpdate(n, "ipi", c->m_ipi), m_more = true;
	  else if (c->m_fpi.length())
	    emitConnectionUpdate(n, "fpi", c->m_fpi), m_more = true;
      }
    if (m_more)
      send();
  } else {
    receive();
    assert(!strcasecmp(OX::ezxml_tag(m_rx),"launching"));
    for (ezxml_t ax = ezxml_child(m_rx, "artifact"); ax; ax = ezxml_next(ax))
      loadArtifact(ax); // Just push the bytes down the pipe, getting a response for each.
    for (ezxml_t cx = ezxml_child(m_rx, "connection"); cx; cx = ezxml_next(cx))
      updateConnection(cx);
    m_more = ezxml_cattr(m_rx, "done") == NULL;
    if (!m_more) {
      // Finally create the local client-side workers.
      Launcher::Instance *i = &instances[0];
      for (unsigned n = 0; n < instances.size(); n++, i++)
	if (&i->m_container->launcher() == this && !i->m_worker) {
	  OU::PValue pv[] = { OU::PVULong("remoteInstance", m_instanceMap[n]), OU::PVEnd };
	  i->m_worker =
	    &i->m_containerApp->createWorker(i->m_impl->m_artifact,
					     i->m_name.c_str(),
					     i->m_impl->m_metadataImpl.m_xml,
					     i->m_impl->m_staticInstance,
					     i->m_slave ? i->m_slave->m_worker : NULL,
					     i->m_hasMaster,
					     pv);
	}    
    }
    m_sending = true;
  }
  return m_more;
}
void Launcher::
setPropertyValue(unsigned remoteInstance, size_t propN, std::string &v) {
  OU::format(m_request, "<control id='%u' set='%zu'>\n%s",
	     remoteInstance, propN, v.c_str());
  send();
  receive();
  assert(!strcasecmp(OX::ezxml_tag(m_rx), "control"));
  const char *err = ezxml_cattr(m_rx, "error");
  if (err)
    throw OU::Error("Error setting property: %s", err);
}

void Launcher::
getPropertyValue(unsigned remoteInstance, size_t propN, std::string &v,
		 bool hex, bool add) {
  OU::format(m_request, "<control id='%u' get='%zu' hex='%d'>\n",
	     remoteInstance, propN, hex ? 1 : 0);
  send();
  receive();
  assert(!strcasecmp(OX::ezxml_tag(m_rx), "control"));
  const char *err = ezxml_cattr(m_rx, "error");
  if (err)
    throw OU::Error("Error getting property: %s", err);
  if (add)
    v += ezxml_txt(m_rx);
  else
    v = ezxml_txt(m_rx);
}

void Launcher::
controlOp(unsigned remoteInstance, OU::Worker::ControlOperation op) {
  OU::format(m_request, "<control id='%u' op='%u'>\n",
	     remoteInstance, op);
  send();
  receive();
  assert(!strcasecmp(OX::ezxml_tag(m_rx), "control"));
  const char *err = ezxml_cattr(m_rx, "error");
  if (err)
    throw OU::Error("Error in control operation: %s", err);
}

OU::Worker::ControlState Launcher::
getState(unsigned remoteInstance) {
  OU::format(m_request, "<control id='%u' getState=''>\n", remoteInstance);
  send();
  receive();
  assert(!strcasecmp(OX::ezxml_tag(m_rx), "control"));
  const char *err;
  size_t state;
  if ((err  = ezxml_cattr(m_rx, "error")) ||
      (err = OX::getNumber(m_rx, "state", &state, NULL, 0, false, true)))
    throw OU::Error("Error in getControlState operation: %s", err);
  return (OU::Worker::ControlState)state;
}

bool Launcher::
wait(unsigned remoteInstance, OCPI::OS::ElapsedTime timeout) {
  OU::format(m_request, "<control id='%u' wait='%" PRIu32 "'>\n", remoteInstance,
	     timeout != 0 ?
	     timeout.seconds() + (timeout.nanoseconds() >= 500000000 ? 1 : 0)
	     : 0);
  send();
  receive();
  assert(!strcasecmp(OX::ezxml_tag(m_rx), "control"));
  const char *err = ezxml_cattr(m_rx, "error");
  if (err)
    throw OU::Error("Error in control operation: %s", err);
  return ezxml_cattr(m_rx, "timeout") != NULL;
}

  }
}
