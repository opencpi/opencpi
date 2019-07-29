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

#ifndef WIP_H
#define WIP_H
#include <stdint.h>
#include <cstring>
#include <vector>
#include <list>
#include <map>
#include "OcpiPValue.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilProtocol.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilWorker.h"
#include "OcpiUtilAssembly.h"
#include "OcpiUuid.h"
#include "ezxml.h"
#include "cdkutils.h"
#include "parameters.h"
#include "port.h"
#include "ocp.h"
#include "clock.h"

namespace OE=OCPI::Util::EzXml;
namespace OU=OCPI::Util;
namespace OA=OCPI::API;
namespace OS=OCPI::OS;

class Port;

#if 0 // this is handled differently now in data.cxx
// We derive a class to implement xi:include parsing, file names, etc.
class Protocol : public OU::Protocol {
public:
  Protocol(Port &port);
  Port &m_port;
  const char *parse(const char *file, ezxml_t prot = NULL);
  const char *parseOperation(ezxml_t op);
};
#endif
class Worker;

class WciPort : public OcpPort {
  size_t m_timeout;
  bool m_resetWhileSuspended;
 public:
  WciPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  inline const char *prefix() const { return "wci"; }
  inline const char *typeName() const { return "WCI"; }
  bool needsControlClock() const;
  bool haveWorkerOutputs() const { return true; }
  void emitPortDescription(FILE *f, Language lang) const;
  const char *deriveOCP();
  size_t decodeWidth() const { return ocp.MAddr.width; }
  size_t timeout() const { return m_timeout; }
  //  void setTimeout(size_t to) { m_timeout = to; } // because it is parsed outside the port xml
  bool resetWhileSuspended() const { return m_resetWhileSuspended; }
  // This is needed at least for assembly synthesis of these ports
  void setResetWhileSuspended(bool rws) { m_resetWhileSuspended =  rws; }
  void emitImplAliases(FILE *f, unsigned n, Language lang);
  void emitImplSignals(FILE *f);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  //  void emitRecordInterfaceConstants(FILE *f);
  //  void emitVerilogPortParameters(FILE *f);
  //  void emitWorkerEntitySignals(FILE *f, std::string &last, unsigned maxPropName);
  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inRecord,
			bool inPackage, bool inWorker, const char *defaultIn,
			const char *defaultOut);
  void emitRecordArray(FILE *f);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
#if 1
  void emitPortSignals(FILE *f, const InstancePort &ip, Language lang, const char *indent, bool &any,
		       std::string &comment, std::string &last, const char *myComment, std::string &exprs);
#else
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt,
		       std::string *signalIn, std::string &clockSignal, std::string &exprs);
#endif
  // void emitInterfaceConstants(FILE *f, Language lang);
  const char *finalizeExternal(Worker &aw, Worker &iw, InstancePort &ip,
			       bool &cantDataResetWhileSuspended);
  void emitSkelSignals(FILE *f);
};

class WmemiPort : public OcpPort {
  bool m_writeDataFlowControl, m_readDataFlowControl;
  uint64_t m_memoryWords;
  size_t m_maxBurstLength;
 public:
  WmemiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err)
    const;
  inline const char *prefix() const { return "mem"; }
  inline const char *typeName() const { return "WMemI"; }
  const char *deriveOCP();
  void emitPortDescription(FILE *f, Language lang) const;
  const char *finalizeExternal(Worker &aw, Worker &iw, InstancePort &ip,
			       bool &cantDataResetWhileSuspended);
};
class WtiPort : public OcpPort {
  size_t m_secondsWidth, m_fractionWidth;
  bool m_allowUnavailable;
  std::string m_secondsWidthExpr, m_fractionWidthExpr;
  WtiPort(const WtiPort &other, Worker &w, std::string &name, const char *&err);
 public:
  WtiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *typeName() const { return "WTI"; }
  inline const char *prefix() const { return "wti"; }
  const char *deriveOCP();
  void emitVhdlShell(FILE *f, Port *wci);
  void emitImplSignals(FILE *f);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
  void emitPortDescription(FILE *f, Language lang) const;
  void emitRecordInterfaceConstants(FILE *f);
  void emitInterfaceConstants(FILE *f, Language lang);
  const char *resolveExpressions(OCPI::Util::IdentResolver &ir);
  const char *finalizeExternal(Worker &aw, Worker &iw, InstancePort &ip,
			       bool &cantDataResetWhileSuspended);
};
class CpPort : public Port {
  CpPort(const CpPort &other, Worker &w , std::string &name, size_t count, const char *&err);
 public:
  CpPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err)
    const;
  inline const char *prefix() const { return "cp"; }
  inline const char *typeName() const { return "CPMaster"; }
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
};
class NocPort : public Port {
  NocPort(const NocPort &other, Worker &w , std::string &name, size_t count,
	  const char *&err);
 public:
  NocPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "noc"; }
  inline const char *typeName() const { return "uNoc"; }
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
};
class SdpPort : public Port {
  SdpPort(const SdpPort &other, Worker &w , std::string &name, size_t count,
	  const char *&err);
 public:
  SdpPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "sdp"; }
  inline const char *typeName() const { return "SDP"; }
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitRecordInterfaceConstants(FILE *f);
  void emitInterfaceConstants(FILE *f, Language lang);
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inRecord,
			bool inPackage, bool inWorker, const char *defaultIn,
			const char *defaultOut);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitPortSignal(FILE *f, bool any, const char *indent, const std::string &fName,
		      const std::string &aname, const std::string &index, bool output,
		      const Port *signalPort, bool external);
};
class MetaDataPort : public Port {
  MetaDataPort(const MetaDataPort &other, Worker &w , std::string &name, size_t count,
		  const char *&err);
 public:
  MetaDataPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "metadata"; }
  inline const char *typeName() const { return "Metadata"; }
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
};
class TimeServicePort : public Port {
  TimeServicePort(const TimeServicePort &other, Worker &w , std::string &name, size_t count,
		  const char *&err);
 public:
  TimeServicePort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "time"; }
  inline const char *typeName() const { return "TimeService"; }
  void emitRecordTypes(FILE *f);
  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inRecord,
			bool inPackage, bool inWorker,
			const char *defaultIn, const char *defaultOut);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitVHDLSignalWrapperPortMap(FILE *f, std::string &last);
#if 1
  void emitPortSignals(FILE *f, const InstancePort &ip, Language lang, const char *indent, bool &any,
		       std::string &comment, std::string &last, const char *myComment, std::string &exprs);
#else
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt,
		       std::string *signalIn, std::string &clockSignal, std::string &exprs);
#endif
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
};
class TimeBasePort : public Port {
  TimeBasePort(const TimeBasePort &other, Worker &w , std::string &name, size_t count,
		  const char *&err);
 public:
  TimeBasePort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "timebase"; }
  inline const char *typeName() const { return "TimeBase"; }
  void emitRecordTypes(FILE *f);
  //  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inWorker,
  //			const char *defaultIn, const char *defaultOut);
  void emitRecordInterface(FILE *f, const char *implName);
  //  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitVHDLSignalWrapperPortMap(FILE *f, std::string &last);
#if 0
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt,
		       std::string *signalIn, std::string &clockSignal, std::string &exprs);
#endif
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
};
class RawPropPort : public Port {
 public:
  RawPropPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  RawPropPort(const RawPropPort &other, Worker &w, std::string &name, size_t count,
	      const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "rawprop"; }
  inline const char *typeName() const { return "RawProperty"; }
  bool needsControlClock() const { return true; }
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
  const char *masterMissing() const;
  const char *slaveMissing() const;
};
// The port for inter-device connections
class DevSignalsPort : public Port {
  Signals m_signals;
  SigMap m_sigmap;
  bool m_hasInputs;
  bool m_hasOutputs;
 public:
  DevSignalsPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  DevSignalsPort(const DevSignalsPort &other, Worker &w, std::string &name, size_t count,
		 const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  inline const char *prefix() const { return "ds"; }
  inline const char *typeName() const { return "DevSignals"; }
  bool haveInputs() const { return m_hasInputs; }
  bool haveWorkerInputs() const { return haveInputs(); }
  bool haveOutputs() const { return m_hasOutputs; }
  bool haveWorkerOutputs() const { return haveOutputs(); }
  void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
  void emitPortSignalsDir(FILE *f, bool output, const char *indent, bool &any,
			  std::string &comment, std::string &last, Attachment *other);
#if 1
  void emitPortSignals(FILE *f, const InstancePort &ip, Language lang, const char *indent, bool &any,
		       std::string &comment, std::string &last, const char *myComment, std::string &exprs);
#else
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt,
		       std::string *signalIn, std::string &clockSignal, std::string &exprs);
#endif
  void emitExtAssignment(FILE *f, bool int2ext, const std::string &extName,
			 const std::string &intName, const Attachment &extAt,
			 const Attachment &intAt, size_t count) const;
};

class LocalMemory {
  public:
    LocalMemory ( )
      : name ( 0 ),
        sizeOfLocalMemory ( 0 )
    {
      // Empty
    }
    const char* name;
    size_t sizeOfLocalMemory;
};

typedef std::list<OU::Property *> Properties;
typedef Properties::const_iterator PropertiesIter;
class Control {
 public:
  uint64_t sizeOfConfigSpace;
  uint32_t controlOps; // bit mask
  Properties properties;
  size_t offset;// temporary while properties are being parsed.
  unsigned ordinal; // ditto
  OU::Property *firstRaw;
  // Scalability
  bool startBarrier;      // Must there be a start barrier among members?
  // Below here, initialization is in initAccess
  bool writables, nonRawWritables, rawWritables;
  bool readables, nonRawReadables, rawReadables; // readables does NOT include parameters
  bool sub32Bits, nonRawSub32Bits;
  bool volatiles, nonRawVolatiles;
  bool nonRawReadbacks, rawReadbacks, builtinReadbacks;
  bool rawProperties;
  unsigned nRunProperties, nNonRawRunProperties, nParameters;
  Control();
  void initAccess();
  void summarizeAccess(OU::Property &p, bool isSpecProperty = false);
};

enum Endian {
  NoEndian, // unspecified
  Neutral,  // doesn't have any relevant functionality, can be used anywhere
  Big,
  Little,
  Static,   // can be provided with a parameter
  Dynamic   // can be provided with an MFLAG
};
#define ENDIANS "none", "neutral", "big", "little", "static", "dynamic"

#define PARSED_ATTRS "name"

// This class represents a connection to a required worker

typedef std::pair<std::string, std::string> StringPair;
class Assembly;
class HdlDevice;
class DataPort;
struct Instance;
class Worker : public OU::Worker {
 public:
  ezxml_t m_xml;
  std::string m_file, m_parentFile, m_fileName;
  Model m_model;
  const char **m_baseTypes;
  const char *m_modelString;
  // These correspond to the worker derived classes
  enum WType {
    Application, Platform, Device, Configuration, Assembly, Container
  } m_type;
  bool m_isDevice; // applies to Interconnect, IO, Adapter, Platform
  WciPort *m_wci; // Null means no control
  bool m_noControl; // no control port on this one. FIXME: nuke this in favor of !m_wci
  bool m_reusable;
  std::string m_specFile;
  const char *m_implName;
  const char *m_specName;
  std::string m_package;
  bool m_isThreaded;
  size_t m_maxPortTypeName;
  Control m_ctl;
  Ports m_ports;
  std::vector<LocalMemory*> m_localMemories;
  Clocks m_clocks;
  Clock *m_wciClock;
  Endian m_endian;
  bool m_needsEndian;               // does any port imply an endian issue?
  const char
    *m_pattern,                     // pattern for signal names within ports
    *m_portPattern,                 // pattern for port names
    *m_staticPattern;               // pattern for rcc static methods
  size_t m_defaultDataWidth;        // SIZE_MAX means not set
  Language m_language;
  ::Assembly *m_assembly;
  // vector of slave worker objects paired with a string of the name of the slave either from name
  // attribute or auto generated
  std::map<std::string, Worker*> m_slaves;
  HdlDevice *m_emulate;
  Worker *m_emulator;               // for test only, the emulator of this worker
  Signals m_signals;
  SigMap  m_sigmap;                 // map signal names to signals
  const char *m_library;            // the component library name where the xml was found
  bool m_outer;                     // only generate the outer skeleton, not the inner one
  OU::Property *m_debugProp;
  OU::Assembly::Properties m_instancePVs;
  FILE *m_mkFile, *m_xmlFile;       // state during parameter processing
  const char *m_outDir;             // state during parameter processing
  ParamConfigs m_paramConfigs;      // the parsed file of all configs
  Build m_build;                    // build info not needed for code gen or artifact
  ParamConfig  *m_paramConfig;      // the config for this Worker.
  Worker *m_parent;           // If this worker is part of an upper level assembly
  bool m_scalable;
  size_t m_requiredWorkGroupSize;    // FIXME: belongs in OclWorker class!
                                    // FIXME: derive from compiled code
  unsigned m_maxLevel;        // when data type processing
  bool m_dynamic;
  bool m_isSlave;
  Worker(ezxml_t xml, const char *xfile, const std::string &parentFile, WType type,
	 Worker *parent, OU::Assembly::Properties *ipvs, const char *&err);
  virtual ~Worker();
  static Worker *
    create(const char *file, const std::string &parentFile, const char *package,
	   const char *outDir, Worker *parent, OU::Assembly::Properties *instancePropertyValues,
	   size_t paramConfig, const char *&err);
  const Ports &ports() const { return m_ports; }
  const char *parseClocks();
  const char *addClock(ezxml_t);
  const char *addClock(const char *name, const char *direction, Clock *&clk);
  Clock &addClock(const char *name, bool output = false);
  Clock &addClock(const std::string &name, bool output = false) { return addClock(name.c_str(), output); }
  Clock &addWciClockReset();
  OU::Property *findProperty(const char *name) const;
  OU::Port *findMetaPort(const char *id, const OU::Port *except) const;
  const char* parseSlaves();
  std::string print_map();
  const char* addSlave(const std::string worker_name, const std::string slave_name);
  virtual OU::Port &metaPort(unsigned long which) const;
  const char
    *addBuiltinProperties(),
    *getPort(const char *name, Port *&p, Port *except = NULL) const,
    *getValue(const char *sym, OU::ExprValue &val) const,
    *getNumber(ezxml_t x, const char *attr, size_t *np, bool *found = NULL,
	       size_t defaultValue = 0, bool setDefault = true) const,
    //    *getBoolean(ezxml_t x, const char *name, bool *b, bool trueOnly),
    *parse(const char *file, const char *parent, const char *package = NULL),
    *parseRcc(const char *package = NULL),
    *parseRccImpl(const char *package),
    *parseOcl(),
    *parseHdl(const char *package = NULL),
    *parseRccAssy(),
    *parseOclAssy(),
    *parseImplControl(ezxml_t &xctl),
    *parseImplLocalMemory(),
    *findPackage(ezxml_t spec, const char *package),
    *parseSpecControl(ezxml_t ps),
    *parseSpec(const char *package = NULL),
    //    *preParseSpecDataPort(ezxml_t x),
    //    *parseSpecPort(Port *p),
    *parseHdlImpl(const char* package = NULL),
    *parseBuildFile(bool optional, bool *missing = NULL, const std::string *parent = NULL),
    *parseBuildXml(ezxml_t x, const std::string &file),
    *startBuildXml(FILE *&f),
    *doProperties(ezxml_t top, const char *parent, bool impl, bool anyIsBad, const char *firstRaw, bool AllRaw),
    *parseHdlAssy(),
    *initImplPorts(ezxml_t xml, const char *element, PortCreate &pc),
    *checkDataPort(ezxml_t impl, DataPort *&sp),
    *addProperty(ezxml_t prop, bool includeImpl, bool anyIsBad, bool isRaw, bool isBuiltin = false),
    // Add a property from an xml string description
    *addProperty(const char *xml, bool includeImpl, bool isBuiltin = false),
    //    *doAssyClock(Instance *i, Port *p),
    *openSkelHDL(const char *suff, FILE *&f),
    *emitVhdlRecordInterface(FILE *f, bool isEntity = false),
    *emitImplHDL( bool wrap = false),
    *emitAssyImplHDL(FILE *f, bool wrap),
    *emitConfigImplHDL(FILE *f),
    *emitContainerImplHDL(FILE *f),
    *emitSkelHDL(),
    *emitBsvHDL(),
    *emitDefsHDL(bool wrap = false),
    *emitVhdlEnts(),
    *emitVhdlWorkerPackage(FILE *f, unsigned maxPropName),
    *emitVhdlWorkerEntity(FILE *f),
    *emitVhdlPackageConstants(FILE *f),
    *writeParamFiles(FILE *mkFile, FILE *xmlFile),
    *emitToolParameters(),
    *emitMakefile(FILE *xmlFile = NULL),
    *emitHDLConstants(size_t config, bool other),
    *setParamConfig(OU::Assembly::Properties *instancePVs, size_t paramConfig,
		    const std::string *parent = NULL),
    *finalizeProperties(),
    *finalizeHDL(),
    *deriveOCP(),
    *hdlValue(const std::string &name, const OU::Value &v, std::string &value,
	      bool param = false, Language = NoLanguage, bool finalized = false),
    *findParamProperty(const char *name, OU::Property *&prop, size_t &nParam,
		       bool includeInitial = false),
    *addConfig(ParamConfig &info, bool fromXml),
    *doParam(ParamConfig &info, PropertiesIter pi, bool fromXml, unsigned nParam),
    *addParamConfigSuffix(std::string &s),
    //    *getParamConfig(const char *id, const ParamConfig *&config),
    *emitImplRCC(),
    *rccMethodName(const char *method, const char *&mName),
    *emitImplOCL(),
    *emitEntryPointOCL(),
    *paramValue(const OU::Member &param, OU::Value &v, std::string &value),
    *rccBaseValue(OU::Value &v, std::string &value, const OU::Member *param = NULL),
    *rccValue(OU::Value &v, std::string &value, const OU::Member &param),
    *rccPropValue(OU::Property &p, std::string &value),
    *emitSkelRCC(),
    *emitSkelOCL(),
    *emitAssyHDL();
  virtual const char
    *resolveExpressions(OCPI::Util::IdentResolver &ir),
    *parseInstance(Worker &parent, Instance &inst, ezxml_t x), // FIXME: should be HdlInstance...
    *emitArtXML(const char *wksFile),
    *emitToolArtXML(),
    *emitWorkersHDL(const char *file),
    *emitAttribute(const char *attr),
    *emitUuid(const OU::Uuid &uuid);
  Port *findPort(const char *name, const Port *except = NULL) const;
  Clock *findClock(const char *name) const;
  virtual void
    emitDeviceSignalMapping(FILE *f, std::string &last, Signal &s, const char *prefix),
    emitDeviceSignal(FILE *f, Language lang, std::string &last, Signal &s, const char *prefix),
    recordSignalConnection(Signal &s, const char *from),
    emitTieoffSignals(FILE *f),
    emitXmlWorkers(FILE *f),
    emitXmlInstances(FILE *f),
    emitXmlConnections(FILE *f);
  void
    emitCppTypesNamespace(FILE *f, std::string &nsName, const std::string &slaveName=""),
    emitDeviceConnectionSignals(FILE *f, const char *iname, bool container),
    setParent(Worker *p), // when it can't happen at construction
    prType(OU::Property &pr, std::string &type),
    emitVhdlPropMemberData(FILE *f, OU::Property &pr, unsigned maxPropName),
    emitVhdlPropMember(FILE *f, OU::Property &pr, unsigned maxPropName, bool in2worker),
    rccPropType(OU::Property &p, std::string &typeDef, std::string &type, std::string &pretty),
    emitWorkersAttribute(),
    deleteAssy(), // just to keep the assembly details out of most files
    emitXmlWorker(FILE *f, bool verbose = false),
    emitInstances(FILE *f, const char *prefix, size_t &index),
    emitInternalConnections(FILE *f, const char *prefix),
    emitVhdlShell(FILE *f),
    emitVhdlSignalWrapper(FILE *f, const char *topinst = "rv"),
    emitVhdlRecordWrapper(FILE *f),
    emitParameters(FILE *f, Language lang, bool useDefaults = true, bool convert = false),
    emitSignals(FILE *f, Language lang, bool records, bool inPackage, bool inWorker,
		bool convert = false),
    rccEmitDimension(size_t numeric, const std::string &expr, const char *surround,
		     std::string &out),
    rccArray(std::string &type, OU::Member &m, bool isFixed, bool &isLast, bool topSeq, bool end),
    rccStruct(std::string &type, size_t nMembers, OU::Member *members, unsigned level,
	      const char *parent, bool isFixed, bool &isLast, bool topSeq, unsigned predef,
	      size_t elementBytes = 0),
    rccMember(std::string &type, OU::Member &m, unsigned level, size_t &offset, unsigned &pad,
	      const char *parent, bool isFixed, bool &isLast, bool topSeq, unsigned predef, bool cnst = false),
    rccType(std::string &type, OU::Member &m, unsigned level, size_t &offset, unsigned &pad,
	    const char *parent, bool isFixed, bool &isLast, bool topSeq, unsigned predef, bool cnst = false),
    rccBaseType(std::string &type, OU::Member &m, unsigned level, size_t &offset, unsigned &pad,
		const char *parent, bool isFixed, bool &isLast, unsigned predefine, bool cnst = false),
    emitPropertyAttributeConstants(FILE *f, Language lang),
    emitSignalMacros(FILE *f, Language lang),
    emitDeviceSignalMappings(FILE *f, std::string &last),
    emitDeviceSignals(FILE *f, Language lang, std::string &last);
};
#define SKEL "-skel"
#define IMPL "-impl"
#define DEFS "-defs"
#define ENTS "-ents"
#define ASSY "-assy"
#define CONTINST "-continst"
#define VHD ".vhd"
#define VER ".v"
#define VERH ".vh"
#define BOOL(b) ((b) ? "true" : "false")

// Attributes common to both OWD and build xml
#define PLATFORM_ATTRS "onlyPlatforms", "excludePlatforms"
#define BUILD_ATTRS PLATFORM_ATTRS, "onlytargets", "excludeTargets"
// Attributes common to all models
#define IMPL_ATTRS BUILD_ATTRS, \
  "name", "spec", "paramconfig", "reentrant", "scaling", "scalable", "controlOperations", "xmlincludedirs", \
    "componentlibraries", "version", "libraries", "includedirs", "sourcefiles"

#define IMPL_ELEMS "componentspec", "properties", "property", "specproperty", "propertysummary", "slave", "xi:include", "controlinterface",  "timeservice", "unoc", "timebase", "sdp"
#define GENERIC_IMPL_CONTROL_ATTRS \
  "name", "SizeOfConfigSpace", "ControlOperations", "Sub32BitConfigProperties"
#define ASSY_ELEMS "instance", "connection", "external"
extern const char
  *checkSuffix(const char *str, const char *suff, const char *last),
  *createTests(const char *file, const char *package, const char *outDir, bool verbose),
  *createCases(const char **args, const char *package, const char *outDir, bool verbose),
  *addLibrary(const char *lib),
  *extractExprValue(const OU::Property &p, const OU::Value &v, OU::ExprValue &val),
  *tryInclude(ezxml_t x, const std::string &parent, const char *element, ezxml_t *parsed,
	      std::string &child, bool optional),
  *parseList(const char *list, const char * (*doit)(const char *tok, void *arg), void *arg),
  *parseControlOp(const char *op, void *arg),
  *vhdlValue(const char *pkg, const std::string &name, const OU::Value &v, std::string &value,
	     bool param = false, bool finalized = false),
  *verilogValue(const OU::Value &v, std::string &value, bool finalized = false),
  *rccValue(OU::Value &v, std::string &value),
  *g_platform, *g_device, *load, *g_os, *g_os_version, *g_arch, **libraries, **mappedLibraries,
  *assembly, *attribute, *platformDir,
  *addLibMap(const char *),
  *findLibMap(const char *file), // returns mapped lib name from dir name of file or NULL
  *propertyTypes[],
  *getNames(ezxml_t xml, const char *file, const char *tag, std::string &name, std::string &fileName),
  *tryOneChildInclude(ezxml_t top, const std::string &parent, const char *element,
		      ezxml_t *parsed, std::string &childFile, bool optional),
  *emitContainerHDL(Worker*, const char *);

extern bool g_dynamic, g_multipleWorkers;
extern void
  doPrev(FILE *f, std::string &last, std::string &comment, const char *myComment),
  vhdlType(const OU::Property &dt, std::string &typeDecl, std::string &type,
	   bool convert = false, bool finalized = false),
  emitConstant(FILE *f, const std::string &prefix, const char *name, size_t val, Language lang, bool ieee = false),
  emitVhdlLibraries(FILE *f),
  emitLastSignal(FILE *f, std::string &last, Language lang, bool end);

extern size_t rawBitWidth(const OU::ValueType &dt);
#endif
