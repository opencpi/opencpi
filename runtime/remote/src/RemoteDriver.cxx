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

#include <cstring>
#include "OcpiOsMisc.h"
#include "OcpiOsEther.h"
#include "OcpiUtilValue.h"
#include "OcpiDriverManager.h"
#include "ContainerManager.h"
#include "RemoteLauncher.h"
#include "RemoteDriver.h"

// This is the "driver" for remote containers, which finds them, constructs them, and 
// in general manages them. It acts as the factory for Remote containers.

// This is similar to simulation HDL containers in that they are discovered by UDP,
// but different, in that one UDP endpoint sources multiple containers for purposes of
// discovery.  But after being discovered a TCP socket is established to talk to the
// server.  The derived object on the client side is a Client in this namespace,
// which inherits the specifically functional Launcher class that knows how to be
// a "network launcher" of applications.

namespace OC = OCPI::Container;
namespace OU = OCPI::Util;
namespace OX = OCPI::Util::EzXml;
namespace OA = OCPI::API;
namespace OL = OCPI::Library;
namespace OE = OCPI::OS::Ether;
namespace OS = OCPI::OS;
namespace OR = OCPI::RDT;
namespace OCPI {
  namespace Remote {

const uint16_t REMOTE_PORT = 17171;
const uint16_t REMOTE_NARGS = 7; // fields in the discovery entries before transports
extern const char *remote;
const unsigned RETRIES = 3;
const unsigned DELAYMS = 500;
const char *remote = "remote";
class Container;
class Artifact : public OC::ArtifactBase<Container,Artifact> {
  friend class Container;
  Artifact(Container &c, OL::Artifact &lart, const OA::PValue *props)
    : OC::ArtifactBase<Container,Artifact>(c, *this, lart, props) {
  }
  virtual ~Artifact() {}
};
class ExternalPort;
class Worker;
class Port : public OC::PortBase<Worker, Port, OCPI::API::ExternalPort> {
  Port( Worker& w, const OU::Port & pmd, const OU::PValue *params)
    :  OC::PortBase<Worker, Port, OCPI::API::ExternalPort> (w, *this, pmd, params) {
  }

  ~Port() {
  }
};
class Application;
class Worker
  : public OC::WorkerBase<Application,Worker,Port> {
  friend class Application;
  unsigned m_remoteInstance;
  Launcher &m_launcher;
  Worker(Application & app, Artifact *art, const char *name, ezxml_t impl, ezxml_t inst,
	 const OC::Workers &/*slaves*/, bool hasMaster, size_t member, size_t crewSize,
	 const OU::PValue *wParams, unsigned remoteInstance);
  virtual ~Worker() {}
  OC::Port &createPort(const OU::Port&, const OU::PValue */*params*/) {
    ocpiAssert("This method is not expected to ever be called" == 0);
    return *(OC::Port*)this;
  }
  void controlOperation(OU::Worker::ControlOperation op) {
    if (getControlMask() & (1 << op))
      m_launcher.controlOp(m_remoteInstance, op);
  }
  void setProperty(const OCPI::API::PropertyInfo &info, const char *val, const OU::Member &m,
		   size_t offset, size_t dimension) const {
    // Pass raw info for (maybe sub-) property access
    m_launcher.setPropertyValue(m_remoteInstance, info.m_ordinal, val, m.m_path, offset, dimension);
  }
  // FIXME: this should be at the lower level for just reading the bytes remotely to enable caching
  // properly, but we have to be careful to cache for remote proxies...
  void getProperty(const OA::PropertyInfo &info, std::string &v, const OU::Member &m, size_t offset,
		   size_t dimension, OA::PropertyOptionList &options,
		   OA::PropertyAttributes *a_attributes = NULL) const {
    m_launcher.getPropertyValue(m_remoteInstance, info.m_ordinal, v, m.m_path, offset, dimension,				options, a_attributes);
  }
  bool wait(OS::Timer *t) {
    return m_launcher.wait(m_remoteInstance, t ? t->getRemaining() : 0);
  }
  void checkControlState() const {
    ((Worker *)this)->setControlState(m_launcher.getState(m_remoteInstance));
  }

  void read(size_t /*offset*/, size_t /*nBytes*/, void */*p_data*/) {}
  void write(size_t /*offset*/, size_t /*nBytes*/, const void */*p_data*/ ) {}
  void setPropertyBytes(const OA::PropertyInfo &/*info*/, size_t /*offset*/,
			const uint8_t */*data*/, size_t /*nBytes*/, unsigned /*idx*/) const {};
  void setProperty8(const OA::PropertyInfo &/*info*/, size_t /*offset*/, uint8_t /*data*/,
		    unsigned /*idx*/) const {}
  void setProperty16(const OA::PropertyInfo &/*info*/, size_t /*offset*/, uint16_t /*data*/,
		     unsigned /*idx*/) const {}
  void setProperty32(const OA::PropertyInfo &/*info*/, size_t /*offset*/, uint32_t /*data*/,
		     unsigned /*idx*/) const {}
  void setProperty64(const OA::PropertyInfo &/*info*/, size_t /*offset*/, uint64_t /*data*/,
		     unsigned /*idx*/) const {}
  void getPropertyBytes(const OA::PropertyInfo &/*info*/, size_t /*offset*/,
			uint8_t */*data*/, size_t /*nBytes*/, unsigned /*idx*/, bool /*string*/)
    const {}
  uint8_t getProperty8(const OA::PropertyInfo &/*info*/, size_t /*offset*/, unsigned /*idx*/) const { return 0; }
  uint16_t getProperty16(const OA::PropertyInfo &/*info*/, size_t /*offset*/, unsigned /*idx*/) const { return 0; }
  uint32_t getProperty32(const OA::PropertyInfo &/*info*/, size_t /*offset*/, unsigned /*idx*/) const  { return 0; }
  uint64_t getProperty64(const OA::PropertyInfo &/*info*/, size_t /*offset*/, unsigned /*idx*/) const  { return 0; }

      
  void propertyWritten(unsigned /*ordinal*/) const {};
  void propertyRead(unsigned /*ordinal*/) const {};
  void prepareProperty(OU::Property &,
		       volatile uint8_t *&/*writeVaddr*/,
		       const volatile uint8_t *&/*readVaddr*/) const {}

  // These scalar and binary accessors must convert to a string value in the remote
  // case, since the RPC is string-based anyway.
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
  void set##pretty##Property(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m, \
			     size_t offset, const run val, unsigned idx) const { \
    OU::Unparser up;							\
    std::string s;							\
    up.unparse##pretty(s, val, true);					\
    setProperty(info, s.c_str(), m, offset + idx * m.m_elementBytes,	\
		m.m_isSequence || m.m_arrayRank ? 1 : 0);		\
  }									\
  void set##pretty##Cached(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m, \
			     size_t offset, const run val, unsigned idx) const { \
    set##pretty##Property(info, m, offset, val, idx); \
  } \
  void set##pretty##SequenceProperty(const OA::PropertyInfo &info, const run *vals, \
				     size_t length) const { \
    OU::Unparser up;					    \
    std::string s;					    \
    for (unsigned n = 0; n < length; ++n)		    \
      up.unparse##pretty(s, *vals++, true);		    \
    setProperty(info, s.c_str(), info, 0, 0);		    \
  }
  // Set a string property value
  // ASSUMPTION:  strings always occupy at least 4 bytes, and
  // are aligned on 4 byte boundaries.  The offset calculations
  // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)      \
  void set##pretty##Property(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m, \
			     size_t offset, const run val, unsigned idx) const { \
    OU::Unparser up;							\
    std::string s;							\
    up.unparse##pretty(s, val, true);					\
    setProperty(info, s.c_str(), m, offset + idx * m.m_elementBytes,	\
		m.m_isSequence || m.m_arrayRank ? 1 : 0);		\
  }									\
  void set##pretty##Cached(const OCPI::API::PropertyInfo &info, const OCPI::Util::Member &m, \
			   size_t offset, const run val, unsigned idx) const { \
    set##pretty##Property(info, m, offset, val, idx);			\
  }									\
  void set##pretty##SequenceProperty(const OA::PropertyInfo &/*p*/, const run */*vals*/, \
				     size_t /*length*/) const {}
  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
  // Get Scalar Property by in fact getting the string and parsing it
  // We get the string because that is the natural remote operation
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		             \
  run get##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member &m, \
			    size_t offset, unsigned idx) const {	             \
    std::string uValue; /* unparsed value */                                         \
    getProperty(info, uValue, m, offset + idx * info.m_elementBytes, \
		m.m_isSequence || m.m_arrayRank ? 1 : 0, OA::PropertyOptionList({ OA::HEX })); \
    OU::ValueType vt(m);						\
    vt.m_isSequence = false;						\
    vt.m_arrayRank = 0;							\
    OU::Value v(vt);							\
    (void)v.parse(uValue.c_str());					\
    return v.m_##pretty;						\
  }									\
  run get##pretty##Cached(const OCPI::API::PropertyInfo &info, const Util::Member &m, \
			  size_t offset, unsigned idx) const {		\
    return get##pretty##Property(info, m, offset, idx);			\
  }									\
  unsigned get##pretty##SequenceProperty(const OA::PropertyInfo &info, run *vals, \
					 size_t length) const {		\
    std::string uValue; /* unparsed value */				\
    getProperty(info, uValue, info, 0, 0, OA::PropertyOptionList({ OA::HEX })); \
    OU::Value v(info);							\
    (void)v.parse(uValue.c_str());					\
    if (vals) {								\
      if (v.m_nTotal > length)						\
	throw OU::Error("property value has more items than buffer");	\
      memcpy(vals, v.m_pUChar, v.m_nTotal * info.m_elementBytes);	\
    }									\
    return OCPI_UTRUNCATE(unsigned, v.m_nTotal);			\
  }
  // ASSUMPTION:  strings always occupy at least 4 bytes, and
  // are aligned on 4 byte boundaries.  The offset calculations
  // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)	\
  void get##pretty##Property(const OCPI::API::PropertyInfo &info, const Util::Member &m, \
			     size_t offset, char *cp, size_t length, unsigned idx) const { \
    assert(m.m_stringLength < length); /* sb checked elsewhere */	\
    std::string uValue; /* unparsed value */				\
    getProperty(info, uValue, m, offset + idx * info.m_elementBytes,	\
		m.m_isSequence || m.m_arrayRank ? 1 : 0, OA::PropertyOptionList({ OA::HEX }));	\
    strncpy(cp, uValue.c_str(), length); \
  }									\
  run get##pretty##Cached(const OCPI::API::PropertyInfo &info, const Util::Member &m, \
			   size_t offset, char *cp, size_t length, unsigned idx) const { \
    get##pretty##Property(info, m, offset, cp, length, idx); \
    return cp; \
  } \
  unsigned get##pretty##SequenceProperty				\
  (const OA::PropertyInfo &/*p*/, char **/*vals*/, size_t /*length*/, char */*buf*/, \
   size_t /*space*/) const { return 0; }

  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
};
class Application
  : public OC::ApplicationBase<Container,Application,Worker> {
  friend class Container;
  Application(Container &c, const char *a_name, const OU::PValue *params)
    : OC::ApplicationBase<Container,Application,Worker>(c, *this, a_name, params) {
  }
  virtual ~Application() {
    // This really has no local (client-side) state and the server-side state is taken down
    // by remotelauncher->appShutdown()
  }
  OC::Worker &
  createWorker(OC::Artifact *art, const char *appInstName, ezxml_t impl, ezxml_t inst,
	       const OC::Workers &slaves, bool hasMaster, size_t member, size_t crewSize,
	       const OU::PValue *wParams) {
    uint32_t remoteInstance;
    if (!OU::findULong(wParams, "remoteInstance", remoteInstance))
      throw OU::Error("Remote ContainerApplication expects remoteInstance parameter");
    return *new Worker(*this, art ? static_cast<Artifact*>(art) : NULL,
		       appInstName ? appInstName : "unnamed-worker", impl, inst, slaves,
		       hasMaster, member, crewSize, wParams, remoteInstance);
  }
};

class Driver;

extern const char *remote;
// The "client" class that knows how to talk to some remote containers.
// There is no parent-child relationship since containers are always
// the children of container drivers.
class Client 
  : public OU::Child<Driver,Client,remote>,
    public Launcher {
  friend class Driver;
protected:
  OS::Socket &m_socket;
  // Take ownership of the provided socket
  Client(Driver &d, const char *a_name, OS::Socket &socket)
    : OU::Child<Driver,Client,remote>(d, *this, a_name),
      Launcher(socket),
      m_socket(socket)
  {
  }
  ~Client() {
    delete &m_socket; // we took ownership at construction
  }
public:
  const std::string &name() const {
    return OU::Child<Driver,Client,remote>::name();
  }
};

class Container
  : public OC::ContainerBase<Driver,Container,Application,Artifact> {
  Client &m_client;
public:
  Container(Client &client, const std::string &a_name,
	    const char *a_model, const char *a_os, const char *a_osVersion, const char *a_arch,
	    const char *a_platform, const char *a_dynamic, const char *a_transports,
            const OA::PValue* /*params*/)
    throw ( OU::EmbeddedException )
    : OC::ContainerBase<Driver,Container,Application,Artifact>(*this, a_name.c_str()),
      m_client(client) {
    ocpiDebug("Construct remote container %s with transports: %s", a_name.c_str(),
	      a_transports ? a_transports : "<none>");
    m_model = a_model;
    m_os = a_os;
    m_osVersion = a_osVersion;
    m_arch = a_arch;
    m_platform = a_platform;
    unsigned nTransports = 0;
    for (const char *p = a_transports; *p; p++)
      if (*p == '|')
	nTransports++;
#if 0
    m_transports.resize(nTransports);
    OC::Transport *t = &m_transports[0];
#endif
    char transport[strlen(a_transports)+1];
    char id[strlen(a_transports)+1];
    for (unsigned n = nTransports; n; n--) { // , t++) {
      int nChars, rv;
      unsigned roleIn, roleOut;
      uint32_t optionsIn, optionsOut;
      if ((rv = sscanf(a_transports, "%[^,],%[^,],%u,%u,0x%x,0x%x|%n", transport, id, &roleIn, &roleOut,
		       &optionsIn, &optionsOut, &nChars)) != 6)
	throw OU::Error("Bad transport string in container discovery: %s: %d", a_transports, rv);
      if (id[0] == ' ') // local sscanf issue - field cannot be empty
	id[0] = '\0';
      addTransport(transport, id, (OR::PortRole)roleIn, (OR::PortRole)roleOut, optionsIn,
		   optionsOut);
      a_transports += nChars;
    }
    OX::parseBool(a_dynamic, NULL, &m_dynamic);
  }
  virtual ~Container()
  throw () {
  }
  bool portsInProcess() { return false; }
  OC::Launcher &launcher() const {
    return m_client;
  }
  OA::ContainerApplication*
  createApplication(const char *a_name, const OU::PValue *props)
    throw (OU::EmbeddedException) {
    return new Application(*this, a_name, props);
  }
  // Fixme - use a background thread to simply monitor the state of the connection to the
  // server to do something when it hangs or crashes.
  // This could include waiting.
  bool needThread() { return false; }
  OC::Artifact &
  createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) {
    return *new Artifact(*this, lart, artifactParams);
  }
};

Worker::
Worker(Application & app, Artifact *art, const char *a_name, ezxml_t impl, ezxml_t inst,
       const OC::Workers &a_slaves, bool a_hasMaster, size_t a_member, size_t a_crewSize,
       const OU::PValue *wParams, unsigned remoteInstance)
  : OC::WorkerBase<Application,Worker,Port>(app, *this, art, a_name, impl, inst, a_slaves,
					    a_hasMaster, a_member, a_crewSize, wParams),
    m_remoteInstance(remoteInstance),
    m_launcher(*static_cast<Launcher *>(&app.parent().launcher())) {
  setControlMask(getControlMask() | (OU::Worker::OpInitialize|
				     OU::Worker::OpStart|
				     OU::Worker::OpStop|
				     OU::Worker::OpRelease));
  // Fake the connections.  The error checking is on the remote side, but this allows
  // the local (client-side) error checks about connectivity to succeed.
  // FIXME: possibly set this just based on what is actually in the launch info
  for (unsigned n = 0; n < m_nPorts; n++)
    connectPort(n);
}

// The driver class owns the containers (like all container driver classes)
// and also owns the clients of those containers.
Driver::Driver() throw() {
  ocpiCheck(pthread_key_create(&s_threadKey, NULL) == 0);
  ocpiDebug("Registering the Remote Container driver");
  const char *env = getenv("OCPI_ENABLE_REMOTE_DISCOVERY");
  if ((m_doNotDiscover = env && env[0] == '1' ? false : true))
    ocpiInfo("Remote container discovery is off.  Use OCPI::API::enableServerDiscovery() or the OCPI_ENABLE_REMOTE_DISCOVERY variable described in the Application Guide.");
}

// The driver entry point to deal with explicitly specified servers, in params or the environment
bool Driver::
useServers(const OU::PValue *params, bool verbose, std::string &error) {
  char *saddr = getenv("OCPI_SERVER_ADDRESS");
  if (saddr && probeServer(saddr, verbose, NULL, NULL, false, error))
    return true;
  if ((saddr = getenv("OCPI_SERVER_ADDRESSES")))
    for (OU::TokenIter li(saddr); li.token(); li.next())
      if (probeServer(li.token(), verbose, NULL, NULL, false, error))
	return true;
  if ((saddr = getenv("OCPI_SERVER_ADDRESS_FILE"))) {
    std::string addrs;
    const char *err = OU::file2String(addrs, saddr, ' ');
    if (err)
      throw OU::Error("The file indicated by the OCPI_SERVER_ADDRESS_FILE environment "
		      "variable, \"%s\", cannot be opened: %s", saddr, err);
    for (OU::TokenIter li(addrs); li.token(); li.next())
      if (probeServer(li.token(), verbose, NULL, NULL, false, error))
	return true;
  }
  for (const OU::PValue *p = params; p && p->name; ++p)
    if (!strcasecmp(p->name, "server")) {
      if (p->type != OA::OCPI_String)
	throw OU::Error("Value of \"server\" parameter is not a string");
      if (probeServer(p->vString, verbose, NULL, NULL, false, error))
	return true;
    }
  return false;
}

void Driver::
configure(ezxml_t xml) {
  OCPI::Driver::Driver::configure(xml);
  std::string error;
  if (useServers(NULL, false, error))
    ocpiBad("Error during remote server processing: %s", error.c_str());
}
// Called either from UDP discovery or explicitly, e.g. from ocpirun
// If the latter, the "containers" argument will be NULL
bool Driver::
probeServer(const char *server, bool verbose, const char **exclude, char *containers,
	    bool discovery, std::string &error) {
  ocpiInfo("probing remote container server: %s", server);
  error.clear();
  OS::Socket *sock = NULL;
  uint16_t port;
  std::string host(server);
  const char *sport = strchr(server, ':');
  if (sport) {
    const char *err;
    if ((err = OU::Value::parseUShort(sport + 1, NULL, port)))
      return OU::eformat(error, "Bad port number in server name: \"%s\"", server);
    host.resize(OCPI_SIZE_T_DIFF(sport, server));
  } else
    port = REMOTE_PORT;
  ezxml_t rx = NULL; // need to explicitly free this below via "bad:" or "out:"
  std::vector<char> rbuf;
  bool taken = false; // whether the socket has been taken by a launcher
  Client *client = OU::Parent<Client>::findChildByName(server);
  if (!containers) {
    if (client)
      goto out; // we already are using this server
    // We are not being called during discovery, but explicitly (when avoiding discovery).
    // Whereas during UDP discovery, this info comes back in the UDP datagram, here we must go
    // get it via TCP by opening the TCP socket early, before excluding specific containers.
    try {
      sock = new OS::Socket(host, port);
    } catch (const std::string &e) {
      return OU::eformat(error, "Error connecting to server \"%s\": %s", server, e.c_str());
    }
    std::string request("<discover>");
    bool eof;
    if (OX::sendXml(sock->fd(), request, "TCP server for discovery", error) ||
	OX::receiveXml(sock->fd(), rx, rbuf, eof, error))
      goto out;
    if (strcmp(OX::ezxml_tag(rx), "discovery") || !(containers = OX::ezxml_content(rx)))
      goto bad;
  }
  if (verbose)
    printf("Received server information from \"%s\".  Available containers are:\n", server);
  // Now process the discovery information per container - either from UDP discovery or from
  // explicit contact via TCP and xml
  for (char *cp = containers, *end; *cp; cp = end) {
    while (isspace(*cp)) cp++;
    if (!(end = strchr(cp, '\n')))
      goto bad;
    *end++ = '\0';
    char *args[REMOTE_NARGS + 1];
    for (char **ap = args; (*ap++ = strsep(&cp, "|")); )
      if ((ap - args) >= REMOTE_NARGS)
	break;
    // cp now points to transports
    std::string cname;
    OU::format(cname, "%s/%s", server, args[0]);
    if (exclude) {
      bool excluded = false;
      for (const char **ap = exclude; *ap; ap++)
	if (!strcasecmp(cname.c_str(), *ap)) {
	  ocpiInfo("Remote container \"%s specifically excluded/ignored", *ap);
	  excluded = true;
	  break;
	}
      if (excluded)
	continue;
    }
    // We have a remote container.  Make sure we have a client (remote launcher) for it.
    if (!client && !discovery) {
      if (!sock) { // if we got the discovery info from UDP, the TCP socket wasn't created
	try {
	  sock = new OS::Socket(host, port);
	} catch (const std::string &e) {
	  OU::format(error, "Error opening socket to server: %s", e.c_str());
	  goto out;
	}
      }
      client = new Client(*this, server, *sock);
      taken = true;
    }
    if (verbose || discovery)
      printf("%-35s  platform %s, model %s, os %s, version %s, arch %s, dynamic %s\n",
	     cname.c_str(), args[5], args[1], args[2], args[3], args[4], args[6]);
    if (verbose)
      printf("  Transports: %s\n", cp);
    ocpiDebug("Creating remote container: \"%s\", model %s, os %s, version %s, arch %s, "
	      "platform %s dynamic %s",
	      cname.c_str(), args[1], args[2], args[3], args[4], args[5], args[6]);
    ocpiDebug("Transports are: '%s'", cp);
    if (!discovery) {
      Container &c = *new Container(*client, cname.c_str(), args[1], args[2], args[3], args[4],
				    args[5], args[6], cp, NULL);
      (void)&c;
    }
  }
  sock = NULL;
  goto out;
 bad:
  OU::format(error, "Bad server container response from \"%s\"", server);
 out:
  if (sock && !taken)
    delete sock;
  if (rx)
    ezxml_free(rx);
  return !error.empty();
}

OC::Container *Driver::
probeContainer(const char *which, std::string &/*error*/, const OA::PValue */*params*/) {
  throw OU::Error("Remote containers may only be discovered, not probed: \"%s\"", which);
}

// Try a discovery (send and receive) on a socket from an interface
bool Driver::
trySocket(std::set<std::string> &servers, OE::Interface &ifc, OE::Socket &s, OE::Address &addr,
	  bool discovery, const char **exclude, bool verbose, std::string &error) {
  // keep track of different addresses discovered when we broadcast.
  std::set<OE::Address,OE::Address::Compare> addrs;
  OE::Packet sendFrame;
  strcpy((char *)sendFrame.payload, "discover");
  unsigned count = 0;
  for (unsigned n = 0; error.empty() && n < RETRIES; n++) {
    if (!s.send(sendFrame, strlen((const char*)sendFrame.payload), addr, 0, &ifc, error))
      break;
    OE::Packet recvFrame;
    OE::Address devAddr;
    size_t length;

    OS::Timer timer(0, DELAYMS * 1000000);
    while (s.receive(recvFrame, length, DELAYMS, devAddr, error)) {
      if (addr.isBroadcast()) {
	if (!addrs.insert(devAddr).second) {
	  ocpiDebug("Received redundant ethernet discovery response");
	  continue;
	}
      } else if (devAddr != addr) {
	ocpiInfo("Received ethernet discovery response from wrong address");
	continue;
      }
      char *response = (char *)recvFrame.payload;
      if (strlen(response) >= length) {
	ocpiInfo("Discovery response invalid from %s: strlen %zu length %zu",
		 devAddr.pretty(), strlen(response), length);
	continue;
      }
      ocpiDebug("Discovery response from %s is:\n%s-- end of discovery",
		devAddr.pretty(), response);
      char *cp = strchr(response, '\n');
      char *sport = strchr(response, ':');
      if (!cp || !sport || cp < sport)
	ocpiBad("Invalid response during container server discovery from %s",
		devAddr.pretty());
      else {
	*cp++ = '\0';
	if (!servers.insert(response).second) {
	  ocpiDebug("Received redundant server discovery response from: \"%s\"", response);
	  continue;
	}
	if (probeServer(response, verbose, exclude, cp, discovery, error))
	  ocpiBad("Discovered server error: %s", error.c_str());
      }
    }
    if (!timer.expired())
      OS::sleep(2);
  }
  if (error.size())
    ocpiInfo("error on interface '%s' when probing for %s: %s",
	     ifc.name.c_str(), addr.pretty(), error.c_str());
  else if (!count && !addr.isBroadcast())
    ocpiInfo("no network probe response on '%s' from '%s' after %u attempts %ums apart",
	     ifc.name.c_str(), addr.pretty(), RETRIES, DELAYMS);
  return count;
}

// Find container servers on this interface, perhaps at one specific address
unsigned Driver::
tryIface(std::set<std::string> &servers, OE::Interface &ifc, OE::Address &devAddr,
	 const char **exclude, bool discovery, bool verbose, std::string &error) {
  if (verbose)
    printf("Trying interface \"%s\"; it is %s, %s, and has IP address: %s\n",
	   ifc.name.c_str(), ifc.up ? "up" : "down",
	   ifc.connected ? "connected" : "not connected",
	   ifc.ipAddr.addrInAddr() ? ifc.ipAddr.pretty() : "<none>");
  unsigned count = 0;
  OE::Interface i("udp", error);
  if (error.length())
    ocpiInfo("Could not open udp interface for discovery: %s", error.c_str());
  else {
    OE::Socket s(i, discovery ? ocpi_discovery : ocpi_master, NULL, 0, error);
    if (error.length())
      ocpiInfo("Could not open socket for udp discovery: %s", error.c_str());
    else {
      count = trySocket(servers, ifc, s, devAddr, discovery, exclude, verbose, error);
      if (error.length())
	ocpiInfo("Error in container server discovery for \"%s\" interface: %s",
		 ifc.name.c_str(), error.c_str());
    }
  }
  return count;
}

// Per driver discovery routine to create devices
// In this case we "discover" container servers, each of which serves us 
// whatever containers are local to that server/system
unsigned Driver::
search(const OA::PValue* params, const char **exclude, bool discoveryOnly) {
  ocpiInfo("Remote container discovery is on and proceeding");
  std::string error;
  unsigned count = 0;
  OE::IfScanner ifs(error);
  if (error.size())
    return 0;
  bool printOnly = discoveryOnly, verbose = false;
  OU::findBool(params, "printOnly", printOnly);
  OU::findBool(params, "verbose", verbose);
  const char *ifName = NULL;
  OU::findString(params, "interface", ifName);
  OE::Interface eif;
  if (verbose)
    printf("Searching for container servers via all active network interfaces.\n");
  std::set<std::string> servers;
  while (ifs.getNext(eif, error, ifName)) {
    if (eif.name == "udp") // the udp pseudo interface is not used for discovery
      continue;
    ocpiInfo("RemoteDriver: Considering interface \"%s\", addr %s, 0x%x",
	     eif.name.c_str(), eif.ipAddr.pretty(), eif.ipAddr.addrInAddr());
    if (eif.up && eif.connected && eif.ipAddr.addrInAddr()) {
      OE::Address bcast(true, REMOTE_PORT);
      count += tryIface(servers, eif, bcast, exclude, printOnly, verbose, error);
      if (error.size()) {
	ocpiInfo("Error during container server discovery on '%s': %s",
		 eif.name.c_str(), error.c_str());
	error.clear();
      }
    } else {
      if (verbose)
	printf("Skipping interface \"%s\"; it is %s, %s, and %s%s\n",
	       eif.name.c_str(), eif.up ? "up" : "down",
	       eif.connected ? "connected" : "not connected",
	       eif.ipAddr.addrInAddr() ? "has IP address: " : "has no IP address",
	       eif.ipAddr.addrInAddr() ? eif.ipAddr.pretty() : "");
      ocpiInfo("In RemoteDriver.cxx Interface '%s' is %s and %s",
	       eif.name.c_str(), eif.up ? "up" : "down",
	       eif.connected ? "connected" : "not connected");
    }
    if (ifName)
      break;
  }
  if (error.size())
    ocpiInfo("Error during container server discovery on '%s': %s",
	     eif.name.c_str(), error.c_str());
  return count;
}

Driver::
~Driver() throw ( ) {
  // Force containers to shutdown before we remove transport globals.
  OU::Parent<Container>::deleteChildren();
  //      if ( m_tpg_no_events ) delete m_tpg_no_events;
  //      if ( m_tpg_events ) delete m_tpg_events;
  ocpiCheck(pthread_key_delete(s_threadKey) == 0);
}

pthread_key_t Driver::s_threadKey;
// Register this driver
OC::RegisterContainerDriver<Driver> driver;

}
}
