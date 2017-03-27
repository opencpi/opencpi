#define __STDC_LIMIT_MACROS
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ocpirh_proxy_base.h"
#include "OcpiApi.hh"
#include "OcpiOsFileSystem.h"
#include "ContainerPort.h"
#include "OcpiApplication.h"

namespace OA = OCPI::API;
namespace OC = OCPI::Container;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;

// Our port base class that services the data flow between OpenCPI ports and RH ports
struct ocpirh_proxy_base::PortBase {
  OA::ExternalPort &m_ocpiPort;
  bool m_in;
  OA::ExternalBuffer *m_buffer;
  uint8_t m_opcode, *m_data;
  size_t  m_length, m_maxLength;
  bool m_end;
  ::PortBase *m_port; // the base class pointer for use with addPort

  PortBase(OCPI::API::ExternalPort &port, bool in)
    : m_ocpiPort(port), m_in(in), m_buffer(NULL), m_opcode(0), m_data(NULL), m_length(0),
      m_maxLength(0), m_end(false), m_port(NULL)
  {}
  virtual ~PortBase() {}
  bool service() {
    bool anyWork = false;
    for (;;) {
      // Attempt to get an OpenCPI buffer for sending data to the app if we don't have one
      for (unsigned count = 0; !m_buffer && count < 100; count++)
	m_buffer = m_in ?
	  m_ocpiPort.getBuffer(m_data, m_maxLength) :
	  m_ocpiPort.getBuffer(m_data, m_length, m_opcode, m_end);
      if (m_buffer && (m_in ? 
		       rh2Ocpi(m_data, m_maxLength, m_length, m_opcode, m_end) :
		       ocpi2Rh(m_data, m_length, m_opcode, m_end))) {
	m_in ? m_buffer->put(m_length, m_opcode, m_end) : m_buffer->release();
	if (m_in)
	  ocpiDebug("Input from RH, putting %zu", m_length);
	else
	  ocpiDebug("Output to RH, got %zu from worker", m_length);
	m_buffer = NULL;
	anyWork = true;
      } else
	break;
    }
    return anyWork;
  }
  virtual bool
  rh2Ocpi(uint8_t *data, size_t maxLen, size_t &length, uint8_t &opcode, bool &end) = 0;
  virtual bool
  ocpi2Rh(const uint8_t *data, size_t length, uint8_t opcode, bool end) = 0;
};

struct ocpirh_proxy_base::PropertyBase : public OA::Property {
  PropertyBase(const OA::Application &app, const std::string &name)
    : OA::Property(app, name.c_str()) {
  }
  // Get the binary/parsed value (inefficiently).
  void getValue(OU::Value &v) {
    std::string s, n;
    bool unreadable;
    m_worker.getProperty(m_ordinal, n, s, &unreadable);
    v.setType(m_info);
    const char *err;
    if (!unreadable && (err = v.parse(s.c_str())))
      throw OU::Error("Unexpectedly bad property value \"%s\": %s", s.c_str(), err);
  }
  void setStringValue(const std::string &s) {
    setStringValue(s.c_str());
  }
};

// These are not supported yet.  These are minimal fakes for compile time
namespace bulkio {
  struct InBooleanPort : public InULongPort {
    InBooleanPort(std::string &name) : InULongPort(name) {}
  };
  struct OutBooleanPort : public OutULongPort {
    OutBooleanPort(std::string &name) : OutULongPort(name) {}
  };
}

namespace {
  // These are aliases to allow us to name these whatever we want.
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
  typedef bulkio::InURLPort In##corba##Port; \
  typedef bulkio::OutURLPort Out##corba##Port;
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
  typedef bulkio::In##corba##Port In##corba##Port; \
  typedef bulkio::Out##corba##Port Out##corba##Port;

OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store) \
	OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)

  inline void
  getCurrentTimestamp(BULKIO::PrecisionUTCTime &tstamp) {
    struct timeval tmp_time;
    struct timezone tmp_tz;
    gettimeofday(&tmp_time, &tmp_tz);
    double wsec = tmp_time.tv_sec;
    double fsec = tmp_time.tv_usec / 1e6;
    tstamp.tcmode = BULKIO::TCM_CPU;
    tstamp.tcstatus = (short) 1;
    tstamp.toff = 0.0;
    tstamp.twsec = wsec;
    tstamp.tfsec = fsec;
  }

  // Generic typed port, input or output. Constructs, holds, and adds the typed rh port
  template <typename RH_PORT_TYPE> struct Port : public ocpirh_proxy_base::PortBase {
    RH_PORT_TYPE &m_rhPort;
    Port(std::string &name, OA::ExternalPort &ocpiPort, ocpirh_proxy_base &app, bool in)
      : PortBase(ocpiPort, in), m_rhPort(*new RH_PORT_TYPE(name)) {
      m_port = &m_rhPort;
    }
  };

  // Input to the app: RH input port
  template <typename RH_PORT_TYPE> struct Rh2OcpiPort : public Port<RH_PORT_TYPE> {
    Rh2OcpiPort(std::string &name, OA::ExternalPort &ocpiPort, ocpirh_proxy_base &app)
      : Port<RH_PORT_TYPE>(name, ocpiPort, app, true) {}

    bool rh2Ocpi(uint8_t *data, size_t maxLength, size_t &length, uint8_t &opcode, bool &end) {
      typedef typename RH_PORT_TYPE::DataTransferType PacketType;
      PacketType *packet = Port<RH_PORT_TYPE>::m_rhPort.getPacket(0);
      if (packet == NULL)
	return false;
      length =  packet->dataBuffer.size() < maxLength ? packet->dataBuffer.size() : maxLength;
      opcode = 0;
      end = false; // someday look at EOS
      memcpy(data, &packet->dataBuffer[0], length);
      delete packet;
      return true;
    }
    bool ocpi2Rh(const uint8_t *data, size_t length, uint8_t opcode, bool end) {
      assert("input/output mismatch" == 0);
    }
  };
  // Output from the app: RH output port
  template <class RH_PORT_TYPE> struct Ocpi2RhPort : public Port<RH_PORT_TYPE> {
    Ocpi2RhPort(std::string &name, OA::ExternalPort &ocpiPort, ocpirh_proxy_base &app)
      : Port<RH_PORT_TYPE>(name, ocpiPort, app, false) {}

    bool ocpi2Rh(const uint8_t *data, size_t length, uint8_t opcode, bool end) {
      typedef typename RH_PORT_TYPE::NativeType DataType;
      assert(length % sizeof(DataType) == 0);
      BULKIO::PrecisionUTCTime ts;
      getCurrentTimestamp(ts);
      std::string dir("Out");
      // Why is this NS qualifier necessary here????
      Port<RH_PORT_TYPE>::m_rhPort.pushPacket((const DataType *)data, length/sizeof(DataType),
					      ts, end, dir);
      return true; // no conditionality here
    }
    bool rh2Ocpi(uint8_t *data, size_t maxLen, size_t &length, uint8_t &opcode, bool &end) {
      assert("input/output mismatch" == 0);
    }
  };

// Map from OpenCPI scalar types to RH types
#define RHTYPE_UChar unsigned char
#define RHTYPE_Bool bool
#define RHTYPE_Char char
#define RHTYPE_Short short
#define RHTYPE_UShort unsigned short
#define RHTYPE_Long CORBA::Long
#define RHTYPE_ULong CORBA::ULong
#define RHTYPE_LongLong CORBA::LongLong
#define RHTYPE_ULongLong CORBA::ULongLong
#define RHTYPE_Float float
#define RHTYPE_Double double
#define RHTYPE_String std::string

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
  struct Ocpi##pretty##Property : public ocpirh_proxy_base::PropertyBase { \
    RHTYPE_##pretty m_rhValue;						\
    Ocpi##pretty##Property(const OA::Application &app, const std::string &name) \
    : ocpirh_proxy_base::PropertyBase(app, name) {			\
    }									\
    RHTYPE_##pretty getValue() {					\
      OU::Value v;							\
      ocpirh_proxy_base::PropertyBase::getValue(v);			\
      return v.m_##pretty;						\
    }									\
  };

OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE
}

PREPARE_LOGGING(ocpirh_proxy_base)
ocpirh_proxy_base::
ocpirh_proxy_base(const char *uuid, const char *label)
: Component(uuid, label), m_application(NULL), m_started(false)
{
  try {
    loadProperties();
  } catch (std::string &e) {
    fprintf(stderr, "OpenCPI Exception: %s\n", e.c_str());
    throw;
  } catch (const char *e) {
    fprintf(stderr, "OpenCPI Exception: %s\n", e);
    throw;
  }
}

ocpirh_proxy_base::~ocpirh_proxy_base() {
  for (unsigned n = 0; n < m_ports.size(); n++)
    delete m_ports[n];
  for (unsigned n = 0; n < m_properties.size(); n++)
    delete m_properties[n];
  delete m_application;
}

// Delegated from the top level class, called from Resource_impl::initialize in try/catch
void ocpirh_proxy_base::
constructor() {
  printf("ocpirh_proxy_base constructor\n");
  try {
    m_application->initialize();
    const OA::ApplicationI &app = m_application->applicationI();
    m_properties.resize(app.nProperties());
    std::string name;
    const OU::Property *p;
    for (unsigned n = 0; (p = app.property(n, name)); n++) {
      std::string mode(p->m_isWritable ? "readwrite" : "readonly");
      switch (p->m_baseType) {

#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	case OA::OCPI_##pretty: {					\
	  typedef Ocpi##pretty##Property P;				\
          P &p = *new P(*m_application, name);				\
	  OU::Value v;							\
	  p.ocpirh_proxy_base::PropertyBase::getValue(v);		\
          RHTYPE_##pretty init = v.m_##pretty;			        \
	  std::string s;						\
	  v.unparse(s);							\
	  ocpiInfo("Registering prop %s with '%s'", name.c_str(), s.c_str()); \
	  /* Tell RH the property exists, and provide an initial value */ \
	  addProperty(p.m_rhValue, init, name, name, mode, "", "external", "configure"); \
	  /* Tell RH how to set the property's value */			\
	  if (p.m_info.m_isWritable && !p.m_info.m_isInitial)		\
	    setPropertyConfigureImpl(p.m_rhValue, &p, &P::set##pretty##Value); \
	  /* Tell RH how to get the property's value */			\
	  if (p.m_info.m_isVolatile)						\
	    /* This is a template function that sets the callback to read the property */ \
	    /* It associates the property by looking at the address of the value, which was */ \
	    /* registered by the value reference passed to add property */ \
	    setPropertyQueryImpl(p.m_rhValue, &p, &P::getValue);	\
	  m_properties[n] = &p;						\
	  break;							\
      }
OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE

      default:
	fprintf(stderr, "Unsupported property type: %s(%u) for property %s\n",
		OU::baseTypeNames[p->m_baseType], p->m_baseType, name.c_str());
      }
    }
    // Initialize our port objects that connect the external ports of the OpenCPI app to the
    // corresponding RH ports of this proxy component
    m_ports.resize(m_application->getPortCount());
#if 0
    - initial values
      - sequence, struct, struct sequence  
      - individual worker properties (as option to ocpisca)
#endif
      for (unsigned n = 0; n < m_ports.size(); n++) {
	std::string name;
	OA::ExternalPort &ocpiPort = m_application->getPort(n, name);
	// Here we use internal/non-API types to get the protocol metadata
	const OU::Port &metaPort =
	  static_cast<OCPI::Container::ExternalPort*>(&ocpiPort)->metaPort();
	assert(metaPort.nOperations() == 1 && metaPort.operations()[0].nArgs() == 1);
	OU::Member &arg = metaPort.operations()[0].args()[0];
	assert((arg.m_isSequence || arg.m_arrayRank) && arg.m_baseType != OA::OCPI_Struct &&
	       arg.m_baseType != OA::OCPI_Type && arg.m_baseType != OA::OCPI_String);
	PortBase *p;
	switch (arg.m_baseType) {

#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	  case OA::OCPI_##pretty:					\
	    if (metaPort.m_provider)					\
	      p = new Rh2OcpiPort<In##corba##Port>(name, ocpiPort, *this); \
	    else							\
	      p = new Ocpi2RhPort<Out##corba##Port>(name, ocpiPort, *this); \
	    break;

	  OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE

	default:
	  assert("unsupported port type" == 0);
	}
	addPort(name, "OpenCPI Application Port", p->m_port);
	m_ports[n] = p;
      }
  } catch (std::string &e) {
    fprintf(stderr, "OpenCPI exception during application initialization: %s\n", e.c_str());
    throw;
  } catch (const char *e) {
    fprintf(stderr, "OpenCPI Exception: %s\n", e);
    throw;
  }
}


int ocpirh_proxy_base::
serviceFunction()
{
  bool any = false;
  for (unsigned n = 0; n < m_ports.size(); n++)
    any = any || m_ports[n]->service();
  return any ? NORMAL : NOOP;
}

/*******************************************************************************************
    Framework-level functions
    These functions are generally called by the framework to perform housekeeping.
*******************************************************************************************/
void ocpirh_proxy_base::start() throw (CORBA::SystemException, CF::Resource::StartError) {
  ocpiInfo("Start method enter");
  if (m_started)
    return;
  m_started = true;
  m_application->start();

  // These two lines are from the normally generated base class
  Component::start();
  ThreadedComponent::startThread();
  ocpiInfo("Start method exit");
}

void ocpirh_proxy_base::stop() throw (CORBA::SystemException, CF::Resource::StopError)
{
  if (!m_started)
    return;
  m_started = false;
  m_application->stop();

  // These lines are from the normally generated base class
  Component::stop();
#if 0
    if (!ThreadedComponent::stopThread()) {
        throw CF::Resource::StopError(CF::CF_NOTSET, "Processing thread did not die");
    }
#else
  ThreadedComponent::stopThread();
#endif
}

void ocpirh_proxy_base::
releaseObject() 
  throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{
    // This function clears the device running condition so main shuts down everything
    try {
        stop();
    } catch (CF::Resource::StopError& ex) {
        // TODO - this should probably be logged instead of ignored
    }
    Component::releaseObject();
}

void ocpirh_proxy_base::
loadProperties() {
  // This is called too early so we add our properties later in the constructor method.
  // which is called by the "initialize" CORBA operation
  // We assume we are entered at the top level of the RPM installation directory
  // We use the environment for the app name to leave the main program args standard for rh
  const char *appname = getenv("OCPI_RH_APP");
  assert(appname);
  char *cwd = getcwd(NULL, 0);
  assert(cwd);
  std::string s("OCPI_LIBRARY_PATH=");
  s += cwd;
  s += "/artifacts";
  putenv(strdup(s.c_str()));   // putenv takes ownership of the string
  fprintf(stderr, "posix cwd: '%s' app: '%s'\n", cwd, appname);
  s = cwd;
  s += "/runs/run";
  char date[100];
  time_t now = time(NULL);
  struct tm nowtm;
  localtime_r(&now, &nowtm);
  strftime(date, sizeof(date), ".%Y%m%d%H%M%S", &nowtm);
  s += date;
  s += ".XXXXXX";
  char *b = strdup(s.c_str());
  if (mkdtemp(b) == NULL)
    throw OU::Error("Can't create temporary run directory for %s: %s (%s)", appname, b,
		    strerror(errno));
  if (chdir(b) || chmod(".", 0755))
    throw OU::Error("Can't enter run directory for %s: %s (%s)", appname, b,
		    strerror(errno));
  // Put in symlinks for all the data files.
  for (OS::FileIterator iter("../../data", "*"); !iter.end(); iter.next())
    if (symlink(("../../data/" + iter.relativeName()).c_str(), iter.relativeName().c_str()))
      throw OU::Error("Can't symlink data file \"%s\" in run directory %s: %s",
		      iter.relativeName().c_str(), b, strerror(errno));
  free(b);
  s = cwd;
  s += "/";
  s += appname;
  fprintf(stderr, "Creating the %s application running in %s\n", appname, b);
  m_application = new OA::Application(s.c_str());
  // That's all we can do until we initialize the application, which we will do in the RH
  // constructor
}
