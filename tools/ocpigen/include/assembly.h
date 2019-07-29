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

#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include "OcpiUtilAssembly.h"
#include "wip.h"
#include "ocp.h"
#include "hdl.h"
#define INST_ATTRS "paramconfig"

struct InstancePort;
struct Connection {
  std::string m_name;
  Attachments m_attachments;
  unsigned m_nExternals;
  Clock *m_clock;
  std::string
    m_masterName, // the signal/bundle name for the output of internal masters
    m_slaveName,  // the signal/bundle name for the output of internal slaves
    m_clockName;  // the signal to connect to internal clocks
  Attachment *m_external; // external assembly port - the last one
  size_t m_count; // width of all attachments
  Connection(OU::Assembly::Connection *c, const char *name = NULL);
  const char *attachPort(InstancePort &ip, size_t index = 0); //, size_t count = 0);
  bool setClock(Clock &c);
  const char *cname() const { return m_name.c_str(); }
};
typedef std::list<Connection*> Connections;
typedef Connections::const_iterator ConnectionsIter;

struct InstanceProperty {
  const OU::Property *property;
  OU::Value value;
  InstanceProperty();
};
typedef std::vector<InstanceProperty> InstanceProperties;

class Assembly;
struct Instance : public OU::IdentResolver {
  //  OCPI::Util::Assembly::Instance *m_instance; // instance in the underlying generic assembly
  ezxml_t     m_xml;
  std::string m_name;
  std::string m_wName;
  Worker       *m_worker;
  Clock       **m_clocks; // mapping of instance's clocks to assembly clocks
  std::vector<InstancePort> m_ports;
  size_t        m_index;  // index within container
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
  const char *m_attach;  // external platform port instance is attached to for io or interconnect
  OCPI::Util::Assembly::Properties m_xmlProperties; // explicit unparsed values for the instance
  InstanceProperties m_properties;                  // fully parsed w/ full knowledge of worker
  bool m_hasConfig;      // for adapter configuration FIXME make normal properties
  size_t m_config;
  ExtMap m_extmap;     // map for externals. FIXME: have HdlInstance class...
  bool   m_emulated;   // is this an instance of a device worker with an emulator?
  bool   m_inserted;   // was this instance auto-inserted?
  Instance();
  const char *cname() const { return m_name.c_str(); }
  const char *init(OCPI::Util::Assembly::Instance *ai, ::Assembly &assy, const char *outDir);
  const char *init(::Assembly &assy, const char *iName, const char *wName, ezxml_t x, 
		   OU::Assembly::Properties &xmlProperties);
  const char *initHDL(::Assembly &assy);
  void emitHdl(FILE *f, const char *prefix, size_t &index);
  void emitDeviceConnectionSignals(FILE *f, Worker &assy);
  const char *getValue(const char *sym, OU::ExprValue &val) const;
};
// To represent an attachment of a connection to an instance port.
// This is currently only used for indexed ports
struct Attachment {
  InstancePort           &m_instPort;   // which instance port
  Connection             &m_connection; // which connection
  size_t                  m_index;      // base of this attachment
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
  bool     m_hasExprs;                 // any signal adaptations with expressions present?
  bool     m_externalize;              // should be made external to the assembly
  std::string m_signalIn, m_signalOut; // Internal signal bundle for connecting here, when appropriate
  std::string m_clockSignal;           // Internal clock signal, when appropriate
  InstancePort();
  InstancePort(Instance *i, Port *p, OU::Assembly::External *ext);
  const char *attach(Attachment *a, size_t index); //, size_t count);
  const char *adjustConnections(FILE *f, Language lang, size_t &unused);
  void
    getClockSignal(bool output, Language lang),
    createConnectionSignals(FILE *f, Language lang),
    init(Instance *i, Port *p, OU::Assembly::External *ext),
    detach(Connection &c), // forget attachment for this connection
    emitConnectionSignal(FILE *f, bool output, Language lang, bool clock = false),
    emitTieoffAssignments(FILE *f);
};
class Assembly {
 public:
  Assembly(Worker &w);
  virtual ~Assembly();
  Worker       &m_assyWorker;
  //  bool          m_isContainer; // FIXME: use class hierarchy to inherit
  //  bool          m_isPlatform;  // FIXME: use class hierarchy to inherit
  Workers       m_workers;
  size_t        m_nWCIs;
  std::vector<Instance>m_instances;
  Connections   m_connections;
  OU::Assembly *m_utilAssembly;
  Language      m_language;
  InstanceProperties m_properties; // property values applied to the whole assembly
  size_t        m_nWti, m_nWmemi;
  const char
    *parseAssy(ezxml_t xml, const char **topAttrs, const char **instAttrs, bool noWorkerOk),
    *externalizePort(InstancePort &ip, const char *name, size_t *ordinal),
    *findPort(OU::Assembly::Port &ap, InstancePort *&found),
    // Add the assembly's parameters to the instance's parameter values list, as needed
    *addAssemblyParameters(OCPI::Util::Assembly::Properties &aips),
    *addInstanceParameters(const Worker &w, const OU::Assembly::Properties &aiprops,
			   InstanceProperty *&ipv),
    *insertAdapter(Connection &c, InstancePort &from, InstancePort &to),
    *parseConnection(OCPI::Util::Assembly::Connection &aConn);
void addParamConfigParameters(const ParamConfig &pc, const OU::Assembly::Properties &aiprops,
			      InstanceProperty *&ipv);
  // Find the instance port connected to an external with this name
  InstancePort *
  findInstancePort(const char *name);
  void
    propagateClocks(),
    emitAssyInstance(FILE *f, Instance *i); //, unsigned nControlInstances);
};

#endif
