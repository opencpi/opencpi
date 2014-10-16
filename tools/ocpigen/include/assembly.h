#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "OcpiUtilAssembly.h"
#include "wip.h"
#include "ocp.h"
#define INST_ATTRS "paramconfig"
//struct Attachment;
//typedef std::list<Attachment*> Attachments;
//typedef Attachments::const_iterator AttachmentsIter;

struct InstancePort;
struct Connection {
  std::string m_name;
  Attachments m_attachments;
  unsigned m_nExternals;
  Clock *m_clock;
  std::string m_masterName, m_slaveName;
  Attachment *m_external; // external assembly port - the last one
  size_t m_count; // width of all attachments
  Connection(OU::Assembly::Connection *c, const char *name = NULL);
  const char *attachPort(InstancePort &ip, size_t index = 0); //, size_t count = 0);
};
typedef std::list<Connection*> Connections;
typedef Connections::const_iterator ConnectionsIter;

struct InstanceProperty {
  OU::Property *property;
  OU::Value value;
  InstanceProperty();
};
typedef std::vector<InstanceProperty> InstanceProperties;

struct ExtTuple {
  Signal *signal;
  size_t index;
  std::string ext;
  bool single; // single signal in a vector
ExtTuple(Signal *signal, size_t index, const std::string &ext, bool single)
: signal(signal), index(index), ext(ext), single(single) {
  }
};
typedef std::list<ExtTuple> ExtMap_;
class ExtMap : public ExtMap_ {
 public:
  Signal *findSignal(const std::string &s, size_t &n) {
    for (ExtMap_::const_iterator i = begin(); i != end(); i++)
      if (!strcasecmp((*i).ext.c_str(), s.c_str())) {
	n = (*i).index;
	return (*i).signal;
      }
    return NULL;
  }
  const char *findSignal(Signal *s, size_t n, bool &isSingle) const {
    for (ExtMap_::const_iterator i = begin(); i != end(); i++)
      if ((*i).signal == s && (*i).index == n) {
	isSingle = (*i).single;
	return (*i).ext.c_str();
      }
    return NULL;
  }
  void push_back(Signal *s, size_t n, const std::string &e, bool single) {
    ExtMap_::push_back(ExtTuple(s, n, e, single));
  }
};
struct Instance {
  OCPI::Util::Assembly::Instance *instance; // instance in the underlying generic assembly
  const char *name;
  const char *wName;
  Worker *worker;
  Clock **m_clocks;      // mapping of instance's clocks to assembly clocks
  InstancePort *m_ports;
  size_t index;      // index within container
  // These types are roles of the instance rather than some hard attribute of the worker
  // They are also conveyed to the runtime in the artifact XML
  // They are more elaborated than worker types since they are only established as
  // workers are used in an assembly
  enum IType {
    Application,   // an application worker that an application can use as such
    Platform,      // a platform device worker
    Device,        // a device worker
    Configuration, // a platform configuration assembly
    Assembly,      // an application assembly
    Interconnect,  // an interconnect (device) worker acting as an attachment to an interconnect
    IO,            // a device worker that is not a platform worker
    Adapter,       // an adapter inserted by code generation
  } m_iType;
  const char *attach;  // external platform port this worker is attached to for io or interconnect
  InstanceProperties properties;
  bool hasConfig;      // hack for adapter configuration FIXME make normal properties
  size_t config;       // hack ditto
  ExtMap m_extmap;     // map for externals. FIXME: have HdlInstance class...
  Instance();
};
// To represent an attachment of a connection to an instance port.
// This is currently only used for indexed ports
struct Attachment {
  InstancePort           &m_instPort;   // which instance port
  Connection             &m_connection; // which connection
  size_t                  m_index;      // base of this attachment
  //  size_t                  m_count;      // width of this attachment, or zero if all
  Attachment(InstancePort &ip, Connection &c, size_t index); // , size_t count);
};

// A port of an instance can be connected more than once, hence
// Attachments - the list of connections attached to this port.
// There are reasons for multiple connections:
// Fan out: an output driving many inputs
// Indexed: different indices connected to different places.
struct InstancePort {
  Instance *m_instance;                // what instance does this belong to
  Attachments m_attachments;           // what connections are attached to this port?
  OU::Assembly::External *m_external;  // corresponding external of assy, for externals
  std::vector<bool> m_connected;       // to ensure indices are connected once
  Port *m_port;                        // The actual port of the instance's or assembly's worker
  OU::Assembly::Role m_role;           // Our role, combining info from the worker port and the assy
  OcpAdapt m_ocp[N_OCP_SIGNALS];       // Information for making the connection, perhaps tieoff etc.
  std::string m_signalIn, m_signalOut; // Internal signal bundle for connecting here, when appropriate
  InstancePort();
  InstancePort(Instance *i, Port *p, OU::Assembly::External *ext);
  const char *attach(Attachment *a, size_t index); //, size_t count);
  const char *createConnectionSignals(FILE *f, Language lang);
  void
    init(Instance *i, Port *p, OU::Assembly::External *ext),
    emitPortSignals(FILE *f, bool out, Language lang, const char *indent,
		    bool &any, std::string &comment, std::string &last),
    emitConnectionSignal(FILE *f, bool output, Language lang),
    connectOcpSignal(OcpSignalDesc &osd, OcpSignal &os, OcpAdapt &oa,
		     std::string &signal, std::string &thisComment, Language lang);
};
class Assembly {
 public:
  Assembly(Worker &w);
  virtual ~Assembly();
  Worker       &m_assyWorker;
  //  bool          m_isContainer; // FIXME: use class hierarchy to inherit
  //  bool          m_isPlatform;  // FIXME: use class hierarchy to inherit
  Workers       m_workers;
  size_t        m_nInstances;
  size_t        m_nWCIs;
  Instance     *m_instances;
  Connections   m_connections;
  OU::Assembly *m_utilAssembly;
  Language      m_language;
  const char
    *parseAssy(ezxml_t xml, const char **topAttrs, const char **instAttrs, bool noWorkerOk,
	       const char *outDir),
    *externalizePort(InstancePort &ip, const char *name, size_t &ordinal),
    *findPort(OU::Assembly::Port &ap, InstancePort *&found),
    *parseConnection(OCPI::Util::Assembly::Connection &aConn);
  // Find the instance port connected to an external with this name
  InstancePort *
  findInstancePort(const char *name);
  void
    emitAssyInstance(FILE *f, Instance *i); //, unsigned nControlInstances);
};

#endif
