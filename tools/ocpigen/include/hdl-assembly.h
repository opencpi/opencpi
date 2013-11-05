#ifndef HDL_ASSEMBLY_H
#define HDL_ASSEMBLY_H

#include "wip.h"
#include "OcpiUtilAssembly.h"
struct Attachment;
typedef std::list<Attachment*> Attachments;
typedef Attachments::const_iterator AttachmentsIter;

struct InstancePort;
struct Connection {
  OCPI::Util::Assembly::Connection *m_connection; // connection in the underlying generic assembly if there is one
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
struct Instance {
  OCPI::Util::Assembly::Instance *instance; // instance in the underlying generic assembly
  const char *name;
  const char *wName;
  Worker *worker;
  Clock **m_clocks;      // mapping of instance's clocks to assembly clocks
  InstancePort *m_ports;
  size_t index;      // index within container
  enum {
    Application, Interconnect, IO, Adapter
  } iType;
  const char *attach;  // external node port this worker is attached to for io or interconnect
  InstanceProperties properties;
  bool hasConfig;      // hack for adapter configuration FIXME make normal properties
  size_t config;
};
struct OcpAdapt {
  const char *expr;
  const char *comment;
  const char *signal;
  OcpSignalEnum other;
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
  OcpAdapt m_ocp[N_OCP_SIGNALS];       // Information for making the connection, perhaps tieoff etc.
  std::string m_signalIn, m_signalOut; // Internal signal bundle for connecting here, when appropriate
  InstancePort();
  InstancePort(Instance *i, Port *p, OU::Assembly::External *ext);
  const char *attach(Attachment *a, size_t index); //, size_t count);
  const char *createConnectionSignals(FILE *f, Language lang);
  void
    init(Instance *i, Port *p, OU::Assembly::External *ext),
    emitPortSignals(FILE *f, bool out, Language lang, const char *indent,
		    bool &any, const char *&comment, std::string &last),
    emitConnectionSignal(FILE *f, bool output, Language lang),
    connectOcpSignal(OcpSignalDesc &osd, OcpSignal &os, OcpAdapt &oa,
		     std::string &signal, std::string &thisComment, Language lang);
};
typedef std::list<Worker *> Workers;
typedef Workers::const_iterator WorkersIter;
class Assembly {
 public:
  Assembly(Worker &w);
  virtual ~Assembly();
  Worker &m_assyWorker;
  bool m_isContainer;
  bool m_isPlatform;
  Worker *m_outside;
  Workers m_workers;
  size_t m_nInstances;
  Instance *m_instances;
  size_t m_nConnections;
  Connections m_connections;
  OU::Assembly *m_utilAssembly;
  const char
    *parseAssy(ezxml_t xml, const char **topAttrs, const char **instAttrs, bool noWorkerOk),
    *externalizePort(InstancePort &ip, const char *name, size_t &ordinal),
    *findPort(OU::Assembly::Port &ap, InstancePort *&found),
    *parseConnection(OCPI::Util::Assembly::Connection &aConn);
  inline const char *myComment() { return m_assyWorker.myComment(); }
  // Find the instance port connected to an external with this name
  InstancePort *
  findInstancePort(const char *name);
  void
  emitAssyInstance(FILE *f, Instance *i, unsigned nControlInstances);
};

#endif
