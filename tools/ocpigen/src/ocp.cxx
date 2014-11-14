#include <assert.h>
#include "OcpiUtilMisc.h"
#include "ocp.h"
#include "hdl.h"
#include "assembly.h"

namespace OE=OCPI::Util::EzXml;
namespace OU=OCPI::Util;

#undef OCP_SIGNAL_MT
#define OCP_SIGNAL_MT(n, w) {#n, true, true, w, true, false, OCP_##n},
#define OCP_SIGNAL_MTR(n, w) {#n, true, true, w, true, true, OCP_##n},
#undef OCP_SIGNAL_ST
#define OCP_SIGNAL_ST(n, w) {#n, true, false, w, true, false, OCP_##n},
#define OCP_SIGNAL_MS(n) {#n, false, true, 0, false, false, OCP_##n},
#define OCP_SIGNAL_MV(n, w) {#n, true, true, w, false, false, OCP_##n},
#define OCP_SIGNAL_MSR(n) {#n, false, true, 0, false, true, OCP_##n},
#define OCP_SIGNAL_MVR(n, w) {#n, true, true, w, false, true, OCP_##n},
#define OCP_SIGNAL_SS(n) {#n, false, false, 0, false, false, OCP_##n},
#define OCP_SIGNAL_SV(n, w) {#n, true, false, w, false, false, OCP_##n},
OcpSignalDesc ocpSignals [N_OCP_SIGNALS+1] = {
OCP_SIGNALS
{0,0,0,0,0,0,N_OCP_SIGNALS}
};

#undef OCP_SIGNAL_MS
#undef OCP_SIGNAL_MV
#undef OCP_SIGNAL_SS
#undef OCP_SIGNAL_SV

static unsigned myfls(uint64_t n) {
  for (int i = sizeof(n)*8; i > 0; i--)
    if (n & ((uint64_t)1 << (i - 1)))
      return i;
  return 0;
}

size_t ceilLog2(uint64_t n) {
  return OCPI_UTRUNCATE(size_t, n ? myfls(n - 1) : 0);
}
size_t floorLog2(uint64_t n) {
  //  ocpiInfo("Floor log2 of %u is %u", n, myfls(n)-1);
  return OCPI_UTRUNCATE(size_t, myfls(n) - 1);
}

OcpPort::
OcpPort(Worker &w, ezxml_t x, Port *sp, int ordinal, WIPType type, const char *defName,
	const char *&err) 
  : Port(w, x, sp, ordinal, type, defName, err),
    m_values(NULL), m_nAlloc(0), m_impreciseBurst(false),
    m_preciseBurst(false), m_dataWidth(0), m_byteWidth(0), m_continuous(false) {
  memset(&ocp, 0, sizeof(ocp));
  if (err)
    return;
  if (sp) {
    // WHAT CAN YOU SPECIFY HERE IN A SPEC PORT?  We assume nothing
  }
  const char *clockName;
  if (!(err = OE::getBoolean(x, "MyClock", &myClock)) &&
      myClock &&
      (clockName = ezxml_cattr(x, "clock")))
    err = OU::esprintf("port \"%s\" refers to clock \"%s\","
		       " and also has MyClock=true, which is invalid",
		       name(), clockName);
  else if (!(err = OE::getBoolean(x, "ImpreciseBurst", &m_impreciseBurst)) &&
	   !(err = OE::getBoolean(x, "Continuous", &m_continuous)) &&
	   !(err = OE::getNumber(x, "DataWidth", &m_dataWidth, 0, 8)) &&
	   !(err = OE::getNumber(x, "ByteWidth", &m_byteWidth, 0, m_dataWidth)))
    err = OE::getBoolean(x, "PreciseBurst", &m_preciseBurst);
  // We can't create clocks at this point based on myclock, since
  // it might depend on what happens with other ports.
}

// Our special copy constructor
OcpPort::
OcpPort(const OcpPort &other, Worker &w , std::string &name, size_t count, const char *&err)
  : Port(other, w, name, count, err) {
  if (err)
    return;
  if (other.m_values) {
    m_values = new uint8_t[m_nAlloc];
    memcpy(m_values, other.m_values, m_nAlloc);
  } else
    m_values = NULL;
  m_nAlloc = other.m_nAlloc;
  m_impreciseBurst = other.m_impreciseBurst;
  m_preciseBurst = other.m_preciseBurst;
  m_dataWidth = other.m_dataWidth;
  m_byteWidth = other.m_byteWidth;
  m_continuous = other.m_continuous;
  ocp = other.ocp;
}

void OcpPort::
emitPortDescription(FILE *f, Language lang) const {
  Port::emitPortDescription(f, lang);
  fprintf(f, "  %s WIP attributes for this %s interface are:\n", hdlComment(lang), typeName());
}

void OcpPort::
emitRecordSignal(FILE *f, std::string &last, const char */*prefix*/, bool inWorker) {
  if (last.size())
    fprintf(f, last.c_str(), ";");
  std::string temp;
  OU::format(temp,
	     "    -- Signals for %s %s port named \"%s\".  See record types above.\n"
	     "    %-*s : in  %s%s_t",
	     typeName(), masterIn() ? "input" : "output", name(),
	     (int)m_worker->m_maxPortTypeName, typeNameIn.c_str(), inWorker ? "worker_" : "",
	     typeNameIn.c_str());
  if (inWorker ? haveWorkerOutputs() : haveOutputs())
    OU::format(last,
	       "%s;\n"
	       "    %-*s : out %s%s_t%%s",
	       temp.c_str(),
	       (int)m_worker->m_maxPortTypeName, typeNameOut.c_str(), inWorker ? "worker_" : "",
	       typeNameOut.c_str());
  else
    OU::format(last, "%s%%s", temp.c_str());
}

void OcpPort::
emitSignals(FILE *f, Language lang, std::string &last, bool /*inPackage*/, bool /*inWorker*/) {
  const char *comment = hdlComment(lang);
  bool mIn = masterIn();
  OcpSignalDesc *osd;
  for (unsigned n = 0; n < count; n++) {
    if (!myClock)
      fprintf(f,
	      "  %s No Clk signal here. The \"%s\" interface uses \"%s\" as clock\n",
	      comment, name(), clock->signal());
    osd = ocpSignals;
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (os->master == mIn && /* strcmp(osd->name, "Clk") && */ os->value) {
	emitSignal(os->signal, f, lang, Signal::IN,
		   last, osd->vector ? (int)os->width : -1, n);
      }
    osd = ocpSignals;
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (os->master != mIn && /* strcmp(osd->name, "Clk") && */ os->value) {
	emitSignal(os->signal, f, lang, Signal::OUT,
		   last, osd->vector ? (int)os->width : -1, n);
      }
  }
}

void OcpPort::
emitRecordInterface(FILE *f, const char *implName) {
  bool mIn = masterIn();
  //      emitPortDescription(p, f);
  fprintf(f, "\n"
	  "  -- These 2 records correspond to the input and output sides of the OCP bundle\n"
	  "  -- for the \"%s\" worker's \"%s\" profile interface named \"%s\"\n",
	  implName, typeName(), name());
  fprintf(f,
	  "\n  -- Record for the %s input (OCP %s) signals for port \"%s\" of worker \"%s\"\n",
	  typeName(), mIn ? "master" : "slave", name(), implName);
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f, "  type %s_t is record\n", in.c_str());
  OcpSignalDesc *osd = ocpSignals;
  for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
    //      if ((osd->master == mIn && strcmp(osd->name, "Clk")) && os->value) {
    if (os->master == mIn && os->value) {
      fprintf(f, "    %-20s: ", osd->name);
      if (osd->type)
	fprintf(f, "ocpi.ocp.%s_t", osd->name);
      else if (osd->vector)
	fprintf(f, "std_logic_vector(%zu downto 0)", os->width - 1);
      else
	fprintf(f, "std_logic");
      fprintf(f, ";\n");
    }
  fprintf(f, "  end record %s_t;\n", in.c_str());
  fprintf(f,
	  "\n  -- Record for the %s output (OCP %s) signals for port \"%s\" of worker \"%s\"\n"
	  "  type %s_t is record\n",
	  typeName(), mIn ? "slave" : "master",
	  name(), implName, out.c_str());
  osd = ocpSignals;
  for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
    if (os->master != mIn /* && strcmp(osd->name, "Clk"))*/ && os->value) {
      fprintf(f, "    %-20s: ", osd->name);
      if (osd->type)
	fprintf(f, "ocpi.ocp.%s_t", osd->name);
      else if (osd->vector)
	fprintf(f, "std_logic_vector(%zu downto 0)", os->width - 1);
      else
	fprintf(f, "std_logic");
      fprintf(f, ";\n");
    }
  fprintf(f, "  end record %s_t;\n", out.c_str());
  if (count > 1)
    emitRecordArray(f);
}

bool OcpPort::
needsControlClock() const {
  return true;
}

// Post processing for deriveOCP
void OcpPort::
fixOCP() {
  OcpSignal *o = ocp.signals;
  OcpSignalDesc *osd = ocpSignals;
  for (unsigned i = 0; i < N_OCP_SIGNALS; i++, o++, osd++)
    if (o->value || o->width) {
      if (osd->vector) {
	if (osd->width)
	  o->width = osd->width;
      } else
	o->width = 1;
      m_nAlloc += o->width;
    }
  m_values = (uint8_t*)calloc(m_nAlloc, 1);
  uint8_t *v = m_values;
  o = ocp.signals;
  osd = ocpSignals;
  for (unsigned i = 0; i < N_OCP_SIGNALS; i++, o++, osd++)
    if (o->value || o->width) {
      o->value = v;
      v += o->width;
    }
}

const char *OcpPort::
deriveOCP() {
  OcpSignal *o = ocp.signals;
  OcpSignalDesc *osd = ocpSignals;
  for (unsigned i = 0; i < N_OCP_SIGNALS; i++, o++, osd++)
    o->master = osd->master;
  static uint8_t s[1]; // a non-zero string pointer
  if (myClock)
    ocp.Clk.value = s;
  return NULL;
}
const char *OcpPort::
doPatterns(unsigned nWip, size_t &maxPortTypeName) {
  const char *err;
  if ((err = Port::doPatterns(nWip, maxPortTypeName)))
    return err;
  if (clock && clock->port == this && clock->m_signal.empty()) {
    std::string sin;
    // ordinal == -2 means suppress ordinal
    if ((err = doPattern(count > 1 ? 0 : -2, nWip, true, !masterIn(), sin)))
      return err;
    asprintf(&ocp.Clk.signal, "%s%s", sin.c_str(), "Clk");
    clock->m_signal = ocp.Clk.signal;
  }
  OcpSignalDesc *osd = ocpSignals;
  for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
    if (os->master == masterIn() && os->value)
      asprintf(&os->signal, "%s%s", fullNameIn.c_str(), osd->name);
  osd = ocpSignals;
  for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
    if (os->master != masterIn() && os->value)
      asprintf(&os->signal, "%s%s", fullNameOut.c_str(), osd->name);
  return NULL;
}

void OcpPort::
emitVerilogSignals(FILE *f) {
  for (unsigned n = 0; n < count; n++) {
    OcpSignalDesc *osd = ocpSignals;
    std::string num;
    OU::format(num, "%u", n);
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (os->master == masterIn() && /* strcmp(osd->name, "Clk")) && */ os->value) {
	char *name;
	asprintf(&name, os->signal, num.c_str());
	if (osd->vector)
	  fprintf(f, "  input  [%3zu:0] %s;\n", os->width - 1, name);
	else
	  fprintf(f, "  input          %s;\n", name);
      }
    osd = ocpSignals;
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (os->master != masterIn() && /* strcmp(osd->name, "Clk")) && */ os->value) {
	char *name;
	asprintf(&name, os->signal, num.c_str());
	if (osd->vector)
	  fprintf(f, "  output [%3zu:0] %s;\n", os->width - 1, name);
	else
	  fprintf(f, "  output         %s;\n", name);
      }
  }
}
void OcpPort::
emitVHDLSignalWrapperPortMap(FILE *f, std::string &last) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  OcpSignalDesc *osd = ocpSignals;
  for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
    if (os->value && os->master) {
      fprintf(f, "%s      %s.%s => %s", last.c_str(),
	      os->master == masterIn() ? in.c_str() : out.c_str(),
	      osd->name, os->signal);
      last = ",\n";
    }
  osd = ocpSignals;
  for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
    if (os->value && !os->master) {
      fprintf(f, "%s      %s.%s => %s", last.c_str(),
	      os->master == masterIn() ? in.c_str() : out.c_str(),
	      osd->name, os->signal);
      last = ",\n";
    }
}

void OcpPort::
emitVHDLRecordWrapperSignals(FILE *f) {
  for (unsigned n = 0; n < count; n++) {
    std::string num;
    OU::format(num, "%u", n);
    std::string in, out;
    OU::format(in, typeNameIn.c_str(), "");
    OU::format(out, typeNameOut.c_str(), "");
    OcpSignalDesc *osd = ocpSignals;
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (os->value) {
	std::string name;
	OU::format(name, os->signal, num.c_str());
	if (os->value) {
	  fprintf(f, "      signal %s : std_logic", name.c_str());
	  if (osd->vector)
	    fprintf(f, "_vector(%zu downto 0)", os->width - 1);
	  fprintf(f, ";\n");
	}
      }
  }

}
void OcpPort::
emitVHDLRecordWrapperAssignments(FILE *f) {
  for (unsigned n = 0; n < count; n++) {
    std::string num;
    OU::format(num, "%u", n);
    std::string in, out;
    OU::format(in, typeNameIn.c_str(), "");
    OU::format(out, typeNameOut.c_str(), "");
    OcpSignalDesc *osd = ocpSignals;
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (os->value) {
	std::string name;
	OU::format(name, os->signal, num.c_str());
	if (os->value) {
	  std::string rec;
	  if (count > 1)
	    OU::format(rec, "%s(%u).%s", os->master == masterIn() ? in.c_str() : out.c_str(),
		       n, osd->name);
	  else
	    OU::format(rec, "%s.%s", os->master == masterIn() ? in.c_str() : out.c_str(),
		       osd->name);
	  fprintf(f, "  %s <= %s;\n",
		  os->master == masterIn() ? name.c_str() : rec.c_str(),
		  os->master == masterIn() ? rec.c_str() : name.c_str());
	}
      }
  }
}
void OcpPort::
emitVHDLRecordWrapperPortMap(FILE *f, std::string &last) {
  for (unsigned n = 0; n < count; n++) {
    std::string num;
    OU::format(num, "%u", n);
    std::string in, out;
    OU::format(in, typeNameIn.c_str(), "");
    OU::format(out, typeNameOut.c_str(), "");
    OcpSignalDesc *osd = ocpSignals;
    for (OcpSignal *os = ocp.signals; osd->name; os++, osd++)
      if (os->value) {
	std::string name;
	OU::format(name, os->signal, num.c_str());
	if (os->value) {
	  fprintf(f, "%s      %s => %s", last.c_str(),
		  name.c_str(), name.c_str());
	  last = ",\n";
	}
      }
  }
}
void OcpPort::
emitConnectionSignal(FILE *f, bool output, Language lang, std::string &signal) {
  if (lang == Verilog) {
    // Generate signals when both sides has the signal configured.
    OcpSignalDesc *osd;
    OcpSignal *os;
    bool wantMaster = master && output || !master && !output;
    for (osd = ocpSignals, os = ocp.signals; osd->name; os++, osd++)
      if (os->master == wantMaster && os->value) {
	fprintf(f, "wire ");
	if (osd->vector)
	  fprintf(f, "[%3zu:0] ", os->width - 1);
	else
	  fprintf(f, "        ");
	fprintf(f, "%s%s;\n", signal.c_str(), osd->name);
      }
  } else {
    std::string tname;
    OU::format(tname, output ? typeNameOut.c_str() : typeNameIn.c_str(), "");
    std::string stype;
    Worker &w = *m_worker;
    // WCI ports on assemblies are always generic generic
    if (type == WCIPort && (master || w.m_assembly))
      OU::format(stype, "wci.wci_%s_%st", output ? "m2s" : "s2m",
		 count > 1 ? "array_" : "");
    else
      OU::format(stype, "%s.%s_defs.%s%s_t", w.m_library, w.m_implName, tname.c_str(),
		 count > 1 ? "_array" : "");
    if (count > 1)
      OU::formatAdd(stype, "(0 to %zu)", count - 1);
    // Make master the canonical type?
    fprintf(f,
	    "  signal %s : %s;\n", signal.c_str(), stype.c_str());
  }	       
}

// Emit for one direction
void OcpPort::
emitPortSignalsDir(FILE *f, bool output, Language lang, const char *indent,
		   bool &any, std::string &comment, std::string &last,
		   OcpAdapt *adapt, Attachments &atts) {
  std::string name;
  OU::format(name, output ? typeNameOut.c_str() : typeNameIn.c_str(), "");
  // only do WCI with individual signals if it is a slave that isn't an assembly
  OcpSignalDesc *osd;
  OcpSignal *os;
  OcpAdapt *oa;
  for (osd = ocpSignals, os = ocp.signals, oa = adapt; osd->name; os++, osd++, oa++)
    // If the signal is in the interface
    if (os->value && (output ? os->master == master : os->master != master)) {
      std::string signal, thisComment;
      connectOcpSignal(*osd, *os, *oa, signal, thisComment, lang, atts);
      /* if (signal.length()) */ {
	// We have a new one, so can close the previous one
	doPrev(f, last, comment, hdlComment(lang));
	if (lang == VHDL) {
	  if (any)
	    fputs(indent, f);
	  fprintf(f, "%s.%s => %s", name.c_str(),
		  //		  p.master == os->master ? out.c_str() : in.c_str(),
		  osd->name, signal.c_str());
	} else {
	  fprintf(f, "  .%s(%s",
		  os->signal, signal.c_str());
	  fprintf(f, ")");
	}
	comment = oa->comment ? oa->comment : thisComment.c_str();
	any = true;
      }
    }
}

void OcpPort::
emitPortSignals(FILE *f, Attachments &atts, Language lang,
		const char *indent, bool &any, std::string &comment, std::string &last,
		const char */*myComment*/, OcpAdapt *adapt) {
  emitPortSignalsDir(f, false, lang, indent, any, comment, last, adapt, atts);
  emitPortSignalsDir(f, true, lang, indent, any, comment, last, adapt, atts);
}

void OcpPort::
connectOcpSignal(OcpSignalDesc &osd, OcpSignal &os, OcpAdapt &oa,
		 std::string &signal, std::string &thisComment, Language lang,
		 Attachments &atts) {
  if (atts.empty()) {
    // A truly unconnected port.  All we want is a tieoff if it is an input
    // We can always use zero since that will assert reset
    if (os.master != master)
      if (lang == VHDL)
	signal = osd.vector ? "(others => '0')" : "'0'";
      else
	OU::format(signal, "%zu'b0", os.width);
    else if (lang == VHDL)
      signal = "open";
    return;
  }
  // Find the other end of the connection
  assert(atts.size() == 1); // OCP connections are always point-to-point
  Connection &c = atts.front()->m_connection;
  assert(c.m_attachments.size() == 2);
  InstancePort *otherIp = NULL;
  Attachment *at;
  for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++)
    if ((*ai)->m_instPort.m_port != this) {
      at = *ai;
      otherIp = &at->m_instPort;
      break;
    }
  assert(otherIp);
  // Decide on our indexing.  We need an index if our attachment is a subset of
  // what we are connecting to, which is either another internal port or an external one.
  // In either case
  std::string &cName = os.master ? c.m_masterName : c.m_slaveName;
  size_t index, top, count = 0; // count for indexing purpose
  if (otherIp->m_port->count > c.m_count) {
    // We're connecting to something bigger: indexing is needed in this port binding
    count = c.m_count;
    index = at->m_index;
    top = index + count - 1;
  }
  std::string temp;
  if (count) {
    std::string num, temp1;
    if (lang == Verilog)
      OU::format(num, "%zu", index);
    OU::format(temp1, cName.c_str(), num.c_str());
    if (count > 1) {
      assert(lang == VHDL);
      OU::format(temp, "%s(%zu to %zu)", temp1.c_str(), index, top);
    } else {
      if (lang == VHDL)
	OU::format(temp, "%s(%zu)", temp1.c_str(), index);
      else
	temp = temp1;
    }
  } else
    OU::format(temp, cName.c_str(), "");
  if (lang == VHDL)
    temp += '.';
  if (oa.expr) {
    std::string other;
    if (oa.other != N_OCP_SIGNALS)
      temp += ocpSignals[oa.other].name;
    OU::formatAdd(signal, oa.expr, temp.c_str());
  } else {
    signal = temp + osd.name;
    OcpPort &other = *static_cast<OcpPort*>(otherIp->m_port);
    if (osd.vector && os.width != other.ocp.signals[osd.number].width) {
      OU::formatAdd(signal, lang == Verilog ? "[%zu:0]" : "(%zu downto 0)", os.width - 1);
      thisComment = "worker is narrower than external, which is OK";
    }
  }
}
