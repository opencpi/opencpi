#include <cstring>
#include "OcpiOsMisc.h"
#include "OcpiOsEther.h"
#include "OcpiUtilValue.h"
#include "ContainerManager.h"
#include "RemoteLauncher.h"
#include "RemoteServer.h"
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
namespace OCPI {
  namespace Remote {

const uint16_t REMOTE_PORT = 17171;
const uint16_t REMOTE_NARGS = 6; // fields in the discovery entries
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
class Port : public OC::PortBase<Worker, Port, ExternalPort> {
  Port( Worker& w, const OU::Port & pmd, const OU::PValue *params)
    :  OC::PortBase< Worker, Port, ExternalPort>
       (w, *this, pmd, pmd.m_provider,
	(1 << OCPI::RDT::ActiveFlowControl) | (1 << OCPI::RDT::ActiveMessage), params) {
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
  Worker(Application & app, Artifact *art, const char *name,
	 ezxml_t impl, ezxml_t inst, OC::Worker *slave, bool hasMaster,
	 const OU::PValue *wParams, unsigned remoteInstance);
  virtual ~Worker() {}
  OC::Port &createPort(const OU::Port&, const OU::PValue */*params*/) {
    return *(OC::Port*)NULL;
  }
  OC::Port &createInputPort(OU::PortOrdinal /*portId*/,      
			    size_t /*bufferCount*/,
			    size_t /*bufferSize*/, 
			    const OU::PValue */*params*/ = NULL)
    throw (OU::EmbeddedException) {
    return *(OC::Port*)NULL;
  }
  OC::Port &createOutputPort(OU::PortOrdinal /*portId*/,
			     size_t /*bufferCount*/,
			     size_t /*bufferSize*/, 
			     const OU::PValue */*props*/ = NULL)
    throw ( OU::EmbeddedException ) {
    return *(OC::Port*)NULL;
  }
  void controlOperation(OU::Worker::ControlOperation op) {
    if (getControlMask() & (1 << op))
      m_launcher.controlOp(m_remoteInstance, op);
  }
  void setPropertyValue(const OU::Property &p, const OU::Value &v) {
    std::string val;
    v.unparse(val);
    m_launcher.setPropertyValue(m_remoteInstance, &p - m_properties, val);
  }
  bool wait(OS::Timer *t) {
    return m_launcher.wait(m_remoteInstance, t ? t->getRemaining() : 0);
  }
  void checkControlState() {
    setControlState(m_launcher.getState(m_remoteInstance));
  }
  // FIXME: this should be at the lower level for just reading the bytes remotely to enable caching propertly
  void getPropertyValue(const OU::Property &p, std::string &v, bool hex, bool add,
			bool /*uncached*/) {
    m_launcher.getPropertyValue(m_remoteInstance, &p - m_properties, v, hex, add);
  }

  void read(size_t /*offset*/, size_t /*nBytes*/, void */*p_data*/) {}
  void write(size_t /*offset*/, size_t /*nBytes*/, const void */*p_data*/ ) {}
  void setPropertyBytes(const OA::PropertyInfo &/*info*/, size_t /*offset*/,
			const uint8_t */*data*/, size_t /*nBytes*/, unsigned /*idx*/) const {};
  void setProperty8(const OA::PropertyInfo &/*info*/, uint8_t /*data*/,
		    unsigned /*idx*/) const {}
  void setProperty16(const OA::PropertyInfo &/*info*/, uint16_t /*data*/,
		     unsigned /*idx*/) const {}
  void setProperty32(const OA::PropertyInfo &/*info*/, uint32_t /*data*/,
		     unsigned /*idx*/) const {}
  void setProperty64(const OA::PropertyInfo &/*info*/, uint64_t /*data*/,
		     unsigned /*idx*/) const {}
  void getPropertyBytes(const OA::PropertyInfo &/*info*/, size_t /*offset*/,
			uint8_t */*data*/, size_t /*nBytes*/, unsigned /*idx*/, bool /*string*/)
    const {}
  uint8_t getProperty8(const OA::PropertyInfo &/*info*/, unsigned /*idx*/) const { return 0; }
  uint16_t getProperty16(const OA::PropertyInfo &/*info*/, unsigned /*idx*/) const { return 0; }
  uint32_t getProperty32(const OA::PropertyInfo &/*info*/, unsigned /*idx*/) const  { return 0; }
  uint64_t getProperty64(const OA::PropertyInfo &/*info*/, unsigned /*idx*/) const  { return 0; }

      
  void propertyWritten(unsigned /*ordinal*/) const {};
  void propertyRead(unsigned /*ordinal*/) const {};
  void prepareProperty(OU::Property&,
		       volatile void *&/*writeVaddr*/,
		       const volatile void *&/*readVaddr*/) {}
  // These property access methods are called when the fast path
  // is not enabled, either due to no MMIO or that the property can
  // return errors. 
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
  void set##pretty##Property(unsigned /*ordinal*/, const run /*val*/, unsigned /*idx*/) const {} \
  void set##pretty##SequenceProperty(const OA::Property &/*p*/,const run */*vals*/, \
				     size_t /*length*/) const {}
  // Set a string property value
  // ASSUMPTION:  strings always occupy at least 4 bytes, and
  // are aligned on 4 byte boundaries.  The offset calculations
  // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)      \
  void set##pretty##Property(unsigned /*ordinal*/, const run /*val*/, \
			     unsigned /*idx*/) const {}		      \
  void set##pretty##SequenceProperty(const OA::Property &/*p*/, const run */*vals*/, \
				     size_t /*length*/) const {}
  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
  // Get Scalar Property
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
  run get##pretty##Property(unsigned /*ordinal*/, unsigned /*idx*/) const { return 0; }	\
  unsigned get##pretty##SequenceProperty(const OA::Property &/*p*/,	\
					 run */*vals*/,			\
					 size_t /*length*/) const { return 0; }
  // ASSUMPTION:  strings always occupy at least 4 bytes, and
  // are aligned on 4 byte boundaries.  The offset calculations
  // and structure padding are assumed to do this.
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)	\
  void get##pretty##Property(unsigned /*ordinal*/, char */*cp*/,	\
			     size_t /*length*/, unsigned /*idx*/) const {} \
  unsigned get##pretty##SequenceProperty				\
  (const OA::Property &/*p*/, char **/*vals*/, size_t /*length*/, char */*buf*/, \
   size_t /*space*/) const { return 0; }

  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#undef OCPI_DATA_TYPE
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
};
class Application
  : public OC::ApplicationBase<Container,Application,Worker> {
  friend class Container;
  Application(Container &c, const char *name, const OU::PValue *params)
    : OC::ApplicationBase<Container,Application,Worker>(c, *this, name, params) {
  }
  virtual ~Application() {
  }
  OC::Worker &
  createWorker(OC::Artifact *art, const char *appInstName,
	       ezxml_t impl, ezxml_t inst, OC::Worker *slave, bool hasMaster,
	       const OU::PValue *wParams) {
    uint32_t remoteInstance;
    if (!OU::findULong(wParams, "remoteInstance", remoteInstance))
      throw OU::Error("Remote ContainerApplication expects remoteInstance parameter");
    return *new Worker(*this, art ? static_cast<Artifact*>(art) : NULL,
		       appInstName ? appInstName : "unnamed-worker", impl, inst, slave,
		       hasMaster, wParams, remoteInstance);
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
  Client(Driver &d, const char *name, OS::Socket &socket)
    : OU::Child<Driver,Client,remote>(d, *this, name),
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
  Container(Client &client, const std::string &name,
	    const char *model, const char *os, const char *osVersion, const char *platform,
	    const char *dynamic, const OA::PValue* /*params*/)
    throw ( OU::EmbeddedException )
    : OC::ContainerBase<Driver,Container,Application,Artifact>(*this, name.c_str()),
      m_client(client) {
    m_model = model;
    m_os = os;
    m_osVersion = osVersion;
    m_platform = platform;
    OX::parseBool(dynamic, NULL, &m_dynamic);
  }
  virtual ~Container()
  throw () {
  }
  OC::Launcher &launcher() const {
    return m_client;
  }
  OA::ContainerApplication*
  createApplication(const char *name, const OU::PValue *props)
    throw (OU::EmbeddedException) {
    return new Application(*this, name, props);
  }
  bool needThread() { return false; }
  OC::Artifact &
  createArtifact(OCPI::Library::Artifact &lart, const OA::PValue *artifactParams) {
    return *new Artifact(*this, lart, artifactParams);
  }
};

Worker::
Worker(Application & app, Artifact *art, const char *name,
       ezxml_t impl, ezxml_t inst, OC::Worker *slave, bool hasMaster, const OU::PValue *wParams,
       unsigned remoteInstance)
  : OC::WorkerBase<Application,Worker,Port>(app, *this, art, name, impl, inst, slave, hasMaster,
					    wParams),
    m_remoteInstance(remoteInstance),
    m_launcher(*static_cast<Launcher *>(&app.parent().launcher())) {
  setControlMask(getControlMask() | (OU::Worker::OpInitialize|
				     OU::Worker::OpStart|
				     OU::Worker::OpStop|
				     OU::Worker::OpRelease));
}

// The driver class owns the containers (like all container driver classes)
// and also owns the clients of those containers.
class Driver : public OC::DriverBase<Driver, Container, remote>,
	       public OU::Parent<Client> {
public:
  static pthread_key_t s_threadKey;
  Driver() throw() {
    ocpiCheck(pthread_key_create(&s_threadKey, NULL) == 0);
    ocpiDebug("Registering the Remote Container driver");
    g_probeServer = probeServer;
  }
  // Called either from UDP discovery or explicitly, e.g. from ocpirun
  // If the latter, the "containers" argument will be NULL
  bool
  probeServer(const char *server, bool /*verbose*/, const char **exclude, char *containers,
	      std::string &error) {
    ocpiDebug("probing remote container server: %s", server);
    error.clear();
    OS::Socket *sock = NULL;
    uint16_t port;
    std::string host(server);
    const char *sport = strchr(server, ':');
    if (sport) {
      const char *err;
      if ((err = OU::Value::parseUShort(sport + 1, NULL, port)))
	return OU::eformat(error, "Bad port number in server name: \"%s\"", server);
      host.resize(sport - server);
    } else
      port = REMOTE_PORT;
    ezxml_t rx = NULL; // need to explicitly free this below via "bad:" or "out:"
    std::vector<char> rbuf;
    bool taken = false; // whether the socket has been taken by a launcher
    Client *client = OU::Parent<Client>::findChildByName(server);
    if (!containers) {
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
    // Now process the discovery information per container - either from UDP discovery or from
    // explicit contact via TCP and xml
    for (char *cp = containers, *end; *cp; cp = end) {
      while (isspace(*cp)) cp++;
      if (!(end = strchr(cp, '\n')))
	goto bad;
      *end++ = '\0';
      char *args[REMOTE_NARGS + 1];
      for (char **ap = args; (*ap++ = strsep(&cp, "|")); )
	if ((ap - args) > REMOTE_NARGS)
	  goto bad;
      std::string cname;
      OU::format(cname, "remote:%s:%s", host.c_str(), args[0]);
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
      if (!client) {
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
      ocpiDebug("Creating remote container: \"%s\", model %s, os %s, version %s, platform %s dynamic %s",
		cname.c_str(), args[1], args[2], args[3], args[4], args[5]);
      Container &c = *new Container(*client, cname.c_str(), args[1], args[2], args[3], args[4],
				    args[5], NULL);
      (void)&c;
    }
    sock = NULL;
    return false;
  bad:
    OU::format(error, "Bad server container response from \"%s\"", server);
  out:
    if (sock && !taken)
      delete sock;
    if (rx)
      ezxml_free(rx);
    return !error.empty();
  }
  OC::Container *
  probeContainer(const char *which, std::string &/*error*/, const OA::PValue */*params*/) {
    throw OU::Error("Remote containers may only be discovered, not probed: \"%s\"", which);
  }
  // Try a discovery (send and receive) on a socket from an interface
  bool
  trySocket(std::set<std::string> &servers, OE::Interface &ifc, OE::Socket &s,
	    OE::Address &addr, bool /*discovery*/, const char **exclude, std::string &error) {
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
	  if (probeServer(response, false, exclude, cp, error))
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
  unsigned
  tryIface(std::set<std::string> &servers, OE::Interface &ifc, OE::Address &devAddr,
	   const char **exclude, bool discovery, std::string &error) {
    unsigned count = 0;
    OE::Interface i("udp", error);
    if (error.length())
      ocpiInfo("Could not open udp interface for discovery: %s", error.c_str());
    else {
      OE::Socket s(i, discovery ? ocpi_discovery : ocpi_master, NULL, 0, error);
      if (error.length())
	ocpiInfo("Could not open socket for udp discovery: %s", error.c_str());
      else {
	count = trySocket(servers, ifc, s, devAddr, discovery, exclude, error);
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
  unsigned
  search(const OA::PValue* props, const char **exclude, bool discoveryOnly)
    throw ( OU::EmbeddedException ) {
    if (g_suppressRemoteDiscovery)
      return 0;
    std::string error;
    unsigned count = 0;
    OE::IfScanner ifs(error);
    if (error.size())
      return 0;
    const char *ifName = NULL;
    OU::findString(props, "interface", ifName);
    OE::Interface eif;
    ocpiDebug("Searching for container servers in interfaces");
    std::set<std::string> servers;
    while (ifs.getNext(eif, error, ifName)) {
      if (eif.name == "udp") // the udp pseudo interface is not used for discovery
	continue;
      ocpiDebug("RemoteDriver: Considering interface \"%s\", addr 0x%x",
		eif.name.c_str(), eif.ipAddr.addrInAddr());
      if (eif.up && eif.connected && eif.ipAddr.addrInAddr()) {
	OE::Address bcast(true, REMOTE_PORT);
	count += tryIface(servers, eif, bcast, exclude, discoveryOnly, error);
	if (error.size()) {
	  ocpiDebug("Error during container server discovery on '%s': %s",
		    eif.name.c_str(), error.c_str());
	  error.clear();
	}
      } else
	ocpiDebug("In RemoteDriver.cxx Interface '%s' is %s and %s",
		  eif.name.c_str(), eif.up ? "up" : "down",
		  eif.connected ? "connected" : "not connected");
      if (ifName)
	break;
    }
    if (error.size())
      ocpiInfo("Error during container server discovery on '%s': %s",
	       eif.name.c_str(), error.c_str());
    return count;
  }
  ~Driver() throw ( ) {
    // Force containers to shutdown before we remove transport globals.
    OU::Parent<Container>::deleteChildren();
    //      if ( m_tpg_no_events ) delete m_tpg_no_events;
    //      if ( m_tpg_events ) delete m_tpg_events;
    ocpiCheck(pthread_key_delete(s_threadKey) == 0);
  }
  static bool
  probeServer(const char *server, bool verbose, const char **exclude, std::string &error) {
    return Driver::getSingleton().probeServer(server, verbose, exclude, NULL, error);
  }

};

pthread_key_t Driver::s_threadKey;
// Register this driver
OC::RegisterContainerDriver<Driver> driver;
  }
}
