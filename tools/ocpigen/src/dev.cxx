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

#include "assembly.h"
#include "hdl.h"

DevSignalsPort::
DevSignalsPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, DevSigPort, "dev", err),
    m_hasInputs(false), m_hasOutputs(false) {
  if ((err = Signal::parseSignals(x, w.m_file, m_signals, m_sigmap, &w)))
    return;
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    switch (s.m_direction) {
    case Signal::IN:
      if (m_master)
	m_hasInputs = true;
      else
	m_hasOutputs = true;
      break;
    case Signal::OUT:
      if (m_master)
	m_hasOutputs = true;
      else
	m_hasInputs = true;
      break;
    default:
      err = "Unsupported signal direction (not in or out) in devsignals port";
      return;
    }
  }
}

// Our special copy constructor
DevSignalsPort::
DevSignalsPort(const DevSignalsPort &other, Worker &w, std::string &name, size_t a_count,
	       const char *&err)
  : Port(other, w, name, a_count, err) {
  if (err)
    return;
  m_signals = other.m_signals;
  m_sigmap = other.m_sigmap;
  m_hasInputs = other.m_hasInputs;
  m_hasOutputs = other.m_hasOutputs;
}

Port &DevSignalsPort::
clone(Worker &w, std::string &name, size_t a_count, OCPI::Util::Assembly::Role *, const char *&err)
  const {
  return *new DevSignalsPort(*this, w, name, a_count, err);
}

void DevSignalsPort::
emitRecordTypes(FILE *f) {
  fprintf(f, "\n");
  if (m_hasInputs)
    fprintf(f,
	    "  -- Record for DevSignals input signals for port \"%s\" of worker \"%s\"\n"
	    "  alias worker_%s_in_t is work.%s_defs.%s_in_t;\n",
	    pname(), m_worker->m_implName, pname(),
	    m_worker->m_implName, pname());
  if (m_hasOutputs)
    fprintf(f,
	    "  -- Record for DevSignals output signals for port \"%s\" of worker \"%s\"\n"
	    "  alias worker_%s_out_t is work.%s_defs.%s_out_t;\n",
	    pname(), m_worker->m_implName, pname(),
	    m_worker->m_implName, pname());
}

void DevSignalsPort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  // Define input record
  if (m_hasInputs) {
    fprintf(f,
	    "\n"
	    "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	    "  type %s_t is record\n",
	    m_master ? "master" : "slave", pname(), implName, in.c_str());
    std::string last;
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
      Signal &s = **si;
      if (s.m_direction == (m_master ? Signal::IN : Signal::OUT))
	emitSignal(s.m_name.c_str(), f, VHDL, Signal::NONE, last,
		   s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    }
    emitLastSignal(f, last, VHDL, false);
    fprintf(f,
	    "  end record %s_t;\n", in.c_str());
  }
  if (m_hasOutputs) {
    fprintf(f,
	    "\n"
	    "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	    "  type %s_t is record\n",
	    m_master ? "master" : "slave", pname(), implName, out.c_str());
    std::string last;
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
      Signal &s = **si;
      if (s.m_direction == (m_master ? Signal::OUT : Signal::IN))
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
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, bool /*clock*/, std::string &signal) {
  if ((output && !haveOutputs()) || (!output && !haveInputs()))
    return;
  std::string tname, suff, stype;
  m_worker->addParamConfigSuffix(suff);
  OU::format(tname, output ? typeNameOut.c_str() : typeNameIn.c_str(), "");
  OU::format(stype, "%s%s.%s_defs.%s%s_t", m_worker->m_library, suff.c_str(),
	     m_worker->m_implName, tname.c_str(), m_arrayCount ? "_array" : "");
  fprintf(f,
	  "  signal %s : %s;\n", signal.c_str(), stype.c_str());
}

// Emit for one direction: note the connection is optional
void DevSignalsPort::
emitPortSignalsDir(FILE *f, bool output, const char *indent, bool &any, std::string &comment,
		   std::string &last, Attachment *other) {
  std::string port;
  OU::format(port, output ? typeNameOut.c_str() : typeNameIn.c_str(), "");
  std::string conn;
  if (other)
    OU::format(conn,
	       m_master != output ?
	       other->m_connection.m_slaveName.c_str() :
	       other->m_connection.m_masterName.c_str(), "");
  if (!m_master)
    output = !output; // signals that are included are not relevant
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++)
    if (((*si)->m_direction == Signal::IN && !output) ||
	((*si)->m_direction == Signal::OUT && output))
      for (size_t n = 0; n < count(); n++) {
	std::string myindex;
	if (m_arrayCount)
	  OU::format(myindex, "(%zu)", n);
	std::string otherName = conn;
	if (other) {
	  if (other->m_instPort.m_port->count() > count() || m_arrayCount)
	    OU::formatAdd(otherName, "(%zu)", n + other->m_index);
	  OU::formatAdd(otherName, ".%s", (*si)->m_name.c_str());
	} else
	  otherName =
	    m_master == ((*si)->m_direction == Signal::IN) ?
	    ((*si)->m_width ? "(others => '0')" : "'0'") : "open";
	doPrev(f, last, comment, hdlComment(VHDL));
	fprintf(f, "%s%s%s.%s => %s",
		any ? indent : "", port.c_str(), myindex.c_str(), (*si)->m_name.c_str(),
		otherName.c_str());
	any = true;
      }
}
// These signal bundles are defined on both sides independently, so they must
// be assigned individually...
void DevSignalsPort::
emitPortSignals(FILE *f, const InstancePort &ip, Language /*lang*/, const char *indent,
		bool &any, std::string &comment, std::string &last, const char */*myComment*/,
		std::string &/*exprs*/) {
  Attachment *at = ip.m_attachments.front();
  Attachment *otherAt = NULL;
  if (at) {
    Connection &c = at->m_connection;
    // We need to know the indexing of the other attachment
    for (AttachmentsIter ai = c.m_attachments.begin(); ai != c.m_attachments.end(); ai++)
      if (*ai != at) {
	otherAt = *ai;
	break;
      }
    assert(otherAt);
  }
  if (haveInputs())
    emitPortSignalsDir(f, false, indent, any, comment, last, otherAt);
  if (haveOutputs())
    emitPortSignalsDir(f, true, indent, any, comment, last, otherAt);
}

void DevSignalsPort::
emitExtAssignment(FILE *f, bool int2ext, const std::string &extName, const std::string &intName,
		  const Attachment &extAt, const Attachment &intAt, size_t connCount) const {
  // We can't assume record type compatibility so we must assign individual signals.
  for (size_t n = 0; n < connCount; n++) {
    std::string ours = extName;
    if (m_arrayCount)
      OU::formatAdd(ours, "(%zu)", extAt.m_index + n);
    std::string theirs = intName;
    OU::formatAdd(theirs, "(%zu)", intAt.m_index + n);
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++)
      if ((int2ext && (*si)->m_direction == Signal::IN) ||
	  (!int2ext && (*si)->m_direction == Signal::OUT))
      fprintf(f, "  %s.%s <= %s.%s;\n",
	      int2ext ? ours.c_str() : theirs.c_str(), (*si)->cname(),
	      int2ext ? theirs.c_str() : ours.c_str(), (*si)->cname());
  }
}
