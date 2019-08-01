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

// This contains generic port declarations
#ifndef PORT_H
#define PORT_H
#include <cstddef>
#include <string>
#include <cassert>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilAssembly.h"
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
  TimeBase,     // TimeBase port - basis for time service
  PropPort,     // raw property port for shared SPI/I2C
  RCCPort,      // An RCC port
  DevSigPort,   // a port between devices
  SDPPort,
  NWIPTypes
};

class DataPort;
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

struct InstancePort;
// FIXME: have "implPort" class??
class Port {
protected:
  const Port *m_internal; // internal port that was cloned to make this one
  bool m_morphed;      // spec->impl transitioned
  Worker *m_worker;    // spec: FIXME: name this a reference 
public:
  // These members are for spec ports
  std::string m_name;  // spec:
  size_t m_ordinal;    // spec:
  size_t m_arrayCount; // spec: 0 means no array.  FIXME: can this change in impl???
  std::string m_countExpr;
  bool m_master;       // spec
  ezxml_t m_xml;       // spec or impl
  WIPType m_type;      // spec with WDI, with limited types, then impl ports
  // These members are for impl ports
  std::string fullNameIn, fullNameOut; // used during HDL generation
  std::string typeNameIn, typeNameOut; // impl: used during HDL generation
  const char *pattern; // impl: pattern if it overrides global pattern
  // These three are really for OCP ports, but its good to keep it generic where possible.
  Clock *m_clock;      // impl:
  size_t m_clockPort;  // impl: used temporarily until other port has clocks defined
  bool m_myClock;      // impl
  ezxml_t m_specXml;   // impl
  Port(Worker &w, ezxml_t x, Port *sp, int ordinal, WIPType type, const char *defaultName,
       const char *&err);
  Port(const Port &other, Worker &w, std::string &name, size_t count, const char *&err);
  // Virtual constructor - every derived class must have one
  virtual Port &clone(Worker &w, std::string &name, size_t count,
		      OCPI::Util::Assembly::Role *role, const char *&err) const;
  virtual ~Port();
  // count whether an array or not
  size_t count() const { return m_arrayCount ? m_arrayCount : 1; }
  bool isArray() const { return m_arrayCount || !m_countExpr.empty(); }
  Worker &worker() const { return *m_worker; }
  const char *parseClock(ezxml_t);
  virtual const char *parse();    // second pass parsing for ports referring to each other
  virtual const char *resolveExpressions(OCPI::Util::IdentResolver &ir);
  virtual bool masterIn() const;  // Are master signals inputs at this port?
  Clock &addMyClock(bool output);
  const char *addMyClock(const char *direction);
  virtual const char *checkClock();
  // This is not cname to deal with a multiple-inheritance issue.  The runtime classes use cname
  const char *pname() const { return m_name.c_str(); }
  const char *doPattern(int n, unsigned wn, bool in, bool master, std::string &suff,
			bool port = false);
  void emitConstant(FILE *f, const char *nameFormat, Language lang, size_t n) const;
  bool isCloned() const { return m_internal != NULL; }
  virtual void emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inRecord,
				bool inPackage, bool inWorker, const char *defaultIn = NULL,
				const char *defaultOut = NULL);
  virtual bool haveInputs() const { return true; }
  virtual bool haveWorkerInputs() const { return true; }
  virtual bool haveOutputs() const { return true; }
  virtual bool haveWorkerOutputs() const { return true; }
  virtual const char *typeName() const = 0;
  virtual const char *prefix() const = 0;
  virtual bool isOCP() const { return false; }
  virtual bool isData() const { return false; }
  virtual DataPort *dataPort() { return NULL; }
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
  virtual void emitRecordInterfaceConstants(FILE *f);
  virtual void emitInterfaceConstants(FILE *f, Language lang);
  virtual void emitRecordArray(FILE *f);
  //  virtual void emitWorkerEntitySignals(FILE *f, std::string &last, unsigned maxPropName);
  virtual void emitSignals(FILE *f, Language lang, std::string &last, bool inPackage,
			   bool inWorker, bool convert = false);
  virtual void emitVerilogSignals(FILE *f);
  //  virtual void emitVerilogPortParameters(FILE *f);
  virtual void emitVHDLShellPortMap(FILE *f, std::string &last);
  virtual void emitVHDLSignalWrapperPortMap(FILE *f, std::string &last);
  virtual void emitVHDLRecordWrapperSignals(FILE *f);
  virtual void emitVHDLRecordWrapperAssignments(FILE *f);
  virtual void emitVHDLRecordWrapperPortMap(FILE *f, std::string &last);
  virtual void emitConnectionSignal(FILE *f, bool output, Language lang, bool clock, std::string &signal);
  virtual void getClockSignal(const InstancePort &ip, Language lang, std::string &s);
#if 1
  virtual void emitPortSignals(FILE *f, const InstancePort &ip, Language lang, const char *indent, bool &any,
			       std::string &comment, std::string &last, const char *myComment,
			       std::string &exprs);
#else
  virtual void emitPortSignals(FILE *f, Attachments &atts, Language lang,
			       const char *indent, bool &any, std::string &comment,
			       std::string &last, const char *myComment, OcpAdapt *adapt,
			       std::string *signalIn, std::string &clockSignal, std::string &exprs);
#endif
  virtual void emitPortSignal(FILE *f, bool any, const char *indent, const std::string &fName,
			      const std::string &aName, const std::string &index, bool output,
			      const Port *signalPort, bool external);
  virtual const char *fixDataConnectionRole(OCPI::Util::Assembly::Role &role);
  virtual const char *doPatterns(unsigned nWip, size_t &maxPortTypeName);
  virtual void emitXML(std::string &out);
  virtual const char *finalizeRccDataPort();
  virtual const char *finalizeHdlDataPort();
  virtual const char *finalizeOclDataPort();
  virtual const char *emitRccCppImpl(FILE *f); 
  virtual void emitRccCImpl(FILE *f); 
  virtual void emitRccCImpl1(FILE *f); 
  virtual void emitRccArgTypes(FILE *f, bool &first);
  virtual void emitExtAssignment(FILE *f, bool int2ext, const std::string &extName,
				 const std::string &intName, const Attachment &extAt,
				 const Attachment &intAt, size_t count) const;
  virtual const char *masterMissing() const { return "(others => '0')"; }
  virtual const char *slaveMissing() const { return "(others => '0')"; }
  virtual const char *finalizeExternal(Worker &aw, Worker &iw, InstancePort &ip,
				       bool &cantDataResetWhileSuspended);
  // isProducer is passed here as the known role for this port of the connection
  virtual const char *adjustConnection(Connection &c, bool isProducer, OcpAdapt *myAdapt,
				       bool &myHasExpr, ::Port &otherPort, OcpAdapt *otherAdapt,
				       bool &otherHasExpr, Language lang, size_t &unused);
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
