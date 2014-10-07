/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
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
#include "OcpiUtilImplementation.h"
#include "OcpiUtilAssembly.h"
#include "OcpiUuid.h"
#include "ezxml.h"
#include "cdkutils.h"
#include "parameters.h"
#include "port.h"
#include "ocp.h"

namespace OE=OCPI::Util::EzXml;
namespace OU=OCPI::Util;
namespace OA=OCPI::API;

class Port;

// We derive a class to implement xi:include parsing, file names, etc.
class Protocol : public OU::Protocol {
public:
  Protocol(Port &port);
  Port &m_port;
  const char *parse(const char *file, ezxml_t prot = NULL);
  const char *parseOperation(ezxml_t op);
};

class Worker;

struct Scaling {
  size_t m_min, m_max, m_modulo, m_default;
  Scaling();
  const char *parse(Worker &w, ezxml_t x);
};
#define SCALING_ATTRS "min", "max", "modulo", "default"
#define OCPI_PADDINGS \
  OCPI_PADDING(None) \
  OCPI_PADDING(Replicate) \
  OCPI_PADDING(Zero) \
  OCPI_PADDING(Wrap)

#define OCPI_PADDING(x) x,
  enum Padding { OCPI_PADDINGS };
#undef OCPI_PADDING

struct Overlap {
  size_t m_left, m_right;
  Padding m_padding;
  Overlap();
  const char *parse(ezxml_t x);
};
#define OVERLAP_ATTRS "left", "right", "padding"
// This structure is per-dimension
struct Partitioning {
  Scaling  m_scaling;
  Overlap  m_overlap;
  size_t   m_sourceDimension;
  Partitioning();
  const char *parse(Worker &w, ezxml_t x);
};
#define PARTITION_ATTRS SCALING_ATTRS, OVERLAP_ATTRS, "source"
#define OCPI_DISTRIBUTIONS \
  OCPI_DISTRIBUTION(All) \
  OCPI_DISTRIBUTION(Cyclic) \
  OCPI_DISTRIBUTION(First) \
  OCPI_DISTRIBUTION(Balanced) \
  OCPI_DISTRIBUTION(Directed) \
  OCPI_DISTRIBUTION(Random) \
  OCPI_DISTRIBUTION(Hashed) \

#define OCPI_DISTRIBUTION(x) x,
  enum Distribution { OCPI_DISTRIBUTIONS DistributionLimit };

#undef OCPI_DISTRIBUTION

#define DISTRIBUTION_ATTRS "distribution", "hashfield"

#define SPEC_DATA_PORT_ATTRS "Name", "Producer", "Count", "Optional", "Protocol", "buffersize"
class DataPort : public OcpPort {
 protected:
  Protocol *m_protocol;
  bool m_isProducer;
  bool m_isOptional;
  bool m_isBidirectional;
  size_t m_nOpcodes;
  size_t m_minBufferCount;
  size_t m_bufferSize;
  Port *m_bufferSizePort;
  // Scalability
  bool m_isScalable;
  std::string m_scaleExpr;
  Distribution m_defaultDistribution;
  Partitioning m_defaultPartitioning;
  std::string m_defaultHashField;
  struct OpScaling {
    Distribution                m_distribution;
    OU::Member                 *m_hashField;
    Partitioning                m_defaultPartitioning; // default for all args
    bool                        m_multiple;
    bool                        m_allSeeOne;
    bool                        m_allSeeEnd;
    std::vector<Partitioning *> m_partitioning; // tricky: these pointers are arrays for dims
    bool                        m_isPartitioned;
    OpScaling(size_t nArgs);
    const char *parse(DataPort &dp, OU::Operation &op, ezxml_t x);
  };
  std::vector<OpScaling*> m_opScaling;
  bool m_isPartitioned;
  
  // This constructor is used when data port is inherited
  DataPort(Worker &w, ezxml_t x, Port *sp, int ordinal, WIPType type, const char *&err);
  DataPort(const DataPort &other, Worker &w , std::string &name, size_t count,
	   OCPI::Util::Assembly::Role *role, const char *&err);
 public:
  // Virtual constructor - every derived class must have one
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err)
    const;
  inline const char *typeName() const { return "WDI"; }
  inline const char *prefix() const { return "data"; }
  bool matchesDataProducer(bool isProducer) const { return m_isProducer == isProducer; }
  // This constructor is used when data port *is* the derived class (WDIPort)
  DataPort(Worker &w, ezxml_t x, int ordinal, const char *&err);
  bool isData() const { return true; }
  bool isDataProducer() const { return m_isProducer; } // call isData first
  bool isDataOptional() const { return m_isOptional; } // call isData first
  bool isDataBidirectional() const { return m_isBidirectional; } // call isData first
  bool isOptional() const { return m_isOptional; }
  const char *parse();
  const char *parseDistribution(ezxml_t x, Distribution &d, std::string &hash);
  const char *finalize();
  const char *fixDataConnectionRole(OU::Assembly::Role &role);
  void initRole(OCPI::Util::Assembly::Role &role);
  void emitOpcodes(FILE *f, const char *pName, Language lang);
  void emitPortDescription(FILE *f, Language lang) const;
  void emitRecordDataTypes(FILE *f);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitImplSignals(FILE *f);
  void emitXML(FILE *f);
  void emitRccCppImpl(FILE *f);
  void emitRccCImpl(FILE *f);
  void emitRccCImpl1(FILE *f);
  static const char *adjustConnection(const char *masterName,
				      Port &prodPort, OcpAdapt *prodAdapt,
				      Port &consPort, OcpAdapt *consAdapt,
				      Language lang);
  virtual const char *adjustConnection(Port &consumer, const char *masterName, Language lang,
				       OcpAdapt *prodAdapt, OcpAdapt *consAdapt);
  const char *finalizeHdlDataPort();
  const char *finalizeRccDataPort();
  const char *finalizeOclDataPort();
};
class WciPort : public OcpPort {
  size_t m_timeout;
  bool m_resetWhileSuspended;
 public:
  WciPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  inline const char *prefix() const { return "wci"; }
  inline const char *typeName() const { return "WCI"; }
  bool needsControlClock() const;
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
  void emitWorkerEntitySignals(FILE *f, std::string &last, unsigned maxPropName);
  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inWorker);
  void emitRecordArray(FILE *f);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt);
};

class RccPort : public DataPort {
public:
  RccPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
};
class OclPort : public RccPort {
public:
  OclPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
};

class WsiPort : public DataPort {
  ~WsiPort();
  bool m_abortable;
  bool m_earlyRequest;
  bool m_regRequest; // request is registered
  WsiPort(const WsiPort &other, Worker &w , std::string &name, size_t count,
	  OCPI::Util::Assembly::Role *role, const char *&err);
 public:
  WsiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  bool masterIn() const;
  inline const char *prefix() const { return "wsi"; }
  inline const char *typeName() const { return "WSI"; }
  void emitPortDescription(FILE *f, Language lang) const;
  const char *deriveOCP();
  void emitVhdlShell(FILE *f, Port *wci);
  const char *adjustConnection(Port &consumer, const char *masterName, Language lang,
			       OcpAdapt *prodAdapt, OcpAdapt *consAdapt);
  void emitImplAliases(FILE *f, unsigned n, Language lang);
  void emitSkelSignals(FILE *f);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
};
class WmiPort : public DataPort {
  bool m_talkBack;
  size_t m_mflagWidth; // kludge for shep - FIXME
  WmiPort(const WmiPort &other, Worker &w , std::string &name, size_t count,
	  OCPI::Util::Assembly::Role *role, const char *&err);
 public:
  WmiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *prefix() const { return "mem"; }
  inline const char *typeName() const { return "WMI"; }
  const char *deriveOCP();
  void emitPortDescription(FILE *f, Language lang) const;
  const char *adjustConnection(Port &consumer, const char *masterName, Language lang,
			       OcpAdapt *prodAdapt, OcpAdapt *consAdapt);
  void emitImplAliases(FILE *f, unsigned n, Language lang);
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
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
};
class WtiPort : public OcpPort {
  size_t m_secondsWidth, m_fractionWidth;
  bool m_allowUnavailable;
  WtiPort(const WtiPort &other, Worker &w, std::string &name, const char *&err);
 public:
  WtiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  Port &clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role *role,
	      const char *&err) const;
  inline const char *typeName() const { return "WTI"; }
  inline const char *prefix() const { return "wti"; }
  bool haveWorkerOutputs() const;
  const char *deriveOCP();
  void emitRecordInputs(FILE *f);
  void emitRecordOutputs(FILE *f);
  //  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inWorker);
  void emitPortDescription(FILE *f, Language lang) const;
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
  void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
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
  void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
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
  void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
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
  void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inWorker);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitVHDLShellPortMap(FILE *f, std::string &last);
  void emitVHDLSignalWrapperPortMap(FILE *f, std::string &last);
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt);
  void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
};
class RawPropPort : public Port {
 public:
  RawPropPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  inline const char *prefix() const { return "rawprop"; }
  inline const char *typeName() const { return "RawProperty"; }
  bool needsControlClock() const { return true; }
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
};
// The port for inter-device connections
class DevSignalsPort : public Port {
  Signals m_signals;
  bool m_hasInputs;
  bool m_hasOutputs;
 public:
  DevSignalsPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);
  void emitRecordTypes(FILE *f);
  void emitRecordInterface(FILE *f, const char *implName);
  inline const char *prefix() const { return "ds"; }
  inline const char *typeName() const { return "DevSignals"; }
  bool haveInputs() const { return master ? m_hasInputs : m_hasOutputs; }
  bool haveWorkerInputs() const { return haveInputs(); }
  bool haveOutputs() const { return master ? m_hasOutputs : m_hasInputs; }
  bool haveWorkerOutputs() const { return haveOutputs(); }
  void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
  void emitPortSignalsDir(FILE *f, bool output, const char *indent, bool &any,
			  std::string &comment, std::string &last, std::string &conn,
			  std::string &index);
  void emitPortSignals(FILE *f, Attachments &atts, Language lang,
		       const char *indent, bool &any, std::string &comment,
		       std::string &last, const char *myComment, OcpAdapt *adapt);
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
  bool readbacks, nonRawReadbacks, rawReadbacks;
  bool rawProperties;
  unsigned nRunProperties, nNonRawRunProperties, nParameters;
  Control();
  void initAccess();
  void summarizeAccess(OU::Property &p);
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
struct Parsed {
  std::string m_name, m_file, m_parent, m_fileName;
  ezxml_t m_xml;
  Parsed(ezxml_t xml,        // if non-zero, the xml.  If not, then parse the file.
	 const char *file,   // The file, either where this is embedded or its own file
	 const std::string &parent, // The file referencing this file
	 const char *tag,
	 const char *&err);
};

enum Model {
  NoModel,
  HdlModel,
  RccModel,
  OclModel
};

// This class represents a connection to a required worker

typedef std::vector<Clock*> Clocks;
typedef Clocks::const_iterator ClocksIter;
typedef std::list<Worker *> Workers;
typedef Workers::const_iterator WorkersIter;
class Assembly;
class Worker : public Parsed, public OU::IdentResolver {
 public:
  Model m_model;
  const char **m_baseTypes;
  const char *m_modelString;
  bool m_isDevice;
  WciPort *m_wci; // Null means no control
  bool m_noControl; // no control port on this one.
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
  int m_defaultDataWidth;           // initialized to -1 to allow zero
  Language m_language;
  Assembly *m_assembly;
  Worker *m_slave;
  Signals m_signals;
  const char *m_library;            // the component library name where the xml was found
  bool m_outer;                     // only generate the outer skeleton, not the inner one
  OU::Property *m_debugProp;
  OU::Assembly::Properties *m_instancePVs;
  FILE *m_mkFile, *m_xmlFile;       // state during parameter processing
  const char *m_outDir;             // state during parameter processing
  ParamConfigs m_paramConfigs;      // the parsed file of all configs
  ParamConfig  *m_paramConfig;      // the config for this Worker.
  // Scalability
  bool m_scalable;            // Is this worker scalable at all?
  std::string m_validScaling; // Expression for error checking overall scaling
  Scaling m_scaling;
  std::map<std::string, Scaling> m_scalingParameters;
  Worker(ezxml_t xml, const char *xfile, const std::string &parent,
		  OU::Assembly::Properties *ipvs, const char *&err);
  virtual ~Worker();
  static Worker *
    create(const char *file, const std::string &parent, const char *package, const char *outDir,
	   OU::Assembly::Properties *instancePropertyValues, size_t paramConfig, const char *&err);
  bool nonRaw(PropertiesIter pi);
  Clock *addClock();
  Clock *addWciClockReset();
  OU::Property *findProperty(const char *name);
  const char
    *getPort(const char *name, Port *&p, Port *except = NULL) const,
    *getValue(const char *sym, OU::ExprValue &val) const,
    *getNumber(ezxml_t x, const char *attr, size_t *np, bool *found = NULL,
	       size_t defaultValue = 0, bool setDefault = true),
    *getBoolean(ezxml_t x, const char *name, bool *b, bool trueOnly),
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
    *parseConfigFile(const char *dir),
    *doProperties(ezxml_t top, const char *parent, bool impl, bool anyIsBad),
    *doScaling(ezxml_t x),
    *parseHdlAssy(),
    *initImplPorts(ezxml_t xml, const char *element, PortCreate &pc),
    *checkDataPort(ezxml_t impl, Port *&sp),
    *addProperty(ezxml_t prop, bool includeImpl),
    //    *doAssyClock(Instance *i, Port *p),
    *openSkelHDL(const char *suff, FILE *&f),
    *emitVhdlRecordInterface(FILE *f),
    *emitUuidHDL(const OU::Uuid &uuid),
    *emitImplHDL( bool wrap = false),
    *emitAssyImplHDL(FILE *f, bool wrap),
    *emitConfigImplHDL(FILE *f),
    *emitContainerImplHDL(FILE *f),
    *emitSkelHDL(),
    *emitBsvHDL(),
    *emitDefsHDL(bool wrap = false),
    *emitVhdlWorkerPackage(FILE *f, unsigned maxPropName),
    *emitVhdlWorkerEntity(FILE *f),
    *emitVhdlPackageConstants(FILE *f),
    *emitToolParameters(),
    *setParamConfig(OU::Assembly::Properties *instancePVs, size_t paramConfig),
    *deriveOCP(),
    *hdlValue(const OU::Value &v, std::string &value, bool param = false, Language = NoLanguage),
    *findParamProperty(const char *name, OU::Property *&prop, size_t &nParam),
    *addConfig(ParamConfig &info, size_t &nConfig),
    *doParam(ParamConfig &info, PropertiesIter pi, unsigned nParam, size_t &nConfig),
    //    *getParamConfig(const char *id, const ParamConfig *&config),
    *emitImplRCC(),
    *rccMethodName(const char *method, const char *&mName),
    *emitImplOCL(),
    *emitEntryPointOCL(),
    *paramValue(OU::Value &v, std::string &value),
    *rccValue(OU::Value &v, std::string &value, bool param = false),
    *rccPropValue(OU::Property &p, std::string &value),
    *emitSkelRCC(),
    *emitSkelOCL(),
    *emitAssyHDL();
  virtual const char
    *emitArtXML(const char *wksFile),
    *emitWorkersHDL(const char *file),
    *emitAttribute(const char *attr);
  Port *findPort(const char *name, Port *except = NULL) const;
  Clock *findClock(const char *name) const;
  void
    emitWorkersAttribute(),
    deleteAssy(), // just to keep the assembly details out of most files
    emitXmlWorkers(FILE *f),
    emitXmlWorker(FILE *f),
    emitInstances(FILE *f, const char *prefix, size_t &index),
    emitInternalConnections(FILE *f, const char *prefix),
    emitVhdlShell(FILE *f),
    emitVhdlSignalWrapper(FILE *f, const char *topinst = "rv"),
    emitVhdlRecordWrapper(FILE *f),
    emitParameters(FILE *f, Language lang, bool useDefaults = true, bool convert = false),
    //    emitPortDescription(Port *p, FILE *f, Language lang),
    emitSignals(FILE *f, Language lang, bool records, bool inPackage, bool inWorker),
    emitRccStruct(FILE *f, size_t nMembers, OU::Member *members, unsigned indent,
		  const char *parent, bool isFixed, bool &isLast, bool topSeq),
    printRccMember(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
		   const char *parent, bool isFixed, bool &isLast, bool topSeq),
    printRccType(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
		 const char *parent, bool isFixed, bool &isLast, bool topSeq),
    printRccBaseType(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
		     const char *parent, bool isFixed, bool &isLast),
    emitDeviceSignals(FILE *f, Language lang, std::string &last);
};

#define SKEL "-skel"
#define IMPL "-impl"
#define DEFS "-defs"
#define ASSY "-assy"
#define VHD ".vhd"
#define VER ".v"
#define VERH ".vh"
#define BOOL(b) ((b) ? "true" : "false")

#define IMPL_ATTRS \
  "name", "spec", "paramconfig", "reentrant", "scaling", "scalable", "controlOperations"
#define IMPL_ELEMS "componentspec", "properties", "property", "specproperty", "propertysummary", "xi:include", "controlinterface",  "timeservice", "unoc"
#define GENERIC_IMPL_CONTROL_ATTRS \
  "name", "SizeOfConfigSpace", "ControlOperations", "Sub32BitConfigProperties"
#define ASSY_ELEMS "instance", "connection", "external"
extern const char
  *parseList(const char *list, const char * (*doit)(const char *tok, void *arg), void *arg),
  *parseControlOp(const char *op, void *arg),
  *vhdlValue(const OU::Value &v, std::string &value, bool param = false),
  *rccValue(OU::Value &v, std::string &value),
  *container, *platform, *device, *load, *os, *os_version, **libraries, **mappedLibraries, *assembly, *attribute,
  *addLibMap(const char *),
  *findLibMap(const char *file), // returns mapped lib name from dir name of file or NULL
  *propertyTypes[],
  *getNames(ezxml_t xml, const char *file, const char *tag, std::string &name, std::string &fileName),
  *tryOneChildInclude(ezxml_t top, const std::string &parent, const char *element,
		      ezxml_t *parsed, std::string &childFile, bool optional),
  *emitContainerHDL(Worker*, const char *),
  *emitArtOCL(Worker *);

extern void
  doPrev(FILE *f, std::string &last, std::string &comment, const char *myComment),
  vhdlType(const OU::ValueType &dt, std::string &typeDecl, std::string &type,
	   bool convert = false),
  emitVhdlLibraries(FILE *f),
  addLibrary(const char *lib),
  emitLastSignal(FILE *f, std::string &last, Language lang, bool end);

extern size_t ceilLog2(uint64_t n), floorLog2(uint64_t n);

#endif
