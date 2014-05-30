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

namespace OE=OCPI::Util::EzXml;
namespace OU=OCPI::Util;
namespace OA=OCPI::API;

#define myCalloc(t, n) ((t *)calloc(sizeof(t), (n)))
inline void *myCrealloc_(void *p, size_t s, size_t o, size_t add) {
  void *np = realloc(p, s*(o + add));
  if (np)
    memset((char *)(np) + o*s, 0, add*s);
  return np;
}
// Add to end, with zeroing
#define myCrealloc(t, p, o, additional) \
  ((t *)myCrealloc_(p, sizeof(t), (o), (additional)))

#if 0
enum ControlOp {
#define CONTROL_OP(x, c, t, s1, s2, s3, s4)  ControlOp##c,
  OCPI_CONTROL_OPS
#undef CONTROL_OP
  NoOp
};
#endif
class Port;

// We derive a class to implement xi:include parsing, file names, etc.
class Protocol : public OU::Protocol {
public:
  Protocol(Port &port);
  Port &m_port;
  const char *parse(const char *file, ezxml_t prot = NULL);
  const char *parseOperation(ezxml_t op);
};

struct WDI {
  //  unsigned dataValueWidth;
  //  unsigned dataValueGranularity;
  // bool diverseDataSizes;
  // unsigned maxMessageValues;
  bool continuous;
  bool isProducer;
  bool isBidirectional;
  // bool variableMessageLength;
  // bool zeroLengthMessages;
  bool isOptional;
  size_t minBufferCount;
  size_t nOpcodes;
  size_t bufferSize;
};
// OCP_SIGNAL_I(name,
// OCP_SIGNAL_IV(name,
// OCP_SIGNAL_O(
// OCP_SIGNAL_OV(
#define OCP_SIGNALS \
  OCP_SIGNAL_MS(Clk) \
  OCP_SIGNAL_MVR(MAddr, 0) \
  OCP_SIGNAL_MVR(MAddrSpace, 1) \
  OCP_SIGNAL_MVR(MBurstLength, 0) \
  OCP_SIGNAL_MSR(MBurstSingleReq) \
  OCP_SIGNAL_MVR(MByteEn, 0) \
  OCP_SIGNAL_MTR(MCmd, 3) \
  OCP_SIGNAL_MV(MData, 0) \
  OCP_SIGNAL_MV(MDataByteEn, 0) \
  OCP_SIGNAL_MV(MDataInfo, 0) \
  OCP_SIGNAL_MS(MDataLast) \
  OCP_SIGNAL_MS(MDataValid) \
  OCP_SIGNAL_MV(MFlag, 0) \
  OCP_SIGNAL_MSR(MBurstPrecise) \
  OCP_SIGNAL_MVR(MReqInfo, 0)		\
  OCP_SIGNAL_MSR(MReqLast) \
  OCP_SIGNAL_MS(MReset_n) \
  OCP_SIGNAL_MS(MRespAccept) \
  OCP_SIGNAL_SS(SCmdAccept) \
  OCP_SIGNAL_SV(SData, 0) \
  OCP_SIGNAL_SS(SDataAccept) \
  OCP_SIGNAL_SV(SDataInfo, 0) \
  OCP_SIGNAL_SV(SDataThreadBusy, 1) \
  OCP_SIGNAL_SV(SFlag, 0) \
  OCP_SIGNAL_ST(SResp, 2) \
  OCP_SIGNAL_SS(SRespLast) \
  OCP_SIGNAL_SS(SReset_n) \
  OCP_SIGNAL_SV(SThreadBusy, 1) \
  /**/
#define OCP_SIGNAL_MT(n,w) OCP_SIGNAL_MV(n,w)
#define OCP_SIGNAL_ST(n,w) OCP_SIGNAL_SV(n,w)
#define OCP_SIGNAL_MS(n) OCP_SIGNAL(n)
#define OCP_SIGNAL_MV(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL_MSR(n) OCP_SIGNAL(n)
#define OCP_SIGNAL_MVR(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL_MTR(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL_SS(n) OCP_SIGNAL(n)
#define OCP_SIGNAL_SV(n, w) OCP_SIGNAL(n)
#define OCP_SIGNAL(n) OCP_##n,
enum OcpSignalEnum {
OCP_SIGNALS
N_OCP_SIGNALS
};
#undef OCP_SIGNAL

struct OcpSignalDesc {
  const char *name;
  bool vector;
  bool master;
  size_t width;
  bool type;
  bool request;
  OcpSignalEnum number;
};
// A bit redundant from the above, but for adhoc signals
struct Signal;
typedef std::list<Signal *> Signals;
typedef Signals::iterator SignalsIter;
struct Signal {
  std::string m_name;
  enum Direction { IN, OUT, INOUT, BIDIRECTIONAL } m_direction;
  size_t m_width;
  bool m_differential;
  std::string m_pos; // pattern for positive if not %sp
  std::string m_neg; // pattern for negative if not %sn
  const char *m_type;
  Signal();
  const char * parse(ezxml_t);
  static const char *parseSignals(ezxml_t x, Signals &signals);
  static void deleteSignals(Signals &signals);
  static Signal *find(Signals &signals, const char *name); // poor man's map
};

extern OcpSignalDesc ocpSignals[N_OCP_SIGNALS+1];
struct OcpSignal {
  uint8_t *value;
  size_t width;
  const char *signal;
  bool master;
};
union OcpSignals {
  OcpSignal signals[N_OCP_SIGNALS];
  struct {
#define OCP_SIGNAL(n) OcpSignal n;
OCP_SIGNALS
#undef OCP_SIGNAL_MS
#undef OCP_SIGNAL_MV
#undef OCP_SIGNAL_SS
#undef OCP_SIGNAL_SV
#undef OCP_SIGNAL_MSR
#undef OCP_SIGNAL_MVR
#undef OCP_SIGNAL_MTR
#undef OCP_SIGNAL
  };
};

// Generic control attributes

struct WCI {
  char *name;
  bool resetWhileSuspended;
  size_t timeout;
};

struct WTI {
  size_t secondsWidth, fractionWidth;
  bool allowUnavailable;
};
struct WSI {
  WDI wdi;
  bool abortable;
  bool earlyRequest;
  bool regRequest; // request is registered
};
struct WMI {
  WDI wdi;
  bool talkBack;
  size_t mflagWidth;// kludge for shep - FIXME
};
struct WMemI {
  bool writeDataFlowControl, readDataFlowControl;
  uint64_t memoryWords;
  size_t maxBurstLength;
};
enum WIPType{
  NoPort,
  WCIPort,
  WSIPort,
  WMIPort,
  WDIPort, // used temporarily
  WMemIPort,
  WTIPort,
  CPPort,       // Control master port, ready to connect to OCCP
  NOCPort,      // NOC port, ready to support CP and DP
  MetadataPort, // Metadata to/from platform worker
  TimePort,     // TimeService port
  NWIPTypes
};
struct Clock {
  const char *name;
  const char *signal;
  std::string reset;
  Port *port;
  bool assembly; // This clock is at the assembly level
  size_t ordinal; // within the worker
  Clock();
};
class Worker;
union Profiles {
    WDI wdi;
    WSI wsi;
    WMI wmi;
    WCI wci;
    WMemI wmemi;
    WTI wti;
};
class Port {
public:
  const char *name;
  std::string fullNameIn, fullNameOut; // used during HDL generation
  std::string typeNameIn, typeNameOut; // used during HDL generation
  Worker *worker;
  size_t count;
  //  bool isExternal;          // external port of an assembly (not part of worker)
  bool isData;		    // data plane port, model independent
  const char *pattern;      // pattern if it overrides global pattern
  WIPType type;
  size_t dataWidth;
  size_t byteWidth; // derived
  bool impreciseBurst;// used in multiple types, but not all
  bool preciseBurst;  // used in multiple types, but not all
  Clock *clock;
  Port *clockPort; // used temporarily
  bool myClock;
  OcpSignals ocp;
  uint8_t *values;
  bool master;
  Protocol *protocol; // a pointer to make copying easier
  ezxml_t implXml;
  union Profiles {
    WDI wdi;
    WSI wsi;
    WMI wmi;
    WCI wci;
    WMemI wmemi;
    WTI wti;
  } u;
  Port(const char *name, Worker *, bool isData, WIPType type,
       ezxml_t implXml, size_t count = 1, bool master = false);
  // Are master signals inputs at this port?
  inline bool masterIn() {
  return
    type == WCIPort ? (master ? 0 : 1) :
    type == WTIPort ? (master ? 0 : 1) :
    type == WMemIPort ? (master ? 0 : 1) :
    type == WMIPort ? (master ? 0 : 1) :
    type == WSIPort ? (u.wdi.isProducer ? 0 : 1) :
    false;
  }
  inline bool isOCP() {
    switch (type) {
    case WCIPort:
    case WSIPort:
    case WMIPort:
    case WMemIPort:
    case WTIPort:
      return true;
    default:;
    }
    return false;
  }
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
  Control();
  uint64_t sizeOfConfigSpace;
  bool writables, nonRawWritables, rawWritables;
  bool readables, nonRawReadables, rawReadables; // readables does NOT include parameters
  bool sub32Bits, nonRawSub32Bits;
  bool volatiles, nonRawVolatiles;
  bool readbacks, nonRawReadbacks, rawReadbacks;
  unsigned nRunProperties, nNonRawRunProperties, nParameters;
  uint32_t controlOps; // bit mask
  Properties properties;
  size_t offset;// temporary while properties are being parsed.
  unsigned ordinal; // ditto
  bool rawProperties; // for HDL - should properties be provided "raw" to the worker?
  OU::Property *firstRaw;
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

enum Language {
  NoLanguage,
  Verilog,
  VHDL,
  C,
  CC
};



#define PARSED_ATTRS "name"
struct Parsed {
  std::string m_name, m_file, m_parent, m_fileName;
  ezxml_t m_xml;
  Parsed(ezxml_t xml,        // if non-zero, the xml.  If not, then parse the file.
	 const char *file,   // The file, either where this is embedded or its own file
	 const char *parent, // The file referencing this file
	 const char *tag,
	 const char *&err);
};

enum Model {
  NoModel,
  HdlModel,
  RccModel,
  OclModel
};
static inline const char *hdlComment(Language lang) { return lang == VHDL ? "--" : "//"; }

typedef std::vector<Port*> Ports;
typedef Ports::const_iterator PortsIter;
typedef std::vector<Clock*> Clocks;
typedef Clocks::iterator ClocksIter;
class Assembly;
class Worker : public Parsed {
 public:
  Model m_model;
  const char *m_modelString;
  bool m_isDevice;
  bool m_noControl; // no control port on this one.
  bool m_reusable;
  const char *m_specFile;
  const char *m_implName;
  const char *m_specName;
  bool m_isThreaded;
  size_t m_maxPortTypeName;
  Control m_ctl;
  Ports m_ports;
  std::vector<LocalMemory*> m_localMemories;
  Clocks m_clocks;
  Endian m_endian;
  bool m_needsEndian;               // does any port imply an endian issue?
  const char 
    *m_pattern,                     // pattern for signal names within ports
    *m_portPattern;                 // pattern for port names
  const char *m_staticPattern;      // pattern for rcc static methods
  int m_defaultDataWidth;           // initialized to -1 to allow zero
  Language m_language;
  Assembly *m_assembly;
  Signals m_signals;
  const char *m_library;            // the component library name where the xml was found
  bool m_outer;                     // only generate the outer skeleton, not the inner one
  OU::Property *m_debugProp;
  OU::Assembly::Properties *m_instancePVs;
  FILE *m_mkFile, *m_xmlFile;       // state during parameter processing
  const char *m_outDir;             // state during parameter processing
  ParamConfigs m_paramConfigs;      // the parsed file of all configs
  ParamConfig  *m_paramConfig;      // the config for this Worker.
  Worker(ezxml_t xml, const char *xfile, const char *parent,
	 OU::Assembly::Properties *ipvs, const char *&err);
  virtual ~Worker();
  static Worker *
    create(const char *file, const char *parent, const char *package, const char *outDir,
	   OU::Assembly::Properties *instancePropertyValues, size_t paramConfig, const char *&err);
  bool nonRaw(PropertiesIter pi);
  Clock *addClock();
  Clock *addWciClockReset();
  const char
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
    *parseSpecControl(ezxml_t ps),
    *parseSpec(const char *package = NULL),
    *parsePort(ezxml_t x),
    *parseHdlImpl(const char* package = NULL),
    *parseConfigFile(const char *dir),
    *doProperties(ezxml_t top, const char *parent, bool impl, bool anyIsBad),
    *parseHdlAssy(),
    *initImplPorts(ezxml_t xml, const char *element, const char *prefix, WIPType type),
    *doPattern(Port *p, int n, unsigned wn, bool in, bool master, std::string &suff,
	       bool port = false),
    *checkClock(Port *p),
    *checkDataPort(ezxml_t impl, Port **dpp, WIPType type),
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
    *emitVhdlWorkerEntity(FILE *f, unsigned maxPropName),
    *emitVhdlPackageConstants(FILE *f),
    *emitToolParameters(),
    *setParamConfig(OU::Assembly::Properties *instancePVs, size_t paramConfig),
    *deriveOCP(),
    *hdlValue(const OU::Value &v, std::string &value),
    *findParamProperty(const char *name, OU::Property *&prop, size_t &nParam),
    *addConfig(ParamConfig &info),
    *doParam(ParamConfig &info, PropertiesIter pi, unsigned nParam),
    //    *getParamConfig(const char *id, const ParamConfig *&config),
    *emitImplRCC(),
    *rccValue(OU::Value &v, std::string &value),
    *rccPropValue(OU::Property &p, std::string &value),
    *emitAssyHDL();
  virtual const char
    *emitArtXML(const char *wksFile),
    *emitWorkersHDL(const char *file),
    *emitAttribute(const char *attr);
  inline const char *myComment() const { return hdlComment(m_language); }
  void
    addAccess(OU::Property &p),
    emitWorkersAttribute(),
    deleteAssy(), // just to keep the assembly details out of most files
    emitRecordSignal(FILE *f, std::string &last, size_t maxPortTypeName, Port *p,
		     const char *prefix = ""),
    emitWorkers(FILE *f),
    emitWorker(FILE *f),
    emitInstances(FILE *f, const char *prefix, size_t &index),
    emitInternalConnections(FILE *f, const char *prefix),
    emitVhdlShell(FILE *f),
    emitVhdlSignalWrapper(FILE *f, const char *topinst = "rv"),
    emitVhdlRecordWrapper(FILE *f),
    emitParameters(FILE *f, Language lang, bool convert = false),
    emitPortDescription(Port *p, FILE *f, Language lang),
    emitSignals(FILE *f, Language lang, bool onlyDevices = false, bool records = false,
		bool inPackage = false),
    emitDeviceSignals(FILE *f, Language lang, std::string &last);
};


#if 0
// Are master signals inputs at this port?
static inline bool masterIn(Port *p) {
  return
    p->type == WCIPort ? 1 :
    p->type == WTIPort ? 1 :
    p->type == WMemIPort ? (p->u.wmemi.isSlave ? 1 : 0) :
    p->type == WMIPort ? 0 :
    p->type == WSIPort ? (p->u.wdi.isProducer ? 0 : 1) :
    false;
}
#endif

#define SKEL "-skel"
#define IMPL "-impl"
#define DEFS "-defs"
#define ASSY "-assy"
#define VHD ".vhd"
#define VER ".v"
#define VERH ".vh"
#define BOOL(b) ((b) ? "true" : "false")

#define IMPL_ATTRS "name", "spec", "paramconfig", "reentrant"
#define IMPL_ELEMS "componentspec", "properties", "property", "specproperty", "propertysummary", "xi:include", "controlinterface",  "timeservice", "unoc"
#define GENERIC_IMPL_CONTROL_ATTRS \
  "SizeOfConfigSpace", "ControlOperations", "Sub32BitConfigProperties"
#define ASSY_ELEMS "instance", "connection", "external"
extern const char
  *parseList(const char *list, const char * (*doit)(const char *tok, void *arg), void *arg),
  *parseControlOp(const char *op, void *arg),
  *vhdlValue(const OU::Value &v, std::string &value),
  *rccValue(OU::Value &v, std::string &value),
  *container, *platform, *device, *load, *os, *os_version, **libraries, **mappedLibraries, *assembly, *attribute,
  *addLibMap(const char *),
  *findLibMap(const char *file), // returns mapped lib name from dir name of file or NULL
  *propertyTypes[],
  *getNames(ezxml_t xml, const char *file, const char *tag, std::string &name, std::string &fileName),
  *tryOneChildInclude(ezxml_t top, const char *parent, const char *element,
		      ezxml_t *parsed, const char **childFile, bool optional),
  *emitContainerHDL(Worker*, const char *),
  *emitImplOCL(Worker*),
  *emitSkelRCC(Worker*),
  *emitSkelOCL(Worker*),
  *emitArtOCL(Worker *);

extern void
  emitVhdlLibraries(FILE *f),
  addLibrary(const char *lib),
  emitLastSignal(FILE *f, std::string &last, Language lang, bool end);

extern size_t ceilLog2(uint64_t n);

#endif
