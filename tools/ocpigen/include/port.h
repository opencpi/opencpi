// This contains generic port declarations
#ifndef PORT_H
#define PORT_H
#include <cstddef>
#include <string>
#include <cassert>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilAssembly.h"
#include "OcpiUtilPort.h"
#include "ocpigen.h"

// FIXME: this will not be needed when we fully migrate to classes...
enum WIPType {
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
  PropPort,     // raw property port for shared SPI/I2C
  RCCPort,      // An RCC port
  DevSigPort,   // a port between devices
  NWIPTypes
};

class Port;
struct OcpAdapt;
struct Clock;
class Protocol;
class Worker;
struct Connection;
struct Attachment;
// Bad that these are here, but it allows this file to be leaf, which is good
typedef std::list<Attachment*> Attachments;
typedef Attachments::const_iterator AttachmentsIter;

// FIXME: have "implPort" class??
class Port : public OCPI::Util::Port {
 protected:
  bool m_clone;
public:
  // These members are for spec ports
  Worker *m_worker;    // spec: FIXME: make this a reference 
  size_t count;        // spec: FIXME: can this change in impl???
  bool master;         // spec
  WIPType type;        // spec with WDI, with limited types, then impl ports
  // These members are for impl ports
  std::string fullNameIn, fullNameOut; // used during HDL generation
  std::string typeNameIn, typeNameOut; // impl: used during HDL generation
  const char *pattern; // impl: pattern if it overrides global pattern
  // These three are really for OCP ports, but its good to keep it generic where possible.
  Clock *clock;        // impl:
  Port *clockPort;     // impl: used temporarily until other port has clocks defined
  bool myClock;        // impl
  ezxml_t m_specXml;   // impl
  bool m_implOnly;     // not in spec
  Port(Worker &w, ezxml_t x, Port *sp, int ordinal, WIPType type, const char *defaultName,
       const char *&err);
  Port(const Port &other, Worker &w, std::string &name, size_t count, const char *&err);
  // Virtual constructor - every derived class must have one
  virtual Port &clone(Worker &w, std::string &name, size_t count,
		      OCPI::Util::Assembly::Role *role, const char *&err) const;
  virtual ~Port();
  virtual const char *parse();    // second pass parsing for ports referring to each other
  virtual bool masterIn() const;  // Are master signals inputs at this port?
  void addMyClock();
  virtual const char *checkClock();
  inline const char *name() const { return m_name.c_str(); }
  const char *doPattern(int n, unsigned wn, bool in, bool master, std::string &suff,
			bool port = false);
  virtual void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inWorker,
				const char *defaultIn = NULL, const char *defaultOut = NULL);
  virtual bool haveInputs() const { return true; }
  virtual bool haveWorkerInputs() const { return true; }
  virtual bool haveOutputs() const { return true; }
  virtual bool haveWorkerOutputs() const { return true; }
  virtual const char *typeName() const = 0;
  virtual const char *prefix() const = 0;
  virtual bool isOCP() const { return false; }
  virtual bool isData() const { return false; }
  virtual bool isDataProducer() const { assert(isData()); return false; }
  virtual bool isDataOptional() const { assert(isData()); return false; }
  virtual bool isDataBidirectional() const { assert(isData()); return false; }
  virtual bool isOptional() const { assert(isData()); return false; }
  virtual bool matchesDataProducer(bool /*isProducer*/) const { return false; }
  virtual void emitPortDescription(FILE *f, Language lang) const;
  virtual const char *deriveOCP();
  virtual void initRole(OCPI::Util::Assembly::Role &role);
  // Does this port imply the need for a control clock/reset in this worker?
  virtual bool needsControlClock() const;
  virtual void emitVhdlShell(FILE *f, Port *wci);
  virtual void emitImplAliases(FILE *f, unsigned n, Language lang);
  virtual void emitImplSignals(FILE *f);
  virtual void emitSkelSignals(FILE *f);
  virtual void emitRecordTypes(FILE *f);
  virtual void emitRecordDataTypes(FILE *f);
  virtual void emitRecordInputs(FILE *f);
  virtual void emitRecordOutputs(FILE *f);
  virtual void emitRecordInterface(FILE *f, const char *implName);
  virtual void emitRecordArray(FILE *f);
  //  virtual void emitWorkerEntitySignals(FILE *f, std::string &last, unsigned maxPropName);
  virtual void emitSignals(FILE *f, Language lang, std::string &last, bool inPackage,
			   bool inWorker);
  virtual void emitVerilogSignals(FILE *f);
  virtual void emitVHDLShellPortMap(FILE *f, std::string &last);
  virtual void emitVHDLSignalWrapperPortMap(FILE *f, std::string &last);
  virtual void emitVHDLRecordWrapperSignals(FILE *f);
  virtual void emitVHDLRecordWrapperAssignments(FILE *f);
  virtual void emitVHDLRecordWrapperPortMap(FILE *f, std::string &last);
  virtual void emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal);
  virtual void emitPortSignals(FILE *f, Attachments &atts, Language lang,
			       const char *indent, bool &any, std::string &comment,
			       std::string &last, const char *myComment, OcpAdapt *adapt);
  virtual const char *fixDataConnectionRole(OCPI::Util::Assembly::Role &role);
  virtual const char *doPatterns(unsigned nWip, size_t &maxPortTypeName);
  virtual void emitXML(std::string &out);
  virtual const char *finalizeRccDataPort();
  virtual const char *finalizeHdlDataPort();
  virtual const char *finalizeOclDataPort();
  virtual void emitRccCppImpl(FILE *f); 
  virtual void emitRccCImpl(FILE *f); 
  virtual void emitRccCImpl1(FILE *f); 
  virtual void emitExtAssignment(FILE *f, bool int2ext, const std::string &extName,
				 const std::string &intName, const Attachment &extAt,
				 const Attachment &intAt, size_t count) const;
  virtual const char *masterMissing() const { return "(others => '0')"; }
  virtual const char *slaveMissing() const { return "(others => '0')"; }
};

// Factory function template for port types
template <typename ptype> Port *createPort(Worker &w, ezxml_t x, Port *sp, int ordinal,
const char *&err) {
  err = NULL;
  Port *p = new ptype(w, x, sp, ordinal, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}
// A port creation function
typedef Port *PortCreate(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err);

#endif
