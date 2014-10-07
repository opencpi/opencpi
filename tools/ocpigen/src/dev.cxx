#include "assembly.h"
#include "hdl.h"

DevSignalsPort::
DevSignalsPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, DevSigPort, "dev", err),
    m_hasInputs(false), m_hasOutputs(false) {
  if ((err = Signal::parseSignals(x, w.m_file, m_signals)))
    return;
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    switch (s.m_direction) {
    case Signal::IN:
      m_hasInputs = true;
      break;
    case Signal::OUT:
      m_hasOutputs = true;
      break;
    default:
      err = "Unsupported signal direction (not in or out) in devsignals port";
      return;
    }
  }
}

void DevSignalsPort::
emitRecordTypes(FILE *f) {
  fprintf(f, "\n");
  if (master ? m_hasInputs : m_hasOutputs)
    fprintf(f,
	    "  -- Record for DevSignals input signals for port \"%s\" of worker \"%s\"\n"
	    "  alias worker_%s_in_t is work.%s_defs.%s_in_t;\n",
	    name(), m_worker->m_implName, name(),
	    m_worker->m_implName, name());
  if (master ? m_hasOutputs : m_hasInputs)
    fprintf(f,
	    "  -- Record for DevSignals output signals for port \"%s\" of worker \"%s\"\n"
	    "  alias worker_%s_out_t is work.%s_defs.%s_out_t;\n",
	    name(), m_worker->m_implName, name(),
	    m_worker->m_implName, name());
}

void DevSignalsPort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  // Define input record
  if (master ? m_hasInputs : m_hasOutputs) {
    fprintf(f,
	    "\n"
	    "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	    "  type %s_t is record\n",
	    master ? "master" : "slave", name(), implName, in.c_str());
    std::string last;
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
      Signal &s = **si;
      if (s.m_direction == (master ? Signal::IN : Signal::OUT))
	emitSignal(s.m_name.c_str(), f, VHDL, Signal::NONE, last,
		   s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    }
    emitLastSignal(f, last, VHDL, false);
    fprintf(f,
	    "  end record %s_t;\n", in.c_str());
  }
  if (master ? m_hasOutputs : m_hasInputs) {
    fprintf(f,
	    "\n"
	    "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	    "  type %s_t is record\n",
	    master ? "master" : "slave", name(), implName, out.c_str());
    std::string last;
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
      Signal &s = **si;
      if (s.m_direction == (master ? Signal::OUT : Signal::IN))
	emitSignal(s.m_name.c_str(), f, VHDL, Signal::NONE, last,
		   s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    }
    emitLastSignal(f, last, VHDL, false);
    fprintf(f,
	    "  end record %s_t;\n", out.c_str());
  }
  emitRecordArray(f);
}

void DevSignalsPort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, std::string &signal) {
  if (output && !haveOutputs() ||
      !output && !haveInputs())
    return;
  std::string tname;
  OU::format(tname, output ? typeNameOut.c_str() : typeNameIn.c_str(), "");
  std::string stype;
  OU::format(stype, "%s.%s_defs.%s%s_t", m_worker->m_library, m_worker->m_implName,
	     tname.c_str(),
	     count > 1 ? "_array" : "");
  //  if (count > 1)
  //    OU::formatAdd(stype, "(0 to %zu)", count - 1);
  fprintf(f,
	  "  signal %s : %s;\n", signal.c_str(), stype.c_str());
}

// Emit for one direction
void DevSignalsPort::
emitPortSignalsDir(FILE *f, bool output, const char *indent, bool &any, std::string &comment,
		   std::string &last, std::string &conn, std::string &index) {
  std::string port;
  OU::format(port, output ? typeNameOut.c_str() : typeNameIn.c_str(), "");

  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++)
    if ((*si)->m_direction == Signal::IN && !output ||
	(*si)->m_direction == Signal::OUT && output) {
      doPrev(f, last, comment, hdlComment(VHDL));
      fprintf(f, "%s%s.%s => %s%s.%s",
	      any ? indent : "", port.c_str(), (*si)->m_name.c_str(),
	      conn.c_str(),
	      index.c_str(), (*si)->m_name.c_str());
      any = true;
    }
}
// These signal bundles are defined on both sides independently, so they must
// be assigned individually...
void DevSignalsPort::
emitPortSignals(FILE *f, Attachments &atts, Language /*lang*/, const char *indent,
		bool &any, std::string &comment, std::string &last, const char */*myComment*/,
		OcpAdapt */*adapt*/) {
  Attachment *at = atts.front();
  Connection *c = at ? &at->m_connection : NULL;
  // We need to know the indexing of the other attachment
  Attachment *otherAt = NULL;
  for (AttachmentsIter ai = c->m_attachments.begin(); ai != c->m_attachments.end(); ai++)
    if (*ai != at) {
      otherAt = *ai;
      break;
    }
  assert(otherAt);
  std::string index;
  // Indexing is necessary when only when we are smaller than the other
  if (count < otherAt->m_instPort.m_port->count)
    if (c->m_count > 1)
      OU::format(index, "(%zu to %zu)", otherAt->m_index, otherAt->m_index + c->m_count - 1);
    else
      OU::format(index, "(%zu)", otherAt->m_index);
  std::string conn = master ? c->m_slaveName.c_str() : c->m_masterName.c_str();
  if (haveInputs())
    emitPortSignalsDir(f, false, indent, any, comment, last, conn, index);
  if (haveOutputs())
    emitPortSignalsDir(f, true, indent, any, comment, last, conn, index);
}
