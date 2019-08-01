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

#include "hdl.h"
#include "assembly.h"

SdpPort::
SdpPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : Port(w, x, sp, ordinal, SDPPort, "sdp", err) {
}

// Our special copy constructor
SdpPort::
SdpPort(const SdpPort &other, Worker &w , std::string &arg_name, size_t count,
		const char *&err)
  : Port(other, w, arg_name, count, err) {
  if (err)
    return;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &SdpPort::
clone(Worker &w, std::string &arg_name, size_t count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  return *new SdpPort(*this, w, arg_name, count, err);
}

void SdpPort::
emitRecordInterfaceConstants(FILE *f) {
  Port::emitRecordInterfaceConstants(f); // for our "arrayness"
}
void SdpPort::
emitInterfaceConstants(FILE *f, Language lang) {
  Port::emitInterfaceConstants(f, lang);
}

// This is basically a clone of the RawPropPort - a platform port type with fixed type,
// that may be an array.  FIXME: have a base class for this behavior
void SdpPort::
emitRecordInterface(FILE *f, const char *implName) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is sdp.sdp.%s_t;\n"
	  "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is sdp.sdp.%s_t;\n",
	  typeName(), pname(), implName,
	  in.c_str(), m_master ? "s2m" : "m2s",
	  typeName(), pname(), implName,
	  out.c_str(), m_master ? "m2s" : "s2m");
  emitRecordArray(f);
  fprintf(f,
	  "  subtype %s_data_t is dword_array_t(0 to to_integer(sdp_width)-1);\n"
	  "  subtype %s_data_t is dword_array_t(0 to to_integer(sdp_width)-1);\n",
	  in.c_str(), out.c_str());
  // When we have a count, we define the data array type
  if (isArray())
      fprintf(f,
	      "  type %s_data_array_t is array(0 to ocpi_port_%s_count-1) of %s_data_t;\n"
	      "  type %s_data_array_t is array(0 to ocpi_port_%s_count-1) of %s_data_t;\n",
	      in.c_str(), pname(), in.c_str(), out.c_str(), pname(), out.c_str());
#if 0
      fprintf(f,
	      "  type %s_data_array_t is array(0 to ocpi_port_%s_count-1) of "
	      "dword_array_t(0 to to_integer(sdp_width)-1);\n"
	      "  type %s_data_array_t is array(0 to ocpi_port_%s_count-1) of "
	      "dword_array_t(0 to to_integer(sdp_width)-1);\n",
	      in.c_str(), pname(), out.c_str(), pname());
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "\n"
	  "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is sdp.sdp.%s_t;\n"
	  "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	  "  alias %s_t is sdp.sdp.%s_t;\n",
	  typeName(), pname(), implName, in.c_str(), m_master ? "s2m" : "m2s",
	  typeName(), pname(), implName, out.c_str(), m_master ? "m2s" : "s2m");
  emitRecordArray(f);
  fprintf(f, "  subtype %s_data_t is dword_array_t(0 to integer(sdp_width)-1);\n", pname());
  if (isArray()) {
    std::string scount;
    if (m_countExpr.length())
      OU::format(scount, "ocpi_port_%s_count", pname());
    else
      OU::format(scount, "%zu", m_arrayCount);
    fprintf(f,
	    "  type %s_data_array_t is array(0 to %s-1) of %s_data_t;\n"
	    "  type %s_data_array_t is array(0 to %s-1) of %s_data_t;\n",
	    in.c_str(), scount.c_str(), pname(),
	    out.c_str(), scount.c_str(), pname());
  }
#endif
}

void SdpPort::
emitRecordTypes(FILE */*f*/) {
}

void SdpPort::
emitConnectionSignal(FILE *f, bool output, Language /*lang*/, bool /*clock*/, std::string &signal) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  std::string suff;
  m_worker->addParamConfigSuffix(suff);
  fprintf(f, "  signal %s : %s%s.%s_defs.%s%s_t;\n",
	  signal.c_str(), m_worker->m_implName, suff.c_str(), m_worker->m_implName,
	  output ? out.c_str() : in.c_str(), isArray() ? "_array" : "");
  //  if (m_arrayCount || m_countExpr.length())
  //    fprintf(f, "(0 to %s.%s_constants.ocpi_port_%s_count-1)", m_worker->m_implName,
  //	    m_worker->m_implName, pname());
  //  fprintf(f, ";\n");
  fprintf(f, "  signal %s_data : %s%s.%s_defs.%s_data%s_t;\n", signal.c_str(),
	  m_worker->m_implName, suff.c_str(), m_worker->m_implName,
	  output ? out.c_str() : in.c_str(),
	  isArray() ? "_array" : "");
#if 0
  if (isArray())
      fprintf(f,
	      "  signal %s : %s_%s_array_t;\n"
	      "  signal %s_data : %s_%s_data_array_t;\n",
	      signal.c_str(), pname(), output ? "out" : "in",
	      signal.c_str(), pname(), output ? "out" : "in"); 
  else
    fprintf(f,
	    "  signal %s : %s_%s_t;\n"
	    "  signal %s_data : dword_array_t(0 to to_integer(sdp_width)-1);\n",
	    signal.c_str(), pname(), output ? "_out" : "_in",
	    signal.c_str());
#endif
}

void SdpPort::
emitRecordSignal(FILE *f, std::string &last, const char *aprefix, bool inRecord, bool inPackage,
		 bool inWorker, const char *defaultIn, const char *defaultOut) {
  Port::emitRecordSignal(f, last, aprefix, inRecord, inPackage, inWorker, defaultIn, defaultOut);
  fprintf(f, last.c_str(), ";\n");
  std::string in, out;
  OU::format(in, "%s_in_data", pname());
  OU::format(out, "%s_out_data", pname());
  if (isArray()) {
    std::string scount;
    if (m_countExpr.length())
      OU::format(scount, "ocpi_port_%s_count", pname());
    else
      OU::format(scount, "%zu", m_arrayCount);
    OU::format(last,
	       "  %-*s : in  %s_array_t;\n"
	       "  %-*s : out %s_array_t%%s",
	       (int)m_worker->m_maxPortTypeName, in.c_str(), in.c_str(),
	       (int)m_worker->m_maxPortTypeName, out.c_str(), out.c_str());
  } else
    OU::format(last,
	       "  %-*s : in  dword_array_t(0 to to_integer(%s)-1);\n"
	       "  %-*s : out dword_array_t(0 to to_integer(%s)-1)%%s",
	       (int)m_worker->m_maxPortTypeName, in.c_str(),
	       inRecord ? "sdp_width" : "unsigned(sdp_width)",
	       (int)m_worker->m_maxPortTypeName, out.c_str(),
	       inRecord ? "sdp_width" : "unsigned(sdp_width)");
}

void SdpPort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  Port::emitVHDLShellPortMap(f, last);
  std::string in;
  OU::format(in, typeNameIn.c_str(), "");
  fprintf(f,
	  "%s"
	  "    %s_in_data => %s_in_data,\n"
	  "    %s_out_data => %s_out_data\n",
	  last.c_str(), pname(), pname(), pname(), pname());
}

void SdpPort::
emitPortSignal(FILE *f, bool any, const char *indent, const std::string &fName,
	       const std::string &aName, const std::string &index, bool output,
	       const Port *signalPort, bool external) {
  std::string
    formal(fName), formal_data(fName + "_data"),
    actual(aName + index), actual_data(aName + "_data" + index),
    empty;
  if (signalPort) {
    std::string suff;
    m_worker->addParamConfigSuffix(suff);
    if (output) {
      if (aName == "open") {
	actual = "open";
	actual_data = "open";
      } else {
	OU::format(formal, "%s%s.%s_defs.%s%s",
		   external ? "work" : signalPort->worker().m_implName,
		   external ? "" : suff.c_str(),
		   signalPort->worker().m_implName, signalPort->pname(),
		   external ? "_out" : "_in");
	formal_data = formal + "_data";
	if (index.empty() && signalPort->isArray()) {
	  formal += "_array";
	  formal_data += "_array";
	}
	OU::formatAdd(formal, "_t(%s)", fName.c_str());
	OU::formatAdd(formal_data, "_t(%s_data)", fName.c_str());
      }
    } else {
      if (aName.empty()) {
	actual = m_master ? slaveMissing() : masterMissing();
	actual_data = "(others => (others => '0'))";
      } else {
	OU::format(actual, "%s%s.%s_defs.%s%s_t(%s%s)",
		   m_worker->m_implName, suff.c_str(), m_worker->m_implName,
		   fName.c_str(), isArray() ? "_array" : "",
		   aName.c_str(), index.c_str());
	OU::format(actual_data, "%s%s.%s_defs.%s_data%s_t(%s_data%s)", m_worker->m_implName,
		   suff.c_str(), m_worker->m_implName, fName.c_str(),
		   isArray() ? "_array" : "", aName.c_str(),
		   index.c_str());
      }
    }
  }
  Port::emitPortSignal(f, any, indent, formal, actual, empty, output, NULL, false);
  fprintf(f, ",\n");
  Port::emitPortSignal(f, true, indent, formal_data, actual_data, empty, output, NULL, false);
}
