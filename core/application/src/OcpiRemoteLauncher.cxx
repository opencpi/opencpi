#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <set>
#include "OcpiLauncher.h"
#include "OcpiUtilValue.h"
namespace OA = OCPI::API;
namespace OC = OCPI::Container;
namespace OE = OCPI::Util::EzXml;
namespace OL = OCPI::Library;
namespace OCPI {
  namespace Application {

bool
receiveXml(int fd, ezxml_t &rx, std::vector<char> &buf, std::string &error) {
  ezxml_free(rx);
  uint32_t len;
  ssize_t n = read(fd, (char *)&len, sizeof(len));
  if (n != sizeof(len) || len > 64*1024) {
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
  const char *err;
  if ((err = OE::ezxml_parse_str(&buf[0], len, rx))) {
    OU::format(error, "xml parsing error: %s", err);
    return true;
  }
  if ((err = ezxml_cattr(rx, "error"))) {
    OU::format(error, "Container server error: %s", err);
    return true;
  }
  return false;
}

RemoteLauncher::
~RemoteLauncher() {
  ezxml_free(m_rx);
}
void RemoteLauncher::
receive() {
  std::string error;
  if (receiveXml(m_fd, m_rx, m_response, error))
    throw OU::Error("error reading from container server \"%s\": %s",
		    m_name.c_str(), error.c_str());
}

void RemoteLauncher::
send() {
  std::string error;
  assert(m_request.length());
  OU::formatAdd(m_request, "<%s>\n", m_tag);
  if (sendXml(m_fd, m_request, "container server", error))
    throw OU::Error("%s", error.c_str());
  m_request.clear();
  m_sending = false;
}
bool
sendXml(int fd, const std::string &request, const char *msg, std::string &error) {
  uint32_t len = OCPI_UTRUNCATE(uint32_t, request.size() + 1);
  struct iovec iov[2];
  iov[0].iov_base = (char*)&len;
  iov[0].iov_len = sizeof(uint32_t);
  iov[1].iov_base = (void*)request.c_str();
  iov[1].iov_len = request.length()+1;
  ssize_t n, total = iov[0].iov_len + iov[1].iov_len;
  do n = writev(fd, iov, 2); while (n > 0 && (total -= n));
  return n > 0 ? false : OU::eformat(error, "Error writing to %s: %s", msg, strerror(errno));
}

// This scheme is ours so that it is somewhat readable, xml friendly, and handles NULLs
static void
encodeDescriptor(const std::string &s, std::string &out) {
  OU::formatAdd(out, "%zu.", s.length());
  OU::Unparser up;
  const char *cp = s.data();
  for (size_t n = s.length(); n; n--, cp++)
    up.unparseChar(out, *cp, true);
}
static void
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
// Tell the server that this artifact is needed
// The server will ask for it if it doesn't already have it.
// The name and mtime are fluff - the uuid and length are critical
void RemoteLauncher::
emitArtifact(const OCPI::Library::Artifact &art) {
  m_artifacts.insert(&art);
  OU::formatAdd(m_request,
		"  <artifact name='%s' mtime='%'" PRIu32 " uuid='%s' length='%" PRIu64
		"' id='%p'/>\n",
		art.name().c_str(), OCPI_UTRUNCATE(uint32_t,art.mtime()), art.uuid().c_str(),
		art.length(), &art);
}
// FIXME: someday allow for instance parameters?
void RemoteLauncher::
emitInstance(const char *name, size_t artN, const Launcher::Instance &i, const char *slave) {
  m_instances.insert(&i);
  OU::formatAdd(m_request,
		"  <instance name='%s' artifact='%zu' worker='%u'",
		name, artN, i.m_impl->m_ordinal);
  if (i.m_impl->m_staticInstance)
    OU::formatAdd(m_request, " static='%s'",
		  ezxml_cattr(i.m_impl->m_staticInstance, "name"));
  if (slave)
    OU::formatAdd(m_request, " slave='%s'", slave);
  if (i.m_propValues.size()) {
    m_request += ">\n";
    const OU::Value *vals = &i.m_propValues[0];
    for (unsigned n = 0; n < i.m_propValues.size(); n++, vals++) {
      OU::formatAdd(m_request,
		    "    <property n='%u' v='", n);
      vals->unparse(m_request, NULL, true); // FIXME: someday an XML unparser for perfect quoting
      m_request += "'/>\n";
    }
    m_request += "  </instance>\n";
  } else
    m_request += "/>\n";
}
void RemoteLauncher::
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
// This connection is one of ours, either internal or external
void RemoteLauncher::
emitConnection(const Launcher::Connection &c) { 
  m_connections.insert(&c);
  OU::formatAdd(m_request, "  <connection id='%p'>\n", &c);
  if (c.m_launchOut == this)
    emitPort(*c.m_instOut, c.m_nameOut, c.m_paramsOut, "from");
  if (c.m_launchIn == this)
    emitPort(*c.m_instIn, c.m_nameIn, c.m_paramsIn, "to");
  if (c.m_url)
    OU::formatAdd(m_request, "  <url name='%s' url='%s'/>\n",
		  c.m_launchOut == this ? c.m_nameIn : c.m_nameOut, c.m_url);
  m_request += "  </connection>\n";
}
void RemoteLauncher::
emitConnectionUpdate(Launcher::Connection &c, const char *iname, std::string &sinfo) {
  if (m_request.empty())
    m_request = "<update>\n";
  OU::formatAdd(m_request, "  <connection id='%p' %s=\"", &c, iname);
  // This encoding escapes double quotes and allows for embedded null characters
  encodeDescriptor(sinfo, m_request);
  m_request += "\"/>\n";
}
void RemoteLauncher::
loadArtifact(ezxml_t ax) {
  // This is called when receiving a message that is requesting artifact downloads.
  // We simply send the data in band one after the other.
  assert(sizeof(uint64_t) >= sizeof(intptr_t));
  uint64_t n64;
  bool found;
  const char *err = OE::getNumber64(ax, "id", &n64, &found);
  if (err || !found)
    throw OU::Error("Bad artifact load request from container server: %s (%u)",
		    err ? err : "", found);
  const OL::Artifact &l = *(const OL::Artifact*)n64;
  if (m_artifacts.find(&l) == m_artifacts.end())
    throw OU::Error("Bad artifact load request from container server: %p not found", &l);
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
void RemoteLauncher::
updateInstance(Launcher::Instances &instances, ezxml_t ix) {
  uint64_t i64, w64;
  bool found;
  const char *err = OE::getNumber64(ix, "id", &i64, &found);
  if (err || !found ||
      (err = OE::getNumber64(ix, "wid", &w64, &found)) ||
       !found)
    throw OU::Error("Bad instance update from container server: %s (%u)", err ? err : "", found);
  OCPI::Application::Launcher::Instance &i =
    *(OCPI::Application::Launcher::Instance*)i64;
  ocpiCheck(&i >= &instances[0] && &i < &instances[instances.size()]);
  // We are being informed from the server that a worker instance has been created.
  // The local container app needs to know about it and the overall app needs to know about it.
  i.m_worker = &i.m_containerApp->createWorker(i.m_impl->m_artifact,
					       i.m_name.c_str(),
					       i.m_impl->m_metadataImpl.m_xml,
					       i.m_impl->m_staticInstance,
					       i.m_slave ? i.m_slave->m_worker : NULL);
}

void RemoteLauncher::
updateConnection(ezxml_t cx) {
  uint64_t c64;
  bool found;
  const char *err = OE::getNumber64(cx, "id", &c64, &found);
  if (err || !found)
    throw OU::Error("Bad connection update from container server: %s (%u)",
		    err ? err : "", found);
  OCPI::Application::Launcher::Connection &c =
    *(OCPI::Application::Launcher::Connection*)c64;
  // We should only get this for connections that we are one side of.
  const char *info;
  if (c.m_launchIn) {
    if ((info = ezxml_cattr(cx, "ipi")))
      decodeDescriptor(info, c.m_ipi);
    else if ((info = ezxml_cattr(cx, "fpi")))
      decodeDescriptor(info, c.m_fpi);
  } else if (c.m_launchOut) {
    if ((info = ezxml_cattr(cx, "iui")))
      decodeDescriptor(info, c.m_iui);
    else if ((info = ezxml_cattr(cx, "fui")))
      decodeDescriptor(info, c.m_fui);
  }
}

bool RemoteLauncher::
launch(Launcher::Instances &instances, Launcher::Connections &connections) {
  m_request = "<application>\n";
  // Send artifact info: keep track of the ordinals of each artifact
  typedef std::map<OL::Artifact*, unsigned> Artifacts;
  typedef Artifacts::value_type ArtifactsPair;
  typedef Artifacts::const_iterator ArtifactsIter;
  Artifacts artifacts;
  std::set<const OL::Implementation *> implementations;
  Launcher::Instance *i = &instances[0];
  unsigned nArtifacts = 0;
  for (unsigned n = 0; n < instances.size(); n++, i++)
    if (&i->m_container->launcher() == this) {
      OL::Artifact &a = i->m_impl->m_artifact;
      ArtifactsIter arti = artifacts.find(&a);
      unsigned artN;
      if (arti == artifacts.end()) {
	emitArtifact(a);    // name, mtime, uuid, length, ptr
	artifacts[&a] = artN = nArtifacts++;
      } else
	artN = arti->second;
      emitInstance(i->m_name.c_str(), artN, *i,
		   i->m_slave && &i->m_slave->m_container->launcher() == this ?
		   i->m_slave->m_name.c_str() : NULL);
    }
  Launcher::Connection *c = &connections[0];
  for (unsigned n = 0; n < connections.size(); n++, c++)
    if (c->m_launchIn == this || c->m_launchOut == this)
      emitConnection(*c);
  send();
  return m_more = true; // more to do
}
// Here we process the response from the initial XML-based launch request.
// We get here because we "asked for more".
// We in fact ping-pong between receiving responses and sending requests
bool RemoteLauncher::
work(Launcher::Instances &instances, Launcher::Connections &connections) {
  m_more = false;
  if (m_sending) {
    // Time to send request for more stuff.  The only reason after the initial send,
    // is to advance the state of the connections
    Launcher::Connection *c = &connections[0];
    for (unsigned n = 0; n < connections.size(); n++, c++)
      if (c->m_launchIn == this) {
	if (c->m_launchOut && c->m_launchOut != this) {
	  // We are input.  See what there is to do:
	  if (c->m_iui.length())
	    emitConnectionUpdate(*c, "iui", c->m_iui), m_more = true;
	  else if (c->m_fui.length())
	    emitConnectionUpdate(*c, "fui", c->m_fui), m_more = true;
	}
      } else if (c->m_launchOut == this && c->m_launchIn && c->m_launchIn != this) {
	  // We are output.  See what there is to do:
	  if (c->m_ipi.length())
	    emitConnectionUpdate(*c, "ipi", c->m_ipi), m_more = true;
	  else if (c->m_fpi.length())
	    emitConnectionUpdate(*c, "fpi", c->m_fpi), m_more = true;
      }
    if (m_more)
      send();
  } else {
    receive();
    assert(!strcasecmp(OE::ezxml_tag(m_rx),"launching"));
    for (ezxml_t ax = ezxml_child(m_rx, "artifact"); ax; ax = ezxml_next(ax))
      loadArtifact(ax); // Just push the bytes down the pipe, getting a response for each.
    for (ezxml_t ix = ezxml_child(m_rx, "instance"); ix; ix = ezxml_next(ix))
      updateInstance(instances, ix);
    for (ezxml_t cx = ezxml_child(m_rx, "connection"); cx; cx = ezxml_next(cx))
      updateConnection(cx);
    m_more = ezxml_cattr(m_rx, "done") == NULL;
    m_sending = true;
  }
  return m_more;
}

  }
}
