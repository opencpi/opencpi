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

#include <inttypes.h>
#include <assert.h>
#include <cstdio>
#include <climits>

#include "OcpiUtilMisc.h"
#include "hdl.h"
#include "assembly.h"

namespace OU = OCPI::Util;

void
emitSignal(const char *signal, FILE *f, Language lang, Signal::Direction dir,
	   std::string &last, int width, unsigned n, const char *pref,
	   const char *type, const char *value, const char *widthExpr) {
  int pad = 22 - (int)strlen(signal);
  char *name;
  std::string num;
  OU::format(num, "%u", n);
  ocpiCheck(asprintf(&name, signal, num.c_str()) > 0);
  if (lang == VHDL) {
    const char *io =
      dir == Signal::NONE ? "" :
      (dir == Signal::IN || dir == Signal::UNUSED ? "in  " :
       (dir == Signal::OUT ? "out " : "inout "));
    if (last.size())
      fprintf(f, last.c_str(), ";");
    if (width < 0) {
      OU::format(last, "  %s  %s%*s: %s %s%s%s%%s\n",
		 pref, name, pad, "", io, type ? type : "std_logic",
		 value ? " := " : "", value ? value : "");
    } else {
      std::string sw;
      if (!widthExpr)
	OU::format(sw, "%u", width);
      OU::format(last, "  %s  %s%*s: %s std_logic_vector(%s-1 downto 0)%%s\n",
		 pref, name, pad, "", io, widthExpr ? widthExpr : sw.c_str());
    }
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

// This is the encoded bit width when encoded for std_logic_vectors
size_t
rawBitWidth(const OU::ValueType &dt) {
  switch (dt.m_baseType) {
  case OA::OCPI_Bool:
    return 1;
  case OA::OCPI_Enum:
    return OU::bitsForMax(dt.m_nEnums - 1);
  case OA::OCPI_String:
    return (dt.m_stringLength+1) * 8;
  default:
    return dt.m_nBits;
  }
}
// This is the encoded bit width in string form, subject to the data type being finalized or not
static const char *
rawBitWidthCstr(const OU::Member &dt, bool finalized = false) {
  static std::string s;
  switch (dt.m_baseType) {
  case OA::OCPI_Bool:
    return "1";
  case OA::OCPI_Enum:
    return OU::format(s, "%zu", OU::bitsForMax(dt.m_nEnums - 1));
  case OA::OCPI_String:
    return finalized ?
      OU::format(s, "%zu", (dt.m_stringLength+1) * 8) :
      OU::format(s, "(%s_string_length+1)*8", dt.cname());
  default:
    return OU::format(s, "%zu", dt.m_nBits);
  }
}
// This is the width as it lives in the OU::Value object
// which is based on the "run" value of the OCPI_DATA_TYPE macro
// which is in bytes
size_t
rawValueBytes(const OU::ValueType &dt) {
  switch (dt.m_baseType) {
  case OA::OCPI_Bool:
    return sizeof(bool);// must be consistent with the "run" value
  case OA::OCPI_Enum:
    return sizeof(uint32_t);
  case OA::OCPI_String:
    return dt.m_stringLength+1; // this must be special cased in the caller anyway
  default:
    return dt.m_nBits / CHAR_BIT;
  }
}
// Third arg is saying that the type must be appropriate for VHDL to pass
// to verilog
static void
vhdlBaseType(const OU::Member &dt, std::string &s, bool convert, bool finalized) {
  if (convert)
    OU::formatAdd(s, "std_logic_vector((%s)-1 downto 0)", rawBitWidthCstr(dt, finalized));
  else if (dt.m_baseType == OA::OCPI_String)
    OU::formatAdd(s, "string_t"); // (0 to %zu)", dt.m_stringLength);
  else if (dt.m_baseType == OA::OCPI_Enum)
    OU::formatAdd(s, "%s_t", dt.cname());
  else {
    for (const char *cp = OU::baseTypeNames[dt.m_baseType]; *cp; cp++)
      s += (char)tolower(*cp);
    s += "_t";
  }
}
static void
vhdlArrayType(const OU::Property &dt, size_t rank, const size_t */*dims*/, std::string &decl,
	      std::string &type, bool convert, bool finalized) {
  if (convert) {
    OU::format(type, "std_logic_vector(%zu*(%s)-1 downto 0)", dt.m_nItems,
	       rawBitWidthCstr(dt, finalized));
    return;
  }
  // Single dimensional arrays when type is neither enum nor string us built-in array types
  if (rank == 1 && dt.m_baseType != OA::OCPI_String && dt.m_baseType != OA::OCPI_Enum) {
    for (const char *cp = OU::baseTypeNames[dt.m_baseType]; *cp; cp++)
      type += (char)tolower(*cp);
    type += "_array_t";
    return;
  }
  type = dt.m_name + "_array_t";
  //  else if (dt.m_baseType == OA::OCPI_String) {
  //    OU::formatAdd(s,
  //		  "type %%s_t is array(0 to natural range <>) of string_t(0 to %zu)",
  //		  dt.m_stringLength);
  //		  //"type %%s_t is array(0 to %zu) of string_t(0 to %zu)",
  //		  //  		  dims[0] - 1, dt.m_stringLength);
  //    else {
  rank = dt.m_arrayRank + (dt.m_isSequence ? 1 : 0);
  OU::formatAdd(decl, "type %s_array_t is array (", dt.m_name.c_str());
  for (unsigned i = 0; i < rank; i++) {
    std::string asize;
    size_t dim = i >= dt.m_arrayRank ? dt.m_sequenceLength : dt.m_arrayDimensions[i];
    static std::string null;
    std::string expr =
      i >= dt.m_arrayRank ? dt.m_sequenceLengthExpr : dt.m_arrayDimensionsExprs[i];
    if (expr.empty())
      OU::format(asize, "0 to %zu", dim-1);
    else
      asize = "natural range <>";

      //      asize = expr; // for now simply expressions should work;
    // allow parameter values to determine the size of the array
    //      OU::formatAdd(s, "%s0 to %zu", i ? ", " : "", dims[i] - 1);
    OU::formatAdd(decl, "%s%s", i ? ", " : "", asize.c_str());
  }
  decl += ") of ";
  if (dt.m_baseType == OA::OCPI_String) {
    decl += "string_t(0 to ";
    if (dt.m_stringLengthExpr.empty())
      OU::formatAdd(decl, "%zu", dt.m_stringLength);
    else {
      //      fprintf(stderr, "strlenexp:%s\n", dt.m_stringLengthExpr.c_str());
      ocpiCheck("arrays of strings must have fixed stringlength"==0);
    }
    //      s += dt.m_stringLengthExpr; // for now simply expressions should work;
    decl += ")";
  } else
    vhdlBaseType(dt, decl, convert, finalized);
}
// Custom types are for enumerations or string arrays - otherwise built-in types are used.
void
vhdlType(const OU::Property &dt, std::string &decl, std::string &type, bool convert,
	 bool finalized) {
  decl.clear();
  if (!convert && dt.m_baseType == OA::OCPI_Enum) {
    OU::format(decl, "type %s_t is (", dt.m_name.c_str());
    for (const char **ap = dt.m_enums; *ap; ap++)
      OU::formatAdd(decl, "%s%s_e", ap == dt.m_enums ? "" : ", ", *ap);
    decl += ")";
    type = dt.m_name + "_t";
    if (dt.m_isSequence || dt.m_arrayDimensions)
      decl += "; ";
  }
  if (dt.m_isSequence) {
    std::vector<size_t> seqdims(dt.m_arrayRank + 1);
    seqdims[0] = dt.m_sequenceLength;
    for (unsigned n = 0; n < dt.m_arrayRank; n++)
      seqdims[n+1] = dt.m_arrayDimensions[n];
    vhdlArrayType(dt, dt.m_arrayRank+2, &seqdims[0], decl, type, convert, finalized);
  } else if (dt.m_arrayDimensions)
    vhdlArrayType(dt, dt.m_arrayRank, dt.m_arrayDimensions, decl, type, convert, finalized);
  else {
    type.clear();
    vhdlBaseType(dt, type, convert, finalized);
  }
 }
static struct VhdlUnparser : public OU::Unparser {
  const char *m_name;
  bool m_finalized;
  bool
  elementUnparse(const OU::Value &v, std::string &s, unsigned nSeq, bool hex, char comma,
		 bool wrap, const Unparser &up) const {
    if (wrap) s+= '(';
    bool r = Unparser::elementUnparse(v, s, nSeq, hex, comma, false, up);
    if (wrap) s+= ')';
    return r;
  }
  bool
  dimensionUnparse(const OU::Value &v, std::string &s, unsigned nseq, size_t dim,
		   size_t offset, size_t nItems, bool hex, char comma,
		   const Unparser &up) const {
    if (dim + 1 == v.m_vt->m_arrayRank && v.m_vt->m_arrayDimensions[dim] == 1)
      s += "0 => ";
    return Unparser::dimensionUnparse(v, s, nseq, dim, offset, nItems, hex, comma, up);
  }

  // We wrap the basic value in a conversion function, and also suppress
  // the suppression of zeroes...
  bool
  valueUnparse(const OU::Value &v, std::string &s, unsigned nSeq, size_t nArray, bool hex,
	       char comma, bool /*wrap*/, const Unparser &up) const {
    switch (v.m_vt->m_baseType) {
    case OA::OCPI_Enum:
    case OA::OCPI_Bool:
      Unparser::valueUnparse(v, s, nSeq, nArray, hex, comma, false, up);
      break;
    case OA::OCPI_String:
      assert(m_name);
      s += "to_string(";
      {
	std::string temp;
	Unparser::valueUnparse(v, temp, nSeq, nArray, hex, comma, false, up);
	if (temp == "\"\"")
	  s += temp;
	else
	  OU::formatAdd(s, "\"%s\"", temp.c_str());
      }
      if (m_finalized)
	OU::formatAdd(s, ", %zu)", v.m_vt->m_stringLength);
      else
	OU::formatAdd(s, ", %s_string_length)", m_name);
      break;
    default:
      s += "to_";
      for (const char *cp = OU::baseTypeNames[v.m_vt->m_baseType]; *cp; cp++)
	s += (char)tolower(*cp);
      s += '(';
      Unparser::valueUnparse(v, s, nSeq, nArray, hex, comma, false, up);
      s += ')';
    }
    return false;
  }
  bool
  unparseBool(std::string &s, bool val, bool) const {
    s += val ? "btrue" : "bfalse";
    return !val;
  }
  bool
  unparseChar(std::string &s, char val, bool) const {
    if (isprint(val)) {
      s += '\'';
      if (val == '\'')
	s += '\'';
      s += val;
      s += '\'';
    } else
      OU::formatAdd(s, "character'val(%u)", val & 0xff);
    return val == 0;
  }
  bool
  unparseString(std::string &s, const char *val, bool) const {
    if (!val || !*val) {
      s += "\"\"";
      return true;
    }
    for (const char *cp = val; *cp; cp++)
      if (isprint(*cp)) {
	if (*cp == '"')
	  s += '"';
	s += *cp;
      } else {
	ocpiBad("Illegal string character converting to VHDL: 0x%x, replaced by space", *cp & 0xff);
	s += ' ';
      }
    return false;
  }
  bool
  unparseFloat(std::string &s, float val, bool hex) const {
    return unparseDouble(s, val, hex);
  }
  bool
  unparseDouble(std::string &s, double val, bool hex) const {
    size_t len = s.length();
    bool zip = Unparser::unparseDouble(s, val, hex);
    // Make it VHDL friendly by making sure there is a decimal point on the fraction.
    bool dot = false;
    for (const char *cp = s.c_str() + len; *cp; cp++)
      if (*cp == 'e' || *cp == 'E') {
	// An exponent without a decimal point.
	std::string e(cp);
	s.resize(OCPI_SIZE_T_DIFF(cp, s.c_str()));
	s += ".0";
	s += e;
	dot = true;
	break;
      } else if (*cp == '.') {
	dot = true;
	break;
      }
    if (!dot)
      s += ".0";
    return zip;
  }
} vhdlUnparser;

static void
vhdlInnerValue(const std::string &name, const char *pkg, const OU::Value &v, bool finalized,
	       std::string &s) {
  if (v.needsComma())
    s += "(";
#if 1
  if (v.m_vt->m_baseType == OA::OCPI_Enum && pkg) {
    s += pkg;
    s += ".";
  }
#endif
  vhdlUnparser.m_name = name.c_str();
  vhdlUnparser.m_finalized = finalized;
  v.unparse(s, &vhdlUnparser, true);
  if (v.m_vt->m_baseType == OA::OCPI_Enum)
    s += "_e";
  if (v.needsComma())
    s += ")";
}

// Convert the vhdl constant value to a property readback value.
// The constant value is what is provided in the generic
// The readback value is what is fed to the readback module for muxing
// into the control plane output datapath
static void
vhdlConstant2Readback(const OU::Property &pr, const std::string &val, std::string &out) {
  std::string decl, type;
  vhdlType(pr, decl, type, false);
  if (decl.length() && pr.m_baseType == OA::OCPI_Enum)
      OU::format(out, "to_unsigned(%s_t'pos(%s),ulong_t'length)", pr.m_name.c_str(), val.c_str());
  //    else if (pr.m_arrayDimensions)
  //    OU::format(out, "%s_array_t(%s)", OU::baseTypeNames[pr.m_baseType], val.c_str());
  //    else
  //      out = val;
  else
    out = val;
}


// Convert a value in v for passing between vhdl _rv and verilog.
// The "v" string might be a variable name.
static const char*
vhdlConvert(const std::string &name, const OU::ValueType &dt, std::string &v, std::string &s,
	    bool toVerilog = true) {
  if (dt.m_arrayRank) {
    if (toVerilog) {
      if (dt.m_baseType == OA::OCPI_String) {
	// This is pretty hideous
	s += "ocpi.types.slv(";
	if (isalpha(v[0])) {
	  for (size_t i = 0; i < dt.m_arrayDimensions[0]; i++)
	    OU::formatAdd(s, "%s%s(%zu)", i == 0 ? "" : "&", name.c_str(), i);
	  s += ")";
	} else {
	  size_t len = v.length();
	  char last = 0;
	  for (size_t i = 0; i < len; i++) {
	    if (v[i] == ',' && last == ')')
	      v[i] = '&';
	    last = v[i];
	  }
	  OU::formatAdd(s, "%s)", v.c_str());
	}
      } else
	if (isalpha(v[0]))
	  OU::formatAdd(s, "ocpi.types.slv(%s_array_t(%s))", OU::baseTypeNames[dt.m_baseType],
			v.c_str());
	else
	  OU::formatAdd(s, "ocpi.types.slv(%s_array_t'%s)", OU::baseTypeNames[dt.m_baseType],
			v.c_str());
    } else if (dt.m_baseType == OA::OCPI_String)
      OU::formatAdd(s, "%s_array_t(to_%s_t(%s,%zu))", name.c_str(),
		    name.c_str(), v.c_str(), dt.m_stringLength);
    else
      OU::formatAdd(s, "ocpi.types.to_%s_array(%s)",
		    OU::baseTypeNames[dt.m_baseType], v.c_str());
  } else if (dt.m_baseType == OA::OCPI_Enum) {
    if (toVerilog)
      OU::formatAdd(s, "std_logic_vector(to_unsigned(%s_t'pos(%s), %zu))",
		    name.c_str(), v.c_str(), OU::bitsForMax(dt.m_nEnums - 1));
    else
      OU::formatAdd(s, "%s_t'val(to_integer(unsigned(%s)))",
		    name.c_str(), v.c_str());
  } else if (dt.m_baseType == OA::OCPI_Bool || dt.m_baseType == OA::OCPI_String) {
    if (toVerilog)
      OU::formatAdd(s, "from_%s(%s)", OU::baseTypeNames[dt.m_baseType], v.c_str());
    else
      OU::formatAdd(s, "to_%s(%s)", OU::baseTypeNames[dt.m_baseType], v.c_str());
  } else if (toVerilog)
    OU::formatAdd(s, "from_%s(%s)", OU::baseTypeNames[dt.m_baseType], v.c_str());
  else
    OU::formatAdd(s, "%s_t(%s)", OU::baseTypeNames[dt.m_baseType], v.c_str());

  return s.c_str();
}
// Provide a string suitable for initializing a generic
// This will be in our own code, not the user's code, so we can count
// on the visibility of our packages and libraries.
// If param==true, the value is used in a top level generic setting in tools
const char *
vhdlValue(const char *pkg, const std::string &name, const OU::Value &v, std::string &s,
	  bool convert, bool finalized) {
  std::string tmp;
  vhdlInnerValue(name, pkg, v, finalized, tmp);
  if (convert) {
    //    if (v.m_vt->m_baseType == OA::OCPI_Enum)
    //      OU::format(tmp, "%zu", (size_t)v.m_ULong);
    vhdlConvert(name, *v.m_vt, tmp, s);
  } else
    s += tmp;
  return s.c_str();
}

const char*
verilogValue(const OU::Value &v, std::string &s, bool finalized) {
  const OU::ValueType &dt = *v.m_vt;
#if 0
  if (dt.m_baseType == OA::OCPI_String) {
    bool indirect = dt.m_arrayRank || dt.m_isSequence;
    s = "\"";
    for (size_t n = 0; n < dt.m_nItems; n++) {
      const char *cp = indirect ? v.m_pString[n] : v.m_String;
      for (size_t i = 0; i <= dt.m_stringLength; i++) {
	int c = (unsigned char)(cp ? *cp : 0);
	switch(c) {
	case '\n':
	  s += "\\n"; break;
	case '\t':
	  s += "\\t"; break;
	case '\\':
	  s += "\\\\"; break;
	case '"':
	  s += "\\\""; break;
	default:
	  if (isprint(c))
	    s += (char)c;
	  else
	    OU::formatAdd(s, "\\%03u", c);
	}
	if (c)
	  cp++;
      }
    }
    s += "\"";
  } else
#endif
   {
    // Everything else is linear in memory
    // How many bits in the std_logic_vector
    assert(finalized);
    size_t bits = rawBitWidth(dt); // bits in verilog constant
    // How many bytes per scalar value
    size_t bytes = rawValueBytes(dt);
    const uint8_t *data = dt.m_arrayRank || dt.m_isSequence || dt.m_baseType == OA::OCPI_String ?
      v.m_pUChar : &v.m_UChar;
    OU::format(s, "%zu'%c", bits * dt.m_nItems, (bits & 3) ? 'b' : 'h');
    for (size_t n = 0; n < dt.m_nItems; n++, data += bytes) {
      if (dt.m_baseType == OA::OCPI_String && (dt.m_arrayRank || dt.m_isSequence))
	data = (uint8_t*)(v.m_pString && v.m_pString[n] ? v.m_pString[n] : "");
      bool null = false;
      size_t vbytes = OU::roundUp(bits, 8)/8; // use only the byte we need
      for (size_t i = 0; i < vbytes && i * 8 < bits; i++) {
	uint8_t d = data[vbytes-1-i];
	if (bits & 3) // binary
	  for (int nn = (i == 0 ? (int)bits & 7 : 8) - 1; nn >= 0; nn--)
	    s += d & (1 << nn) ? '1' : '0';
	else {
	  if (dt.m_baseType == OA::OCPI_String) {
	    d = data[i];
	    if (null)
	      d = 0;
	    else if (!d)
	      null = true;
	  }
	  if (i != 0 || !(bits & 4))
	    s += "0123456789abcdef"[d >> 4];
	  s += "0123456789abcdef"[d & 0xf];
	}
      }
    }
  }
  return s.c_str();
}
const char *Worker::
hdlValue(const std::string &a_name, const OU::Value &v, std::string &value, bool convert,
	 Language lang, bool finalized) {
  if (lang == NoLanguage)
    lang = m_language;
  return lang == VHDL ?
    vhdlValue(NULL, a_name, v, value, convert, finalized) : verilogValue(v, value, finalized);
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
	if (decl.length() && convert)
	  type = decl;
	if (useDefaults) {
	  if (pr.m_default)
	    vhdlValue(NULL, pr.m_name.c_str(), *pr.m_default, value, convert);
	} else {
	  std::string tmp;
	  OU::format(tmp, "work.%s_constants.%s", m_implName, pr.m_name.c_str());
	  if (convert)
	    vhdlConvert(pr.m_name, pr, tmp, value);
	  else
	    value = tmp;
	}
	emitSignal(pr.m_name.c_str(), f, lang, Signal::IN, last, -1, 0, "  ", type.c_str(),
		   value.empty() ? NULL : value.c_str());
      } else {
#if 0
	  int64_t i64 = 0;
	  if (pr.m_default)
	    switch (pr.m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			\
	    case OA::OCPI_##pretty:					\
	      i64 = *(int64_t*)&pr.m_default->m_##pretty; break;
	    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	    default:;
	    }
	if (pr.m_baseType == OA::OCPI_Bool)
	  fprintf(f, "  parameter [0:0] %s = 1'b%u;\n",
		  pr.m_name.c_str(), (i64 != 0) & 1);
        else if (pr.m_baseType == OA::OCPI_Enum)
	  fprintf(f, "  parameter [%zu:0] %s = %zu'd%zu;\n",
		  OU::bitsForMax(pr.m_nEnums - 1) - 1, pr.m_name.c_str(),
		  OU::bitsForMax(pr.m_nEnums - 1), (size_t)pr.m_default->m_ULong);
	else
	  fprintf(f, "  parameter [%zu:0] %s = %zu'h%llx;\n",
		  pr.m_nBits - 1, pr.m_name.c_str(), pr.m_nBits, (long long)i64);
#else
	std::string value;
	if (useDefaults && pr.m_default)
	  verilogValue(*pr.m_default, value);
	// If there is no default value, then parameters must be specified when instantiated
	fprintf(f, "  parameter [(%s)-1:0] %s%s%s;\n", rawBitWidthCstr(pr), pr.m_name.c_str(),
		value.empty() ? "" : " = ", value.c_str());
#endif
      }
    }
  }
  if (!first && lang == VHDL) {
    emitLastSignal(f, last, lang, true);
    fprintf(f, "  );\n");
  }
}

// Default may be overridden
void Worker::
emitDeviceSignal(FILE *f, Language lang, std::string &last, Signal &s, const char *prefix) {
  std::string name;
  if (s.m_differential) {
    OU::format(name, s.m_pos.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, s.m_direction, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    last += prefix;
    OU::format(name, s.m_neg.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, s.m_direction, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
  } else if (s.m_direction == Signal::INOUT && !s.m_pin) {
    OU::format(name, s.m_in.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, Signal::IN, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    last += prefix;
    OU::format(name, s.m_out.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, Signal::OUT, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    last += prefix;
    OU::format(name, s.m_oe.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, Signal::OUT, last, -1, 0, "", s.m_type);
  } else if (s.m_direction == Signal::OUTIN) {
    assert(!s.m_pin);
    OU::format(name, s.m_in.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, Signal::OUT, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    last += prefix;
    OU::format(name, s.m_out.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, Signal::IN, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
    last += prefix;
    OU::format(name, s.m_oe.c_str(), s.m_name.c_str());
    emitSignal(name.c_str(), f, lang, Signal::IN, last, -1, 0, "", s.m_type);
  }
#if 0
  else if (s.m_pin && s.m_directionExpr.size())
    emitSignal(s.m_name.c_str(), f, lang, Signal::INOUT, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
#endif
  else
    emitSignal(s.m_name.c_str(), f, lang, s.m_direction, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
}

void Worker::
emitDeviceSignalMappings(FILE *f, std::string &last) {
  bool anyExpr = false;
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    ocpiDebug("DeviceSignalMappings %s: %zu %u %d",
	      s.cname(), m_paramConfig->nConfig, s.m_direction, m_type == Container);
    if ((s.m_directionExpr.size() || s.m_widthExpr.size()) && m_type != Container)
      anyExpr = true;
    else
      emitDeviceSignalMapping(f, last, s, "");
  }
  if (anyExpr) {
    fputs("\n", f); // last.c_str(), f);
    fprintf(f,
	    "  %s There are additional signals whose direction or width are parameterized and are not\n"
	    "  %s mapped here. They are defined in comments in the generics.vhd file for each\n"
	    "  %s specific build configuration.  They are inserted below in the copy of this\n"
	    "  %s file that is created in each target directory.\n"
	    "  %s_parameterized_signal_map\n", hdlComment(VHDL), hdlComment(VHDL),
	    hdlComment(VHDL), hdlComment(VHDL), hdlComment(VHDL));
  }
}
// A prefix means for every line
void Worker::
emitDeviceSignals(FILE *f, Language lang, std::string &last) {
  bool anyExpr = false;
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++)
    if (((*si)->m_directionExpr.size() || (*si)->m_widthExpr.size()) && lang == VHDL)
      anyExpr = true;
    else
      emitDeviceSignal(f, lang, last, **si, "");
  if (anyExpr) {
    emitLastSignal(f, last, lang, true);  // we're still not sure there will non-UNUSED signals
    fprintf(f,
	    "  %s There are additional signals whose direction or width are parameterized and are not\n"
	    "  %s defined here. They are defined in comments in the generics.vhd file for each\n"
	    "  %s specific build configuration.  They are inserted below in the copy of this\n"
	    "  %s file that is created in each target directory.\n"
	    "  %s_parameterized_signal_decls\n", hdlComment(lang), hdlComment(lang), hdlComment(lang),
	    hdlComment(lang), hdlComment(lang));
  }
}

// Used in various places:
// 1. In the worker component declaration in the defs file via emitVhdlRecordInterface
// 2. In the verilog defs file directly
// 3. In the signal-to-record wrapper entity
// 4. In the actual worker vhdl entity
void Worker::
emitSignals(FILE *f, Language lang, bool useRecords, bool inPackage, bool inWorker,
	    bool convert) {
  const char *comment = hdlComment(lang);
  std::string init = lang == VHDL ? "  port (\n" : "";
  std::string last = init;
  if (m_type != Container)
    for (auto ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
      Clock &c = **ci;
      if (!c.m_port && !c.m_internal) {
	if (last.empty())
	  fprintf(f,
		  "    %s Clock(s) not associated with one specific port:\n", comment);
	emitSignal(c.signal(), f, lang, c.m_output ? Signal::OUT : Signal::IN, last, -1, 0);
	if (c.m_reset.size())
	  // FIXME: FOR THE INNER WORKER TO HAVE A POSITIVE RESET
	  emitSignal(inWorker && !strcasecmp(c.reset(), "wci_reset_n") ?
		     "wci_reset" : c.reset(),
		     f, lang, Signal::IN, last, -1, 0);
      }
    }
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    emitLastSignal(f, last, lang, false);
    p->emitPortDescription(f, lang);
    // Some ports are basically an array of interfaces.
    if (useRecords && lang == VHDL)
      p->emitRecordSignal(f, last, "", true, inPackage, inWorker);
    else
      p->emitSignals(f, lang, last, inPackage, inWorker, convert);
  }
  if (m_signals.size()) {
    emitLastSignal(f, last, lang, false);
    fprintf(f, "  \n  %s Extra signals not part of any WIP interface:\n", comment);
    emitDeviceSignals(f, lang, last);
  }
  if (last != init) {
    emitLastSignal(f, last, lang, true);
    fprintf(f, ");\n");
  } else if (lang == Verilog)
    fprintf(f, ");\n");
}

// Add to the referenced string the value that is the number of elements of the property
static void prElemsAdd(OU::Property &pr, const std::string &prefix, std::string &s) {
  if (pr.m_isSequence) {
    if (pr.m_sequenceLengthExpr.length())
      s += prefix + "_sequence_length";
    else
      OU::formatAdd(s, "%zu", pr.m_sequenceLength);
    if (pr.m_arrayRank)
      s += "*";
  }
  for (unsigned n = 0; n < pr.m_arrayRank; n++) {
    if (n)
      s += "*";
    if (pr.m_arrayDimensionsExprs[0].length())
      OU::formatAdd(s, "%s_array_dimensions(%u)", prefix.c_str(), n);
    else
      OU::formatAdd(s, "%zu", pr.m_arrayDimensions[n]);
  }
}

// produce the type for a signal or record member declaration
// The cases are:
// Basic type uses the base type name, except:
// -- enum types have a generated enum type
// -- generated enum types are <pname>_t
// Sequence or array types uses the base/builtin array type except
// -- arrays of enum types have a generated array type of the generated enum type
// -- string arrays have a generated array type
// -- generated array types are <pname>_array_t
void Worker::
prType(OU::Property &pr, std::string &type) {
  // Now we will be using built-in base types or built-in array types
  std::string prefix;
  OU::format(prefix, "work.%s_constants.%s", m_implName, pr.m_name.c_str());
  if (!pr.m_arrayRank && !pr.m_isSequence) {
    if (pr.m_baseType == OA::OCPI_Enum)
      type = prefix;
    else
      type = OU::baseTypeNames[pr.m_baseType];
    type += "_t";
    if (pr.m_baseType == OA::OCPI_String) {
      std::string len;
      if (pr.m_stringLengthExpr.length())
	OU::format(len, "%s_string_length", prefix.c_str());
      else
	OU::format(len, "%zu", pr.m_stringLength);
      OU::formatAdd(type, "(0 to %s)", len.c_str());
    }
    return;
  }
  type =
    pr.m_baseType == OA::OCPI_Enum || pr.m_baseType == OA::OCPI_String ? prefix.c_str() :
    OU::baseTypeNames[pr.m_baseType];
  type += "_array_t";
  if (!(pr.m_baseType == OA::OCPI_Enum || pr.m_baseType == OA::OCPI_String) ||
      (pr.m_isSequence && pr.m_sequenceLengthExpr.length()) ||
      (pr.m_arrayRank && pr.m_arrayDimensionsExprs[0].length())) {
    type += "(0 to ";
#if 1
    prElemsAdd(pr, prefix, type);
#else
    if (pr.m_isSequence) {
      if (pr.m_sequenceLengthExpr.length())
	type += prefix + "_sequence_length";
      else
	OU::formatAdd(type, "%zu", pr.m_sequenceLength);
      if (pr.m_arrayRank)
	type += "*";
    }
    for (unsigned n = 0; n < pr.m_arrayRank; n++) {
      if (n)
	type += "*";
      if (pr.m_arrayDimensionsExprs[0].length())
	OU::formatAdd(type, "%s_array_dimensions(%u)", prefix.c_str(), n);
      else
	OU::formatAdd(type, "%zu", pr.m_arrayDimensions[n]);
    }
#endif
    type += "-1)";
  }
}

static char *
tempName(char *&temp, unsigned len, const char *fmt, ...) {
  va_list ap;
  if (temp)
    free(temp);
  temp = NULL; // suppress compiler warning
  char *mytemp;
  va_start(ap, fmt);
  ocpiCheck(vasprintf(&mytemp, fmt, ap) >= 0);
  va_end(ap);
  ocpiCheck(asprintf(&temp, "%-*s", len, mytemp) > 0);
  free(mytemp);
  return temp;
}

void Worker::
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
void Worker::
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
    } else if (pr.m_isParameter)
      // This is redundant with generics on purpose.
      emitVhdlPropMemberData(f, pr, maxPropName);
    if (pr.m_isReadable && !pr.m_isParameter)
      fprintf(f, "    %s : Bool_t;\n", tempName(temp, maxPropName, "%s_read",
						pr.m_name.c_str()));
    free(temp);
  } else if (pr.m_isVolatile || (pr.m_isReadable && !pr.m_isWritable))
    emitVhdlPropMemberData(f, pr, maxPropName);
}

void
emitVhdlLibraries(FILE *f) {
  if ((libraries && libraries[0])
#if 1
      || (mappedLibraries && mappedLibraries[0])
#endif
) {
    bool first = true;
    const char **mp;
    for (mp = mappedLibraries; mp && *mp; mp++, first = false)
      fprintf(f, "%s %s", first ? "library" : ",", *mp);
    for (const char **lp = libraries; lp && *lp; lp++) {
      const char *l = strrchr(*lp, '/');
      for (mp = mappedLibraries; mp && *mp; mp++)
	if (!strcasecmp(*mp, l ? l + 1 : *lp))
	  break;
      if (!mp || !*mp) {
	fprintf(f, "%s %s", first ? "library" : ",", l ? l + 1 : *lp);
	first = false;
      }
    }
    fprintf(f, ";\n");
  }
}

const char *Worker::
emitVhdlPackageConstants(FILE *f) {
  char ops[OU::Worker::OpsLimit + 1 + 1];
  for (unsigned op = 0; op <= OU::Worker::OpsLimit; op++)
    ops[OU::Worker::OpsLimit - op] = '0';
  ops[OU::Worker::OpsLimit+1] = 0;
  if (m_wci)
    for (unsigned op = 0; op <= OU::Worker::OpsLimit; op++)
      ops[OU::Worker::OpsLimit - op] = m_ctl.controlOps & (1u << op) ? '1' : '0';
  if (!m_ctl.nNonRawRunProperties) {
    fprintf(f, "-- no properties for this worker\n");
    //	      "  constant properties : ocpi.wci.properties_t(1 to 0) := "
    //"(others => (0,x\"00000000\",0,0,0,false,false,false,false));\n");
  } else {
    fprintf(f,
	    "  constant properties : ocpi.wci.properties_t(0 to %u) := (\n"
	    "  --#   bits    offset bytes-1 slen seqhdr elems write read    vol   debug\n" ,
	    m_ctl.nNonRawRunProperties - 1);
    unsigned n = 0;
    const char *last = NULL;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      OU::Property &pr = **pi;
      if (!pr.m_isRaw && (!pr.m_isParameter || pr.m_isReadable)) {
#if 1
	std::string nElements;
	std::string prefix;
	if ((pr.m_isSequence && pr.m_sequenceLengthExpr.length()) ||
	    (pr.m_arrayRank && pr.m_arrayDimensionsExprs[0].length()))
	  OU::format(prefix, "work.%s_constants.%s", m_implName, pr.m_name.c_str());
	prElemsAdd(pr, prefix, nElements);
	if (nElements.empty())
	  nElements = "1";
#else
	size_t nElements = 1;
	if (pr->m_arrayRank)
	  nElements *= pr->m_nItems;
	if (pr->m_isSequence)
	  nElements *= pr->m_sequenceLength; // can't be zero
#endif
	fprintf(f, "%s%s%s   %2u => (%2zu, %s_offset, %s_nbytes_1, %s%s, %3zu, %s, %s %s %s %s)",
		last ? ", -- " : "", last ? last : "", last ? "\n" : "", n,
		pr.m_nBits,
		pr.cname(), // offset
		pr.cname(), // nbytes-1 (not including sequence header)
		pr.m_baseType == OA::OCPI_String ? pr.cname() : "",
		pr.m_baseType == OA::OCPI_String ? "_string_length" : "0",
		pr.m_isSequence ? pr.m_align : 0,
		nElements.c_str(),
		pr.m_isWritable ? "true, " : "false,",
		pr.m_isReadable ? "true, " : "false,",
		pr.m_isVolatile ? "true, " : "false,",
		pr.m_isDebug    ? "true"  : "false");
	last = pr.m_name.c_str();
	n++;
      }
    }
    fprintf(f, "  -- %s\n  );\n", last);
  }
  if (!m_noControl)
    fprintf(f,
	    "  constant worker : ocpi.wci.worker_t := "
	    "(work.%s_constants.ocpi_port_%s_MAddr_width, work.%s_constants.ocpi_sizeof_non_raw_properties, \"%s\");\n",
	    m_implName, m_wci->pname(), m_implName, ops);
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
	  "use work.%s_constants.all, work.%s_defs.all;\n"
	  "package %s_worker_defs is\n",
	  m_implName, m_implName, m_implName);
    {
    fprintf(f,"\n"
	    "  -- The following record is for the writable properties of worker \"%s\"\n"
	    "  -- and/or the read strobes of volatile or readonly properties\n"
	    "  -- and/or the constant values of parameter properties (redundant with generics)\n"
	    "  type worker_props_in_t is record\n",
	    m_implName);
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if (!(*pi)->m_isRaw && ((*pi)->m_isWritable || (*pi)->m_isReadable || (*pi)->m_isParameter))
	emitVhdlPropMember(f, **pi, maxPropName, true);
    if (m_ctl.rawProperties)
      fprintf(f, "    %-*s : wci.raw_in_t;\n", maxPropName, "raw");
    fprintf(f,
	    "  end record worker_props_in_t;\n");
  }
  if (m_ctl.nonRawReadbacks || m_ctl.rawReadables) {
    fprintf(f,"\n"
	    "  -- The following record is for the readable properties of worker \"%s\"\n"
	    "  type worker_props_out_t is record\n",
	    m_implName);
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if (!(*pi)->m_isParameter && !(*pi)->m_isRaw && !(*pi)->m_isBuiltin &&
	  ((*pi)->m_isVolatile || ((*pi)->m_isReadable && !(*pi)->m_isWritable)))
	emitVhdlPropMember(f, **pi, maxPropName, false);
    if (m_ctl.rawProperties)
      fprintf(f, "    %-*s : wci.raw_out_t;\n", maxPropName, "raw");
    fprintf(f,
	    "  end record worker_props_out_t;\n");
  }
  if (m_ctl.nonRawReadbacks || m_ctl.rawReadables || m_ctl.builtinReadbacks) {
    fprintf(f,"-- internal props_out combining internal and from-worker\n"
	    "  type internal_props_out_t is record\n");
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if (!(*pi)->m_isParameter && !(*pi)->m_isRaw &&
	  ((*pi)->m_isVolatile || ((*pi)->m_isReadable && !(*pi)->m_isWritable)))
	emitVhdlPropMember(f, **pi, maxPropName, false);
    if (m_ctl.rawProperties)
      fprintf(f, "    %-*s : wci.raw_out_t;\n", maxPropName, "raw");
    fprintf(f,
	    "  end record internal_props_out_t;\n");
  }
  // Generate record types to easily and compactly plumb interface signals internally
  for (unsigned n = 0; n < m_ports.size(); n++)
    m_ports[n]->emitRecordTypes(f);
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
	  "use work.%s_worker_defs.all, work.%s_defs.all, work.%s_constants.all;\n"
	  "entity %s%sworker is\n",
	  m_implName, m_implName, m_implName, version() < 2 ? m_implName : "", version() < 2 ? "_" : "");
  emitParameters(f, VHDL);

  emitSignals(f, VHDL, true, true, true);
  fprintf(f, "\nend entity %s%sworker;\n", version() < 2 ? m_implName : "", version() < 2 ? "_" : "");
  return NULL;
}

const char *Worker::
emitVhdlRecordInterface(FILE *f, bool isEntity) {
  const char *err = NULL;
  const char *compOrEnt = isEntity ? "entity" : "component";
  // Generate record types to easily and compactly plumb interface signals internally
  if (!isEntity)
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitRecordInterface(f, m_implName);
  fprintf(f,
	  "\n%s %s_rv--__\n  is\n", compOrEnt, m_implName);
  emitParameters(f, VHDL);
  emitSignals(f, VHDL, true, true, false);
  fprintf(f,
	  "end %s %s_rv--__\n;\n\n", compOrEnt, m_implName);
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
	    "Library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	    "Library ocpi; use ocpi.all, ocpi.types.all;\n");
    emitVhdlLibraries(f);
    fprintf(f,
	    "\n"
	    "-- Package with constant definitions for instantiating this worker\n"
	    "package %s_constants is\n", m_implName);
    bool first = true;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      OU::Property &p = **pi;
      std::string decl, type;
      // Introduce a constant for string/sequence/array lengths that are parameterized
      if (p.m_baseType == OA::OCPI_String) // && p.m_stringLengthExpr.length())
	fprintf(f, "  constant %s_string_length : natural;\n", p.m_name.c_str());
      if (p.m_isSequence && p.m_sequenceLengthExpr.length())
	fprintf(f, "  constant %s_sequence_length : natural;\n", p.m_name.c_str());
      if (p.m_arrayRank && p.m_arrayDimensionsExprs[0].length())
	fprintf(f, "  constant %s_array_dimensions : dimensions_t(0 to %zu);\n",
		p.m_name.c_str(), p.m_arrayRank-1);
      if (!p.m_isRaw && (!p.m_isParameter || p.m_isReadable)) {
	fprintf(f, "  constant %s_offset : unsigned(31 downto 0);\n", p.m_name.c_str());
	fprintf(f, "  constant %s_nbytes_1 : natural;\n", p.m_name.c_str());
      }
      vhdlType(p, decl, type, false);
      if (decl.length() || p.m_isParameter) {
	if (first) {
	  fprintf(f,
		  " -- Declarations of parameter properties.\n"
		  " -- The actual values used are in the package body,\n"
		  " -- which is generated for each configuration.\n");
	  first = false;
	}
	if (decl.length()) {
	  fprintf(f, "  ");
	  if (!strcasecmp("ocpi_endian", p.m_name.c_str())) {
	    // FIXME: a more general solution to built-in enumeration types
	    fprintf(f, "alias ocpi_endian_t is ocpi.types.endian_t");
#if 0
		    "  alias little_e is ocpi.types.little_e[return ocpi_endian_t];\n"
		    "  alias big_e is ocpi.types.big_e[return ocpi_endian_t];\n"
		    "  alias dynamic_e is ocpi.types.dynamic_e[return ocpi_endian_t]"
#endif
	  } else
	    fputs(decl.c_str(), f);
	  fprintf(f, ";\n");
	}
	if (p.m_isParameter)
	  fprintf(f, "  constant %s : %s;\n", p.m_name.c_str(), type.c_str());
      }
    }
    fprintf(f, "  constant ocpi_sizeof_non_raw_properties: natural;\n");
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitRecordInterfaceConstants(f);
    fprintf(f,
	    "end package %s_constants;\n"
	    "-- Package with definitions for instantiating this worker\n"
	    "Library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	    "Library ocpi; use ocpi.all, ocpi.types.all;\n",
	    m_implName);
    emitVhdlLibraries(f);
    fprintf(f,
	    "use work.%s_constants.all;\n"
	    "package %s_defs is\n", m_implName, m_implName);
    if ((err = emitVhdlRecordInterface(f)))
      return err;
    if (m_outer) {
      fprintf(f, "end package %s_defs;\n", m_implName);
      fclose(f);
      return NULL;
    }
    fprintf(f,
	    "\ncomponent %s--__\n is\n", m_implName);
    emitParameters(f, lang, true, true);
  } else
    fprintf(f,
	    "\n"
	    //	    "`default_nettype none\n" // leave this up to the developer
	    "`ifndef NOT_EMPTY_%s\n"
	    "(* box_type=\"user_black_box\" *)\n"
	    "`endif\n"
	    "module %s//__\n(\n", m_implName, m_implName);
  emitSignals(f, lang, false, true, false, true);
  if (lang == VHDL) {
    fprintf(f,
	    "end component %s--__\n;\n",
	    m_implName);
    fprintf(f, "end package %s_defs;\n", m_implName);
  } else {
    fprintf(f, "`include \"generics.vh\"\n");
#if 0
    // Now we emit parameters in the body.
    emitParameters(f, lang);
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitVerilogPortParameters(f);
#endif
    // Now we emit the declarations (input, output, width) for each module port
    for (auto ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
      Clock &c = **ci;
      if (!c.m_port && !c.m_internal) {
	fprintf(f, "  %s      %s;\n", c.m_output ? "output" : "input", c.signal());
	if (c.m_reset.size())
	  fprintf(f, "  input      %s;\n", c.reset());
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
	std::string name;
	if (s->m_differential) {
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
	} else if (s->m_direction == Signal::INOUT && !s->m_pin) {
	  OU::format(name, s->m_in.c_str(), s->m_name.c_str());
	  if (s->m_width)
	    fprintf(f, "  input [%3zu:0] %s;\n", s->m_width - 1, name.c_str());
	  else
	    fprintf(f, "  input         %s;\n", name.c_str());
	  OU::format(name, s->m_out.c_str(), s->m_name.c_str());
	  if (s->m_width)
	    fprintf(f, "  output [%3zu:0] %s;\n", s->m_width - 1, name.c_str());
	  else
	    fprintf(f, "  output         %s;\n", name.c_str());
	  OU::format(name, s->m_oe.c_str(), s->m_name.c_str());
	  fprintf(f, "  output         %s;\n", name.c_str());
	} else if (s->m_direction == Signal::OUTIN) {
	  OU::format(name, s->m_in.c_str(), s->m_name.c_str());
	  if (s->m_width)
	    fprintf(f, "  output [%3zu:0] %s;\n", s->m_width - 1, name.c_str());
	  else
	    fprintf(f, "  output         %s;\n", name.c_str());
	  OU::format(name, s->m_out.c_str(), s->m_name.c_str());
	  if (s->m_width)
	    fprintf(f, "  input [%3zu:0] %s;\n", s->m_width - 1, name.c_str());
	  else
	    fprintf(f, "  input         %s;\n", name.c_str());
	  OU::format(name, s->m_oe.c_str(), s->m_name.c_str());
	  fprintf(f, "  input         %s;\n", name.c_str());
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

// Some tools may require entity declarations when instantiating workers
const char *Worker::
emitVhdlEnts() {
  const char *err;
  FILE *f;
  if ((err = openOutput(m_implName, m_outDir, "", ENTS, VHD, NULL, f)))
    return err;
  const char *comment = hdlComment(VHDL);
  printgen(f, comment, m_file.c_str());
  fprintf(f,
	  "%s This file contains the VHDL entity declaration for the worker\n"
	  "%s with spec name \"%s\" and implementation name \"%s\".\n"
	  "%s It is needed for instantiating the worker for certain tools.\n",
	  comment, comment, m_specName,
	  m_implName, comment);
  fprintf(f,
          "-- Entity declaration with definitions for instantiating this worker\n"
          "Library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
          "Library ocpi; use ocpi.all, ocpi.types.all;\n");
  emitVhdlLibraries(f);
  fprintf(f,
          "use work.%s_constants.all;\n"
          "use work.%s_defs.all;\n",
          m_implName, m_implName);
  if ((err = emitVhdlRecordInterface(f, true)))
    return err;
  fprintf(f,
          "-- Entity declaration with definitions for instantiating this worker\n"
          "Library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
          "Library ocpi; use ocpi.all, ocpi.types.all;\n");
  emitVhdlLibraries(f);
  fprintf(f,
          "use work.%s_constants.all;\n"
          "use work.%s_defs.all;\n",
          m_implName, m_implName);
  fprintf(f,
    "\nentity %s--__\n is\n", m_implName);
  emitParameters(f, VHDL, true, true);
  emitSignals(f, VHDL, false, true, false, true);
  fprintf(f,
    "end entity %s--__\n;\n",
    m_implName);
  fclose(f);
  return NULL;
}


void Worker::
emitVhdlShell(FILE *f) {
  fprintf(f,
	  "library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;\n"
	  "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n"
	  "architecture rtl of %s_rv--__\n  is\n",
	  m_implName);
  if (!m_wci) {
    // with no control interface we have to directly generate wci_reset and wci_is_operating
    fprintf(f,
	    "begin\n"
	    "  wci_reset <= not wci_Reset_n");
    // For each data interface we aggregate a peer reset.
    for (unsigned i = 0; i < m_ports.size(); i++) {
      Port *p = m_ports[i];
      // We are reset if the control is reset OR any output ports are reset.
      // Input ports may be in reset and that doesn't reset us since we won't see any data
      // anyway.  This asymmetry is necessary to prevent reset deadlock.
      if (p->isData() and p->isDataProducer()) {
	fprintf(f, " or not %s.%s",
		p->typeNameIn.c_str(),
		ocpSignals[p->m_master ? OCP_SReset_n : OCP_MReset_n].name);
      }
    }
    fprintf(f,
	    ";\n"
	    "  wci_is_operating <= not wci_reset;\n");
  } else {
    fprintf(f,
	    "begin\n");
    if (m_ctl.nonRawReadbacks || m_ctl.builtinReadbacks || m_ctl.rawReadables) {
      fprintf(f, "  internal_props_out <=\n    (");
      // Assign all members
      const char *last = "";
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
	OU::Property &p = **pi;
	if (!p.m_isParameter && !p.m_isRaw && (p.m_isVolatile || (p.m_isReadable && !p.m_isWritable))) {
	  if (p.m_isSequence) {
	    fprintf(f, "%s%s_length => props_%s%s_length",
		    last, p.cname(), p.m_isBuiltin ? "builtin_" : "from_worker.", p.cname());
	    last = ",\n     ";
	  }
	  fprintf(f, "%s%s => props_%s%s", last, p.cname(), p.m_isBuiltin ? "builtin_" : "from_worker.",
		  p.cname());
	  last = ",\n     ";
	}
      }
      if (m_ctl.rawProperties)
	fprintf(f, "%sraw => props_from_worker.raw", last);
      fprintf(f, ");\n");
    }

    fprintf(f,
	    "  -- This instantiates the WCI/Control module/entity generated in the *_impl.vhd file\n"
	    "  -- With no user logic at all, this implements writable properties.\n"
	    "  wci : entity work.%s_wci\n"
            "    generic map(ocpi_debug => ocpi_debug, endian => ocpi_endian)\n"
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
	    "             is_big_endian     => wci_is_big_endian,\n"
	    "             abort_control_op  => wci_abort_control_op");
    if (m_scalable)
      fprintf(f,
	      ",\n"
	      "             waiting           => wci_waiting,\n"
	      "             barrier           => wci_barrier");
    if (m_ctl.nonRawReadbacks || m_ctl.rawProperties || m_ctl.builtinReadbacks)
      fprintf(f,
	      ",\n"
	      "             props_from_worker => internal_props_out");
    // if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
      fprintf(f,
	      ",\n"
	      "             props_to_worker   => props_to_worker");
    fprintf(f, ");\n");
  }
  for (unsigned i = 0; i < m_ports.size(); i++)
    m_ports[i]->emitVhdlShell(f, m_wci);
  fprintf(f,
	  "worker : entity work.%s%sworker\n", version() < 2 ? m_implName : "", version() < 2 ? "_" : "");
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
  if (m_type != Container)
    for (auto ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
      Clock *c = *ci;
      if (!c->m_port) {
	fprintf(f, "%s    %s => %s", last.c_str(), c->signal(), c->signal());
	last = ",\n";
	if (c->m_reset.size()) {
	  // FIXME: FOR EXPOSING POSITIVE RESET TO INNER WORKER
	  const char *reset = !strcasecmp(c->reset(), "wci_reset_n") ? "wci_reset" :
	    c->reset();
	  fprintf(f, "%s    %s => %s", last.c_str(), reset, reset);
	}
      }
    }
  for (unsigned i = 0; i < m_ports.size(); i++)
    m_ports[i]->emitVHDLShellPortMap(f, last);
  //  if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
  if (!m_noControl)
    fprintf(f, ",\n    props_in => props_to_worker");
  if (m_ctl.nonRawReadbacks || m_ctl.rawReadables)
    fprintf(f, ",\n    props_out => props_from_worker");
  emitDeviceSignalMappings(f, last);
  fprintf(f, ");\n");
  fprintf(f, "end rtl;\n");
}

void Worker::
emitDeviceSignalMapping(FILE *f, std::string &last, Signal &s, const char *prefix) {
  assert(m_paramConfig);
  std::string name;
  if (s.m_direction == Signal::UNUSED && m_type != Container)
    return;
  if (s.m_differential) {
    OU::format(name, s.m_pos.c_str(), s.m_name.c_str());
    fprintf(f, "%s%s      %s => %s,\n", last.c_str(), prefix, name.c_str(), name.c_str());
    OU::format(name, s.m_neg.c_str(), s.m_name.c_str());
    fprintf(f, "%s      %s => %s", prefix, name.c_str(), name.c_str());
  } else if (!s.m_pin && (s.m_direction == Signal::INOUT || s.m_direction == Signal::OUTIN)) {
    OU::format(name, s.m_in.c_str(), s.m_name.c_str());
    fprintf(f, "%s%s      %s => %s,\n", last.c_str(), prefix, name.c_str(), name.c_str());
    OU::format(name, s.m_out.c_str(), s.m_name.c_str());
    fprintf(f, "%s      %s => %s,\n", prefix, name.c_str(), name.c_str());
    OU::format(name, s.m_oe.c_str(), s.m_name.c_str());
    fprintf(f, "%s      %s => %s", prefix, name.c_str(), name.c_str());
  } else
    fprintf(f, "%s%s      %s => %s", last.c_str(), prefix, s.m_name.c_str(), s.m_name.c_str());
  last = ",\n";
}

void Worker::
emitVhdlSignalWrapper(FILE *f, const char *topinst) {
    fprintf(f,
	    "\n"
	    "-- This is the wrapper entity that does NOT use records for ports\n"
	    "-- It \"wraps\" the _rv entity that DOES use records for ports\n"
	    "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	    "library ocpi; use ocpi.all, ocpi.types.all;\n"
	    "use work.%s_defs.all, work.%s_constants.all;\n",
	    m_implName, m_implName);
    emitVhdlLibraries(f);
    fprintf(f,
	    "entity %s--__\n  is\n", m_implName);
    emitParameters(f, m_language, false, true);
    emitSignals(f, m_language, false, false, false);
    fprintf(f, "end entity %s--__\n;\n", m_implName);
    fprintf(f,
	    "library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;\n"
	    "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n");
    emitVhdlLibraries(f);
    fprintf(f,
	    "architecture rtl of %s--__\nis\n",
	    m_implName);
    // Insert the conversion functions as needed
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if ((*pi)->m_isParameter && (*pi)->m_baseType == OA::OCPI_String && (*pi)->m_arrayRank) {
	std::string alen;
	if ((*pi)->m_arrayDimensionsExprs.size())
	  OU::format(alen, "to_integer(unsigned(%s))-1", (*pi)->m_arrayDimensionsExprs[0].c_str());
	else
	  OU::format(alen, "%zu-1", (*pi)->m_arrayDimensions[0]);
	fprintf(f,
		  "  function to_%s_t(v: std_logic_vector; length : natural) return %s_array_t is\n"
		  "    variable a : %s_array_t(0 to %s);\n"
		  "  begin\n"
		  "    for i in 0 to a'right loop\n"
		  "      for j in 0 to length loop\n"
		  "        a(i)(j) := char_t(v(v'left - (i*(length+1)+j)*8 downto\n"
		  "                            v'left-7 - (i*(length+1)+j)*8));\n"
		  "      end loop;\n"
		  "    end loop;\n"
		  "    return a;\n"
		  "  end to_%s_t;\n",
		(*pi)->m_name.c_str(),
		(*pi)->m_name.c_str(),
		(*pi)->m_name.c_str(),
		alen.c_str(),
		(*pi)->m_name.c_str());
      }
    fprintf(f,
	    "begin\n"
	    "  %s: entity work.%s_rv--__\n",
	    topinst, m_implName);
    bool first = true;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if ((*pi)->m_isParameter) {
	if (first) {
	  fprintf(f,
		  "    generic map(\n");
	}
	// Here we are wrapping the _rv instance, but our generics must be
	// verilog compatible, so we need to convert the verilog values to vhdl
	std::string tmp;
	fprintf(f,  "%s      %s => %s",
		first ? "" : ",\n", (*pi)->m_name.c_str(),
		vhdlConvert((*pi)->m_name, **pi, (*pi)->m_name, tmp, false));
	first = false;
      }
    if (!first)
      fprintf(f, ")\n");
    std::string init = "    port map(\n";
    std::string last = init;
    if (m_type != Container)
      for (auto ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
	Clock *c = *ci;
	if (!c->m_port) {
	  if (last.empty())
	    fprintf(f,
		    "  -- Clock(s) not associated with one specific port:\n");
	  fprintf(f, "%s      %s => %s", last.c_str(), c->signal(), c->signal());
	  last = ",\n";
	  if (c->m_reset.size())
	    fprintf(f, "%s      %s => %s", last.c_str(), c->reset(), c->reset());
	}
      }
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitVHDLSignalWrapperPortMap(f, last);
    emitDeviceSignalMappings(f, last);
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
	    "use work.%s_defs.all, work.%s_constants.all;\n",
	    m_implName, m_implName);
    emitVhdlLibraries(f);
    fprintf(f,
	    "entity %s_rv--__\n  is\n", m_implName);
    emitParameters(f, VHDL, false);
    emitSignals(f, VHDL, true, false, false);
    fprintf(f, "end entity %s_rv--__\n;\n", m_implName);
    fprintf(f,
	    "library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;\n"
	    "library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions\n");
    emitVhdlLibraries(f);
    fprintf(f,
	    "architecture rtl of %s_rv--__\n  is\n", m_implName);
    // Define individual signals to work around isim bug that it can't use indexed records in actuals
    // What a waste of time for a vendor bug
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitVHDLRecordWrapperSignals(f);
    fprintf(f,
	    "begin\n");
    // Assign individual signals to work around isim bug that it can't use indexed records in actuals
    // What a waste of time for a vendor bug
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitVHDLRecordWrapperAssignments(f);
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
	std::string tmp;
	fprintf(f,  "%s      %s => %s",
		first ? "" : ",\n", p.m_name.c_str(),
		vhdlConvert(p.m_name.c_str(), p, p.m_name, tmp));
	//		p.m_baseType == OA::OCPI_Bool ? "ocpi.util.slv(" : "",
	//		p.m_name.c_str(),
	//		p.m_baseType == OA::OCPI_Bool ? ")" : "");
	first = false;
      }
    if (!first)
      fprintf(f, ")");
    if ((m_clocks.size() && m_type != Container) || m_ports.size() || m_signals.size()) {
      fprintf(f, "\n    port map(\n");
      std::string last;
      if (m_type != Container)
	for (auto ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
	  Clock *c = *ci;
	  if (!c->m_port && !c->m_internal) {
	    if (last.empty())
	      fprintf(f,
		      "  -- Clock(s) not associated with one specific port:\n");
	    fprintf(f, "%s      %s => %s", last.c_str(), c->signal(), c->signal());
	    last = ",\n";
	  }
	}
      for (unsigned i = 0; i < m_ports.size(); i++)
	m_ports[i]->emitVHDLRecordWrapperPortMap(f, last);
      emitDeviceSignalMappings(f, last);
      fprintf(f, ")");
    }
    fprintf(f, ";\nend rtl;\n");
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
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
    OU::Property &pr = **pi;
    if (pr.m_isRaw)
      continue;
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
	    "--               |   | <worker>_impl.vhd file. Architecture is in your      |    |\n"
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
	    "use work.%s_worker_defs.all, work.%s_defs.all, work.%s_constants.all;\n",
	    m_implName, m_implName, m_implName);
    emitVhdlLibraries(f);
    fprintf(f,
	    "entity %s_rv--__\n  is\n", m_implName);
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
    for (unsigned n = 0; n < p->count(); n++)
      p->emitImplAliases(f, n, lang);
  }
  if (m_language == VHDL) {
    unsigned n = 0;
    for (unsigned i = 0; i < m_ports.size(); i++)
      m_ports[i]->emitImplSignals(f);
    fprintf(f,
	    "end entity %s_rv--__\n;\n"
	    "\n", m_implName);
    if (m_wci && !m_outer) {
      size_t decodeWidth = m_wci->decodeWidth();
      fprintf(f,
	      "-- Here we define and implement the WCI interface module for this worker,\n"
	      "-- which can be used by the worker implementer to avoid all the OCP/WCI issues\n"
	      "library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	      "library ocpi; use ocpi.all, ocpi.types.all, ocpi.util.all;\n"
	      "use work.%s_worker_defs.all, work.%s_constants.all, work.%s_defs.all;\n"
	      "entity %s_wci is\n"
	      "  generic(ocpi_debug : bool_t; endian : endian_t);\n"
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
	      "    %-*s : out bool_t;            -- shorthand for state==operating_e\n"
	      "    %-*s : out bool_t;            -- for endian-switchable workers\n"
	      "    %-*s : out bool_t%s            -- forcible abort a control-op when\n"
	      "                                                -- worker uses 'done' to delay it\n",
	      m_implName, m_implName, m_implName, m_implName,
	      maxPropName, "inputs", m_wci->typeNameIn.c_str(),
	      maxPropName, "done",
	      maxPropName, "error",
	      maxPropName, "finished",
	      maxPropName, "attention",
	      maxPropName, "outputs", m_wci->typeNameOut.c_str(),
	      maxPropName, "reset",
	      maxPropName, "control_op",
	      maxPropName, "state",
	      maxPropName, "is_operating",
	      maxPropName, "is_big_endian",
	      maxPropName, "abort_control_op",
#if 1
	      ";"
#else
	      !m_scalable && m_ctl.nRunProperties == 0 ? " " : ";"
#endif
	      );
      if (m_scalable)
	fprintf(f,
		"    %-*s : in  bool_t;           -- for scalable workers that do barriers\n"
		"    %-*s : out bool_t;           -- for scalable workers that do barriers\n"
		"    %-*s : out uchar_t;           -- for scalable workers\n"
		"    %-*s : out uchar_t%s           -- for scalable workers\n",
		maxPropName, "waiting",
		maxPropName, "barrier",
		maxPropName, "crew",
		maxPropName, "rank",
#if 1
	        ";"
#else
		m_ctl.nRunProperties == 0 ? " " : ";"
#endif
	      );
      // Record for property-related inputs to the worker - writable values and strobes,
      // readable strobes
      if (m_ctl.nonRawReadbacks || m_ctl.builtinReadbacks || m_ctl.rawReadables)
	fprintf(f, "    props_from_worker  : in  internal_props_out_t;\n");
#if 1
      fprintf(f, "    props_to_worker    : out worker_props_in_t");
#else
      if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
	fprintf(f, "%s    props_to_worker    : out worker_props_in_t",
		m_ctl.nonRawReadbacks || m_ctl.rawReadables ? ";\n" : "");
#endif
      fprintf(f,
	      "\n"
	      ");\n"
	      "end entity;\n");
      fprintf(f,
	      "architecture rtl of %s_wci is\n"
	      //	    "  signal my_clk   : std_logic; -- internal usage of output\n"
	      "  signal my_reset : bool_t; -- internal usage of output\n"
	      "  signal my_big_endian : bool_t;\n",
	      m_implName);
      if (m_ctl.nonRawWritables || m_ctl.nonRawReadables || m_ctl.rawProperties)
	fprintf(f, "  signal raw_to_worker : wci.raw_in_t;\n");
      if (m_ctl.nNonRawRunProperties) {
	unsigned nProps_1 = m_ctl.nNonRawRunProperties - 1;
	fprintf(f,
		"  -- signals for property reads and writes\n"
		"  signal offsets       : "
		"wci.offset_a_t(0 to %u);  -- offsets within each property\n"
		//		"  signal indices       : "
		//		"wci.offset_a_t(0 to %u);  -- array index for array properties\n"
		"  signal hi32          : "
		"bool_t;                 -- high word of 64 bit value\n"
		"  signal nbytes_1      : "
		"types.byte_offset_t;       -- # bytes minus one being read/written\n",
		nProps_1);
	if (m_ctl.nonRawWritables)
	  fprintf(f,
		  "  -- signals between the decoder and the writable property registers\n"
		  "  signal write_enables : "
		  "bool_array_t(0 to %u);\n"
		  "  signal data          : "
		  "wci.data_a_t(0 to %u);   -- data being written, right justified\n",
		  nProps_1, nProps_1);
	if (m_ctl.nonRawReadables) {
	  fprintf(f,
		  "  -- signals between the decoder and the readback mux\n"
		  "  signal read_enables  : bool_array_t(0 to %u);\n"
		  "  signal read_index    : unsigned(ocpi.util.width_for_max(%u)-1 downto 0);\n"
#if 1
		  "  signal readback_data : wci.data_a_t(work.%s_worker_defs.properties'range);\n",
		  nProps_1, nProps_1, m_implName
#else
		  "  signal readback_data : wci.data_a_t(0 to %u);\n",
		  nProps_1, m_ctl.nNonRawRunProperties-1
#endif
		    );
	  fprintf(f,
		  "  -- The output to SData from nonRaw properties\n"
		  "  signal nonRaw_SData  : std_logic_vector(31 downto 0);\n");
	}
	bool first = true;
	for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
	  OU::Property &pr = **pi;
	  if (pr.m_isRaw)
	    continue;
	  std::string type;
	  prType(pr, type);
	  char *temp = NULL;
	  tempName(temp, maxPropName, "%s_value", pr.m_name.c_str());
	  if ((pr.m_isParameter && pr.m_isReadable) ||
	      (!pr.m_isParameter &&
	       ((pr.m_isWritable && pr.m_isReadable && !pr.m_isVolatile) ||
		pr.m_baseType == OA::OCPI_Enum ||
		(pr.m_baseType == OA::OCPI_String && (pr.m_arrayRank || pr.m_isSequence))))) {
	    if (first) {
	      fprintf(f,
		      "  -- internal signals between property registers and the readback mux\n"
		      "  -- for those that are writable, readable, and not volatile\n"
		      "  -- or enumerations or string arrays/sequences\n");
	      first = false;
	    }
	    fprintf(f, "  signal my_%s : ", temp);
	    //	    if (pr.m_baseType == OA::OCPI_String && (pr.m_arrayRank || pr.m_isSequence))
	    //	      fprintf(f, "string_array_t(%s_t'range(0), ");
	    //	    else
	    fprintf(f, "%s;\n", pr.m_baseType == OA::OCPI_Enum ? "ulong_t" : type.c_str());
	  }
	  if (!pr.m_isParameter && pr.m_isWritable && (pr.m_arrayRank || pr.m_isSequence)) {
	    // Writable array/sequence properties need their default values defined in a
	    // constant since the primitives for holding them take an unconstrained array
	    // as a generic argument. FIXME: make this array-to-scalar a method elsewhere
	    OU::ValueType vt(pr.m_baseType);
	    vt.m_stringLength = pr.m_stringLength;
	    const OU::Value def(vt);
	    std::string vv;
	    vhdlValue(NULL, pr.m_name.c_str(), pr.m_default ? *pr.m_default : def, vv, false);
	    fprintf(f,
		    "  -- Constant default value of array/sequence property\n"
		    "  constant my_default_%s : %s := ", temp, type.c_str());
	    if (pr.m_default)
	      fprintf(f, "%s", vv.c_str());
	    else
	      fprintf(f, "(others => %s)", vv.c_str());
	    fprintf(f, ";\n");
	  }
	  if (!pr.m_isParameter && (pr.m_isVolatile || (pr.m_isReadable && !pr.m_isWritable))) {
	    if (pr.m_isSequence) {
	      fprintf(f,
		      "  -- readback value that is the sequence length *OR* a dword of data\n"
		      "  signal my_sequence_data_%s : dword_t;\n", pr.cname());
	      fprintf(f,
		      "  -- offset value that is net of the initial sequence length\n"
		      "  signal my_sequence_offset_%s : "
		      "unsigned(width_for_max(%zu-%zu-1)-1 downto 0);\n",
		      pr.cname(), pr.m_nBytes, pr.m_align);
	    }
	    if (pr.m_baseType == OA::OCPI_Enum) {
	      // We need a separate readback signal for volatile enumerations
	      // This is the signal that will be decoded from the worker's enumeration
	      // type to feed into the readback
	      std::string utype = "ulong_t";
	      if (pr.m_isSequence || pr.m_arrayRank)
		OU::format(utype, "ulong_array_t(0 to %zu-1)",
			   pr.m_nItems * (pr.m_isSequence ? pr.m_sequenceLength : 1));
	      fprintf(f,
		      "  -- signal for conveying the volatile enum value to readback mux\n"
		      "  signal my_volatile_%s : %s;\n", temp, utype.c_str());
	    }
	  }
	  free(temp);
	}
      }
#if 0
      if (m_ctl.rawReadables)
	fprintf(f,
		"  signal my_is_read : bool_t;\n");
#endif
      fprintf(f,
	      "  -- temp signals to workaround isim/fuse crash bug\n"
	      "  signal MFlag   : std_logic_vector(18 downto 0);\n"
	      "  signal wciAddr : std_logic_vector(31 downto 0);\n"
	      "begin\n"
              "  is_big_endian                          <= my_big_endian;\n"
	      "  wciAddr(inputs.MAddr'range)            <= inputs.MAddr;\n"
	      "%s",
	      m_scalable ?
	      "  MFlag                                  <= inputs.MFlag;\n" :
	      "  MFlag                                  <= util.slv0(17) & inputs.MFlag;\n");
      if (m_ctl.rawProperties)
	fprintf(f, "  props_to_worker.raw <= raw_to_worker;\n");
      if (decodeWidth < 32)
	fprintf(f,
              "  wciAddr(31 downto inputs.MAddr'length) <= (others => '0');\n");
      fprintf(f,
	      "  outputs.SFlag(0)                       <= attention;\n"
	      "  outputs.SFlag(1)                       <= %s; -- waiting for barrier\n"
	      "  outputs.SFlag(2)                       <= finished;\n"
	      //	    "  my_clk <= inputs.Clk;\n"
	      "  my_reset                               <= to_bool(inputs.MReset_n = '0');\n"
	      "  reset                                  <= my_reset;\n",
	      m_scalable ? "waiting" : "'0'");
      if (m_ctl.nNonRawRunProperties)
	fprintf(f,
		"  wci_decode : component wci.decoder\n"
		"      generic map(worker               => work.%s_worker_defs.worker,\n"
		"                  ocpi_debug           => ocpi_debug,\n"
		"                  endian               => endian,\n"
		"                  properties           => work.%s_worker_defs.properties)\n",
		m_implName, m_implName);
      else
	fprintf(f,
		"  wci_decode : component wci.no_props_decoder\n"
		"      generic map(worker               => work.%s_worker_defs.worker,\n"
		"                  ocpi_debug           => ocpi_debug,\n"
		"                  endian               => endian)\n",
		m_implName);
      fprintf(f,
	      "      port map(   ocp_in.Clk           => inputs.Clk,\n"
	      "                  ocp_in.Maddr         => wciAddr,\n"
	      "                  ocp_in.MAddrSpace    => %s,\n"
	      "                  ocp_in.MByteEn       => %s,\n"
	      "                  ocp_in.MCmd          => inputs.MCmd,\n"
	      "                  ocp_in.MData         => %s,\n"
	      "                  ocp_in.MFlag         => MFlag,\n"
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
	      "                  is_operating         => is_operating,\n"
	      "                  is_big_endian        => my_big_endian,\n");
      fprintf(f,
	      "                  raw_in               => %s,\n"
	      "                  raw_out              => %s,\n",
	      m_ctl.rawProperties ? "props_from_worker.raw" : "wci.raw_out_zero",
	      m_ctl.rawProperties ? "raw_to_worker"  : "open");
      if (m_scalable)
	fprintf(f,
		"                  barrier              => barrier,\n"
		"                  crew                 => crew,\n"
		"                  rank                 => rank,\n");
      else
	fprintf(f,
		"                  barrier              => open,\n"
		"                  crew                 => open,\n"
		"                  rank                 => open,\n");
      fprintf(f,
	      "                  abort_control_op     => abort_control_op");
#if 0
      if (m_ctl.nRunProperties)
	fprintf(f,
		",\n"
		"                  raw_offset           => %s,\n"
		"                  is_read              => %s,\n"
		"                  is_write             => %s",
		m_ctl.rawProperties ? "props_to_worker.raw_address" : "open",
		m_ctl.rawReadables ? "my_is_read" : "open",
		m_ctl.rawWritables ? "props_to_worker.raw_is_write" : "open");
#endif
      if (m_ctl.nNonRawRunProperties)
	fprintf(f,
		",\n"
		"                  write_enables        => %s,\n"
		"                  read_enables         => %s,\n"
		"                  offsets              => offsets,\n"
		//		"                  indices              => indices,\n"
		"                  hi32                 => hi32,\n"
		"                  nbytes_1             => nbytes_1,\n"
		"                  data_outputs         => %s,\n"
		"                  read_index           => %s",
		m_ctl.nonRawWritables ? "write_enables" : "open",
		m_ctl.nonRawReadables ? "read_enables" : "open",
		m_ctl.nonRawWritables ? "data" : "open",
		m_ctl.nonRawReadables ? "read_index" : "open");
      fprintf(f, ");\n");
      if (m_ctl.nonRawReadables)
	fprintf(f,
		"  readback : component wci.readback\n"
		"    generic map(work.%s_worker_defs.properties, ocpi_debug)\n"
		"    port map(   read_index   => read_index,\n"
		"                data_inputs  => readback_data,\n"
		"                data_output  => nonRaw_SData);\n",
		m_implName);
      if (m_ctl.readables)
	fprintf(f, "  outputs.SData <= %s;\n",
		m_ctl.nonRawReadables ?
		(m_ctl.rawReadables ?
		 "props_from_worker.raw.data when its(raw_to_worker.is_read) else nonRaw_SData" :
		 "nonRaw_SData") :
		(m_ctl.rawReadables ? "props_from_worker.raw.data" : "(others => '0')"));
      n = 0;
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
	OU::Property &pr = **pi;
	if (pr.m_isRaw)
	  continue;
	const char *name = pr.m_name.c_str();
	bool isStringArray = pr.m_baseType == OA::OCPI_String && (pr.m_isSequence || pr.m_arrayRank);
	if (pr.m_isParameter) {
#if 1
	  std::string constValue, val;
	  OU::format(constValue, "work.%s_constants.%s", m_implName, name);
	  fprintf(f, "  props_to_worker.%s <= %s;\n", name, constValue.c_str());
	  if (pr.m_isReadable) {
	    vhdlConstant2Readback(pr, constValue, val);
	    fprintf(f, "  my_%s_value <= %s;\n", name, val.c_str());
	  } else
	    continue;
#else
	  if (pr.m_isReadable) {
	    std::string constValue, val;
	    OU::format(constValue, "work.%s_constants.%s", m_implName, name);
	    vhdlConstant2Readback(pr, constValue, val);
	    fprintf(f, "  my_%s_value <= %s;\n", name, val.c_str());
	  } else
	    continue;
#endif
	} else if (pr.m_isWritable) {
	  if (isStringArray) {
	    size_t len = OU::roundUp(pr.m_stringLength+4, 4);
	    std::string out;
	    OU::format(out, "%s%s%s", pr.m_isReadable ? "my_" : "props_to_worker.",
		       name, pr.m_isReadable ? "_value" : "");
	    fprintf(f,
		    "  -- String arrays require wrapper to convert to the string_array_t\n"
		    "  %s_property_write_wrapper : block\n"
		    "    signal sa_temp : string_array_t(%s_array_t'range, 0 to %zu-1);\n"
		    "    function defcnv return string_array_t is\n"
		    "      variable sa_def : string_array_t(%s_array_t'range, 0 to %zu-1);\n"
		    "    begin\n"
		    "      -- convert default value to the generic string array type\n"
		    "      for i in %s_array_t'range loop\n"
		    "        for j in 0 to %zu-1 loop\n"
		    "          sa_def(i,j) := my_default_%s_value(i)(j);\n"
		    "        end loop;\n"
		    "      end loop;\n"
		    "      return sa_def;\n"
		    "    end function defcnv;\n"
		    "  begin\n"
		    "    -- convert stored string array value to the specific type\n"
		    "    g0: for i in %s_array_t'range generate\n"
		    "      g1: for j in 0 to %zu-1 generate\n"
		    "        %s(i)(j) <= sa_temp(i,j);\n"
		    "      end generate g1;\n"
		    "    end generate g0;\n",
		    name, name, len, name, len, name, pr.m_stringLength+1, name, name,
		    pr.m_stringLength+1, out.c_str());
	  }
	  fprintf(f,
		  "  %s_property : component ocpi.props.%s%s_property\n"
		  "    generic map(worker       => work.%s_worker_defs.worker,\n"
		  "                property     => work.%s_worker_defs.properties(%u)",
		  name, OU::baseTypeNames[pr.m_baseType == OA::OCPI_Enum ?
					  OA::OCPI_ULong : pr.m_baseType],
		  pr.m_arrayRank || pr.m_isSequence ? "_array" : "",
		  m_implName, m_implName, n);
	  if (pr.m_default || pr.m_arrayRank || pr.m_isSequence) {
	    fprintf(f,
		    ",\n"
		    "                default      => ");
	    if (isStringArray)
	      fprintf(f, "defcnv");
	    else if (pr.m_arrayRank || pr.m_isSequence)
	      fprintf(f, "my_default_%s_value", pr.m_name.c_str());
	    else {
	      std::string vv;
	      vhdlValue(NULL, pr.m_name.c_str(), *pr.m_default, vv, false);
	      if (pr.m_baseType == OA::OCPI_Enum)
		fprintf(f, "to_ulong(%s_t'pos(%s))", pr.m_name.c_str(), vv.c_str());
	      else
		fputs(vv.c_str(), f);
	    }
	  }
	  fprintf(f,
		  ")\n"
		  "    port map(   clk          => inputs.Clk,\n"
		  "                reset        => my_reset,\n"
		  "                is_big_endian=> my_big_endian,\n"
		  "                write_enable => write_enables(%u),\n"
		  "                data         => data(%u)(%zu downto 0),\n",
		  n, n,
		  pr.m_nBits >= 32 || pr.m_arrayRank || pr.m_isSequence ?
		  31 : (pr.m_baseType == OA::OCPI_Bool ? 0 : pr.m_nBits-1));
	  //	  if ((pr.m_isSequence || pr.m_arrayRank) && pr.m_baseType != OA::OCPI_String)
	  //	    fprintf(f,
	  //		    "                %s_t(value)    => ", pr.m_name.c_str());
	  //	  else
	    fprintf(f,
		    "                value        => ");
	  if (isStringArray)
	    fprintf(f, "sa_temp,\n");
	  else if ((pr.m_isReadable && !pr.m_isVolatile) || pr.m_baseType == OA::OCPI_Enum)
	    fprintf(f, "my_%s_value, -- for readback and worker\n", name);
	  else
	    fprintf(f, "props_to_worker.%s,\n", name);
	  if (pr.m_isInitial)
	    fprintf(f, "                written      => open");
	  else
	    fprintf(f, "                written      => props_to_worker.%s_written", name);
	  if (pr.m_arrayRank || pr.m_isSequence || pr.m_baseType == OA::OCPI_String)
	    fprintf(f, ",\n"
		    "                offset        => offsets(%u)(width_for_max(work.%s_worker_defs.properties(%u).bytes_1)-1 downto 0)",
		    n, m_implName, n);
	  if (pr.m_arrayRank || pr.m_isSequence) {
#if 0
	    fprintf(f,
		    "                index        => indices(%u)(%zu downto 0),\n",
		    n, decodeWidth-1);
#endif
	    if (pr.m_isInitial)
	      fprintf(f, ",\n"
		      "                any_written  => open");
	    else
	      fprintf(f, ",\n"
		      "                any_written  => props_to_worker.%s_any_written", name);
	    if (pr.m_baseType != OA::OCPI_String &&
		pr.m_nBits != 64)
	      fprintf(f, ",\n"
		      "                nbytes_1     => nbytes_1");
	  }
	  if (pr.m_nBits == 64)
	    fprintf(f, ",\n"
		    "                hi32         => hi32");
#if 0
	  if (pr.m_baseType == OA::OCPI_String)
	    fprintf(f, ",\n"
		    "                offset        => offsets(%u)(%zu downto 0)",
		    n, decodeWidth-1);
#endif
	  fprintf(f, ");\n");
	  if (isStringArray)
	    // String arrays require a wrapper to convert to the generic string_array_t
	    fprintf(f, "  end block; -- end of wrapper for writable string_array conversion\n");
	  if (pr.m_baseType == OA::OCPI_Enum) {
	    size_t bits = OU::bitsForMax(pr.m_nEnums - 1);
	    fprintf(f,
		    "  -- work around isim 14.6 bug since this did not work:\n"
		    "  -- work.%%s_constants.%%s_t'val(to_integer(my_%%s_value));\n"
		    // "  with to_integer(my_%s_value) select props_to_worker.%s <= \n",
		    "  with my_%s_value(%zu-1 downto 0) select props_to_worker.%s <= \n",
		    name, bits, name);
	    for (unsigned nn = 0; nn < pr.m_nEnums; nn++) {
	      std::string val;
	      for (unsigned b = 0; b < bits; b++)
		val += nn & (1u << (bits-1-b)) ? "1" : "0";
	      fprintf(f, "    %s_e when \"%s\",\n", pr.m_enums[nn], val.c_str());
	    }
	    fprintf(f, "    %s_e when others;\n", pr.m_enums[0]);
	  } else if (pr.m_isReadable && !pr.m_isVolatile)
	    fprintf(f, "  props_to_worker.%s <= my_%s_value;\n", name, name);
	}
	if (pr.m_isReadable) {
	  std::string var; // the value fed into the readback
	  if (pr.m_baseType == OA::OCPI_Enum && !pr.m_isParameter &&
	      (pr.m_isVolatile || !pr.m_isWritable))
	    OU::format(var, "my_volatile_%s_value", name);
	  else if (pr.m_isParameter || (!pr.m_isVolatile && pr.m_isWritable))
	    OU::format(var, "my_%s_value", name);
	  else if (pr.m_isVolatile || !pr.m_isWritable)
	    OU::format(var, "props_from_worker.%s", name);
	  if (isStringArray)
	    fprintf(f,
		    "  -- String arrays require wrapper to convert to the generic string_array_t\n"
		    "  %s_property_read_wrapper : block\n"
		    "    signal sa_temp : string_array_t(%s_array_t'range, 0 to %zu-1);\n"
		    "  begin\n"
		    "    -- convert stored string array value to the specific type\n"
		    "    g0: for i in %s_array_t'range generate\n"
		    "      g1: for j in 0 to %zu-1 generate\n"
		    "        sa_temp(i,j) <= %s(i)(j);\n"
		    "      end generate g1;\n"
		    "    end generate g0;\n",
		    name, name, OU::roundUp(pr.m_stringLength+1, 4), name, pr.m_stringLength+1,
		    var.c_str());
	  fprintf(f,
		  "  %s_readback : component ocpi.props.%s_read%s_property\n"
		  "    generic map(worker       => work.%s_worker_defs.worker,\n"
		  "                property     => work.%s_worker_defs.properties(%u))\n"
		  "    port map(",
		  pr.m_name.c_str(),
		  OU::baseTypeNames[pr.m_baseType == OA::OCPI_Enum ?
				    OA::OCPI_ULong : pr.m_baseType],
		  pr.m_arrayRank || pr.m_isSequence ? "_array" : "",
		  m_implName, m_implName, n);
	  fprintf(f, "   value        => ");
	  if (isStringArray)
	    fprintf(f, "sa_temp,\n");
	  else if (pr.m_isSequence || pr.m_arrayRank)
	    fprintf(f, "%s_array_t(%s),\n",
		      OU::baseTypeNames[pr.m_baseType == OA::OCPI_Enum ?
					OA::OCPI_ULong : pr.m_baseType],
		    var.c_str());
	  else
	    fprintf(f, "%s,\n", var.c_str());
	  fprintf(f,   "                is_big_endian=> my_big_endian,\n");
	  if ((pr.m_isVolatile || !pr.m_isWritable) && pr.m_isSequence)
	    fprintf(f,
		    "                data_out     => my_sequence_data_%s,\n"
		    "                offset       => my_sequence_offset_%s",
		    pr.cname(), pr.cname());
	  else {
	    fprintf(f,   "                data_out     => readback_data(%u)", n);
	    if (pr.m_arrayRank || pr.m_isSequence || pr.m_baseType == OA::OCPI_String)
	      fprintf(f, ",\n"
		      "                offset        => offsets(%u)"
		      "(width_for_max(work.%s_worker_defs.properties(%u).bytes_1)-1 downto 0)",
		      n, m_implName, n);
	  }
	  if (pr.m_nBits == 64)
	    fprintf(f, ",\n"
		    "                hi32       => hi32");
	  else if ((pr.m_arrayRank || pr.m_isSequence) && pr.m_baseType != OA::OCPI_String)
	    fprintf(f, ",\n"
		    "                nbytes_1     => nbytes_1");
	  // provide read enable to suppress out-of-bound reads
	  if (pr.m_baseType == OA::OCPI_String)
	    fprintf(f, ",\n                read_enable  => read_enables(%u));\n", n);
	  else
	    fprintf(f, ");\n");
	  if (!pr.m_isParameter)
	    fprintf(f, "  props_to_worker.%s_read <= read_enables(%u);\n", pr.m_name.c_str(), n);
	  if (!pr.m_isParameter && (pr.m_isVolatile || !pr.m_isWritable)) {
	    if (pr.m_isSequence) {
	      fprintf(f,
		      "  my_sequence_offset_%s <= (others => '0')\n"
		      "    when offsets(%u)(width_for_max(%zu-1)-1 downto 0) = 0\n"
		      "    else resize(offsets(%u)(width_for_max(%zu-1)-1 downto 0) - %zu,\n"
		      "                width_for_max(%zu-%zu-1));\n"
		      "  readback_data(%u) <=\n"
		      "    slv(props_from_worker.%s_length)\n"
		      "    when offsets(%u)(width_for_max(%zu-1)-1 downto 0) = 0\n"
		      "    else my_sequence_data_%s;\n",
		      pr.cname(), n, pr.m_nBytes, n, pr.m_nBytes, pr.m_align, pr.m_nBytes,
		      pr.m_align, n, pr.cname(), n, pr.m_nBytes, pr.cname());
	      if (pr.m_baseType == OA::OCPI_Enum) {
		fprintf(f,
			"gen_volatile_%s:\n"
			"  for i in 0 to %zu-1 generate\n",
			name, pr.m_sequenceLength);
#if 0 // this is what is sensible, but does not work on isim
	      fprintf(f,
		      "    my_volatile_%s_value(i) <=\n"
		      "      to_ulong(work.%s_constants.%s_t'pos(props_from_worker.%s(i)));\n",
		      name, m_implName, name, name);
#else
	      fprintf(f,
		      "    with props_from_worker.%s(i) select my_volatile_%s_value(i) <=\n",
		      name, name);
	      for (unsigned nn = 0; nn < pr.m_nEnums; nn++)
		fprintf(f, "   to_ulong(%u)  when %s_e,\n", nn, pr.m_enums[nn]);
	      fprintf(f, "   to_ulong(0) when others;\n");
#endif
	      fprintf(f, "  end generate gen_volatile_%s;\n", name);
	      }
	    } else if (pr.m_baseType == OA::OCPI_Enum)
	      fprintf(f, "  my_volatile_%s_value    <= "
		      "to_ulong(work.%s_constants.%s_t'pos(props_from_worker.%s));\n",
		      name, m_implName, name, name);
	  }
	  if (isStringArray)
	    // String arrays require a wrapper to convert to the generic string_array_t
	    fprintf(f, "end block; -- end of wrapper for readable string_array conversion\n");
	} else if (m_ctl.nonRawReadables)
	  fprintf(f, "  readback_data(%u) <= (others => '0');\n", n);
	n++;
      }
#if 0
      // Tieoff all readback paths for raw properties
      if (m_ctl.nonRawReadables && m_ctl.rawProperties) {
	bool raw = false;
	n = 0;
	for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
	  if ((*pi) == m_ctl.firstRaw)
	    raw = true;
	  if (!(*pi)->m_isParameter) {
	    if (raw)
	      fprintf(f, "  readback_data(%u) <= (others => '0');\n", n);
	    n++;
	  }
	}
      }
#endif
      fprintf(f,
	      "end architecture rtl;\n");
    }
    if (!m_outer)
      emitVhdlShell(f);
    emitVhdlSignalWrapper(f); // can we avoid this?
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
	      "architecture rtl of %s%sworker is\n"
	      "begin\n",
	      m_version < 2 ? m_implName : "", m_version < 2 ? "_" : "");
      for (unsigned i = 0; i < m_ports.size(); i++)
	m_ports[i]->emitSkelSignals(f);
      if (m_signals.size())
	for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
	  Signal &s = **si;
	  const char *val = s.m_width ? "(others => '0')" : "'0'";
	  std::string name;
	  if (s.m_direction == Signal::OUT)
	    if (s.m_differential) {
	      OU::format(name, s.m_pos.c_str(), s.m_name.c_str());
	      fprintf(f, "  %s <= %s;\n", name.c_str(), val);
	      OU::format(name, s.m_neg.c_str(), s.m_name.c_str());
	      fprintf(f, "  %s <= %s;\n", name.c_str(), val);
	    } else
	      fprintf(f, "  %s <= %s;\n", s.m_name.c_str(), val);
	  else if (s.m_direction == Signal::INOUT) {
	      OU::format(name, s.m_out.c_str(), s.m_name.c_str());
	      fprintf(f, "  %s <= %s;\n", name.c_str(), val);
	      OU::format(name, s.m_oe.c_str(), s.m_name.c_str());
	      fprintf(f, "  %s <= '0';\n", name.c_str());
	  } else if (s.m_direction == Signal::OUTIN) {
	      OU::format(name, s.m_in.c_str(), s.m_name.c_str());
	      fprintf(f, "  %s <= %s;\n", name.c_str(), val);
	  }
	}
      fprintf(f,
	      "end rtl;\n");
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
      fprintf(f, "// For worker interface named \"%s\"", p->cname());
      num = "";
    } else
      fprintf(f, "// For worker interfaces named \"%s0\" to \"%s%zu\"",
	      p->cname(), p->cname(), p->count - 1);
    fprintf(f, " WIP Attributes are:\n");
    switch (p->m_type) {
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
      switch (p->m_type) {
      case WCIPort:
	fprintf(f, "typedef Wci_Es#(%zu) ", p->ocp.MAddr.width);
	break;
      case WSIPort:
	fprintf(f, "typedef Wsi_E%c#(%zu,%zu,%zu,%zu,%zu) ",
		p->m_master ? 'm' : 's',
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
      fprintf(f, "I_%s%s;\n", p->cname(), num);
    }
  }
  fprintf(f,
	  "\n// Define the wrapper module around the real verilog module \"%s\"\n"
	  "interface V%sIfc;\n"
	  "  // First define the various clocks so they can be used in BSV across the OCP interfaces\n",
	  m_implName, m_implName);
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      fprintf(f, "  interface I_%s%s i_%s%s;\n", p->cname(), num, p->cname(), num);
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
      fprintf(f, "%sClock i_%sClk", last.c_str(), p->cname());
      last = ", ";
    }
  }
  // Now we must enumerate the various reset inputs as parameters
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->m_type == WCIPort && (p->m_master && p->ocp.SReset_n.value ||
			       !p->m_master && p->ocp.MReset_n.value)) {
      if (p->count > 1)
	fprintf(f, "%sVector#(%zu,Reset) i_%sRst", last.c_str(), p->count, p->cname());
      else
	fprintf(f, "%sReset i_%sRst", last.c_str(), p->cname());
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
	      p->cname(), p->clock->signal, p->cname());
    else
      fprintf(f, "  // Interface \"%s\" uses clock on interface \"%s\"\n", p->cname(), p->clock->port->cname());
  }
  fprintf(f, "\n  // Reset inputs for worker interfaces that have one\n");
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      if (p->m_type == WCIPort && (p->m_master && p->ocp.SReset_n.value ||
				 !p->m_master && p->ocp.MReset_n.value)) {
	const char *signal;
	asprintf((char **)&signal,
		 p->m_master ? p->ocp.SReset_n.signal : p->ocp.MReset_n.signal, nn);
	if (p->count > 1)
	  fprintf(f, "  input_reset  i_%s%sRst(%s) = i_%sRst[%u];\n",
		  p->cname(), num, signal, p->cname(), nn);
	else
	  fprintf(f, "  input_reset  i_%sRst(%s) = i_%sRst;\n",
		  p->cname(), signal, p->cname());
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
      fprintf(f, "interface I_%s%s i_%s%s;\n", p->cname(), num, p->cname(), num);
      OcpSignalDesc *osd;
      OcpSignal *os;
      unsigned o;
      const char *reset;
      if (p->m_type == WCIPort && (p->m_master && p->ocp.SReset_n.value ||
				 !p->m_master && p->ocp.MReset_n.value)) {
	asprintf((char **)&reset, "i_%s%sRst", p->cname(), num);
      } else
	reset = "no_reset";
      for (o = 0, os = p->ocp.signals, osd = ocpSignals; osd->name; osd++, os++, o++)
	if (os->value) {
	  char *signal;
	  asprintf(&signal, os->signal, nn);

	  // Inputs
	  if (p->m_master != os->m_master && o != OCP_Clk &&
	      (p->m_type != WCIPort || o != OCP_MReset_n && o != OCP_SReset_n)) {
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
		      p->clock->port->cname(), reset);
	    else
	      fprintf(f, "  method %c%s (%s) enable((*inhigh*)en%u) clocked_by(i_%sClk) reset_by(%s);\n",
		      tolower(osd->name[0]), osd->name + 1, signal, en++,
		      p->clock->port->cname(), reset);
	  }
	  if (p->m_master == os->m_master)
	    fprintf(f, "  method %s %c%s clocked_by(i_%sClk) reset_by(%s);\n",
		    signal, tolower(osd->name[0]), osd->name + 1,
		    p->clock->port->cname(), reset);
	}
      fprintf(f, "endinterface: i_%s%s\n\n", p->cname(), num);
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
	    (p->m_type != WCIPort ||
	     !(o == OCP_MReset_n && !p->m_master || o == OCP_SReset_n && p->m_master))) {
	  fprintf(f, "%si_%s%s_%c%s", last.c_str(), p->cname(), num, tolower(osd->name[0]), osd->name+1);
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
	    (p->m_type != WCIPort ||
	     !(o == OCP_MReset_n && !p->m_master || o == OCP_SReset_n && p->m_master))) {
	  fprintf(f, "%si_%s%s_%c%s", last.c_str(), p->cname(), num, tolower(osd->name[0]), osd->name+1);
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
      fprintf(f, "%sClock i_%sClk", last.c_str(), p->cname());
      last = ", ";
    }
  }
  // Now we must enumerate the various reset inputs as parameters
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->m_type == WCIPort && (p->m_master && p->ocp.SReset_n.value ||
			       !p->m_master && p->ocp.MReset_n.value)) {
      if (p->count > 1)
	fprintf(f, "%sVector#(%zu,Reset) i_%sRst", last.c_str(), p->count, p->cname());
      else
	fprintf(f, "%sReset i_%sRst", last.c_str(), p->cname());
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
      fprintf(f, "%si_%sClk", last.c_str(), p->cname());
      last = ", ";
    }
  }
  for (n = 0; n < m_ports.size(); n++) {
    Port *p = m_ports[n];
    if (p->m_type == WCIPort && (p->m_master && p->ocp.SReset_n.value ||
			       !p->m_master && p->ocp.MReset_n.value)) {
      fprintf(f, "%si_%sRst", last.c_str(), p->cname());
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
