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

#include <cassert>
#include <climits>
#include "hdl.h"
#include "assembly.h"

// nameOrdinal -1 is for using the default name without a number appended
// if sp != NULL we are morphing a spec port to a concrete impl port
Port::
Port(Worker &w, ezxml_t x, Port *sp, int nameOrdinal, WIPType type, const char *defaultName,
     const char *&err)
  : m_internal(NULL), m_morphed(false), m_worker(&w), m_ordinal(0), m_arrayCount(0), m_master(false),
    m_xml(x), m_type(type), pattern(NULL), m_clock(NULL), m_clockPort(SIZE_MAX), m_myClock(false),
    m_specXml(x) {
  if (sp) {
    // A sort of copy constructor from a spec port to an impl port
    m_morphed = true;
    m_name = sp->m_name;
    m_ordinal = sp->m_ordinal;
    m_arrayCount = sp->m_arrayCount; // may be overridden?
    m_countExpr = sp->m_countExpr;
    m_master = sp->m_master;
    m_specXml = sp->m_xml;
    m_clock = sp->m_clock;
    m_clockPort = sp->m_clockPort;
    m_myClock = sp->m_myClock;
  } else {
    const char *name = ezxml_cattr(x, "Name");
    m_name = name ? name : "";
    const char *portImplName = ezxml_cattr(x, "implName");
    if (m_name.empty()) {
      if (portImplName)
	m_name = portImplName;
      else if (defaultName)
	if (nameOrdinal == -1)
	  m_name = defaultName;
	else
	  OU::format(m_name, "%s%u", defaultName, nameOrdinal);
      else {
	err = "Missing \"name\" attribute for port";
	return;
      }
    } else if (portImplName) {
      err = OU::esprintf("Both \"Name\" and \"ImplName\" attributes of %s element are present",
			 x->name);
      return;
    }
    if (w.findPort(m_name.c_str())) {
      err = OU::esprintf("Can't create port named \"%s\" since it already exists",
			 m_name.c_str());
      return;
    }
    if ((err = OE::getBoolean(m_xml, "master", &m_master)) ||
	(err = OE::getExprNumber(m_xml, "count", m_arrayCount, NULL, m_countExpr, &w)))
      return;
    m_ordinal = w.m_ports.size();
  }
  pattern = ezxml_cattr(m_xml, "Pattern");
  if (sp)
    w.m_ports[m_ordinal] = this;
  else if (m_type == WCIPort && !m_master) {
    // we manage the port list here, and a slave control port must be
    // the first, which means we need to update ordinals when we add a control port
    // after other ports are established
    for (PortsIter pi = w.m_ports.begin(); pi != w.m_ports.end(); pi++)
      (*pi)->m_ordinal++;
    m_ordinal = 0;
    w.m_ports.insert(w.m_ports.begin(), this);
  } else
    w.m_ports.push_back(this);
}

// Only used by cloning - a special copy constructor
// Note we don't clone the m_countExpr since the count must be resolved at the
// instance's port in the assembly that we are cloning/externalizing.
Port::
Port(const Port &other, Worker &w, std::string &name, size_t a_count, const char *&err)
  : m_internal(&other), m_morphed(false), m_worker(&w), m_name(name), m_ordinal(w.m_ports.size()),
    m_arrayCount(a_count), m_master(other.m_master), m_xml(other.m_xml), m_type(other.m_type),
    pattern(NULL), m_clock(NULL), m_clockPort(SIZE_MAX), m_myClock(false), m_specXml(other.m_specXml)
{
  err = NULL; // this is the base class for everything
  for (PortsIter pi = w.m_ports.begin(); pi != w.m_ports.end(); pi++)
    if (!strcasecmp(name.c_str(), (**pi).pname())) {
      err = OU::esprintf("Duplicate port name \"%s\" for worker \"%s\"",
			 name.c_str(), w.cname());
      return;
    }
  w.m_ports.push_back(this);
}

Port &Port::
clone(Worker &, std::string &, size_t, OCPI::Util::Assembly::Role *, const char *&) const {
  throw OU::Error("Port type: %s cannot be cloned", typeName());
}

Port::
~Port() {
}

// Second pass parsing when ports might refer to one another.
const char *Port::
parse() {
  return NULL;
}

const char *Port::
resolveExpressions(OU::IdentResolver &ir) {
  return m_countExpr.length() ?
    parseExprNumber(m_countExpr.c_str(), m_arrayCount, NULL, &ir) : NULL;
}

bool Port::
needsControlClock() const {
  return false;
}

// Default
bool Port::
masterIn() const {
  return !m_master;
}

static const char *wipNames[] =
  { "Unknown", "WCI", "WSI", "WMI", "WDI", "WMemI", "WTI", "CPMaster",
	"uNOC", "Metadata", "TimeService", "TimeBase", "RawProperty", "SDP", 0};
const char *Port::
typeName() const { return wipNames[m_type]; }

const char *Port::
doPattern(int n, unsigned wn, bool in, bool master, std::string &suff, bool port) {
  const char *pat = port ? m_worker->m_portPattern : (pattern ? pattern : m_worker->m_pattern);
  const char *holdPat = pat;
  if (!pat) {
    suff = "";
    return 0;
  }
  char
    c,
    *s = (char *)malloc(strlen(pname()) + strlen(pat) * 3 + 10),
    *base = s;
  while ((c = *pat++)) {
    if (c != '%')
      *s++ = c;
    else if (!*pat)
      *s++ = '%';
    else {
      bool myMaster = master;
      //      bool noOrdinal = false;
      if (*pat == '!') {
	myMaster = !master;
	pat++;
      }
      if (*pat == '*') {
	//	noOrdinal = true;
	pat++;
      }
      switch (*pat++) {
      case '%':
	*s++ = '%';
	break;
      case 'm':
	*s++ = myMaster ? 'm' : 's';
	break;
      case 'M':
	*s++ = myMaster ? 'M' : 'S';
	break;
      case '0': // zero origin ordinal-within-profile
      case '1':
	sprintf(s, "%u", wn + ((unsigned)pat[-1] - '0'));
	while (*s) s++;
	break;
      case 'i':
	*s++ = in ? 'i' : 'o';
	break;
      case 'I':
	*s++ = in ? 'i' : 'o';
	break;
      case 'n':
	strcpy(s, in ? "in" : "out");
	while (*s)
	  s++;
	break;
      case 'N':
	strcpy(s, in ? "In" : "Out");
	while (*s)
	  s++;
	break;
      case 's': // interface name as is
      case 'S': // capitalized interface name
	strcpy(s, pname());
	if (pat[-1] == 'S')
	  *s = (char)toupper(*s);
	while (*s)
	  s++;
	// Port indices are not embedded in VHDL names since they are proper arrays
	if (isArray())
	  switch (n) {
	  case -1:
	    *s++ = '%';
	    *s++ = 's'; // allow for inserting ordinals later, optionally, hence %s
	    break;
	  case -2:
	    break;
	  default:
	    sprintf(s, "%u", n);
	    while (*s)
	      s++;
	  }
	break;
      case 'W': // capitalized profile name
	strcpy(s, typeName());
	s++;
        while (*s) {
	  *s = (char)tolower(*s);
	  s++;
	}
	break;
      case 'w': // lower case profile name
	strcpy(s, typeName());
        while (*s) {
	  *s = (char)tolower(*s);
	  s++;
	}
	break;
      default:
	free(base);
	return OU::esprintf("Invalid pattern rule: %s", holdPat);
      }
    }
  }
  *s++ = 0;
  suff = base;
  free(base);
  return NULL;
}

const char *Port::
doPatterns(unsigned nWip, size_t &maxPortTypeName) {
  const char *err;
  bool mIn = masterIn();
  // ordinal == -1 means insert "%u" into the name for using later
  if ((err = doPattern(-1, nWip, true, !mIn, fullNameIn)) ||
      (err = doPattern(-1, nWip, false, !mIn, fullNameOut)) ||
      (err = doPattern(-1, nWip, true, !mIn, typeNameIn, true)) ||
      (err = doPattern(-1, nWip, false, !mIn, typeNameOut, true)))
    return err;
  if (typeNameIn.length() > maxPortTypeName)
      maxPortTypeName = typeNameIn.length();
  if (typeNameOut.length() > maxPortTypeName)
    maxPortTypeName = typeNameOut.length();
  return NULL;
}

void Port::
emitPortDescription(FILE *f, Language lang) const {
  const char *comment = hdlComment(lang);
  std::string nbuf;
  if (isArray()) {
    if (m_countExpr.length())
      nbuf = m_countExpr;
    else
      OU::format(nbuf, " %zu", m_arrayCount);
  }
  fprintf(f,
	  "\n  %s The%s %s interface%s named \"%s\", with \"%s\" acting as %s%s:\n",
	  comment, nbuf.c_str(), typeName(),
	  nbuf.length() ? "s" : "", pname(), m_worker->m_implName,
	  isOCP() ? "OCP " : "", masterIn() ? "slave" : "master");
  if (m_clockPort != SIZE_MAX)
    fprintf(f, "  %s   Clock: uses the clock from interface named \"%s\"\n", comment,
	    m_worker->m_ports[m_clockPort]->pname());
  else if (m_myClock)
    fprintf(f, "  %s   Clock: this interface has its own clock, named \"%s\"\n", comment,
	    m_clock->signal());
  else if (m_clock)
    fprintf(f, "  %s   Clock: this interface uses the worker's clock named \"%s\"\n", comment,
	    m_clock->signal());
}

const char *Port::
finalizeRccDataPort() {
  return NULL;
}
const char *Port::
finalizeOclDataPort() {
  return NULL;
}
const char *Port::
finalizeHdlDataPort() {
  return NULL;
}

const char *Port::
deriveOCP() {
  return "Unexpected deriveOCP on non-OCP port";
}

void Port::
emitVhdlShell(FILE *, Port*) {
}

void Port::
emitImplAliases(FILE */*f*/, unsigned /*n*/, Language /*lang*/) {
}
void Port::emitImplSignals(FILE */*f*/) {
}

void Port::
emitSkelSignals(FILE */*f*/) {
}

void Port::
emitRecordTypes(FILE *f) {
  fprintf(f,"\n"
	  "  -- The following record(s) are for the inner/worker interfaces for port \"%s\"\n",
	  pname());
  emitRecordDataTypes(f);
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  // Introduce record for data port, with common signals
  fprintf(f,
	  "  type worker_%s_t is record\n", 
	  in.c_str());
  emitRecordInputs(f);
  fprintf(f,
	  "  end record worker_%s_t;\n", in.c_str());
  if (haveWorkerOutputs()) {
    fprintf(f,
	    "  type worker_%s_t is record\n", out.c_str());
    emitRecordOutputs(f);
    fprintf(f,
	    "  end record worker_%s_t;\n", out.c_str());
  }
}

void Port::
emitRecordDataTypes(FILE *) {}
void Port::
emitRecordInputs(FILE *) {}
void Port::
emitRecordOutputs(FILE *) {}
void Port::
emitRecordInterface(FILE */*f*/, const char */*implName*/) {}
// This not DEFAULT behavior, but basic/common behavior and should be called
// first unless not needed
void Port::
emitRecordInterfaceConstants(FILE *f) {
  if (isArray())
    fprintf(f, "  constant ocpi_port_%s_count : natural;\n", pname());
}
// This not DEFAULT behavior, but basic/common behavior and should be called
// first unless not needed
void Port::
emitInterfaceConstants(FILE *f, Language lang) {
  if (isArray())
    emitConstant(f, "ocpi_port_%s_count", lang, m_arrayCount);
}

void Port::
emitRecordArray(FILE *f) {
  if (isArray()) {
    std::string scount;
    OU::format(scount, "ocpi_port_%s_count", pname());
    if (haveInputs()) {
      std::string in;
      OU::format(in, typeNameIn.c_str(), "");
      fprintf(f,
	      "  type %s_array_t is array(0 to %s-1) of %s_t;\n",
	      in.c_str(), scount.c_str(), in.c_str());
    }
    if (haveOutputs()) {
      std::string out;
      OU::format(out, typeNameOut.c_str(), "");
      fprintf(f,
	      "  type %s_array_t is array(0 to %s-1) of %s_t;\n",
	      out.c_str(), scount.c_str(), out.c_str());
    }
  }
}

void Port::
emitRecordSignal(FILE *f, std::string &last, const char *aprefix, bool /*useRecord*/,
		 bool /*inPackage*/, bool inWorker,
		 const char */*defaultIn*/, const char */*defaultOut*/) {
  if (inWorker ? haveWorkerInputs() : haveInputs()) {
    if (last.size())
      fprintf(f, last.c_str(), ";\n");
    std::string in;
    OU::format(in, typeNameIn.c_str(), "");
    OU::format(last,
	       "  %-*s : in  %s%s%s_t%%s",
	       (int)m_worker->m_maxPortTypeName, in.c_str(), aprefix, in.c_str(),
	       isArray() ? "_array" : "");
  }
  if (inWorker ? haveWorkerOutputs() : haveOutputs()) {
    if (last.size())
      fprintf(f, last.c_str(), ";\n");
    std::string out;
    OU::format(out, typeNameOut.c_str(), "");
    OU::format(last,
	       "  %-*s : out %s%s%s_t%%s",
	       (int)m_worker->m_maxPortTypeName, out.c_str(), aprefix, out.c_str(),
	       isArray() ? "_array" : "");
  }
}

// Default for all (non-ocp!) types is to just use record signals for VHDL anyway
void Port::
emitSignals(FILE *f, Language lang, std::string &last, bool inPackage, bool inWorker,
	    bool /*convert*/) {
  if (lang == VHDL) {
    std::string aprefix;
    if (!inPackage)
      OU::format(aprefix, "work.%s_defs.", m_worker->m_implName);
    emitRecordSignal(f, last, aprefix.c_str(), false, inPackage, inWorker);
  }	  
}

void Port::
emitVerilogSignals(FILE */*f*/) {}

void Port::
emitConstant(FILE *f, const char *nameFormat, Language lang, size_t n) const {
  std::string s;
  OU::format(s, nameFormat, pname());
  fprintf(f, "%s%s %s %zu;\n", lang == VHDL ? "  constant " : "localparam ", s.c_str(),
	  lang == VHDL ? ": natural :=" : "=", n);
}

#if 0
void Port::
emitVerilogPortParameters(FILE */*f*/) {}
#endif
void Port::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  if (haveWorkerInputs()) {
    std::string in;
    OU::format(in, typeNameIn.c_str(), "");
    fprintf(f,
	    "%s    %s => %s",
	    last.c_str(),
	    in.c_str(), in.c_str());
    last = ",\n";
  }
  if (haveWorkerOutputs()) {
    std::string out;
    OU::format(out, typeNameOut.c_str(), "");
    fprintf(f,
	    "%s    %s => %s",
	    last.c_str(),
	    out.c_str(), out.c_str());
  }
}

void Port::
emitVHDLSignalWrapperPortMap(FILE *f, std::string &last) {
  emitVHDLShellPortMap(f, last);
}

void Port::
emitVHDLRecordWrapperSignals(FILE */*f*/) {
}
void Port::
emitVHDLRecordWrapperAssignments(FILE */*f*/) {
}

void Port::
emitVHDLRecordWrapperPortMap(FILE */*f*/, std::string &/*last*/) {
  ocpiAssert("Non-application port types can't be in VHDL assy wrappers" == 0);
}

void Port::
emitConnectionSignal(FILE */*f*/, bool /*output*/, Language /*lang*/, bool /*clock*/, std::string &/*signal*/) {
}

void Port::
emitPortSignals(FILE *f, const InstancePort &ip, Language /*lang*/, const char *indent,
		bool &any, std::string &comment, std::string &last, const char *myComment,
		std::string &/*exprs*/) {
  const Attachments &atts = ip.m_attachments;
  doPrev(f, last, comment, myComment);
  std::string in, out, index, empty;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  Attachment *at = atts.size() ? atts.front() : NULL;
  assert(!at || at->m_instPort.m_port == this);
  std::string mName, sName;
  Attachment *otherAt = NULL;
  if (atts.size()) {
    Connection &c = at->m_connection;
    // We need to know the indexing of the other attachment
    for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++)
      if (*ai != at) {
	otherAt = *ai;
	break;
      }
    assert(otherAt);
    // Indexing is necessary only when we are smaller than the other
    if (m_arrayCount < otherAt->m_instPort.m_port->m_arrayCount) {
      if (c.m_count > 1 || isArray())
	OU::format(index, "(%zu to %zu)", otherAt->m_index, otherAt->m_index + c.m_count - 1);
      else
	OU::format(index, "(%zu)", otherAt->m_index);
    }
    OU::format(mName, c.m_masterName.c_str(), "");
    OU::format(sName, c.m_slaveName.c_str(), "");
#if 0 // busted until m_count=0 indicates non-array ports properly
    // If the instance port is an array but the other is not, index the instance port
    if ((m_arrayCount || !m_countExpr.empty()) && otherAt->m_instPort.m_port->m_arrayCount <= 1) {
      in += "(0)";
      out += "(0)";
    }
#endif
  } else
    mName = sName = "open";
  InstancePort *other;
  // input, then output
  if (haveInputs()) {
    other = at && at->m_instPort.m_signalIn.empty() ? &otherAt->m_instPort : NULL;
    emitPortSignal(f, any, indent, in, m_master ? (at ? sName : empty) : (at ? mName : empty),
		   index, false, other ? other->m_port : NULL, other && other->m_external);
    any = true;
  }
  if (haveOutputs()) {
    if (haveInputs())
      fprintf(f, ",\n");
    other = at && at->m_instPort.m_signalOut.empty() ? &otherAt->m_instPort : NULL;
    emitPortSignal(f, any, indent, out, m_master ? mName : sName, index, true,
		   other ? other->m_port : NULL, other && other->m_external);
  }
}

void Port::
emitPortSignal(FILE *f, bool any, const char *indent, const std::string &fName,
	       const std::string &aName, const std::string &index, bool /*output*/,
	       const Port */*signalPort*/, bool /*external*/) {
  fprintf(f, "%s%s => ", any ? indent : "", fName.c_str());
  if (aName.empty()) {
    const char *missing = m_master ? slaveMissing() : masterMissing();
    if (isArray())
      fprintf(f, "(others => %s)", missing);
    else
      fprintf(f, "%s", missing);
  } else
    fprintf(f, "%s%s", aName.c_str(), index.c_str());
}

void Port::
emitXML(std::string &) {}

const char *Port::
emitRccCppImpl(FILE *) {
  return NULL;
}
void Port::
emitRccCImpl(FILE *) {}
void Port::
emitRccCImpl1(FILE *) {}
const char *Port::
finalizeExternal(Worker &/*aw*/, Worker &/*iw*/, InstancePort &/*ip*/,
		 bool &/*cantDataResetWhileSuspended*/) {
  return NULL;
}
void Port::
emitRccArgTypes(FILE *, bool &) {}

RawPropPort::
RawPropPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, PropPort, "rawProps", err) {
}

// Our special copy constructor
RawPropPort::
RawPropPort(const RawPropPort &other, Worker &w , std::string &name, size_t a_count,
	    const char *&err)
  : Port(other, w, name, a_count, err) {
  if (err)
    return;
}

Port &RawPropPort::
clone(Worker &w, std::string &name, size_t a_count, OCPI::Util::Assembly::Role *, const char *&err)
  const {
  RawPropPort *rpp = new RawPropPort(*this, w, name, a_count, err);
  // We need to externalize the fact that the count was an expression, to preserve "array-ness"
  if (m_countExpr.length())
    OU::format(rpp->m_countExpr, "%zu", a_count);
  return *rpp;
}

void RawPropPort::
emitRecordTypes(FILE *f) {
  fprintf(f,
	  "\n"
	  "  -- Record for the RawProp input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_in_t is wci.raw_prop_%s_t;\n"
	  "  -- Record for the RawProp  output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_out_t is wci.raw_prop_%s_t;\n",
	  pname(), m_worker->m_implName, pname(), m_master ? "in" : "out",
	  pname(), m_worker->m_implName, pname(), m_master ? "out" : "in");
}

void RawPropPort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is wci.raw_prop_%s_t;\n"
	  "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is wci.raw_prop_%s_t;\n",
	  typeName(), pname(), implName,
	  in.c_str(), m_master ? "in" : "out",
	  typeName(), pname(), implName,
	  out.c_str(), m_master ? "out" : "in");
  if (isArray())
      fprintf(f,
	      "  subtype %s_array_t is wci.raw_prop_%s_array_t(0 to ocpi_port_%s_count-1);\n"
	      "  subtype %s_array_t is wci.raw_prop_%s_array_t(0 to ocpi_port_%s_count-1);\n",
	      in.c_str(), m_master ? "in" : "out", pname(),
	      out.c_str(), m_master ? "out" : "in", pname());
}

void RawPropPort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, bool /*clock*/, std::string &signal) {
  fprintf(f, "  signal %s : wci.raw_prop_%s%s_t",
	  signal.c_str(), m_master == output ? "out" : "in",
	  isArray() ? "_array" : "");
  if (isArray()) {
    Worker &w = *m_worker;
    std::string lib(w.m_library);
    w.addParamConfigSuffix(lib);
    fprintf(f, "(0 to %s.%s_constants.ocpi_port_%s_count-1)", lib.c_str(), w.m_implName, pname());
  }
  fprintf(f, ";\n");
}

const char *RawPropPort::
masterMissing() const {
  return "wci.raw_prop_out_zero";
}
const char *RawPropPort::
slaveMissing() const {
  return "wci.raw_prop_in_zero";
}

CpPort::
CpPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, CPPort, "cp", err) {
}

// Our special copy constructor
CpPort::
CpPort(const CpPort &other, Worker &w , std::string &name, size_t a_count, const char *&err)
  : Port(other, w, name, a_count, err) {
  if (err)
    return;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &CpPort::
clone(Worker &w, std::string &name, size_t a_count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  return *new CpPort(*this, w, name, a_count, err);
}

void CpPort::
emitRecordTypes(FILE *f) {
  fprintf(f,
	  "\n"
	  "  -- Record for the CPMaster input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_in_t is platform.platform_pkg.occp_out_t;\n"
	  "  -- Record for the CPMaster  output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_out_t is platform.platform_pkg.occp_in_t;\n",
	  pname(), m_worker->m_implName, pname(), pname(), m_worker->m_implName, pname());
}
void CpPort::
emitRecordInterface(FILE *f, const char *implName) {
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.occp_out_t;\n"
	  "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.occp_in_t;\n",
	  typeName(), pname(), implName, typeNameIn.c_str(),
	  typeName(), pname(), implName, typeNameOut.c_str());
}

void CpPort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, bool /*clock*/, std::string &signal) {
  fprintf(f, "  signal %s : platform.platform_pkg.occp_%s_t;\n",
	  signal.c_str(), m_master == output ? "in" : "out");
}

NocPort::
NocPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, NOCPort, "unoc", err) {
}

// Our special copy constructor
NocPort::
NocPort(const NocPort &other, Worker &w , std::string &name, size_t a_count,
		const char *&err)
  : Port(other, w, name, a_count, err) {
  if (err)
    return;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &NocPort::
clone(Worker &w, std::string &name, size_t a_count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  return *new NocPort(*this, w, name, a_count, err);
}

void NocPort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.unoc_master_%s_t;\n"
	  "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.unoc_master_%s_t;\n",
	  typeName(), pname(), implName, in.c_str(), m_master ? "in" : "out",
	  typeName(), pname(), implName, out.c_str(), m_master ? "out" : "in");
}

void NocPort::
emitRecordTypes(FILE */*f*/) {
}

void NocPort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, bool /*clock*/, std::string &signal) {
  fprintf(f, "  signal %s : platform.platform_pkg.unoc_master_%s_t;\n",
	  signal.c_str(), m_master == output ? "out" : "in" );
}


TimeServicePort::
TimeServicePort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, TimePort, "time", err) {
}
// Our special copy constructor
TimeServicePort::
TimeServicePort(const TimeServicePort &other, Worker &w , std::string &name, size_t a_count,
		const char *&err)
  : Port(other, w, name, a_count, err) {
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &TimeServicePort::
clone(Worker &w, std::string &name, size_t a_count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  return *new TimeServicePort(*this, w, name, a_count, err);
}

void TimeServicePort::
emitRecordTypes(FILE *f) {
  fprintf(f,
	  "\n"
	  "  -- Record for the TimeService output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_out_t is platform.platform_pkg.time_service_t;\n",
	  pname(), m_worker->m_implName, pname());
}

void TimeServicePort::
emitRecordSignal(FILE *f, std::string &last, const char *aprefix, bool /*useRecord*/,
		 bool /*inPackage*/, bool /*inWorker*/, const char *, const char *) {
  if (last.size())
    fprintf(f, last.c_str(), ";");
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  OU::format(last,
	     "  %-*s : %s  %s%s_t%%s",
	     (int)m_worker->m_maxPortTypeName, 
	     m_master ? out.c_str() : in.c_str(),
	     m_master ? "out" : "in", aprefix,
	     m_master ? out.c_str() : in.c_str());
}

void TimeServicePort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.time_service_t;\n",
	  typeName(), pname(), implName, m_master ? out.c_str() : in.c_str());
}

void TimeServicePort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "%s    %s => %s",
	  last.c_str(),
	  m_master ? out.c_str() : in.c_str(),
	  m_master ? out.c_str() : in.c_str());
}

void TimeServicePort::
emitVHDLSignalWrapperPortMap(FILE *f, std::string &last) {
  emitVHDLShellPortMap(f, last);
}

void TimeServicePort::
emitPortSignals(FILE *f, const InstancePort &ip, Language /*lang*/, const char *indent,
		bool &any, std::string &comment, std::string &last, const char *myComment,
		std::string &/*exprs*/) {
  const Attachments &atts = ip.m_attachments;
  doPrev(f, last, comment, myComment);
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  // Only one direction - master outputs to slave
  fprintf(f, "%s%s => ",
	  any ? indent : "",
	  m_master ? out.c_str() : in.c_str());
  //	fputs(p.master ? c.m_masterName.c_str() : c.m_slaveName.c_str(), f);
  const Attachment *at = atts.front();
  const Connection *c = at ? &at->m_connection : NULL;
  fputs(at ? c->m_masterName.c_str() : "open", f);
}

void TimeServicePort::
emitConnectionSignal(FILE *f, bool /*output*/, Language /*lang*/, bool /*clock*/, std::string &signal) {
  fprintf(f, "  signal %s : platform.platform_pkg.time_service_t;\n", signal.c_str());
}

TimeBasePort::
TimeBasePort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, TimeBase, "timebase", err) {
}
// Our special copy constructor
TimeBasePort::
TimeBasePort(const TimeBasePort &other, Worker &w , std::string &name, size_t a_count,
		const char *&err)
  : Port(other, w, name, a_count, err) {
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &TimeBasePort::
clone(Worker &w, std::string &name, size_t a_count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  return *new TimeBasePort(*this, w, name, a_count, err);
}

void TimeBasePort::
emitRecordTypes(FILE *f) {
  fprintf(f,
	  "\n"
	  "  -- Record for the Timebase output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_out_t is platform.platform_pkg.time_base_%s_t;\n"
	  "  alias worker_%s_in_t is platform.platform_pkg.time_base_%s_t;\n",
	  pname(), m_worker->m_implName,
	  pname(), m_master ? "out" : "in",
	  pname(), m_master ? "in" : "out");
}

#if 0
void TimeBasePort::
emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool /*inWorker*/,
		 const char *, const char *) {
  if (last.size())
    fprintf(f, last.c_str(), ";");
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  OU::format(last,
	     "  %-*s : %s  %s%s_t%%s",
	     (int)m_worker->m_maxPortTypeName, 
	     m_master ? out.c_str() : in.c_str(),
	     m_master ? "out" : "in", prefix,
	     m_master ? out.c_str() : in.c_str());
}
#endif
void TimeBasePort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Records for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.time_base_%s_t;\n"
	  "  alias %s_t is platform.platform_pkg.time_base_%s_t;\n",
	  typeName(), pname(), implName,
	  in.c_str(), m_master ? "in" : "out",
	  out.c_str(), m_master ? "out" : "in");
}

#if 0
void TimeBasePort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "%s    %s => %s",
	  last.c_str(),
	  m_master ? out.c_str() : in.c_str(),
	  m_master ? out.c_str() : in.c_str());
}
#endif
void TimeBasePort::
emitVHDLSignalWrapperPortMap(FILE *f, std::string &last) {
  emitVHDLShellPortMap(f, last);
}

#if 0
void TimeBasePort::
emitPortSignals(FILE *f, Attachments &atts, Language /*lang*/, const char *indent,
		bool &any, std::string &comment, std::string &last, const char *myComment,
		OcpAdapt */*adapt*/) {
  doPrev(f, last, comment, myComment);
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  // Only one direction - master outputs to slave
  fprintf(f, "%s%s => ",
	  any ? indent : "",
	  m_master ? out.c_str() : in.c_str());
  //	fputs(p.master ? c.m_masterName.c_str() : c.m_slaveName.c_str(), f);
  Attachment *at = atts.front();
  Connection *c = at ? &at->m_connection : NULL;
  fputs(at ? c->m_masterName.c_str() : "open", f);
}
#endif

void TimeBasePort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, bool /*clock*/, std::string &signal) {
  fprintf(f, "  signal %s : platform.platform_pkg.time_base_%s_t;\n", signal.c_str(),
	  m_master == output ? "out" : "in");
}

MetaDataPort::
MetaDataPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, MetadataPort, "metadata", err) {
}

// Our special copy constructor
MetaDataPort::
MetaDataPort(const MetaDataPort &other, Worker &w , std::string &name, size_t a_count,
		const char *&err)
  : Port(other, w, name, a_count, err) {
  if (err)
    return;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &MetaDataPort::
clone(Worker &w, std::string &name, size_t a_count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  return *new MetaDataPort(*this, w, name, a_count, err);
}

void MetaDataPort::
emitRecordTypes(FILE *f) {
  fprintf(f,
	  "\n"
	  "  -- Record for the CPMaster input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_in_t is platform.platform_pkg.metadata_out_t;\n"
	  "  -- Record for the CPMaster  output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias worker_%s_out_t is platform.platform_pkg.metadata_in_t;\n",
	  pname(), m_worker->m_implName, pname(), pname(), m_worker->m_implName, pname());
}

void MetaDataPort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.metadata_%s_t;\n"
	  "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is platform.platform_pkg.metadata_%s_t;\n",
	  typeName(), pname(), implName,
	  in.c_str(), m_master ? "in" : "out",
	  typeName(), pname(), implName,
	  out.c_str(), m_master ? "out" : "in");
}

void MetaDataPort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, bool /*clock*/, std::string &signal) {
  fprintf(f, "  signal %s : platform.platform_pkg.metadata_%s_t;\n",
	  signal.c_str(), (output && m_master) || (!output && !m_master) ? "out" : "in");
}

const char *Port::
fixDataConnectionRole(OU::Assembly::Role &role) {
  if (role.m_knownRole)
    return OU::esprintf("Role of port %s of worker %s in connection is incompatible with a port"
			" of type \"%s\"",
			pname(), m_worker->m_implName, typeName());
  role.m_provider = !m_master;
  role.m_bidirectional = false;
  role.m_knownRole = true;
  return NULL;
}

void Port::
initRole(OCPI::Util::Assembly::Role &) {
}

void Port::
emitExtAssignment(FILE *f, bool int2ext, const std::string &extName, const std::string &intName,
		  const Attachment &extAt, const Attachment &intAt, size_t connCount) const {
  std::string ours = extName;
  if (isArray()) { //connCount < m_arrayCount || m_countExpr.size()) {
    if (connCount == 1)
      OU::formatAdd(ours, "(%zu)", extAt.m_index);
    else
      OU::formatAdd(ours, "(%zu to %zu)", extAt.m_index, extAt.m_index + connCount - 1);
  }
  std::string theirs = intName;
  if (connCount == 1)
    OU::formatAdd(theirs, "(%zu)", intAt.m_index);
  else
    OU::formatAdd(theirs, "(%zu to %zu)", intAt.m_index, intAt.m_index + connCount - 1);
  fprintf(f, "  %s <= %s;\n",
	  int2ext ? ours.c_str() : theirs.c_str(),
	  int2ext ? theirs.c_str() : ours.c_str());
}
const char *Port::
adjustConnection(Connection &/*c*/, bool /*isProducer*/, OcpAdapt */*myAdapt*/, bool &/*myHasExpr*/,
		 ::Port &/*otherPort*/, OcpAdapt */*otherAdapt*/, bool &/*otherHasExpr*/,
		 Language /*lang*/, size_t &/*unused*/) {
  return NULL;
}
void Port::
getClockSignal(const InstancePort &/*ip*/, Language /*lang*/, std::string &/*s*/) {
  assert("unexpected call to getClockSignal"==0);
}
