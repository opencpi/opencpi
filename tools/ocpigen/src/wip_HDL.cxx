/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
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
 *  OpenCPI is free software: you can redistribute it and/for modify
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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <assert.h>
#include <cstdio>
#include "OcpiUtilMisc.h"
#include "wip.h"

namespace OU = OCPI::Util;

#if 0
const char *Worker::
doPattern(Port *p, int n, unsigned wn, bool in, bool master, std::string &suff, bool port) {
  const char *pat = port ? m_portPattern : (p->pattern ? p->pattern : m_pattern);
  if (!pat) {
    suff = "";
    return 0;
  }
  char
    c,
    *s = (char *)malloc(strlen(p->name()) + strlen(m_pattern) * 3 + 10),
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
	sprintf(s, "%u", wn + (pat[-1] - '0'));
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
	strcpy(s, p->name());
	if (pat[-1] == 'S')
	  *s = (char)toupper(*s);
	while (*s)
	  s++;
	// Port indices are not embedded in VHDL names since they are proper arrays
	if (p->count > 1)
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
	strcpy(s, p->typeName());
	s++;
        while (*s) {
	  *s = (char)tolower(*s);
	  s++;
	}
	break;
      case 'w': // lower case profile name
	strcpy(s, p->typeName());
        while (*s) {
	  *s = (char)tolower(*s);
	  s++;
	}
	break;
      default:
	free(base);
	return OU::esprintf("Invalid pattern rule: %s", m_pattern);
      }
    }
  }
  *s++ = 0;
  suff = base;
  free(base);
  return 0;
}
#endif
void
emitSignal(const char *signal, FILE *f, Language lang, Signal::Direction dir,
	   std::string &last, int width, unsigned n, const char *pref,
	   const char *type, const char *value) {
  int pad = 22 - (int)strlen(signal);
  char *name;
  std::string num;
  OU::format(num, "%u", n);
  asprintf(&name, signal, num.c_str());
  if (lang == VHDL) {
    const char *io = dir == Signal::IN ? "in " : (dir == Signal::OUT ? "out" : "inout");
    if (last.size())
      fprintf(f, last.c_str(), ";");
    if (width < 0) {
      OU::format(last, "  %s  %s%*s: %s %s%s%s%%s\n",
		 pref, name, pad, "", io, type ? type : "std_logic",
		 value ? " := " : "", value ? value : "");
    } else
      OU::format(last, "  %s  %s%*s: %s std_logic_vector(%u downto 0)%%s\n",
		 pref, name, pad, "", io, width - 1);
  } else {
    const char *io = dir == Signal::IN ? "input" : (dir == Signal::OUT ? "output" : "inout");
    if (last.size())
      fprintf(f, last.c_str(), ",");
    if (width < 0)
      OU::format(last, "  %s%%s %*s%s %s\n",
		 name, pad, "", hdlComment(lang), io);
    else
      OU::format(last, "  %s%%s %*s%s %s [%3u:0]\n",
		 name, pad, "", hdlComment(lang), io,
		 width - 1);
  }
  free(name);
}

void
emitLastSignal(FILE *f, std::string &last, Language lang, bool end) {
  if (last.size()) {
    fprintf(f, last.c_str(), end ? "" : (lang == VHDL ? ";" : ","));
    last = "";
  }
}

static void
vhdlBaseType(const OU::ValueType &dt, std::string &s, bool convert) {
  if (dt.m_baseType == OA::OCPI_Bool && convert)
    s = "std_logic_vector(0 to 0)";
  else {
    for (const char *cp = OU::baseTypeNames[dt.m_baseType]; *cp; cp++)
      s += (char)tolower(*cp);
    s += "_t";
  }
}
static void
vhdlArrayType(const OU::ValueType &dt, size_t rank, const size_t *dims, std::string &s,
	      bool convert) {
  s += "array";
  for (unsigned i = 0; i < rank; i++)
    OU::formatAdd(s, "%s0 to %zu", i == 0 ? "(" : ", ", dims[i] - 1);
  s += ") of ";
  vhdlBaseType(dt, s, convert);
}
// Only put out types that have parameters, otherwise the
// built-in types are used.
void
vhdlType(const OU::ValueType &dt, std::string &decl, std::string &type, bool convert) {
  if (dt.m_baseType == OA::OCPI_String && dt.m_stringLength)
    OU::format(decl, "array(0 to %zu) of char_t", dt.m_stringLength);
  else if (dt.m_arrayDimensions)
    vhdlArrayType(dt, dt.m_arrayRank, dt.m_arrayDimensions, decl, convert);
  else if (dt.m_isSequence) {
    decl = "type seqarray_t is ";
    vhdlArrayType(dt, 1, &dt.m_sequenceLength, decl, convert);
    decl += "; type seq is record length : ulong_t; data : array : seqarray_t; end record";
  } else
    vhdlBaseType(dt, type, convert);
 }
static struct VhdlUnparser : public OU::Unparser {
  void
  elementUnparse(const OU::Value &v, std::string &s, unsigned nSeq, bool hex, char comma,
		 bool wrap, const Unparser &up) const {
    if (wrap) s+= '(';
    Unparser::elementUnparse(v, s, nSeq, hex, comma, false, up);
    if (wrap) s+= ')';
    
  }
  // We wrap the basic value in a conversion function, and also suppress
  // the suppression of zeroes...
  bool
  valueUnparse(const OU::Value &v, std::string &s, unsigned nSeq, size_t nArray, bool hex,
	       char comma, bool /*wrap*/, const Unparser &up) const {
    s += "to_";
    for (const char *cp = OU::baseTypeNames[v.m_vt->m_baseType]; *cp; cp++)
      s += (char)tolower(*cp);
    s += '(';
    Unparser::valueUnparse(v, s, nSeq, nArray, hex, comma, false, up);
    s += ')';
    return false;
  }
  bool 
  unparseBool(std::string &s, bool val, bool) const {
    s += val ? "btrue" : "bfalse";
    return !val;
  }
} vhdlUnparser;
#if 0
static void
vhdlScalar(const OU::Value &v, std::string &s, bool param) {
  switch (v.m_vt->m_baseType) {
  case OA::OCPI_Bool:
    if (param)
      OU::format(s, "b%s", v.m_Bool ? "true" : "false");
    else {
      s = "b";
      v.unparse(s, &vhdlUnparser, true);
    }
    break;
  case OA::OCPI_Char:
  case OA::OCPI_UChar:
  case OA::OCPI_ULong:
  case OA::OCPI_Long:
  case OA::OCPI_UShort:
  case OA::OCPI_Short:
  case OA::OCPI_LongLong:
  case OA::OCPI_ULongLong:
  case OA::OCPI_Float:
  case OA::OCPI_Double:
    {
      s = "to_";
      for (const char *cp = OU::baseTypeNames[v.m_vt->m_baseType]; *cp; cp++)
	s += (char)tolower(*cp);
      s += "(";
      if (v.m_vt->m_baseType == OA::OCPI_UChar)
	OU::formatAdd(s, "%u", (uint8_t)v.m_UChar);
      else if (v.m_vt->m_baseType == OA::OCPI_Char) 
	OU::formatAdd(s, "%d", (int8_t)v.m_Char);
      else
	v.unparse(s, &vhdlUnparser, true);
      s += ")";
    }
    break;
  default:
    s = "<unsupported type for VHDL parameter/generic>";
  }

}
#endif
// Provide a string suitable for initializing a generic
// This will be in our own code, not the user's code, so we can count
// on the visibility of our packages and libraries.
// If param==true, the value is used in a top level generic setting in tools
const char *
vhdlValue(const OU::Value &v, std::string &s, bool) {
  if (v.needsComma())
    s += "(";
  v.unparse(s, &vhdlUnparser, true);
  if (v.needsComma())
    s += ")";
  return s.c_str();
}

static const char*
verilogValue(const OU::Value &v, std::string &s, bool) {
  if (v.m_vt->m_baseType == OA::OCPI_Bool)
    OU::format(s, "1'b%u", v.m_Bool ? 1 : 0);
  else
    OU::format(s, "%zu'h%" PRIx32, v.m_vt->m_nBits/4, v.m_ULong);
  return s.c_str();
}
const char *Worker::
hdlValue(const OU::Value &v, std::string &value, bool param, Language lang) {
  if (lang == NoLanguage)
    lang = m_language;
  return lang == VHDL ?
    vhdlValue(v, value, param) : verilogValue(v, value, param);
}

void Worker::
emitParameters(FILE *f, Language lang, bool useDefaults, bool convert) {
  bool first = true;
  std::string last;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
    OU::Property &pr = **pi;
    if (pr.m_isParameter) {
      if (first) {
	if (lang == VHDL)
	  fprintf(f, "  generic (\n");
	first = false;
      }
      if (lang == VHDL) {
	std::string value, decl, type;
	vhdlType(pr, decl, type, convert);
	if (decl.length())
	  //	  OU::format(type, "work.%s_defs.%s_t", m_implName, pr.m_name.c_str());
	  OU::format(type, "%s_t", pr.m_name.c_str());
	if (useDefaults) {
	  if (pr.m_default) {
	    std::string vhv;
	    vhdlValue(*pr.m_default, vhv);
	    if (pr.m_baseType == OA::OCPI_Bool && convert)
	      OU::format(value, "ocpi.util.slv(%s)", vhv.c_str());
	    else
	      value = vhv;
	  }
	} else
	  OU::format(value, "work.%s_defs.%s", m_implName, pr.m_name.c_str());
	emitSignal(pr.m_name.c_str(), f, lang, Signal::IN, last, -1, 0, "  ",
		   type.c_str(), value.empty() ? NULL : value.c_str());
      } else {
#if 0
	  if (pr->m_baseType == OA::OCPI_Bool) {
	    type = "std_logic";
	    OU::format(value, "'%llu'", i64);
	  } else {
	    OU::format(type, "std_logic_vector(%zu downto 0)", pr->m_nBits - 1);
	    OU::format(value, "x\"");
	    for (unsigned n = 0; n < pr->m_nBits; n += 4)
	      OU::formatAdd(value, "%c", "0123456789abcdef"[(i64 >> (pr->m_nBits - n - 4)) & 0xf]);
	    OU::formatAdd(value, "\"");
	  }
#endif
	  int64_t i64 = 0;
	  if (pr.m_default)
	    switch (pr.m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			\
	    case OA::OCPI_##pretty:					\
	      i64 = (int64_t)pr.m_default->m_##pretty; break;
	    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	    default:;
	    }
	if (pr.m_baseType == OA::OCPI_Bool)
	  fprintf(f, "  parameter [0:0] %s = 1'b%u;\n",
		  pr.m_name.c_str(), (i64 != 0) & 1);
	else
	  fprintf(f, "  parameter [%zu:0] %s = %zu'h%llx;\n",
		  pr.m_nBits - 1, pr.m_name.c_str(), pr.m_nBits, (long long)i64);
      }
    }
  }
  if (!first && lang == VHDL) {
    emitLastSignal(f, last, lang, true);
    fprintf(f, "  );\n");
  }
}

void Worker::
emitDeviceSignals(FILE *f, Language lang, std::string &last) {
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal *s = *si;
    if (s->m_differential) {
      std::string name;
      OU::format(name, s->m_pos.c_str(), s->m_name.c_str());
      emitSignal(name.c_str(), f, lang, s->m_direction, last,
		 s->m_width ? (int)s->m_width : -1, 0, "", s->m_type);
      OU::format(name, s->m_neg.c_str(), s->m_name.c_str());
      emitSignal(name.c_str(), f, lang, s->m_direction, last,
		 s->m_width ? (int)s->m_width : -1, 0, "", s->m_type);
    } else
      emitSignal(s->m_name.c_str(), f, lang, s->m_direction, last,
		 s->m_width ? (int)s->m_width : -1, 0, "", s->m_type);
  }
}

// Used in various places:
// 1. In the worker component declaration in the defs file via emitVhdlRecordInterface
// 2. In the verilog defs file directly
// 3. In the signal-to-record wrapper entity
// 4. In the actual worker vhdl entity
void Worker::
emitSignals(FILE *f, Language lang, bool useRecords, bool inPackage, bool inWorker) {
  const char *comment = hdlComment(lang);
  std::string init = lang == VHDL ? "  port (\n" : "";
  std::string last = init;
  for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!c->port) {
      if (last.empty())
	fprintf(f,
		"    %s Clock(s) not associated with one specific port:\n", comment);
      emitSignal(c->signal, f, lang, Signal::IN, last, -1, 0);
      if (c->reset.size())
	// FIXME: TOTAL HACK FOR THE INNER WORKER TO HAVE A POSITIVE RESET
	emitSignal(inWorker && !strcasecmp(c->reset.c_str(), "wci_reset_n") ?
		   "wci_reset" : c->reset.c_str(),
		   f, lang, Signal::IN, last, -1, 0);
    }
  }
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    emitLastSignal(f, last, lang, false);
    p->emitPortDescription(f, lang);
    // Some ports are basically an array of interfaces.
    if (useRecords && lang == VHDL)
      p->emitRecordSignal(f, last, "", inWorker);
    else
      p->emitSignals(f, lang, last, inPackage, inWorker);
  }
  if (m_signals.size()) {
    emitLastSignal(f, last, lang, false);
    fprintf(f, "  \n  %s   Extra signals not part of any WIP interface:\n", comment);
    emitDeviceSignals(f, lang, last);
  }
  if (last != init) {
    emitLastSignal(f, last, lang, true);
    fprintf(f, ");\n");
  } else if (lang == Verilog)
    fprintf(f, ");\n");
}

static void
prType(OU::Property &pr, std::string &type) {
  size_t nElements = 1;
  if (pr.m_arrayRank)
    nElements *= pr.m_nItems;
  if (pr.m_isSequence)
    nElements *= pr.m_sequenceLength; // can't be zero
  if (pr.m_baseType == OA::OCPI_String)
    if (pr.m_arrayRank || pr.m_isSequence)
      OU::format(type,
		 "String_array_t(0 to %zu, 0 to %zu)",
		 nElements-1, (pr.m_stringLength+4)/4*4-1);
    else
      OU::format(type, "String_t(0 to %zu)", pr.m_stringLength);
  else if (pr.m_arrayRank || pr.m_isSequence)
    OU::format(type, "%s_array_t(0 to %zu)",
	       OU::baseTypeNames[pr.m_baseType], nElements - 1);
  else
    OU::format(type, "%s_t", OU::baseTypeNames[pr.m_baseType]);
}

static char *
tempName(char *&temp, unsigned len, const char *fmt, ...) {
  va_list ap;
  if (temp)
    free(temp);
  char *mytemp;
  va_start(ap, fmt);
  vasprintf(&mytemp, fmt, ap);
  va_end(ap);
  asprintf(&temp, "%-*s", len, mytemp);
  free(mytemp);
  return temp;
}

#if 0
static void
emitVhdlValue(FILE *f, size_t width, uint64_t value, bool singleLine = true) {
  if (width & 3) {
    fprintf(f, "b\"");
    for (int b = (int)(width-1); b >= 0; b--)
      fprintf(f, "%c", value & (1 << b) ? '1' : '0');
  } else {
    fprintf(f, "x\"");
    for (int b = (int)(width-4); b >= 0; b -= 4)
      fprintf(f, "%x", (unsigned)((value >> b) & 0xf));
  }
  fprintf(f, "\"");
  if (singleLine)
    fprintf(f, "; -- 0x%0*" PRIx64 "\n", (int)OU::roundUp(width, 4)/4, value);
}

static void
emitVhdlScalar(FILE *f, OU::Property &pr) {
  if (pr.m_baseType == OA::OCPI_Bool)
    fprintf(f, "bfalse");
  else if (pr.m_baseType == OA::OCPI_String) {
    size_t len = pr.m_arrayRank ? (pr.m_stringLength+4)/4*4-1 : pr.m_stringLength+1;
    fprintf(f, "(");
    for (size_t n = 0; n < len; n++) {
      if (n)
	fprintf(f, ", ");
      emitVhdlValue(f, 8, 0, false);
    }
    fprintf(f, ")");
  } else
    emitVhdlValue(f, pr.m_nBits, 0, false);
}

static void
emitVhdlDefault(FILE *f, OU::Property &pr, const char *eq)
{
  fprintf(f, " %s ", eq);
  if (pr.m_arrayRank) {
    fprintf(f, "(");
    for (unsigned n = 0; n < pr.m_nItems; n++) {
      if (n != 0)
	fprintf(f, ", ");
      emitVhdlScalar(f, pr);
    }
    fprintf(f, ")");
  } else
    emitVhdlScalar(f, pr);
}

static void
emitVhdlProp(FILE *f, OU::Property &pr, std::string &last, bool writable,
	     unsigned maxPropName, bool &first, bool worker = false)
{
  const char *name = pr.m_name.c_str();

  if (last.size())
    fprintf(f, "%s", last.c_str());
  last = ";\n";
  if (first)
    fprintf(f,
	    "    -- %s for this worker's %s properties\n",
	    writable ? (worker ? "Registered inputs" : "Outputs") : (worker ? "Outputs" : "Inputs"),
	    writable ? "writable" : "volatile or readonly");
  first = false;
  std::string type;
  prType(pr, type);
  char *temp = NULL;
  const char *valueDir = writable ? (worker ? "in " : "out") : (worker ? "out" : "in ");
  if (pr.m_isSequence)
    fprintf(f,
	    "    %s : %s %s;\n",
	    tempName(temp, maxPropName, "%s_%s", name, writable ? "length" : "read_length"),
	    valueDir, "ulong_t");
  fprintf(f,
	  "    %s : %s %s",
	  tempName(temp, maxPropName, "%s_%s", name, writable ? "value" : "read_value"),
	  valueDir, type.c_str());
#if 0
  if (!writable && !worker)
    emitVhdlDefault(f, pr, ":=");
#endif
  if (writable && pr.m_isWritable && !pr.m_isInitial) {
    fprintf(f,
	    ";\n    %s : %s Bool_t",
	    tempName(temp, maxPropName, "%s_written", name), valueDir);
    if (pr.m_arrayRank || pr.m_stringLength > 3 || pr.m_isSequence)
      fprintf(f,
	      ";\n    %s : %s Bool_t", 
	      tempName(temp, maxPropName, "%s_any_written", name), valueDir);
  }
  free(temp); // FIXME: make proper string versions of this junk
}
#endif
static void
emitVhdlPropMemberData(FILE *f, OU::Property &pr, unsigned maxPropName) {
  std::string type;
  if (pr.m_isSequence) {
    type = pr.m_name;
    type += "_length";
    fprintf(f, "    %-*s : ULong_t;\n", maxPropName, type.c_str());
  }
  prType(pr, type);
  fprintf(f, "    %-*s : %s;\n", maxPropName, pr.m_name.c_str(), type.c_str());
}
static void
emitVhdlPropMember(FILE *f, OU::Property &pr, unsigned maxPropName, bool in2worker) {
  if (in2worker) {
    char *temp = NULL;
    if (pr.m_isWritable) {
      emitVhdlPropMemberData(f, pr, maxPropName);
      if (!pr.m_isInitial) {
	fprintf(f, "    %s : Bool_t;\n", tempName(temp, maxPropName, "%s_written",
						  pr.m_name.c_str()));
	if (pr.m_arrayRank || pr.m_stringLength > 3 || pr.m_isSequence)
	  fprintf(f,
		  "    %s : Bool_t;\n", 
		  tempName(temp, maxPropName, "%s_any_written", pr.m_name.c_str()));
      }
    }
    //    if (pr.m_isVolatile || pr.m_isReadable && !pr.m_isWritable)
    if (pr.m_isReadable)
      fprintf(f, "    %s : Bool_t;\n", tempName(temp, maxPropName, "%s_read",
						pr.m_name.c_str()));
    free(temp);
  } else if (pr.m_isVolatile || pr.m_isReadable && !pr.m_isWritable)
    emitVhdlPropMemberData(f, pr, maxPropName);
}

bool Worker::
nonRaw(PropertiesIter pi) {
  return pi != m_ctl.properties.end() &&
    (!m_ctl.rawProperties ||
     m_ctl.firstRaw &&
     strcasecmp(m_ctl.firstRaw->m_name.c_str(), (*pi)->m_name.c_str()));
}

void
emitVhdlLibraries(FILE *f) {
  if (libraries && libraries[0]
#if 1
 || mappedLibraries && mappedLibraries[0]
#endif
) {
    bool first = true;
    const char **mp;
#if 1
    for (mp = mappedLibraries; mp && *mp; mp++, first = false)
      fprintf(f, "%s %s", first ? "library" : ",", *mp);
#endif
    for (const char **lp = libraries; lp && *lp; lp++) {
      const char *l = strrchr(*lp, '/');
#if 1
      for (mp = mappedLibraries; mp && *mp; mp++)
	if (!strcasecmp(*mp, l ? l + 1 : *lp))
	  break;
      if (!mp || !*mp)
#endif
	{
	fprintf(f, "%s %s", first ? "library" : ",", l ? l + 1 : *lp);
	first = false;
      }
    }
    fprintf(f, ";\n");
  }
}

const char *Worker::
emitVhdlPackageConstants(FILE *f) {
  size_t decodeWidth = 0, rawBase = 0;
  char ops[OU::Worker::OpsLimit + 1 + 1];
  for (unsigned op = 0; op <= OU::Worker::OpsLimit; op++)
    ops[OU::Worker::OpsLimit - op] = '0';
  ops[OU::Worker::OpsLimit+1] = 0;
  if (m_wci) {
    decodeWidth = m_wci->decodeWidth();
    for (unsigned op = 0; op <= OU::Worker::OpsLimit; op++)
      ops[OU::Worker::OpsLimit - op] = m_ctl.controlOps & (1 << op) ? '1' : '0';
    if (m_ctl.firstRaw)
      rawBase = m_ctl.firstRaw->m_offset;
  }
#if 0
  fprintf(f,
	  "\n"
	  "-- Worker-specific definitions that are needed outside entities below\n"
	  "package body %s_defs is\n", m_implName);
#endif
  if (!m_noControl)
    fprintf(f,
	    "  constant worker : ocpi.wci.worker_t := (%zu, %zu, \"%s\");\n",
	    decodeWidth, rawBase, ops);
  if (!m_ctl.nRunProperties) {
    fprintf(f, "-- no properties for this worker\n");
    //	      "  constant properties : ocpi.wci.properties_t(1 to 0) := "
    //"(others => (0,x\"00000000\",0,0,0,false,false,false,false));\n");
  } else {
    fprintf(f, "  constant properties : ocpi.wci.properties_t(0 to %u) := (\n",
	    m_ctl.nRunProperties - 1);
    unsigned n = 0;
    const char *last = NULL;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      OU::Property *pr = *pi;
      if (!pr->m_isParameter) {
	size_t nElements = 1;
	if (pr->m_arrayRank)
	  nElements *= pr->m_nItems;
	if (pr->m_isSequence)
	  nElements *= pr->m_sequenceLength; // can't be zero
	fprintf(f, "%s%s%s   %2u => (%2zu, x\"%08zx\", %6zu, %6zu, %6zu, %s %s %s %s)",
		last ? ", -- " : "", last ? last : "", last ? "\n" : "", n,
		pr->m_nBits,
		pr->m_offset,
		pr->m_nBytes - 1,
		pr->m_stringLength,
		nElements,
		pr->m_isWritable ? "true, " : "false,",
		pr->m_isReadable ? "true, " : "false,",
		pr->m_isVolatile ? "true, " : "false,",
		pr->m_isDebug    ? "true" : "false");
	last = pr->m_name.c_str();
	n++;
      }
    }    
    fprintf(f, "  -- %s\n  );\n", last);
  }
#if 0
  fprintf(f, 
	  "end %s_defs;\n", m_implName);
#endif
  return NULL;
}

const char *Worker::
emitVhdlWorkerPackage(FILE *f, unsigned maxPropName) {
  fprintf(f,
	  "-- This package defines types needed for the inner worker entity's generics or ports\n"
	  "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	  "library ocpi; use ocpi.all, ocpi.types.all;\n");
  emitVhdlLibraries(f);
  fprintf(f,
	  "package %s_worker_defs is\n",
	  m_implName);
  if (m_ctl.writables || m_ctl.readbacks || m_ctl.rawProperties) {
    fprintf(f,"\n"
	    "  -- The following record is for the writable properties of worker \"%s\"\n"
	    "  -- and/or the read strobes of volatile or readonly properties\n"
	    "  type worker_props_in_t is record\n", 
	    m_implName);
    for (PropertiesIter pi = m_ctl.properties.begin(); nonRaw(pi); pi++)
      if (!(*pi)->m_isParameter && ((*pi)->m_isWritable || (*pi)->m_isReadable))
	emitVhdlPropMember(f, **pi, maxPropName, true);
    if (m_ctl.rawProperties)
      fprintf(f,
	      "    %-*s : unsigned(%zu downto 0); -- raw property address\n"
	      "    %-*s : std_logic_vector(3 downto 0);\n"
	      "    %-*s : Bool_t;\n"
	      "    %-*s : Bool_t;\n"
	      "    %-*s : std_logic_vector(31 downto 0);\n",
	      maxPropName, "raw_address", m_wci->decodeWidth() - 1,
	      maxPropName, "raw_byte_enable", 
	      maxPropName, "raw_is_read", 
	      maxPropName, "raw_is_write", 
	      maxPropName, "raw_data");
    fprintf(f,
	    "  end record worker_props_in_t;\n");
  }
  if (m_ctl.readbacks || m_ctl.rawReadables) {
    fprintf(f,"\n"
	    "  -- The following record is for the readable properties of worker \"%s\"\n"
	    "  type worker_props_out_t is record\n", 
	    m_implName);
    for (PropertiesIter pi = m_ctl.properties.begin(); nonRaw(pi); pi++)
      if (!(*pi)->m_isParameter &&
	  ((*pi)->m_isVolatile || (*pi)->m_isReadable && !(*pi)->m_isWritable))
	emitVhdlPropMember(f, **pi, maxPropName, false);
    if (m_ctl.rawProperties)
      fprintf(f,
	      "    %-*s : std_logic_vector(31 downto 0);\n",
	      maxPropName, "raw_data");
    fprintf(f,
	    "  end record worker_props_out_t;\n");
  }
  // Generate record types to easily and compactly plumb interface signals internally
  for (unsigned n = 0; n < m_ports.size(); n++)
#if 1
    m_ports[n]->emitRecordTypes(f);
#else
  {
    switch (p->type) {
    case WSIPort:
    case WMIPort:
      if (p->u.wdi.nOpcodes > 1) {
	Protocol *prot = p->m_protocol;
	if (prot && prot->operations()) {
	  // See if this protocol has already been defined
	  unsigned nn;
	  for (nn = 0; nn < n; nn++) {
	    Port *pp = m_ports[nn];
	    if (pp->m_protocol && pp->m_protocol->operations() &&
		!strcasecmp(pp->m_protocol->m_name.c_str(),
			    p->m_protocol->m_name.c_str()))
	      break;
	  }
	  if (nn >= n) {
	    fprintf(f,
		    "  -- This enumeration is for the opcodes for protocol %s (%s)\n"
		    "  type %s_OpCode_t is (\n",
		    prot->m_name.c_str(), prot->m_qualifiedName.c_str(),
		    prot->m_name.c_str());
	    OU::Operation *op = prot->operations();
	    for (nn = 0; nn < prot->nOperations(); nn++, op++)
	      fprintf(f, "%s    %s_%s_op_e", nn ? ",\n" : "",
		      prot->m_name.c_str(), op->name().c_str());
	    // If the protocol opcodes do not fill the space, fill it
	    if (nn < p->u.wdi.nOpcodes) {
	      for (unsigned o = 0; nn < p->u.wdi.nOpcodes; nn++, o++)
		fprintf(f, ",%sop%u_e", (o % 10) == 0 ? "\n    " : " ", nn);
	    }
	    fprintf(f, ");\n");
	  }
	} else {
	  fprintf(f, "  subtype %s_OpCode_t is std_logic_vector(%zu downto 0); -- for %zu opcodes\n",
		  p->name(), ceilLog2(p->u.wdi.nOpcodes) - 1, p->u.wdi.nOpcodes);
	}
      }
    default:;
    }
    fprintf(f,"\n"
	    "  -- The following record(s) are for the inner/worker interfaces for port \"%s\"\n",
	    p->name());
    std::string in, out;
    OU::format(in, p->typeNameIn.c_str(), "");
    OU::format(out, p->typeNameOut.c_str(), "");
    switch (p->type) {
    case WCIPort:
      fprintf(f,
	      "  type worker_%s_t is record\n"
	      "    clk              : std_logic;        -- clock for this worker\n"
	      "    reset            : Bool_t;           -- reset for this worker, at least 16 clocks long\n"
	      "    control_op       : wci.control_op_t; -- control op in progress, or no_op_e\n"
              "    state            : wci.state_t;      -- wci state: see state_t\n"
	      "    is_operating     : Bool_t;           -- shorthand for state = operating_e\n"
	      "    abort_control_op : Bool_t;           -- demand that slow control op finish now\n",
	      in.c_str());
      if (m_endian == Dynamic)
	fprintf(f, "    is_big_endian    : Bool_t;           -- for endian-switchable workers\n");
      fprintf(f,
	      "  end record worker_%s_t;\n"
	      "  type worker_%s_t is record\n"
	      "    done             : Bool_t;           -- the pending prop access/config op is done\n"
	      "    error            : Bool_t;           -- the pending prop access/config op is erroneous\n"
	      "    finished         : Bool_t;           -- worker is finished\n"
	      "    attention        : Bool_t;           -- worker wants attention\n"
	      "  end record worker_%s_t;\n",
	      in.c_str(), out.c_str(), out.c_str());
      break;
#if 0
    case WTIPort:
      fprintf(f,
	      "  type worker_%s_t is record\n", in.c_str());
      if (p->u.wti.secondsWidth)
	fprintf(f, "    seconds          : Ulong_t(%zu downto 0);\n", p->u.wti.secondsWidth-1);
      if (p->u.wti.fractionWidth)
	fprintf(f, "    fraction         : Ulong_t(%zu downto 0);\n", p->u.wti.fractionWidth-1);
      if (p->u.wti.allowUnavailable)
	fprintf(f, "    ready            : Bool_t;\n");
      fprintf(f,
	      "  end record worker_%s_t;\n", in.c_str());
      fprintf(f,
	      "  type worker_%s_t is record\n"
	      "    ready            : Bool_t; -- worker wants time\n"
	      "    reset            : Bool_t;\n"
	      "  end record worker_%s_t;\n", out.c_str(), out.c_str());
      break;
#endif
    case WSIPort:
    case WMIPort: // not really correct but will build for now.
      fprintf(f,
	      "  type worker_%s_t is record\n", 
	      in.c_str());
      if (p->clock != m_ports[0]->clock)
	fprintf(f,
		"    clk  : std_logic; -- this ports clk, different from wci_clk");
      fprintf(f,
	      "    reset            : Bool_t;           -- this port is being reset from the outside peer\n"
	      "    ready            : Bool_t;           -- this port is ready for data to be ");
      if (p->masterIn()) {
	fprintf(f,
		"taken\n"
		"                                         -- one or more of: som, eom, valid are true\n");
	if (p->dataWidth) {
	  fprintf(f,
		  "    data             : std_logic_vector(%zu downto 0);\n",
		  p->dataWidth-1);	      
	  if (p->ocp.MByteEn.value)
	    fprintf(f,
		    "    byte_enable      : std_logic_vector(%zu downto 0);\n",
		    p->dataWidth/p->byteWidth-1);
	}
	if (p->u.wdi.nOpcodes > 1)
	  fprintf(f,
		  "    opcode           : %s_OpCode_t;\n",
		  p->m_protocol && p->m_protocol->operations() ?
		  p->m_protocol->m_name.c_str() : p->name());
	fprintf(f,
		p->dataWidth ?
		"    som, eom, valid  : Bool_t;           -- valid means data and byte_enable are present\n" :
		"    som, eom  : Bool_t;\n");
      } else
	fprintf(f, "given\n");
      fprintf(f,
	      "  end record worker_%s_t;\n",
	      in.c_str());
      fprintf(f,
	      "  type worker_%s_t is record\n", out.c_str());
      if (p->masterIn()) {
	fprintf(f,
		"    take             : Bool_t;           -- take data now from this port\n"
		"                                         -- can be asserted when ready is true\n");
      } else {
	fprintf(f,
		"    give             : Bool_t;           -- give data now to this port\n"
		"                                         -- can be asserted when ready is true\n");
	if (p->dataWidth) {
	  fprintf(f,
		  "    data             : std_logic_vector(%zu downto 0);\n",
		  p->dataWidth-1);	      
	  if (p->ocp.MByteEn.value)
	    fprintf(f,
		    "    byte_enable      : std_logic_vector(%zu downto 0);\n",
		    p->dataWidth/p->byteWidth-1);
	}
	if (p->u.wdi.nOpcodes > 1)
	  fprintf(f,
		  "    opcode           : %s_OpCode_t;\n",
		  p->m_protocol && p->m_protocol->operations() ?
		  p->m_protocol->m_name.c_str() : p->name());
	fprintf(f,
		"    som, eom, valid  : Bool_t;            -- one or more must be true when 'give' is asserted\n");
      }
      fprintf(f,
	      "  end record worker_%s_t;\n",
	      out.c_str());
      break;
    case WTIPort:
      fprintf(f,
	      "  type worker_%s_t is record\n", 
	      in.c_str());
      if (p->clock != m_ports[0]->clock)
	fprintf(f, "    clk  : std_logic; -- this ports clk, different from wci_clk");
      if (p->u.wti.allowUnavailable)
	fprintf(f, "    valid    : Bool_t;\n");
      if (p->u.wti.secondsWidth)
	fprintf(f, "    seconds  : unsigned(%zu downto 0);\n", p->u.wti.secondsWidth-1);
      if (p->u.wti.fractionWidth)
	fprintf(f, "    fraction : unsigned(%zu downto 0);\n", p->u.wti.fractionWidth-1);
      fprintf(f,
	      "  end record worker_%s_t;\n",
	      in.c_str());
      if (p->u.wti.allowUnavailable)
	fprintf(f,
		"  type worker_%s_t is record\n"
		"    request    : Bool_t;\n"
		"  end record worker_%s_t;\n",
		out.c_str(),   out.c_str());
      break;
    case CPPort:
      fprintf(f,
	      "\n"
	      "  -- Record for the CPMaster input signals for port \"%s\" of worker \"%s\"\n"
	      "  alias worker_%s_in_t is platform.platform_pkg.occp_out_t;\n"
	      "  -- Record for the CPMaster  output signals for port \"%s\" of worker \"%s\"\n"
	      "  alias worker_%s_out_t is platform.platform_pkg.occp_in_t;\n",
	      p->name(), m_implName, p->name(), p->name(), m_implName, p->name());
      break;
    case TimePort:
      fprintf(f,
	      "\n"
	      "  -- Record for the TimeService output signals for port \"%s\" of worker \"%s\"\n"
	      "  alias worker_%s_out_t is platform.platform_pkg.time_service_t;\n",
	      p->name(), m_implName, p->name());
      break;
    case MetadataPort:
      fprintf(f,
	      "\n"
	      "  -- Record for the CPMaster input signals for port \"%s\" of worker \"%s\"\n"
	      "  alias worker_%s_in_t is platform.platform_pkg.metadata_out_t;\n"
	      "  -- Record for the CPMaster  output signals for port \"%s\" of worker \"%s\"\n"
	      "  alias worker_%s_out_t is platform.platform_pkg.metadata_in_t;\n",
	      p->name(), m_implName, p->name(), p->name(), m_implName, p->name());
      break;
    default:;
    }
  }
#endif
  emitVhdlPackageConstants(f);
  fprintf(f,
	  "end package %s_worker_defs;\n",
	  m_implName);
  return NULL;
}

const char *Worker::
emitVhdlWorkerEntity(FILE *f) {
  fprintf(f,
	  "\n"
	  "-- This is the entity to be implemented, depending on the above record types.\n"
	  "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	  "library ocpi; use ocpi.types.all;\n");
  emitVhdlLibraries(f);
  fprintf(f,
	  "use work.%s_worker_defs.all, work.%s_defs.all;\n"
	  "entity %s_worker is\n",
	  m_implName, m_implName, m_implName);
  emitParameters(f, VHDL);
    
#if 1
  emitSignals(f, VHDL, true, true, true);
#else
  fprintf(f,
	  "  port(\n");
  std::string last;
  for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!c->port) {
      if (last.empty())
	fprintf(f,
		"    -- Clock(s) not associated with one specific port:\n");
      emitSignal(c->signal, f, m_language, Signal::IN, last, -1, 0);
      if (c->reset.size()) {
	// FIXME: TOTAL HACK FOR THE INNER WORKER TO HAVE A POSITIVE RESET
	emitSignal(!strcasecmp(c->reset.c_str(), "wci_reset_n") ? "wci_reset" : c->reset.c_str(),
		   f, m_language, Signal::IN, last, -1, 0);
      }
    }	
  }
  emitLastSignal(f, last, m_language, false);
  for (unsigned i = 0; i < m_ports.size(); i++)
    m_ports[i]->emitWorkerEntitySignals(f, last, maxPropName);
  if (m_signals.size()) {
    emitLastSignal(f, last, m_language, false);
    fprintf(f, "    -- Extra signals not part of any WIP interface:\n");
    emitDeviceSignals(f, m_language, last);
  } else
    last = "";
  emitLastSignal(f, last, m_language, true);
#endif
  fprintf(f, "\nend entity %s_worker;\n", m_implName);
  return NULL;
}

const char *Worker::
emitVhdlRecordInterface(FILE *f) {
  const char *err = NULL;
  //  size_t maxName = 0;
  // Generate record types to easily and compactly plumb interface signals internally
  for (unsigned i = 0; i < m_ports.size(); i++)
    m_ports[i]->emitRecordInterface(f, m_implName);
#if 0
    std::string in, out;
    OU::format(in, p->typeNameIn.c_str(), "");
    OU::format(out, p->typeNameOut.c_str(), "");
    switch (p->type) {
    case WCIPort:
      if (p->master || m_assembly) {
	goto skip_array; // ugh
      }
      // fall into
    case WSIPort:
    case WMIPort:
    case WMemIPort:
    case WTIPort:
      {
	bool mIn = p->masterIn();
	fprintf(f, "\n"
		"  -- These 2 records correspond to the input and output sides of the OCP bundle\n"
		"  -- for the \"%s\" worker's \"%s\" profile interface named \"%s\"\n",
		m_implName, p->typeName(), p->name());
	fprintf(f,
		"\n  -- Record for the %s input (OCP %s) signals for port \"%s\" of worker \"%s\"\n",
		p->typeName(), mIn ? "master" : "slave", p->name(), m_implName);
	fprintf(f, "  type %s_t is record\n", in.c_str());
	OcpSignalDesc *osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
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
		p->typeName(), mIn ? "slave" : "master",
		p->name(), m_implName, out.c_str());
	osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
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
	break;
      }
    case CPPort:
      fprintf(f,
	      "\n"
	      "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.occp_out_t;\n"
	      "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.occp_in_t;\n",
	      p->typeName(), p->name(), m_implName,
	      p->typeNameIn.c_str(),
	      p->typeName(), p->name(), m_implName,
	      p->typeNameOut.c_str());
      break;
    case NOCPort:
      fprintf(f,
	      "\n"
	      "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.unoc_master_%s_t;\n"
	      "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.unoc_master_%s_t;\n",
	      p->typeName(), p->name(), m_implName, in.c_str(), p->master ? "in" : "out",
	      p->typeName(), p->name(), m_implName, out.c_str(), p->master ? "out" : "in");
      break;
    case TimePort:
      fprintf(f,
	      "\n"
	      "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.time_service_t;\n",
	      p->typeName(), p->name(), m_implName,
	      p->master ? out.c_str() : in.c_str());
      break;
    case MetadataPort:
      fprintf(f,
	      "\n"
	      "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.metadata_%s_t;\n"
	      "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.metadata_%s_t;\n",
	      p->typeName(), p->name(), m_implName,
	      in.c_str(), p->master ? "in" : "out",
	      p->typeName(), p->name(), m_implName,
	      out.c_str(), p->master ? "out" : "in");
      break;

    case PropPort:
      fprintf(f,
	      "\n"
	      "  -- Record for the %s input signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.raw_prop_%s_t;\n"
	      "  -- Record for the %s output signals for port \"%s\" of worker \"%s\"\n"
	      "  alias %s_t is platform.platform_pkg.raw_prop_%s_t;\n",
	      p->typeName(), p->name(), m_implName,
	      in.c_str(), p->master ? "in" : "out",
	      p->typeName(), p->name(), m_implName,
	      out.c_str(), p->master ? "out" : "in");
      break;

    default:;
    }
    if (p->count > 1)
      fprintf(f,
	      "  type %s_array_t is array(0 to %zu) of %s_t;\n"
	      "  type %s_array_t is array(0 to %zu) of %s_t;\n",
	      in.c_str(), p->count-1, in.c_str(), 
	      out.c_str(), p->count-1, out.c_str());
  skip_array:
    if (p->typeNameIn.length() > maxName)
      maxName = in.length();
    if (p->typeNameOut.length() > maxName)
      maxName = out.length();
  }
#endif
  fprintf(f,
	  "\ncomponent %s_rv is\n", m_implName);
  emitParameters(f, VHDL);
  emitSignals(f, VHDL, true, true, false);
  fprintf(f,
	  "end component %s_rv;\n\n"
#if 0
	  "attribute box_type: string;\n"
	  "attribute box_type of %s_rv : component is \"user_black_box\";\n",
	  m_implName,
#else
          ,
#endif
	  m_implName);
  return err;
}

// Emit the file that can be used to instantiate the worker
const char *Worker::
emitDefsHDL(bool wrap) {
  const char *err;
  FILE *f;
  Language lang = wrap ? (m_language == VHDL ? Verilog : VHDL) : m_language;
  if ((err = openOutput(m_implName, m_outDir, "", DEFS, lang == VHDL ? VHD : ".vh", NULL, f)))
    return err;
  const char *comment = hdlComment(lang);
  printgen(f, comment, m_file.c_str());
  fprintf(f,
	  "%s This file contains the %s declarations for the worker with\n"
	  "%s  spec name \"%s\" and implementation name \"%s\".\n"
	  "%s It is needed for instantiating the worker.\n"
	  "%s Interface signal names are defined with pattern rule: \"%s\"\n",
	  comment, lang == VHDL ? "VHDL" : "Verilog", comment, m_specName,
	  m_implName, comment, comment, m_pattern);
  if (lang == VHDL) {
    fprintf(f,
	    "Library IEEE; use IEEE.std_logic_1164.all;\n"
	    "Library ocpi; use ocpi.all, ocpi.types.all;\n");
    emitVhdlLibraries(f);
    fprintf(f,
	    "\n"
	    "package %s_defs is\n", m_implName);
    bool first = true;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      OU::Property &p = **pi;
      if (p.m_isParameter) {
	if (first) {
	  fprintf(f,
		  " -- Declarations of parameter properties.\n"
		  " -- The actual values used are in the package body,\n"
		  " -- which is generated for each configuration.\n");
	  first = false;
	}
	std::string decl, type;
	vhdlType(p, decl, type, false);
	if (decl.length())
	  fprintf(f,
		  "  type %s_t is %s;\n"
		  "  constant %s : %s_t;\n",
		  p.m_name.c_str(), decl.c_str(), p.m_name.c_str(), p.m_name.c_str());
	else
	  fprintf(f, "  constant %s : %s;\n", p.m_name.c_str(), type.c_str());
      }
    }
    if ((err = emitVhdlRecordInterface(f)))
      return err;
    fprintf(f,
	    "\ncomponent %s is\n", m_implName);
    emitParameters(f, lang, true, true);
  } else
    fprintf(f,
	    "\n"
	    //	    "`default_nettype none\n" // leave this up to the developer
	    "`ifndef NOT_EMPTY_%s\n"
	    "(* box_type=\"user_black_box\" *)\n"
	    "`endif\n"
	    "module %s//__\n(\n", m_implName, m_implName);
  emitSignals(f, lang, false, true, false);
  if (lang == VHDL) {
    fprintf(f,
	    "end component %s;\n\n",
	    m_implName);
    fprintf(f, "end package %s_defs;\n", m_implName);
  } else {
    // Now we emit parameters in the body.
    emitParameters(f, lang);
    // Now we emit the declarations (input, output, width) for each module port
    for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
      Clock *c = *ci;
      if (!c->port) {
	fprintf(f, "  input      %s;\n", c->signal);
	if (c->reset.size())
	  fprintf(f, "  input      %s;\n", c->reset.c_str());
      }
    }
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitVerilogSignals(f);
    if (m_signals.size()) {
      fprintf(f, "  // Extra signals not part of any WIP interface:\n");
      for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
	Signal *s = *si;
	const char *dir =
	  s->m_direction == Signal::IN ? "input" :
	  (s->m_direction == Signal::OUT ? "output    " : "inout");
	if (s->m_differential) {
	  std::string name;
	  OU::format(name, s->m_pos.c_str(), s->m_name.c_str());
	  if (s->m_width)
	    fprintf(f, "  %s [%3zu:0] %s;\n", dir, s->m_width - 1, name.c_str());
	  else
	    fprintf(f, "  %s         %s;\n", dir, name.c_str());
	  OU::format(name, s->m_neg.c_str(), s->m_name.c_str());
	  if (s->m_width)
	    fprintf(f, "  %s [%3zu:0] %s;\n", dir, s->m_width - 1, name.c_str());
	  else
	    fprintf(f, "  %s         %s;\n", dir, name.c_str());
	} else if (s->m_width)
	  fprintf(f, "  %s [%3zu:0] %s;\n", dir, s->m_width - 1, s->m_name.c_str());
	else
	  fprintf(f, "  %s         %s;\n", dir, s->m_name.c_str());
      }
    }
    // Suppress the "endmodule" when this should not be an empty module definition
    // When standalone, the file will be an empty module definition
    fprintf(f,
	    "\n"
	    "// NOT_EMPTY_%s is defined before including this file when implementing\n"
	    "// the %s worker.  Otherwise, this file is a complete empty definition.\n"
	    "`ifndef NOT_EMPTY_%s\n"
	    "endmodule\n"
	    "`endif\n",
	    m_implName, m_implName,m_implName);
  }
  fclose(f);
  return 0;
}

void Worker::
emitVhdlShell(FILE *f) {
  fprintf(f,
	  "library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;\n"
	  "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n"
	  "architecture rtl of %s_rv is\n",
	  m_implName);
  if (!m_wci) {
    // with no control interface we have to directly generate wci_reset and wci_is_operating
    fprintf(f,
	    "begin\n"
	    "  wci_reset <= not wci_Reset_n");
    // For each data interface we aggregate a peer reset.
    for (unsigned i = 0; i < m_ports.size(); i++) {
      Port *p = m_ports[i];
      if (p->isData()) {
	fprintf(f, " or not %s.%s",
		p->typeNameIn.c_str(),
		ocpSignals[p->master ? OCP_SReset_n : OCP_MReset_n].name);
      }
    }
    fprintf(f,
	    ";\n"
	    "  wci_is_operating <= not wci_reset;\n");
  } else {
#if 0
    if (!wci->ocp.SData.value || !m_ctl.nonRawReadables)
      fprintf(f,
	      "  signal unused : std_logic_vector(31 downto 0);\n");
#endif
    fprintf(f,
	    "begin\n"
	    "  -- This instantiates the WCI/Control module/entity generated in the *_impl.vhd file\n"
	    "  -- With no user logic at all, this implements writable properties.\n"
	    "  wci : entity work.%s_wci\n"
            "    generic map(ocpi_debug => ocpi_debug)\n"
	    "    port map(-- These first signals are just for use by the wci module, not the worker\n"
	    "             inputs            => %s,\n"
	    "             outputs           => %s,\n",
	    m_implName, m_wci->typeNameIn.c_str(), m_wci->typeNameOut.c_str());

    fprintf(f,
	    "             -- These are outputs used by the worker logic\n"
	    "             reset             => wci_reset, -- OCP guarantees 16 clocks of reset\n"
	    "             control_op        => wci_control_op,\n"
	    //	    "             raw_offset    => %s,\n"
	    "             state             => wci_state,\n"
	    "             is_operating      => wci_is_operating,\n"
	    "             done              => wci_done,\n"
	    "             error             => wci_error,\n"
	    "             finished          => wci_finished,\n"
	    "             attention         => wci_attention,\n"
#if 0
	    "             is_read           => %s,\n"
	    "             is_write          => %s,\n",
	    m_ctl.rawProperties ? "props_to_worker.raw_address" : "open",
	    m_ctl.rawReadables ? "my_is_read" : "open",
	    m_ctl.rawWritables ? "props_to_worker.raw_is_write" : "open"
#endif
);
    
    if (m_endian == Dynamic)
      fprintf(f, "             is_big_endian     => wci_is_big_endian,\n");
    fprintf(f,
	    "             abort_control_op  => wci_abort_control_op");
#if 1
    if (m_ctl.nonRawReadbacks || m_ctl.rawReadables)
      fprintf(f,
	      ",\n"
	      "             props_from_worker => props_from_worker");
    if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
      fprintf(f,
	      ",\n"
	      "             props_to_worker   => props_to_worker");
#else
    if (m_ctl.nonRawReadbacks) {
      fprintf(f,
	      ",\n"
	      "             -- These are inputs from the worker for readable property values.\n");
      bool first = true;
      char *temp = NULL;
      for (PropertiesIter pi = m_ctl.properties.begin(); nonRaw(pi); pi++) {
	OU::Property &pr = **pi;
	const char *name = pr.m_name.c_str();
	if (!pr.m_isParameter && (pr.m_isVolatile || pr.m_isReadable && !pr.m_isWritable)) {
	  if (pr.m_isSequence) {
	    fprintf(f, "%s             %s => %s_read_length",
		    first ? "" : ",\n",
		    tempName(temp, maxPropName, "%s_read_length", pr.m_name.c_str()),
		    name);
	    first = false;
	  }
	  fprintf(f, "%s             %s => %s_read_value",
		  first ? "" : ",\n",
		  tempName(temp, maxPropName, "%s_read_value", pr.m_name.c_str()),
		  name);
	  first = false;
	}
      }
    }
    if (m_ctl.nonRawWritables) {
      fprintf(f,
	      ",\n"
	      "            -- These are outputs to the worker for writable property values.\n");
      bool first = true;
      for (PropertiesIter pi = m_ctl.properties.begin(); nonRaw(pi); pi++) {
	OU::Property &pr = **pi;
	const char *name = pr.m_name.c_str();
	if (pr.m_isWritable) {
	  fprintf(f, "%s            %s_value     => %s_value",
		  first ? "" : ",\n", name, name);
	  first = false;
	  if (!pr.m_isInitial) {
	    fprintf(f, ",\n            %s_written     => %s_written",
		    name, name);
	    if (pr.m_arrayRank || pr.m_stringLength >3 || pr.m_isSequence)
	      fprintf(f, ",\n            %s_any_written     => %s_any_written",
		      name, name);
	  }
	  first = false;
	}
      }
    }
    if (m_ctl.rawReadables)
      fprintf(f, ",\n             raw_SData         => raw_SData");
#endif
    fprintf(f, ");\n");
  }
  for (unsigned i = 0; i < m_ports.size(); i++)
    m_ports[i]->emitVhdlShell(f, m_wci);
  fprintf(f,
	  "worker : entity work.%s_worker\n", m_implName);
  bool first = true;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if ((*pi)->m_isParameter) {
      if (first) {
	fprintf(f,
		"  generic map(\n");
      }
      fprintf(f,  "%s    %s => %s",
	      first ? "" : ",\n", (*pi)->m_name.c_str(), (*pi)->m_name.c_str());
      first = false;
    }
  if (!first)
    fprintf(f, ")\n");
  fprintf(f,
	  "  port map(\n");
  std::string last;
  for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!c->port) {
      fprintf(f, "%s    %s => %s", last.c_str(), c->signal, c->signal);
      last = ",\n";
      if (c->reset.size()) {
	// FIXME: HACK FOR EXPOSING POSITIVE RESET TO INNER WORKER
	const char *reset = !strcasecmp(c->reset.c_str(), "wci_reset_n") ? "wci_reset" :
	  c->reset.c_str();
	fprintf(f, "%s    %s => %s", last.c_str(), reset, reset);
      }
    }
  }
  for (unsigned i = 0; i < m_ports.size(); i++)
#if 1
    m_ports[i]->emitVHDLShellPortMap(f, last);
#else
{
    Port *p = m_ports[i];
    std::string in, out;
    OU::format(in, p->typeNameIn.c_str(), "");
    OU::format(out, p->typeNameOut.c_str(), "");
    switch (p->type) {
    case WCIPort:
      fprintf(f,
	      "%s    %s_in.clk => %s.Clk,\n"
	      "    %s_in.reset => wci_reset,\n"
	      "    %s_in.control_op => wci_control_op,\n"
	      "    %s_in.state => wci_state,\n"
	      "    %s_in.is_operating => wci_is_operating,\n"
	      "    %s_in.abort_control_op => wci_abort_control_op,\n",
	      last.c_str(), p->name(), in.c_str(), p->name(), p->name(),
	      p->name(), p->name(), p->name());
      if (m_endian == Dynamic)
	fprintf(f, "    %s_in.is_big_endian => wci_is_big_endian,\n", p->name());
      fprintf(f,
	      "    %s_out.done => wci_done,\n"
	      "    %s_out.error => wci_error,\n"
	      "    %s_out.finished => wci_finished,\n"
	      "    %s_out.attention => wci_attention",
	      p->name(), p->name(), p->name(), p->name());
      last = ",\n";
      break;
    case WSIPort:
    case WMIPort:
      if (p->masterIn()) {
	fprintf(f,
		"%s    %s_in.reset => %s_reset,\n"
	        "    %s_in.ready => %s_ready,\n",
		last.c_str(), p->name(), p->name(), p->name(), p->name());
	if (p->dataWidth)
	  fprintf(f,
		  "    %s_in.data => %s_data,\n",
		  p->name(), p->name());
	if (p->ocp.MByteEn.value)
	  fprintf(f, "    %s_in.byte_enable => %s_byte_enable,\n", p->name(), p->name());
	if (p->u.wdi.nOpcodes > 1)
	  fprintf(f, "    %s_in.opcode => %s_opcode,\n", p->name(), p->name());
	fprintf(f,
		"    %s_in.som => %s_som,\n"
		"    %s_in.eom => %s_eom,\n",
		p->name(), p->name(), p->name(), p->name());
	if (p->dataWidth)
	  fprintf(f,
		  "    %s_in.valid => %s_valid,\n",
		  p->name(), p->name());
	fprintf(f,
		"    %s_out.take => %s_take",
		p->name(), p->name());
	last = ",\n";
      } else {
	fprintf(f,
		"%s    %s_in.reset => %s_reset,\n"
		"    %s_in.ready => %s_ready,\n"
		"    %s_out.give => %s_give,\n",
		last.c_str(), p->name(), p->name(), p->name(), p->name(), p->name(), p->name());
	if (p->dataWidth)
	  fprintf(f,
		  "    %s_out.data => %s_data,\n", p->name(), p->name());
	if (p->ocp.MByteEn.value)
	  fprintf(f, "    %s_out.byte_enable => %s_byte_enable,\n", p->name(), p->name());
	if (p->ocp.MReqInfo.value)
	  fprintf(f, "    %s_out.opcode => %s_opcode,\n", p->name(), p->name());
	fprintf(f,
		"    %s_out.som => %s_som,\n"
		"    %s_out.eom => %s_eom,\n",
		p->name(), p->name(), p->name(), p->name());
	if (p->dataWidth)
	  fprintf(f,
		  "    %s_out.valid => %s_valid",
		  p->name(), p->name());
	last = ",\n";
      }
      break;
    case TimePort:
      fprintf(f,
	      "%s    %s => %s",
	      last.c_str(),
	      p->master ? out.c_str() : in.c_str(),
	      p->master ? out.c_str() : in.c_str());
      break;
    case MetadataPort:
    case CPPort:
    case NOCPort:
    case PropPort:
      fprintf(f,
	      "%s    %s => %s,\n"
	      "    %s => %s",
	      last.c_str(),
	      in.c_str(), in.c_str(),
	      out.c_str(), out.c_str());
    default:;
    }
  }
#endif
  if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
    fprintf(f, ",\n    props_in => props_to_worker");
  if (m_ctl.nonRawReadbacks || m_ctl.rawReadables)
    fprintf(f, ",\n    props_out => props_from_worker");
  if (m_signals.size()) {
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
      Signal *s = *si;
      if (s->m_differential) {
	std::string name;
	OU::format(name, s->m_pos.c_str(), s->m_name.c_str());
	fprintf(f, ",\n    %s => %s", name.c_str(), name.c_str());
	OU::format(name, s->m_neg.c_str(), s->m_name.c_str());
	fprintf(f, ",\n    %s => %s", name.c_str(), name.c_str());
      } else
	fprintf(f, ",\n    %s => %s", s->m_name.c_str(), s->m_name.c_str());
    }
  }
  fprintf(f, ");\n");
  fprintf(f, "end rtl;\n");
}

void Worker::
emitVhdlSignalWrapper(FILE *f, const char *topinst) {
    fprintf(f, 
	    "\n"
	    "-- This is the wrapper entity that does NOT use records for ports\n"
	    "-- It \"wraps\" the _rv entity that DOES use records for ports\n"
	    "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	    "library ocpi; use ocpi.all, ocpi.types.all;\n"
	    "use work.%s_defs.all;\n",
	    m_implName);
    emitVhdlLibraries(f);
    fprintf(f,
	    "entity %s--__\n is\n", m_implName);
    emitParameters(f, m_language, false);
    emitSignals(f, m_language, false, false, false);
    fprintf(f, "end entity %s--__\n;\n", m_implName);
    fprintf(f,
	    "library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;\n"
	    "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n");
    emitVhdlLibraries(f);
    fprintf(f,
	    "architecture rtl of %s--__\nis begin\n"
	    "  %s: entity work.%s_rv\n",
	    m_implName, topinst, m_implName);
    bool first = true;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if ((*pi)->m_isParameter) {
	if (first) {
	  fprintf(f,
		  "    generic map(\n");
	}
	fprintf(f,  "%s      %s => %s",
		first ? "" : ",\n", (*pi)->m_name.c_str(), (*pi)->m_name.c_str());
	first = false;
      }
    if (!first)
      fprintf(f, ")\n");
    std::string init = "    port map(\n";
    std::string last = init;
    for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
      Clock *c = *ci;
      if (!c->port) {
	if (last.empty())
	  fprintf(f,
		  "  -- Clock(s) not associated with one specific port:\n");
	fprintf(f, "%s      %s => %s", last.c_str(), c->signal, c->signal);
	last = ",\n";
	if (c->reset.size())
	  fprintf(f, "%s      %s => %s", last.c_str(), c->reset.c_str(), c->reset.c_str());
      }
    }
    for (unsigned i = 0; i < m_ports.size(); i++)
#if 1
      m_ports[i]->emitVHDLSignalWrapperPortMap(f, last);
#else
 {
      Port *p = m_ports[i];
      bool mIn = p->masterIn();
      std::string in, out;
      OU::format(in, p->typeNameIn.c_str(), "");
      OU::format(out, p->typeNameOut.c_str(), "");
      switch (p->type) {
      case WCIPort:
      case WSIPort:
      case WMIPort:
      case WTIPort:
      case WMemIPort:
	{
	  OcpSignalDesc *osd = ocpSignals;
	  for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	    if (os->value && os->master) {
	      fprintf(f, "%s      %s.%s => %s", last.c_str(),
		      os->master == mIn ? in.c_str() : out.c_str(),
		      osd->name, os->signal);
	      last = ",\n";
	    }
	  osd = ocpSignals;
	  for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	    if (os->value && !os->master) {
	      fprintf(f, "%s      %s.%s => %s", last.c_str(),
		      os->master == mIn ? in.c_str() : out.c_str(),
		      osd->name, os->signal);
	      last = ",\n";
	    }
	}
	break;
      case TimePort:
	fprintf(f,
		"%s      %s => %s",
		last.c_str(),
		p->master ? out.c_str() : in.c_str(),
		p->master ? out.c_str() : in.c_str());
	break;
      case MetadataPort:
      case CPPort:
      case NOCPort:
      case PropPort:
	fprintf(f,
		"%s      %s => %s,\n"
		"      %s => %s",
		last.c_str(),
		in.c_str(), in.c_str(),
		out.c_str(), out.c_str());
      default:;
      }
    }
#endif
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
      Signal *s = *si;
      if (s->m_differential) {
	std::string name;
	OU::format(name, s->m_pos.c_str(), s->m_name.c_str());
	fprintf(f, "%s      %s => %s,\n", last.c_str(), name.c_str(), name.c_str());
	OU::format(name, s->m_neg.c_str(), s->m_name.c_str());
	fprintf(f, "      %s => %s", name.c_str(), name.c_str());
      } else
	fprintf(f, "%s      %s => %s", last.c_str(), s->m_name.c_str(), s->m_name.c_str());
      last = ",\n";
    }
    if (last != init)
      fprintf(f, ")");
    fprintf(f,
	    ";\n"
	    "end rtl;\n");
}

void Worker::
emitVhdlRecordWrapper(FILE *f) {
    fprintf(f, 
	    "\n"
	    "-- This is the wrapper entity that uses records for ports\n"
	    "-- It \"wraps\" the signal-level, Verilog compatible entity that only uses signals for ports\n"
	    "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	    "library ocpi; use ocpi.all, ocpi.types.all;\n"
	    "use work.%s_defs.all;\n",
	    m_implName);
    emitVhdlLibraries(f);
    fprintf(f,
	    "entity %s_rv is\n", m_implName);
    emitParameters(f, VHDL, false);
    emitSignals(f, VHDL, true, false, false);
    fprintf(f, "end entity %s_rv;\n", m_implName);
    fprintf(f,
	    "library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;\n"
	    "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n");
    emitVhdlLibraries(f);
    fprintf(f,
	    "architecture rtl of %s_rv is\n", m_implName);
    // Define individual signals to work around isim bug that it can't use indexed records in actuals
    // What a waste of time for a vendor bug
    for (unsigned i = 0; i < m_ports.size(); i++)
#if 1
      m_ports[i]->emitVHDLRecordWrapperSignals(f);
#else      
      {
      Port *p = m_ports[i];
      if (p->isOCP())
	for (unsigned n = 0; n < p->count; n++) {
	  std::string num;
	  OU::format(num, "%u", n);
	  std::string in, out;
	  OU::format(in, p->typeNameIn.c_str(), "");
	  OU::format(out, p->typeNameOut.c_str(), "");
	  OcpSignalDesc *osd = ocpSignals;
	  for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
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
#endif
    fprintf(f,
	    "begin\n");
    // Assign individual signals to work around isim bug that it can't use indexed records in actuals
    // What a waste of time for a vendor bug
    for (unsigned i = 0; i < m_ports.size(); i++)
#if 1
      m_ports[i]->emitVHDLRecordWrapperAssignments(f);
#else      
      {
      Port *p = m_ports[i];
      bool mIn = p->masterIn();
      if (p->isOCP())
	for (unsigned n = 0; n < p->count; n++) {
	  std::string num;
	  OU::format(num, "%u", n);
	  std::string in, out;
	  OU::format(in, p->typeNameIn.c_str(), "");
	  OU::format(out, p->typeNameOut.c_str(), "");
	  OcpSignalDesc *osd = ocpSignals;
	  for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	    if (os->value) {
	      std::string name;
	      OU::format(name, os->signal, num.c_str());
	      if (os->value) {
		std::string rec;
		if (p->count > 1)
		  OU::format(rec, "%s(%u).%s", os->master == mIn ? in.c_str() : out.c_str(),
			     n, osd->name);
		else
		  OU::format(rec, "%s.%s", os->master == mIn ? in.c_str() : out.c_str(),
			     osd->name);
		fprintf(f, "  %s <= %s;\n",
			os->master == mIn ? name.c_str() : rec.c_str(),
			os->master == mIn ? rec.c_str() : name.c_str());
	      }
	    }
	}
    }
#endif
    fprintf(f,
	    "  assy : work.%s_defs.%s\n",
	    m_implName, m_implName);
    bool first = true;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if ((*pi)->m_isParameter) {
	OU::Property &p = **pi;
	if (first) {
	  fprintf(f,
		  "    generic map(\n");
	}
	fprintf(f,  "%s      %s => %s%s%s",
		first ? "" : ",\n", p.m_name.c_str(),
		p.m_baseType == OA::OCPI_Bool ? "ocpi.util.slv(" : "",
		p.m_name.c_str(),
		p.m_baseType == OA::OCPI_Bool ? ")" : "");
	first = false;
      }
    if (!first)
      fprintf(f, ")\n");

    fprintf(f, "    port map(\n");
    std::string last;
    for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
      Clock *c = *ci;
      if (!c->port) {
	if (last.empty())
	  fprintf(f,
		  "  -- Clock(s) not associated with one specific port:\n");
	fprintf(f, "%s      %s => %s", last.c_str(), c->signal, c->signal);
	last = ",\n";
      }
    }
    for (unsigned i = 0; i < m_ports.size(); i++)
#if 1
      m_ports[i]->emitVHDLRecordWrapperPortMap(f, last);
#else
 {
      Port *p = m_ports[i];
      switch (p->type) {
      case WCIPort:
      case WSIPort:
      case WMIPort:
      case WTIPort:
      case WMemIPort:
	{
	  for (unsigned n = 0; n < p->count; n++) {
	    std::string num;
	    OU::format(num, "%u", n);
	    std::string in, out;
	    OU::format(in, p->typeNameIn.c_str(), "");
	    OU::format(out, p->typeNameOut.c_str(), "");
	    OcpSignalDesc *osd = ocpSignals;
	    for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
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
	break;
      case TimePort:
      case MetadataPort:
      case CPPort:
      case NOCPort:
      default:
	ocpiAssert("Non-application port types can't be in VHDL assy wrappers" == 0);
      }
    }
#endif
    for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
      Signal *s = *si;
      if (s->m_differential) {
	std::string name;
	OU::format(name, s->m_pos.c_str(), s->m_name.c_str());
	fprintf(f, "%s      %s => %s,\n", last.c_str(), name.c_str(), name.c_str());
	OU::format(name, s->m_neg.c_str(), s->m_name.c_str());
	fprintf(f, "      %s => %s", name.c_str(), name.c_str());
      } else
	fprintf(f, "%s      %s => %s", last.c_str(), s->m_name.c_str(), s->m_name.c_str());
      last = ",\n";
    }
    fprintf(f,
	    ");\n"
	    "end rtl;\n");
}

// Generate the readonly implementation file.
// What implementations must explicitly (verilog) or implicitly (VHDL) include.
// The idea is to minimize code in the actual worker implementation (nee skeleton) file,
// without constructing significant "state" or "newly defined internal interfaces".
const char *Worker::
emitImplHDL(bool wrap) {
  const char *err;
  FILE *f;
  Language lang = wrap ? (m_language == VHDL ? Verilog : VHDL) : m_language;
  if ((err = openOutput(m_implName, m_outDir, "", IMPL, lang == VHDL ? VHD : ".vh", NULL, f)))
    return err;
  const char *comment = hdlComment(lang);
  printgen(f, comment, m_file.c_str());
  if (m_assembly)
    return emitAssyImplHDL(f, wrap);
  else if (wrap) {
    // Worker (not assembly) wrapper is just a conversion.
    if (m_language == VHDL)
      // The Verilog wrapper around a VHDL assembly doesn't do anything,
      // Since Verilog can already instantiate based on what is in the VHDL impl file.
      fprintf(f, "// The wrapper to enable instantion from Verilog is in the VHDL -impl.vhd file.\n");
    else
      // The worker is in Verilog, so we implement the record-to-signal wrapper here.
      emitVhdlRecordWrapper(f);
    fclose(f);
    return NULL;
  }
  fprintf(f,
	  "%s This file contains the implementation declarations for worker %s\n"
	  "%s Interface definition signal names are defined with pattern rule: \"%s\"\n\n",
	  comment, m_implName, comment, m_pattern);
  unsigned maxPropName = 18;
  for (PropertiesIter pi = m_ctl.properties.begin(); nonRaw(pi); pi++) {
    OU::Property &pr = **pi;
    size_t len = pr.m_name.length();
    if (pr.m_isWritable) {
      if (pr.m_isInitial)
	len += strlen("_value");
      else if (pr.m_arrayRank || pr.m_stringLength > 3 || pr.m_isSequence)
	len += strlen("_any_written");
      else
	len += strlen("_written");
    } else if (!pr.m_isParameter && pr.m_isReadable)
      len += strlen("_read");
    if (len > maxPropName)
      maxPropName = (unsigned)len;
  }
  // At the top of the file, for the convenience of the implementer, we emit
  // the actual thing that the author implements, the foo_worker entity.
  if (m_language == VHDL) {
    // Put out the port records that the entity needs
    fprintf(f,
	    "--                   OCP-based Control Interface, based on the WCI profile,\n"
	    "--                      used for clk/reset, control and configuration\n"
	    "--                                           /\\\n"
	    "--                                          /--\\\n"
	    "--               +--------------------OCP----||----OCP---------------------------+\n"
	    "--               |                          \\--/                                 |\n"
	    "--               |                           \\/                                  |\n"
	    "--               |                   Entity: <worker>                            |\n"
	    "--               |                                                               |\n"
	    "--               O   +------------------------------------------------------+    O\n"
	    "--               C   |            Entity: <worker>_worker                   |    C\n"
	    "--               P   |                                                      |    P\n"
	    "--               |   | This \"inner layer\" is the code you write, based      |    |\n"
	    "-- Data Input    |\\  | on definitions the in <worker>_worker_defs package,  |    |\\  Data Output\n"
	    "-- Port based  ==| \\ | and the <worker>_worker entity, both in this file,   |   =| \\ Port based\n"
	    "-- on the WSI  ==| / | both in the \"work\" library.                          |   =| / on the WSI\n"
	    "-- OCP Profile   |/  | Package and entity declarations are in this          |    |/  OCP Profile\n"
	    "--               O   | <worker>_impl.vhd file. Architeture is in your       |    |\n"
	    "--               O   |  <worker>.vhd file                                   |    O\n"
	    "--               C   |                                                      |    C\n"
	    "--               P   +------------------------------------------------------+    P\n"
	    "--               |                                                               |\n"
	    "--               |     This outer layer is the \"worker shell\" code which         |\n"
	    "--               |     is automatically generated.  The \"worker shell\" is        |\n"
	    "--               |     defined as the <worker> entity using definitions in       |\n"
	    "--               |     the <worker>_defs package.  The worker shell is also      |\n"
	    "--               |     defined as a VHDL component in the <worker>_defs package, |\n"
	    "--               |     as declared in the <worker>-defs.vhd file.                |\n"
	    "--               |     The worker shell \"architecture\" is also in this file,      |\n"
	    "--               |     as well as some subsidiary modules.                       |\n"
	    "--               +---------------------------------------------------------------+\n"
	    "\n");
    if ((err = emitVhdlWorkerPackage(f, maxPropName)) ||
	(err = emitVhdlWorkerEntity(f)))
      return err;
    
    fprintf(f,
	    "-- The rest of the file below here is the implementation of the worker shell\n"
	    "-- which surrounds the entity to be implemented, above.\n");
  }
  if (m_language == VHDL) {
    //    emitVhdlPackageBody(f);
    fprintf(f, 
	    "\n"
	    "-- This is the entity declaration for the top level record-based VHDL\n"
	    "-- The achitecture for this entity will be in the implementation file\n"
	    "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	    "library ocpi; use ocpi.all, ocpi.types.all;\n"
	    "use work.%s_worker_defs.all, work.%s_defs.all;\n",
	    m_implName, m_implName);
    emitVhdlLibraries(f);
    fprintf(f,
	    "entity %s_rv is\n", m_implName);
    emitParameters(f, m_language, false);
    emitSignals(f, m_language, true, false, false);
    fprintf(f,
	    "  -- these signals are used whether there is a control interface or not.\n"
            "  signal wci_reset         : bool_t;\n"
            "  signal wci_is_operating  : bool_t;\n");

  } else
    // Verilog just needs the module declaration and any other associate declarations
    // required for the module declaration.
    fprintf(f,
	    "`define NOT_EMPTY_%s // suppress the \"endmodule\" in %s%s%s\n"
	    "`include \"%s%s%s\"\n"
	    "`include \"ocpi_wip_defs%s\"\n",
	    m_implName, m_implName, DEFS, VERH, m_implName, DEFS, VERH, VERH);

  // Aliases for port-specific signals, or simple combinatorial "macros".
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    for (unsigned n = 0; n < p->count; n++)
      p->emitImplAliases(f, n, lang);
  }
  if (m_language == VHDL) {
    unsigned n = 0;
    for (unsigned i = 0; i < m_ports.size(); i++)
#if 1
      m_ports[i]->emitImplSignals(f);
#else
 {
      Port *p = m_ports[i];
      switch (p->type) {
      case WCIPort:
	// Record for property-related inputs to the worker - writable values and strobes, readable strobes
	if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
	  fprintf(f, "  signal props_to_worker   : worker_props_in_t;\n");
	if (m_ctl.nonRawReadbacks || m_ctl.rawReadables)
	  fprintf(f, "  signal props_from_worker : worker_props_out_t;\n");
	fprintf(f,
		"  -- wci information into worker\n");
	if (m_endian == Dynamic)
	  fprintf(f, "  signal wci_is_big_endian    : Bool_t;\n");
	fprintf(f,
		"  signal wci_control_op       : wci.control_op_t;\n"
		"  signal raw_offset           : unsigned(work.%s_worker_defs.worker.decode_width-1 downto 0);\n"
		"  signal wci_state            : wci.state_t;\n"
		"  -- wci information from worker\n"
		"  signal wci_attention        : Bool_t;\n"
		"  signal wci_abort_control_op : Bool_t;\n"
		"  signal wci_done             : Bool_t;\n"
		"  signal wci_error            : Bool_t;\n"
		"  signal wci_finished         : Bool_t;\n"
		"  signal wci_is_read          : Bool_t;\n"
		"  signal wci_is_write         : Bool_t;\n", m_implName);
	break;
      case WSIPort:
      case WMIPort:
#if 0
	if (n++ == 0)
	  fprintf(f, "  -- signal bundling functions for WSI ports\n");
	fprintf(f,
		"  impure function wsi_%s_inputs return %s_in_t is begin\n"
		"    return %s_in_t'(",
		p->name(), p->name(), p->name());
	if (p->masterIn())
	  fprintf(f, 
		  "%s_MBurstLength,%s_MByteEn, %s_MCmd, %s_MData,\n"
		  "                    %s_MReqLast, %s_MReset_n",
		  p->name(), p->name(), p->name(), p->name(), p->name(), p->name());
	else
	  fprintf(f, 
		  "%s_SReset_n, %s_SThreadBusy",
		  p->name(), p->name());
	fprintf(f, 
		");\n"
		"  end wsi_%s_inputs;\n",
		p->name());
#endif
	{
	  //	  const char *tofrom = p->masterIn() ? "from" : "to";
	  fprintf(f,
		  "  signal %s_%s  : Bool_t;\n"
		  "  signal %s_ready : Bool_t;\n"
		  "  signal %s_reset : Bool_t; -- this port is being reset from the outside\n",
		  p->name(), p->masterIn() ? "take" : "give", p->name(), p->name());
	  if (p->dataWidth)
	    fprintf(f,
		    "  signal %s_data  : std_logic_vector(%zu downto 0);\n",
		    p->name(),
		    p->dataWidth-1);
	  if (p->ocp.MByteEn.value)
	    fprintf(f, "  signal %s_byte_enable: std_logic_vector(%zu downto 0);\n",
		    p->name(), p->dataWidth / p->byteWidth - 1);		    
	  if (p->preciseBurst)
	    fprintf(f, "  signal %s_burst_length: std_logic_vector(%zu downto 0);\n",
		    p->name(), p->ocp.MBurstLength.width - 1);
	  if (p->u.wdi.nOpcodes > 1) {
	    fprintf(f,
		    "  -- The strongly typed enumeration signal for the port\n"
		    "  signal %s_opcode      : %s_OpCode_t;\n"
		    "  -- The weakly typed temporary signals\n"
		    "  signal %s_opcode_temp : std_logic_vector(%zu downto 0);\n"
		    "  signal %s_opcode_pos  : integer;\n",
		    p->name(), p->m_protocol && p->m_protocol->operations() ?
		    p->m_protocol->m_name.c_str() : p->name(), p->name(), p->ocp.MReqInfo.width - 1, p->name());
	  }
	  fprintf(f,
		  "  signal %s_som   : Bool_t;    -- valid eom\n"
		  "  signal %s_eom   : Bool_t;    -- valid som\n",
		  p->name(), p->name());
	  if (p->dataWidth)
	    fprintf(f,
		    "  signal %s_valid : Bool_t;   -- valid data\n", p->name());
	}
	break;
      default:
	;
      }
    }
#endif
    fprintf(f,
	    "end entity %s_rv;\n"
	    "\n", m_implName);
    if (m_wci) {
      size_t decodeWidth = m_wci->decodeWidth();
      fprintf(f,
	      "-- Here we define and implement the WCI interface module for this worker,\n"
	      "-- which can be used by the worker implementer to avoid all the OCP/WCI issues\n"
	      "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	      "library ocpi; use ocpi.all, ocpi.types.all;\n"
	      "use work.%s_worker_defs.all, work.%s_defs.all;\n"
	      "entity %s_wci is\n"
	      "  generic(ocpi_debug : bool_t);\n"
	      "  port(\n"
	      "    %-*s : in  %s_t;          -- signal bundle from wci interface\n"
	      "    %-*s : in  bool_t := btrue;   -- worker uses this to delay completion\n"
	      "    %-*s : in  bool_t := bfalse;  -- worker uses this to indicate error\n"
	      "    %-*s : in  bool_t := bfalse;  -- worker uses this to indicate finished\n"
	      "    %-*s : in  bool_t := bfalse;  -- worker indicates an attention condition\n"
	      "    %-*s : out %s_t;         -- signal bundle to wci interface\n"
	      "    %-*s : out bool_t;            -- wci reset for worker\n"
	      "    %-*s : out wci.control_op_t;  -- control op in progress, or no_op_e\n"
	      "    %-*s : out wci.state_t;       -- wci state: see state_t\n"
#if 0
	      "    %-*s : out unsigned(%s_worker_defs.worker.decode_width-1 downto 0);\n"
	      "    %-*s : out bool_t;            -- is a config read in progress?\n"
	      "    %-*s : out bool_t;            -- is a config write in progress?\n"
#endif
	      "    %-*s : out bool_t;            -- shorthand for state==operating_e\n",
	      m_implName, m_implName, m_implName,
	      maxPropName, "inputs", m_wci->typeNameIn.c_str(),
	      maxPropName, "done",
	      maxPropName, "error",
	      maxPropName, "finished",
	      maxPropName, "attention",
	      maxPropName, "outputs", m_wci->typeNameOut.c_str(),
	      maxPropName, "reset",
	      maxPropName, "control_op",
	      maxPropName, "state",
	      maxPropName, "is_operating");
      if (m_endian == Dynamic)
	fprintf(f, 
		"    %-*s : out bool_t;           -- for endian-switchable workers\n",
		maxPropName, "is_big_endian");
      fprintf(f,
	      "    %-*s : out bool_t%s            -- forcible abort a control-op when\n"
	      "                                                -- worker uses 'done' to delay it\n",
	      maxPropName, "abort_control_op",
	      m_ctl.nRunProperties == 0 ? " " : ";");
      // Record for property-related inputs to the worker - writable values and strobes, readable strobes
      if (m_ctl.nonRawReadbacks || m_ctl.rawReadables)
	fprintf(f, "    props_from_worker  : in  worker_props_out_t");
      if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
	fprintf(f, "%s    props_to_worker    : out worker_props_in_t",
		m_ctl.nonRawReadbacks || m_ctl.rawReadables ? ";\n" : "");
      fprintf(f,
	      "\n"
	      ");\n"
	      "end entity;\n");
      fprintf(f,
	      "architecture rtl of %s_wci is\n"
	      //	    "  signal my_clk   : std_logic; -- internal usage of output\n"
	      "  signal my_reset : bool_t; -- internal usage of output\n",
	      m_implName);
      if (m_ctl.nNonRawRunProperties) {
	unsigned nProps_1 = m_ctl.nRunProperties - 1;
	fprintf(f,
		"  -- signals for property reads and writes\n"
		"  signal offsets       : "
		"wci.offset_a_t(0 to %u);  -- offsets within each property\n"
		"  signal indices       : "
		"wci.offset_a_t(0 to %u);  -- array index for array properties\n"
		"  signal hi32          : "
		"bool_t;                 -- high word of 64 bit value\n"
		"  signal nbytes_1      : "
		"types.byte_offset_t;       -- # bytes minus one being read/written\n",
		nProps_1, nProps_1);
	if (m_ctl.nonRawWritables)
	  fprintf(f,
		  "  -- signals between the decoder and the writable property registers\n"
		  "  signal write_enables : "
		  "bool_array_t(0 to %u);\n"
		  "  signal data          : "
		  "wci.data_a_t (0 to %u);   -- data being written, right justified\n",
		  nProps_1, nProps_1);
	if (m_ctl.nonRawReadables) {
	  fprintf(f,
		  "  -- signals between the decoder and the readback mux\n"
		  "  signal read_enables  : bool_array_t(0 to %u);\n"
		  "  signal readback_data : wci.data_a_t(work.%s_worker_defs.properties'range);\n",
		  nProps_1, m_implName);
	  fprintf(f,
		  "  -- The output to SData from nonRaw properties\n"
		  "  signal nonRaw_SData  : std_logic_vector(31 downto 0);\n");
	}
	bool first = true;
	for (PropertiesIter pi = m_ctl.properties.begin(); nonRaw(pi); pi++) {
	  OU::Property &pr = **pi;
	  if (!pr.m_isParameter && pr.m_isWritable && pr.m_isReadable && !pr.m_isVolatile) {
	    if (first) {
	      fprintf(f,
		      "  -- internal signals between property registers and the readback mux\n"
		      "  -- for those that are writable, readable, and not volatile\n");
	      first = false;
	    }
	    std::string type;
	    prType(pr, type);
	    char *temp = NULL;
	    fprintf(f,
		    "  signal my_%s : %s;\n",
		    tempName(temp, maxPropName, "%s_value", pr.m_name.c_str()),
		    type.c_str());
	    free(temp);
	  }
	}
      }
      if (m_ctl.rawReadables)
	fprintf(f,
		"  signal my_is_read : bool_t;\n");
      fprintf(f,
	      "  -- temp signal to workaround isim/fuse crash bug\n"
	      "  signal wciAddr : std_logic_vector(31 downto 0);\n"
	      "begin\n"
	      "  wciAddr(inputs.MAddr'range)            <= inputs.MAddr;\n");
      if (m_ctl.rawProperties)
	fprintf(f,
		"  props_to_worker.raw_byte_enable        <= %s;\n",
		m_ctl.sub32Bits ? "inputs.MByteEn" : "(others => '1')");
      if (m_ctl.rawReadables)
	fprintf(f,
		"  props_to_worker.raw_is_read            <= my_is_read;\n");
      if (m_ctl.rawWritables)
	fprintf(f,
		"  props_to_worker.raw_data               <= inputs.MData;\n");
      if (decodeWidth < 32)
	fprintf(f,
              "  wciAddr(31 downto inputs.MAddr'length) <= (others => '0');\n");
      fprintf(f,
	      "  outputs.SFlag(0)                       <= '1' when its(attention) else '0';\n"
	      "  outputs.SFlag(1)                       <= '1'; -- worker is present\n"
	      "  outputs.SFlag(2)                       <= '1' when its(finished) else '0';\n"
	      //	    "  my_clk <= inputs.Clk;\n"
	      "  my_reset                               <= to_bool(inputs.MReset_n = '0');\n"
	      "  reset                                  <= my_reset;\n");
      if (m_ctl.nRunProperties)
	fprintf(f,
		"  wci_decode : component wci.decoder\n"
		"      generic map(worker               => work.%s_worker_defs.worker,\n"
		"                  properties           => work.%s_worker_defs.properties,\n"
		"                  ocpi_debug           => ocpi_debug)\n",
		m_implName, m_implName);
      else
	fprintf(f,
		"  wci_decode : component wci.control_decoder\n"
		"      generic map(worker               => work.%s_worker_defs.worker)\n",
		m_implName);
      fprintf(f,
	      "      port map(   ocp_in.Clk           => inputs.Clk,\n"
	      "                  ocp_in.Maddr         => wciAddr,\n"
	      "                  ocp_in.MAddrSpace    => %s,\n"
	      "                  ocp_in.MByteEn       => %s,\n"
	      "                  ocp_in.MCmd          => inputs.MCmd,\n"
	      "                  ocp_in.MData         => %s,\n"
	      "                  ocp_in.MFlag         => inputs.MFlag,\n"
	      "                  ocp_in.MReset_n      => inputs.MReset_n,\n",
	      m_wci->ocp.MAddrSpace.value ? "inputs.MAddrSpace" : "\"0\"",
	      m_wci->ocp.MByteEn.value ? "inputs.MByteEn" : "\"0000\"",
	      m_wci->ocp.MData.value ? "inputs.MData" : "\"00000000000000000000000000000000\"");
      fprintf(f,
	      "                  done                 => done,\n"
	      "                  error                => error,\n"
	      "                  finished             => finished,\n"
	      "                  resp                 => outputs.SResp,\n"
	      "                  busy                 => outputs.SThreadBusy(0),\n"
	      "                  control_op           => control_op,\n"
	      "                  state                => state,\n"
	      "                  is_operating         => is_operating,\n");
      if (m_endian == Dynamic)
	fprintf(f, "                  is_big_endian        => is_big_endian,\n");
      fprintf(f,
	      "                  abort_control_op     => abort_control_op");
      if (m_ctl.nRunProperties)
	fprintf(f,
		",\n"
		"                  raw_offset           => %s,\n"
		"                  is_read              => %s,\n"
		"                  is_write             => %s",
		m_ctl.rawProperties ? "props_to_worker.raw_address" : "open",
		m_ctl.rawReadables ? "my_is_read" : "open",
		m_ctl.rawWritables ? "props_to_worker.raw_is_write" : "open");
      if (m_ctl.nNonRawRunProperties)
	fprintf(f,
		",\n"
		"                  write_enables        => %s,\n"
		"                  read_enables         => %s,\n"
		"                  offsets              => offsets,\n"
		"                  indices              => indices,\n"
		"                  hi32                 => hi32,\n"
		"                  nbytes_1             => nbytes_1,\n"
		"                  data_outputs         => %s",
		m_ctl.nonRawWritables ? "write_enables" : "open",
		m_ctl.nonRawReadables ? "read_enables" : "open",
		m_ctl.nonRawWritables ? "data" : "open");
      fprintf(f, ");\n");
      if (m_ctl.nonRawReadables)
	fprintf(f,
		"  readback : component wci.readback\n"
		"    generic map(work.%s_worker_defs.properties, ocpi_debug)\n"
		"    port map(   read_enables => read_enables,\n"
		"                data_inputs  => readback_data,\n"
		"                data_output  => nonRaw_SData);\n",
		m_implName);
      if (m_ctl.readables)
	fprintf(f, "  outputs.SData <= %s;\n",
		m_ctl.nonRawReadables ? 
		(m_ctl.rawReadables ? 
		 "props_from_worker.raw_data when its(my_is_read) else nonRaw_SData" :
		 "nonRaw_SData") :
		(m_ctl.rawReadables ? "props_from_worker.raw_data" : "(others => '0')"));
      n = 0;
      for (PropertiesIter pi = m_ctl.properties.begin(); nonRaw(pi); pi++) {
	OU::Property &pr = **pi;
	const char *name = pr.m_name.c_str();
	if (pr.m_isParameter)
	  continue;
	if (pr.m_isWritable) {
	  fprintf(f, 
		  "  %s_property : component ocpi.props.%s%s_property\n"
		  "    generic map(worker       => work.%s_worker_defs.worker,\n"
		  "                property     => work.%s_worker_defs.properties(%u)",
		  name, OU::baseTypeNames[pr.m_baseType],
		  pr.m_arrayRank || pr.m_isSequence ? "_array" : "",
		  m_implName, m_implName, n);
	  if (pr.m_default) {
	    std::string vv;
	    vhdlValue(*pr.m_default, vv);
	    fprintf(f,
		    ",\n"
		    "                default     => %s",
		    vv.c_str());
	  }
		
	  fprintf(f,
		  ")\n"
		  "    port map(   clk          => inputs.Clk,\n"
		  "                reset        => my_reset,\n"
		  "                write_enable => write_enables(%u),\n"
		  "                data         => data(%u)(%zu downto 0),\n",
		  n, n,
		  pr.m_nBits >= 32 || pr.m_arrayRank || pr.m_isSequence ?
		  31 : (pr.m_baseType == OA::OCPI_Bool ? 0 : pr.m_nBits-1));
	  if (pr.m_isReadable && !pr.m_isVolatile)
	    fprintf(f,
		    "                value        => my_%s_value, -- for readback and worker\n", name);
	  else
	    fprintf(f,
		    "                value        => props_to_worker.%s,\n", name);
	  if (pr.m_isInitial)
	    fprintf(f, "                written      => open");
	  else
	    fprintf(f, "                written      => props_to_worker.%s_written", name);
	  if (pr.m_arrayRank || pr.m_isSequence) {
	    fprintf(f, ",\n"
		    "                index        => indices(%u)(%zu downto 0),\n",
		    n, decodeWidth-1);
	    if (pr.m_isInitial)
	      fprintf(f,
		      "                any_written  => open");
	    else
	      fprintf(f,
		      "                any_written  => props_to_worker.%s_any_written", name);
	    if (pr.m_baseType != OA::OCPI_String &&
		pr.m_nBits != 64)
	      fprintf(f, ",\n"
		      "                nbytes_1     => nbytes_1");
	  }
	  if (pr.m_nBits == 64)
	    fprintf(f, ",\n"
		    "                hi32         => hi32");
	  if (pr.m_baseType == OA::OCPI_String)
	    fprintf(f, ",\n"
		    "                offset        => offsets(%u)(%zu downto 0)",
		    n, decodeWidth-1);
	  fprintf(f, ");\n");
	  if (pr.m_isReadable && !pr.m_isVolatile)
	    fprintf(f, "  props_to_worker.%s <= my_%s_value;\n", name, name);
	}
	if (pr.m_isReadable) {
	  fprintf(f, 
		  "  %s_readback : component ocpi.props.%s_read%s_property\n"
		  "    generic map(worker       => work.%s_worker_defs.worker,\n"
		  "                property     => work.%s_worker_defs.properties(%u))\n"
		  "    port map(",
		  pr.m_name.c_str(), OU::baseTypeNames[pr.m_baseType],
		  pr.m_arrayRank || pr.m_isSequence ? "_array" : "",
		  m_implName, m_implName, n);
	  if (pr.m_isVolatile || pr.m_isReadable && !pr.m_isWritable)
	    fprintf(f, "   value        => props_from_worker.%s,\n", name);
	  else
	    fprintf(f, "   value        => my_%s_value,\n", name);
	  fprintf(f,   "                data_out     => readback_data(%u)", n);
	  if (pr.m_baseType == OA::OCPI_String)
	    fprintf(f, ",\n"
		    "                offset       => offsets(%u)(%zu downto 0)",
		    n, decodeWidth-1);
	  else {
	    if (pr.m_arrayRank || pr.m_isSequence) {
	      fprintf(f, ",\n"
		      "                index        => indices(%u)(%zu downto 0)",
		      n, decodeWidth-1);
	      if (pr.m_nBits != 64)
		fprintf(f, ",\n"
			"                nbytes_1     => nbytes_1");
	    }
	    if (pr.m_nBits == 64)
	      fprintf(f, ",\n"
		      "                hi32       => hi32");
	  }
	  // provide read enable to suppress out-of-bound reads
	  if (pr.m_baseType == OA::OCPI_String)
#if 0
	    //	    || pr.m_arrayRank || pr.m_isSequence) // FIXME: check whether this boundary is needed
#endif
	    fprintf(f, ",\n                read_enable  => read_enables(%u));\n", n);
	  else
	    fprintf(f, ");\n");
	  fprintf(f, "  props_to_worker.%s_read <= read_enables(%u);\n", pr.m_name.c_str(), n);
	} else if (m_ctl.nonRawReadables)
	  fprintf(f, "  readback_data(%u) <= (others => '0');\n", n);
	n++; // only for non-parameters
      }
      fprintf(f,
	      "end architecture rtl;\n");
    }
    if (!m_outer)
      emitVhdlShell(f);
    emitVhdlSignalWrapper(f);
  }
  fclose(f);
  return 0;
}

const char *Worker::
openSkelHDL(const char *suff, FILE *&f) {
  const char *err;
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "", suff, m_language == VHDL ? VHD : VER, NULL, f)))
    return err;
  printgen(f, hdlComment(m_language), m_file.c_str(), true);
  return 0;
}
const char *Worker::
emitSkelHDL() {
  FILE *f;
  const char *err = openSkelHDL(SKEL, f);
  if (err)
    return err;
  if (m_language == VHDL) {
    if (m_outer) {
      fprintf(f,
	      "-- This file initially contains the architecture skeleton for worker: %s\n"
	      "-- Note THIS IS THE OUTER skeleton, since the 'outer' attribute was set.\n\n",
	      m_implName);
      fprintf(f,
	      "library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;\n"
	      "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n"
	      "architecture rtl of %s_rv is\n"
	      "begin\n"
	      "end rtl;\n",
	      m_implName);
    } else {
      fprintf(f,
	      "-- This file initially contains the architecture skeleton for worker: %s\n\n",
	      m_implName);
      fprintf(f,
	      "library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;\n"
	      "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n"
	      "architecture rtl of %s_worker is\n"
	      "begin\n"
	      "end rtl;\n",
	      m_implName);
    }
  } else {
    fprintf(f,
	    "// This file contains the implementation skeleton for worker: %s\n\n"
	    "`include \"%s-impl%s\"\n\n",
	    m_implName, m_implName, VERH);
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitSkelSignals(f);
    fprintf(f, "\n\nendmodule //%s\n",  m_implName);
  }
  fclose(f);
  return 0;
}

#define BSV ".bsv"
const char *Worker::
emitBsvHDL() {
#if 1
  assert("NO BSV SUPPORT" == 0);
#else
  const char *err;
  FILE *f;
  if ((err = openOutput(m_implName, m_outDir, "I_", "", BSV, NULL, f)))
    return err;
  const char *comment = "//";
  printgen(f, comment, m_file.c_str());
  fprintf(f,
	  "%s This file contains the BSV declarations for the worker with\n"
	  "%s  spec name \"%s\" and implementation name \"%s\".\n"
	  "%s It is needed for instantiating the worker in BSV.\n"
	  "%s Interface signal names are defined with pattern rule: \"%s\"\n\n",
	  comment, comment, m_specName, m_implName, comment, comment, m_pattern);
  fprintf(f,
	  "package I_%s; // Package name is the implementation name of the worker\n\n"
	  "import OCWip::*; // Include the OpenOCPI BSV WIP package\n\n"
	  "import Vector::*;\n"
	  "// Define parameterized types for each worker port\n"
	  "//  with parameters derived from WIP attributes\n\n",
	  m_implName);
  unsigned n, nn;
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num;
    if (p->count == 1) {
      fprintf(f, "// For worker interface named \"%s\"", p->name());
      num = "";
    } else
      fprintf(f, "// For worker interfaces named \"%s0\" to \"%s%zu\"",
	      p->name(), p->name(), p->count - 1);
    fprintf(f, " WIP Attributes are:\n");
    switch (p->type) {
    case WCIPort:
      fprintf(f, "// SizeOfConfigSpace: %" PRIu64 " (0x%" PRIx64 ")\fn",
	      m_ctl.sizeOfConfigSpace,
	      m_ctl.sizeOfConfigSpace);
      break;
    case WSIPort:
      fprintf(f, "// DataValueWidth: %zu\n", p->m_protocol->m_dataValueWidth);
      fprintf(f, "// MaxMessageValues: %zu\n", p->m_protocol->m_maxMessageValues);
      fprintf(f, "// ZeroLengthMessages: %s\n",
	      p->m_protocol->m_zeroLengthMessages ? "true" : "false");
      fprintf(f, "// NumberOfOpcodes: %zu\n", p->u.wdi.nOpcodes);
      fprintf(f, "// DataWidth: %zu\n", p->dataWidth);
      break;
    case WMIPort:
      fprintf(f, "// DataValueWidth: %zu\n", p->m_protocol->m_dataValueWidth);
      fprintf(f, "// MaxMessageValues: %zu\n", p->m_protocol->m_maxMessageValues);
      fprintf(f, "// ZeroLengthMessages: %s\n",
	      p->m_protocol->m_zeroLengthMessages ? "true" : "false");
      fprintf(f, "// NumberOfOpcodes: %zu\n", p->u.wdi.nOpcodes);
      fprintf(f, "// DataWidth: %zu\n", p->dataWidth);
      break;
    case WMemIPort:
      fprintf(f, "// DataWidth: %zu\n// MemoryWords: %llu (0x%llx)\n// ByteWidth: %zu\n",
	      p->dataWidth, (unsigned long long)p->u.wmemi.memoryWords,
	      (unsigned long long)p->u.wmemi.memoryWords, p->byteWidth);
      fprintf(f, "// MaxBurstLength: %zu\n", p->u.wmemi.maxBurstLength);
      break;
    case WTIPort:
      break;
    default:
      ;
    }
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      switch (p->type) {
      case WCIPort:
	fprintf(f, "typedef Wci_Es#(%zu) ", p->ocp.MAddr.width);
	break;
      case WSIPort:
	fprintf(f, "typedef Wsi_E%c#(%zu,%zu,%zu,%zu,%zu) ",
		p->master ? 'm' : 's',
		p->ocp.MBurstLength.width, p->dataWidth, p->ocp.MByteEn.width,
		p->ocp.MReqInfo.width, p->ocp.MDataInfo.width);
	break;
      case WMIPort:
	fprintf(f, "typedef Wmi_Em#(%zu,%zu,%zu,%zu,%zu,%zu) ",
		p->ocp.MAddr.width, p->ocp.MBurstLength.width, p->dataWidth,
		p->ocp.MDataInfo.width,p->ocp.MDataByteEn.width,
		p->ocp.MFlag.width ? p->ocp.MFlag.width : p->ocp.SFlag.width);
	break;
      case WMemIPort:
	fprintf(f, "typedef Wmemi_Em#(%zu,%zu,%zu,%zu) ",
		p->ocp.MAddr.width, p->ocp.MBurstLength.width, p->dataWidth, p->ocp.MDataByteEn.width);
	break;
      case WTIPort:
      default:
	;
      }
      fprintf(f, "I_%s%s;\n", p->name(), num);
    }
  }
  fprintf(f,
	  "\n// Define the wrapper module around the real verilog module \"%s\"\n"
	  "interface V%sIfc;\n"
	  "  // First define the various clocks so they can be used in BSV across the OCP interfaces\n",
	  m_implName, m_implName);
#if 0
  for (p = m_ports, n = 0; n < m_nPorts; n++, p++) {
    if (p->clock->port == p)
      fprintf(f, "  Clock %s;\n", p->clock->signal);
  }
#endif
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      fprintf(f, "  interface I_%s%s i_%s%s;\n", p->name(), num, p->name(), num);
    }
  }
  fprintf(f,
	  "endinterface: V%sIfc\n\n", m_implName);
  fprintf(f,
	  "// Use importBVI to bind the signal names in the verilog to BSV methods\n"
	  "import \"BVI\" %s =\n"
	  "module vMk%s #(",
	  m_implName, m_implName);
  // Now we must enumerate the various input clocks and input resets as parameters
  std::string last;
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->clock->port == p) {
      fprintf(f, "%sClock i_%sClk", last.c_str(), p->name());
      last = ", ";
    }
  }
  // Now we must enumerate the various reset inputs as parameters
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
			       !p->master && p->ocp.MReset_n.value)) {
      if (p->count > 1)
	fprintf(f, "%sVector#(%zu,Reset) i_%sRst", last.c_str(), p->count, p->name());
      else
	fprintf(f, "%sReset i_%sRst", last.c_str(), p->name());
      last = ", ";
    }
  }
  fprintf(f,
	  ") (V%sIfc);\n\n"
	  "  default_clock no_clock;\n"
	  "  default_reset no_reset;\n\n"
	  "  // Input clocks on specific worker interfaces\n",
	  m_implName);

  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->clock->port == p)
      fprintf(f, "  input_clock  i_%sClk(%s) = i_%sClk;\n",
	      p->name(), p->clock->signal, p->name());
    else
      fprintf(f, "  // Interface \"%s\" uses clock on interface \"%s\"\n", p->name(), p->clock->port->name());
  }
  fprintf(f, "\n  // Reset inputs for worker interfaces that have one\n");
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
				 !p->master && p->ocp.MReset_n.value)) {
	const char *signal;
	asprintf((char **)&signal,
		 p->master ? p->ocp.SReset_n.signal : p->ocp.MReset_n.signal, nn);
	if (p->count > 1)
	  fprintf(f, "  input_reset  i_%s%sRst(%s) = i_%sRst[%u];\n",
		  p->name(), num, signal, p->name(), nn);
	else
	  fprintf(f, "  input_reset  i_%sRst(%s) = i_%sRst;\n",
		  p->name(), signal, p->name());
      }
    }
  }
  unsigned en = 0;
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      fprintf(f, "interface I_%s%s i_%s%s;\n", p->name(), num, p->name(), num);
      OcpSignalDesc *osd;
      OcpSignal *os;
      unsigned o;
      const char *reset;
      if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
				 !p->master && p->ocp.MReset_n.value)) {
	asprintf((char **)&reset, "i_%s%sRst", p->name(), num);
      } else
	reset = "no_reset";
      for (o = 0, os = p->ocp.signals, osd = ocpSignals; osd->name; osd++, os++, o++)
	if (os->value) {
	  char *signal;
	  asprintf(&signal, os->signal, nn);

	  // Inputs
	  if (p->master != os->master && o != OCP_Clk &&
	      (p->type != WCIPort || o != OCP_MReset_n && o != OCP_SReset_n)) {
	    OcpSignalEnum special[] = {OCP_SThreadBusy,
				       OCP_SReset_n,
				       OCP_MReqLast,
				       OCP_MBurstPrecise,
				       OCP_MReset_n,
				       OCP_SDataThreadBusy,
				       OCP_MDataValid,
				       OCP_MDataLast,
				       OCP_SRespLast,
				       OCP_SCmdAccept,
				       OCP_SDataAccept,
				       N_OCP_SIGNALS};
	    OcpSignalEnum *osn;
	    for (osn = special; *osn != N_OCP_SIGNALS; osn++)
	      if ((OcpSignalEnum)o == *osn)
		break;
	    if (*osn != N_OCP_SIGNALS)
	      fprintf(f, "  method %c%s () enable(%s) clocked_by(i_%sClk) reset_by(%s);\n",
		      tolower(osd->name[0]), osd->name + 1, signal,
		      p->clock->port->name(), reset);
	    else
	      fprintf(f, "  method %c%s (%s) enable((*inhigh*)en%u) clocked_by(i_%sClk) reset_by(%s);\n",
		      tolower(osd->name[0]), osd->name + 1, signal, en++,
		      p->clock->port->name(), reset);
	  }
	  if (p->master == os->master)
	    fprintf(f, "  method %s %c%s clocked_by(i_%sClk) reset_by(%s);\n",
		    signal, tolower(osd->name[0]), osd->name + 1,
		    p->clock->port->name(), reset);
	}
      fprintf(f, "endinterface: i_%s%s\n\n", p->name(), num);
    }
  }
  // warning suppression...
  fprintf(f, "schedule (\n");
  last = "";
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      OcpSignalDesc *osd;
      OcpSignal *os;
      unsigned o;
      for (o = 0, os = p->ocp.signals, osd = ocpSignals; osd->name; osd++, os++, o++)
	if (os->value && o != OCP_Clk &&
	    (p->type != WCIPort ||
	     !(o == OCP_MReset_n && !p->master || o == OCP_SReset_n && p->master))) {
	  fprintf(f, "%si_%s%s_%c%s", last.c_str(), p->name(), num, tolower(osd->name[0]), osd->name+1);
	  last = ", ";
	}
    }
  }
  fprintf(f, ")\n   CF  (\n");
  last = "";
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      OcpSignalDesc *osd;
      OcpSignal *os;
      unsigned o;
      for (o = 0, os = p->ocp.signals, osd = ocpSignals; osd->name; osd++, os++, o++)
	if (os->value && o != OCP_Clk &&
	    (p->type != WCIPort ||
	     !(o == OCP_MReset_n && !p->master || o == OCP_SReset_n && p->master))) {
	  fprintf(f, "%si_%s%s_%c%s", last.c_str(), p->name(), num, tolower(osd->name[0]), osd->name+1);
	  last = ", ";
	}
    }
  }
  fprintf(f, ");\n\n");
  fprintf(f, "\nendmodule: vMk%s\n", m_implName);
  fprintf(f,
	  "// Make a synthesizable Verilog module from our wrapper\n"
	  "(* synthesize *)\n"
	  "(* doc= \"Info about this module\" *)\n"
	  "module mk%s#(", m_implName);
  // Now we must enumerate the various input clocks and input resets as parameters
  last = "";
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->clock->port == p) {
      fprintf(f, "%sClock i_%sClk", last.c_str(), p->name());
      last = ", ";
    }
  }
  // Now we must enumerate the various reset inputs as parameters
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
			       !p->master && p->ocp.MReset_n.value)) {
      if (p->count > 1)
	fprintf(f, "%sVector#(%zu,Reset) i_%sRst", last.c_str(), p->count, p->name());
      else
	fprintf(f, "%sReset i_%sRst", last.c_str(), p->name());
      last = ", ";
    }
  }
  fprintf(f, ") (V%sIfc);\n", m_implName);
  fprintf(f,
	  "  let _ifc <- vMk%s(",
	  m_implName);
  last = "";
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->clock->port == p) {
      fprintf(f, "%si_%sClk", last.c_str(), p->name());
      last = ", ";
    }
  }
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
			       !p->master && p->ocp.MReset_n.value)) {
      fprintf(f, "%si_%sRst", last.c_str(), p->name());
      last = ", ";
    }
  }
  fprintf(f, ");\n"
	  "  return _ifc;\n"
	  "endmodule: mk%s\n\n"
	  "endpackage: I_%s\n",
	  m_implName, m_implName);
#endif
  return 0;
}
