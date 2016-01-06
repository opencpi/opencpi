#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <cstring>
#include <set>
#include "OcpiOsSocket.h"
#include "OcpiUtilValue.h"
#include "Container.h"
#include "ContainerApplication.h"
#include "RemoteLauncher.h"

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

// static
bool Launcher::
receiveXml(int fd, ezxml_t &rx, std::vector<char> &buf, bool &eof, std::string &error) {
  ezxml_free(rx);
  rx = NULL;
  uint32_t len;
  eof = false;
  ssize_t n = read(fd, (char *)&len, sizeof(len));
  if (n != sizeof(len) || len > 64*1024) {
    if (n == 0) {
      eof = true;
      error = "EOF on socket read";
    } else
      OU::format(error, "read error: %s (%zu, %zu)", strerror(errno), n, (size_t)len);
    return true;
  }
  ssize_t total = len;
  buf.resize(total);
  for (char *cp = &buf[0]; total && (n = read(fd, cp, len)) > 0; total -= n, cp += n)
    ;
  if (n <= 0) {
    OU::format(error, "message read error: %s (%zu)", strerror(errno), n);
    return true;
  }
  ocpiDebug("Received XML===========================\n%s\nEND XML==========", &buf[0]);
  const char *err;
  if ((err = OX::ezxml_parse_str(&buf[0], len, rx))) {
    OU::format(error, "xml parsing error: %s", err);
    return true;
  }
  if ((err = ezxml_cattr(rx, "error"))) {
    OU::format(error, "Container server error: %s", err);
    return true;
  }
  return false;
}

void Launcher::
receive() {
  std::string error;
  bool eof;
  if (receiveXml(m_fd, m_rx, m_response, eof, error))
    throw OU::Error("error reading from container server \"%s\": %s",
		    m_name.c_str(), error.c_str());
}

void Launcher::
send() {
  std::string error;
  if (sendXml(m_fd, m_request, "container server", error))
    throw OU::Error("%s", error.c_str());
  m_request.clear();
  m_sending = false;
}
// static
bool Launcher::
sendXml(int fd, std::string &request, const char *msg, std::string &error) {
  assert(request.length());
  const char *rb = strchr(request.c_str(), '>');
  const char *sp = strchr(request.c_str(), ' ');
  assert(rb || sp);
  if (sp && sp < rb)
    rb = sp;
  OU::formatAdd(request, "</%.*s>\n", (int)(rb - (request.c_str()+1)), request.c_str() + 1);
  uint32_t len = OCPI_UTRUNCATE(uint32_t, request.size() + 1);
  struct iovec iov[2];
  iov[0].iov_base = (char*)&len;
  iov[0].iov_len = sizeof(uint32_t);
  iov[1].iov_base = (void*)request.c_str();
  iov[1].iov_len = request.length()+1;
  ssize_t n, total = iov[0].iov_len + iov[1].iov_len;
  ocpiDebug("Sending XML===========================\n%s\nEND XML==========", request.c_str());
  do n = writev(fd, iov, 2); while (n > 0 && (total -= n));
  return n > 0 ? false : OU::eformat(error, "Error writing to %s: %s", msg, strerror(errno));
}

// This scheme is ours so that it is somewhat readable, xml friendly, and handles NULLs
void Launcher::
encodeDescriptor(const char *iname, const std::string &s, std::string &out) {
  OU::formatAdd(out, " %s=\"%zu.", iname, s.length());
  OU::Unparser up;
  const char *cp = s.data();
  for (size_t n = s.length(); n; n--, cp++) {
    if (*cp == '\'')
      out += "&apos;";
    else if (*cp == '&')
      out += "&amp;";
    else
      up.unparseChar(out, *cp, true);
  }
  out += "\"";
}
void Launcher::
decodeDescriptor(const char *info, std::string &s) {
  char *end;
  errno = 0;
  size_t infolen = strtoul(info, &end, 0);
  do {
    if (errno || infolen >= 1000 || *end++ != '.')
      break;
    s.resize(infolen);
    const char *start = end;
    end += strlen(start);
    size_t n;
    for (n = 0; n < infolen && *start; n++)
      if (OU::parseOneChar(start, end, s[n]))
	break;
    if (*start || n != infolen)
      break;
    return;
  } while (0);
  throw OU::Error("Invalid Port Descriptor from Container Server in: \"%s\"",
		  info);
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
void Launcher::
emitCrew(const OCPI::Container::Launcher::Crew &crew) {
  OU::formatAdd(m_request, "  <crew size='%zu'", crew.m_size);
  if (crew.m_propValues.size()) {
    m_request += ">\n";
    const OU::Value *vals = &crew.m_propValues[0];
    const unsigned *po = &crew.m_propOrdinals[0];
    for (unsigned n = 0; n < crew.m_propValues.size(); n++, vals++, po++) {
      OU::formatAdd(m_request, "    <property n='%u' v='", *po);
      vals->unparse(m_request, NULL, true); // FIXME: someday an XML unparser for perfect quoting
      m_request += "'/>\n";
    } 
    m_request += "  </crew>\n";
  } else
    m_request += "/>\n";
}
// FIXME: someday allow for instance parameters?
void Launcher::
emitMember(const char *name, unsigned contN, unsigned artN, unsigned crewN,
	   const Launcher::Member &i, int slave) {
  OU::formatAdd(m_request,
		"  <member name='%s' container='%u' artifact='%u' crew='%u' worker='%s'",
		name, contN, artN, crewN, i.m_impl->m_metadataImpl.specName().c_str());
  if (i.m_impl->m_staticInstance)
    OU::formatAdd(m_request, " static='%s'", ezxml_cattr(i.m_impl->m_staticInstance, "name"));
  if (slave >= 0)
    OU::formatAdd(m_request, " slave='%u'", slave);
  if (i.m_doneInstance)
    m_request += " done='1'";
   if (i.m_crew->m_size != 1)
     OU::formatAdd(m_request, " member='%zu'", i.m_member);
   m_request += "/>\n";
}
#if 0
void Launcher::
emitPort(const Launcher::Member &i, const char *port, const OA::PValue *params,
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
#endif
static void 
unparseParams(const OU::PValue *params, std::string &out) {
  for (;params && params->name; params++) {
    OU::formatAdd(out, " %s='", params->name);
    params->unparse(out, true);
    out += '\'';
  }
}
void Launcher::
emitSide(const Launcher::Members &members, Launcher::Port &p, bool input) {
  const char *type = input ? "in" : "out";
  OU::formatAdd(m_request, "    <%s scale='%zu' index='%zu'", type, p.m_scale, p.m_index);
  if (p.m_name)
    OU::formatAdd(m_request, " name='%s'", p.m_name);
  if (p.m_member && p.m_launcher == this)
    OU::formatAdd(m_request, " member='%u'", m_instanceMap[p.m_member - &members[0]]);
  if (p.m_url)
    OU::formatAdd(m_request, " url='%s'", p.m_url);
  if (p.m_initial.length()) {
     encodeDescriptor(input ? "ipi" : "iui", p.m_initial, m_request);
     p.m_initial.clear();
  }
  if (p.m_params.list() || !p.m_url && p.m_launcher != this) {
    m_request += ">\n";
    if (p.m_params.list()) {
      m_request += "<params";
      unparseParams(p.m_params, m_request);
      m_request += "/>\n";
    }
    if (!p.m_url && p.m_launcher != this)
	p.m_metaPort->emitXml(m_request);
    OU::formatAdd(m_request, "    </%s>\n", type);
  } else
    m_request += "/>\n";
}

// This connection is one of ours, either internal or external
void Launcher::
emitConnection(const Launcher::Members &members, Launcher::Connection &c) { 
  OU::formatAdd(m_request,
		"  <connection transport='%s' id='%s' roleIn='%u' roleOut='%u' "
		"buffersize='%zu'>\n",
		c.m_transport.transport.c_str(), c.m_transport.id.c_str(),
		c.m_transport.roleIn, c.m_transport.roleOut, c.m_bufferSize);
  emitSide(members, c.m_in, true);
  emitSide(members, c.m_out, false);
  m_request += "  </connection>\n";
}
void Launcher::
emitConnectionUpdate(unsigned connN, const char *iname, std::string &sinfo) {
  if (m_request.empty())
    m_request = "<update>\n";
  OU::formatAdd(m_request, "  <connection id='%u'", m_connectionMap[connN]);
  // This encoding escapes double quotes and allows for embedded null characters
  encodeDescriptor(iname, sinfo, m_request);
  m_request += "/>\n";
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
  if (c.m_in.m_launcher == this) {
    // The remote is an input, so we should be receiving provider info in the update
    if ((info = ezxml_cattr(cx, "ipi")))
      decodeDescriptor(info, c.m_in.m_initial);
    else if ((info = ezxml_cattr(cx, "fpi")))
      decodeDescriptor(info, c.m_in.m_final);
  } else if (c.m_out.m_launcher == this) {
    // The remote is an input, so we should be receiving user info in the update
    if ((info = ezxml_cattr(cx, "iui")))
      decodeDescriptor(info, c.m_out.m_initial);
    else if ((info = ezxml_cattr(cx, "fui")))
      decodeDescriptor(info, c.m_out.m_final);
  }
}

bool Launcher::
launch(Launcher::Members &instances, Launcher::Connections &connections) {
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
  typedef Artifacts::const_iterator ArtifactsIter;
  Artifacts artifacts;
  unsigned nArtifacts = 0;

  // A temporary map to keep track of which crews we have emitted
  typedef std::map<OC::Launcher::Crew*, unsigned> Crews;
  typedef Crews::const_iterator CrewsIter;
  Crews crews;
  unsigned nCrews = 0;

 // Create map from global instance num to remote instance num
  Launcher::Member *i = &instances[0];
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
      OC::Launcher::Crew &c = *i->m_crew;
      unsigned crewN = 0;
      CrewsIter crewi = crews.find(&c);
      if (crewi == crews.end())  {
	emitCrew(c);
	crews[&c] = crewN = nCrews++;
      }
      emitMember(i->m_name.c_str(), contN, artN, crewN, *i,
		 i->m_slave && &i->m_slave->m_container->launcher() == this ?
		 m_instanceMap[i->m_slave - &instances[0]] : -1);
    }
  m_artifacts.resize(nArtifacts);
  unsigned nConnections = 0;
  Launcher::Connection *c = &connections[0];
  for (unsigned n = 0; n < connections.size(); n++, c++) {
    c->prepare(); // make sure transport parameters are on both sides
    if (c->m_in.m_launcher == this || c->m_out.m_launcher == this) {
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
work(Launcher::Members &instances, Launcher::Connections &connections) {
  m_more = false;
  if (m_sending) {
    m_request = "<update>";
    // Time to send request for more stuff.  The only reason after the initial send,
    // is to advance the state of the connections.  We expect that the caller
    // has advanced the connection since the last receive.
    Launcher::Member *i = &instances[0];
    for (unsigned n = 0; n < instances.size(); n++, i++)
      if (!i->m_worker) {
	m_more = true;
	break;
      }
    Launcher::Connection *c = &connections[0];
    for (unsigned n = 0; n < connections.size(); n++, c++)
      if (c->m_in.m_launcher == this) {
	if (c->m_out.m_launcher && c->m_out.m_launcher != this) {
	  // We are input.  See what there is to do:
	  if (c->m_out.m_initial.length())
	    emitConnectionUpdate(n, "iui", c->m_out.m_initial), m_more = true;
	  else if (c->m_out.m_final.length())
	    emitConnectionUpdate(n, "fui", c->m_out.m_final), m_more = true;
	  c->m_out.m_initial.clear();
	  c->m_out.m_final.clear();
	}
      } else if (c->m_out.m_launcher == this && c->m_in.m_launcher &&
		 c->m_in.m_launcher != this) {
	  // We are output.  See what there is to do:
	  if (c->m_in.m_initial.length())
	    emitConnectionUpdate(n, "ipi", c->m_in.m_initial), m_more = true;
	  else if (c->m_in.m_final.length())
	    emitConnectionUpdate(n, "fpi", c->m_in.m_final), m_more = true;
	  c->m_in.m_initial.clear();
	  c->m_in.m_final.clear();
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
      Launcher::Member *i = &instances[0];
      for (unsigned n = 0; n < instances.size(); n++, i++)
	if (&i->m_container->launcher() == this && !i->m_worker) {
	  OU::PValue pv[] = { OU::PVULong("remoteInstance", m_instanceMap[n]), OU::PVEnd };
	  i->m_worker =
	    &i->m_containerApp->createWorker(i->m_impl->m_artifact, i->m_name.c_str(),
					     i->m_impl->m_metadataImpl.m_xml,
					     i->m_impl->m_staticInstance,
					     i->m_slave ? i->m_slave->m_worker : NULL,
					     i->m_hasMaster,
					     i->m_member, i->m_crew ? i->m_crew->m_size : 1,
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
