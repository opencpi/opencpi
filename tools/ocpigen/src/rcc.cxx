#include <limits.h>
#include "assembly.h"
#include "rcc.h"

namespace OU = OCPI::Util;


// Generate the readonly implementation file.
// What implementations must explicitly (verilog) or implicitly (VHDL) include.
#define RCC_C_HEADER ".h"
#define RCC_C_IMPL "_Worker"
#define RCC_CC_HEADER ".hh"
#define RCC_CC_IMPL "-worker"
#define RCCMAP "_map"

static char *
upperdup(const char *s) {
  char *upper = (char *)malloc(strlen(s) + 1);
  for (char *u = upper; (*u++ = (char)toupper(*s)); s++)
    ;
  return upper;
}

static const char *rccTypes[] = {"none",
				 "RCCBoolean", "RCCChar", "RCCDouble", "RCCFloat", "int16_t", "int32_t", "uint8_t",
				 "uint32_t", "uint16_t", "int64_t", "uint64_t", "RCCChar" };

static void camel(std::string &s, const char *s1, const char *s2 = NULL, const char *s3 = NULL) {
  if (s1) {
    s = (char)toupper(s1[0]);
    s += s1 + 1;
  }
  if (s2) {
    s += (char)toupper(s2[0]);
    s += s2 + 1;
  }
  if (s3) {
    s += (char)toupper(s3[0]);
    s += s3 + 1;
  }
}
static void
upperconstant(std::string &s, const char *s1, const char *s2 = NULL, const char *s3 = NULL) {
  if (s1) {
    for (const char *cp = s1; *cp; cp++)
      s += (char)toupper(*cp);
  }
  if (s2) {
    if (s1)
      s += "_";
    upperconstant(s, s2, s3);
  }
}

static void
printRccArray(FILE *f, OU::Member &m, bool isFixed, bool &isLast, bool topSeq, bool end) {
  if (m.m_arrayRank)
    for (unsigned n = 0; n < m.m_arrayRank; n++)
      fprintf(f, "[%zu]", m.m_arrayDimensions[n]);
  if (topSeq) {
    if (m.m_sequenceLength)
      fprintf(f, "[%zu]", m.m_sequenceLength);
    else
      fprintf(f, "[1]");
  }
  // We align strings on a 4 byte boundary, and implicitly pad them to a 4 byte boundary too
  if (m.m_baseType == OA::OCPI_String) {
    fprintf(f, "[%zu]", isFixed ? (m.m_stringLength + 4) & ~3 : 4);
    if (m.m_stringLength == 0)
      isLast = true;
  }
  if (end) {
    // End of declarator. If we're a sequence we close off the struct.
    fprintf(f, "; /* %8zxx", m.m_offset); 
    if (topSeq)
      fprintf(f, " this is a top level sequence of fixed size elements");
    fprintf(f, " */\n");
  }
}
// Just print the data type, not the "member", with names or arrays etc.
void Worker::
printRccBaseType(FILE *f, OU::Member &m, unsigned level, size_t &offset, unsigned &pad,
		 const char *parent, bool isFixed, bool &isLast, unsigned predefine) {
  if (level > m_maxLevel)
    m_maxLevel = level;
  int indent = (level + 1) * 2;
  if (m.m_baseType == OA::OCPI_Struct) {
    std::string s;
    camel(s, parent, m.m_name.c_str());
    if (level == predefine || predefine == UINT_MAX-1) {
      fprintf(f, "%*sstruct %s {\n", indent, "", s.c_str());
      emitRccStruct(f, m.m_nMembers, m.m_members, level + 1, s.c_str(), isFixed, isLast,
		    false, predefine);
      fprintf(f, "%*s}%s", indent, "", predefine == UINT_MAX-1 ? "" : ";\n");
    } else if (level > predefine)
      fprintf(f, "%*sstruct %s", indent, "", s.c_str());
    else
      emitRccStruct(f, m.m_nMembers, m.m_members, level + 1, s.c_str(), isFixed, isLast, false,
		    predefine);
  } else if (m.m_baseType == OA::OCPI_Type)
    printRccType(f, *m.m_type, level + 1, offset, pad, parent, isFixed, isLast, false, predefine);
  else if (m.m_baseType == OA::OCPI_Enum) {
    if (!strcasecmp("ocpi_endian", m.m_name.c_str())) {
      fprintf(f, "%*s%sRCCEndian", indent, "", m_language == CC ? "OCPI::RCC::" : "");
    } else if (level == predefine || predefine == UINT_MAX-1) {
      fprintf(f, "%*senum %c%s {\n", indent, "", toupper(*m.m_name.c_str()), m.m_name.c_str()+1);
      for (const char **ep = m.m_enums; *ep; ep++) {
	std::string s;
	upperconstant(s, parent, m.m_name.c_str(), *ep);
	fprintf(f, "%*s  %s,\n", indent, "", s.c_str());
      }
      std::string s;
      upperconstant(s, parent, m.m_name.c_str(), "PAD_ = 0x7fffffff");
      fprintf(f, "%*s  %s\n%*s}%s", indent, "", s.c_str(), indent, "",
	      predefine == UINT_MAX-1 ? "" : ";\n");
    } else if (level > predefine)
      fprintf(f, "%*senum %c%s", indent, "", toupper(*m.m_name.c_str()), m.m_name.c_str()+1);
  } else if (level > predefine || predefine == UINT_MAX-1) {
    const char *baseType = m_baseTypes[m.m_baseType];
    if (m_language == C)
      fprintf(f, "%*s%-*s", indent, "", indent ? 13 : 0, baseType);
    else {
      std::string type = !strncmp("RCC", baseType, 3) ? "OCPI::RCC::" : "";
      type += baseType;
      fprintf(f, "%*s%-*s", indent, "", indent ? 24 : 0, type.c_str());
    }
  }
}
static void
topTypeName(std::string &name, OU::Member *m, const char **baseTypes) {
  if (m->m_baseType == OA::OCPI_Struct)
    camel(name, m->m_name.c_str());
  else if (m->m_baseType == OA::OCPI_Type)
    name = "OCPI_Type";
  else
    name = baseTypes[m->m_baseType];
  //  printf("BASETYPE %u:%s\n", m->m_baseType, rccTypes[m->m_baseType]);
}
// Print type for a member, including sequence type etc.
// Note this is called for two different purposes:
// 1. When a higher level structure is being defined (like Properties) and thus
//    the type will be immediately and anonymously used as the type of a struct member
// 2. When a type for a parameter is being defined in advance of its value being defined.
void Worker::
printRccType(FILE *f, OU::Member &m, unsigned level, size_t &offset, unsigned &pad,
	     const char *parent, bool isFixed, bool &isLast, bool topSeq, unsigned predefine) {
  int indent = (level + 1) * 2;
  if (m.m_isSequence && !topSeq) {
    fprintf(f,
	    "%*sstruct {\n"
	    "%*s  uint32_t length;\n",
	    indent, "", indent, "");
    //    offset += 4;
    if (m.m_dataAlign > sizeof(uint32_t)) {
      size_t align = m.m_dataAlign - (unsigned)sizeof(uint32_t);
      fprintf(f, "%*s  char pad%u_[%zu];\n", indent, "", pad++, align);
      //      offset += align;
    }
    printRccBaseType(f, m, level + 1, offset, pad, parent, isFixed, isLast, predefine);
    fprintf(f, " data");
    if (m.m_sequenceLength && isFixed)
      fprintf(f, "[%zu]", m.m_sequenceLength);
    else {
      fprintf(f, "[]");
      isLast = true;
    }
    printRccArray(f, m, isFixed, isLast, false, true);
    fprintf(f, "%*s}", indent, "");
  } else
    printRccBaseType(f, m, level, offset, pad, parent, isFixed, isLast, predefine);
}
// FIXME: a tool-time member class should have this...OCPI::Tools::RCC::Member...
// Returns true when something is variable length.
// strings or sequences are like that unless then are bounded.
void Worker::
printRccMember(FILE *f, OU::Member &m, unsigned level, size_t &offset, unsigned &pad,
	       const char *parent, bool isFixed, bool &isLast, bool topSeq, unsigned predefine)
{
  int indent = (level + 1) * 2;
  if (level > predefine || predefine == UINT_MAX-1) {
    if (offset < m.m_offset) {
      fprintf(f, "%*schar pad%u_[%zu];\n",
	      indent, "", pad++, m.m_offset - offset);
      offset = m.m_offset;
    }
    printRccType(f, m, level, offset, pad, parent, isFixed, isLast, topSeq, predefine);
    //  printf("1name %s offset %u m->m_offset %u bytes %u\n",
    //	 m->m_name.c_str(), offset, m->m_offset, m->m_nBytes);
    fprintf(f, " %s", m.m_name.c_str());
    printRccArray(f, m, isFixed, isLast, topSeq, true);
    offset += m.m_nBytes;
    //  printf("2name %s offset %u m->m_offset %u bytes %u\n",
    //	 m->m_name.c_str(), offset, m->m_offset, m->m_nBytes);
  } else
    printRccType(f, m, level, offset, pad, parent, isFixed, isLast, topSeq, predefine);
}

const char *Worker::
rccMethodName(const char *method, const char *&mName) {
  const char *pat = m_pattern ? m_pattern : m_staticPattern;
  if (!pat) {
    mName = method;
    return 0;
  }
  size_t length =
    strlen(m_implName) + strlen(method) + strlen(pat) + 1;
  char c, *s = (char *)malloc(length);
  mName = s;
  while ((c = *pat++)) {
    if (c != '%')
      *s++ = c;
    else if (!*pat)
      *s++ = '%';
    else switch (*pat++) {
    case '%':
      *s++ = '%';
      break;
    case 'm':
      strcpy(s, method);
      while (*s)
	s++;
      break;
    case 'M':
      *s++ = (char)toupper(method[0]);
      strcpy(s, method + 1);
      while (*s)
	s++;
      break;
    case 'w':
      strcpy(s, m_implName);
      while (*s)
	s++;
      break;
    case 'W':
      *s++ = (char)toupper(m_implName[0]);
      strcpy(s, m_implName + 1);
      while (*s)
	s++;
      break;
    default:
      return OU::esprintf("Invalid pattern rule: %s", m_pattern);
    }
  }
  *s++ = 0;
  return 0;
}

void Worker::
emitRccStruct(FILE *f, size_t nMembers, OU::Member *members, unsigned level,
	      const char *parent, bool isFixed, bool &isLast, bool topSeq, unsigned predefine) {
  size_t offset = 0;
  unsigned pad = 0;
  for (unsigned n = 0; !isLast && n < nMembers; n++, members++)
    printRccMember(f, *members, level, offset, pad, parent, isFixed, isLast, topSeq, predefine);
}

const char *Worker::
rccValue(OU::Value &v, std::string &value, const OU::Member *param) {
  // Convert value to something nice for C
  // In particular, large integer constants do not want to be truncated.
  // From ISO: "The type of an integer constant is the first of the corresponding list
  //            in which its value can be represented."
  // This theoretically means a problem for a decimal version of ULL_MAX
  // This implies that unsigned decimal numbers for the largest type must have
  // the suffix applied.
  switch(v.m_vt->m_baseType) {
    case OA::OCPI_Bool:
      // Bool is special because we allow C++/true/false syntax which is illegal in C
      value += m_language == C ? (v.m_Bool ? "1" : "0") : (v.m_Bool ? "true" : "false");
      break;
    case OA::OCPI_Char: 
    case OA::OCPI_Double:
    case OA::OCPI_Float:
    case OA::OCPI_Short: 
    case OA::OCPI_Long:
    case OA::OCPI_UChar:
    case OA::OCPI_ULong:
    case OA::OCPI_UShort: 
    case OA::OCPI_LongLong:
      // These are ok since there is no risk of truncation and we use C syntax
      v.unparse(value, NULL, true);
      break;
    case OA::OCPI_ULongLong:
      // This can be bad unless we force it to ull since decimal is assumed signed
      // FIXME: perhaps make this an option in the unparser? C++?
      v.unparse(value, NULL, true);
      value += "ull";
      break;
    case OA::OCPI_String:
      value += '\"';
      if (v.m_String && v.m_String[0])
	v.unparse(value, NULL, true);
      value += '\"';
      break;
    case OA::OCPI_Enum:
      assert(param);
      if (!strcasecmp("ocpi_endian", param->m_name.c_str())) {
	if (m_language == CC)
	  value = "OCPI::RCC::";
	upperconstant(value, "RCC", v.m_vt->m_enums[v.m_ULong]);
      } else
	upperconstant(value, m_implName, param->m_name.c_str(), v.m_vt->m_enums[v.m_ULong]);
    default:
      ;
    }
  return value.c_str();
}
const char *Worker::
rccPropValue(OU::Property &p, std::string &value) {
  if (p.m_default)
    return rccValue(*p.m_default, value);
  // Generate a default value
  switch(p.m_baseType) {
  case OA::OCPI_Bool: 
  case OA::OCPI_Char: 
  case OA::OCPI_Double: 
  case OA::OCPI_Float:
  case OA::OCPI_Short:
  case OA::OCPI_Long: 
  case OA::OCPI_UChar:
  case OA::OCPI_Enum:
  case OA::OCPI_ULong:
  case OA::OCPI_UShort:
  case OA::OCPI_LongLong:
  case OA::OCPI_ULongLong:
    value += "0";
    break;
  case OA::OCPI_String:
    value += "\"\""; // this is not NULL
  default:;
  }
  return value.c_str();
}

static const char *cctypes[] = {
  "None",
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #run,
  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
  "Struct", "Enum", "Type",
  0
};
static const char *ccpretty[] = {
  "None",
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) #pretty,
  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
  "Struct", "Enum", "Type",
  0
};

void Worker::
rccPropType(OU::Property &p, std::string &typeDef, std::string &type, std::string &pretty) {
  (void)p;(void)typeDef;(void)type;
  type = cctypes[p.m_baseType];
  pretty = ccpretty[p.m_baseType];
}

const char *Worker::
emitImplRCC() {
  const char *err;
  FILE *f;
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "",
			m_language == C ? RCC_C_IMPL : RCC_CC_IMPL,
			m_language == C ? RCC_C_HEADER : RCC_CC_HEADER, m_implName, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", m_file.c_str());
  fprintf(f, " *\n");
  char *upper = upperdup(m_implName);
  fprintf(f,
	  " * This file contains the implementation declarations for the %s worker in %s\n"
	  " */\n\n"
	  "#ifndef RCC_WORKER_%s_H__\n"
	  "#define RCC_WORKER_%s_H__\n"
	  "#include <RCC_Worker.h>\n",
	  m_implName, m_language == C ? "C" : "C++", upper, upper);
  if ( m_language == CC )
    fprintf(f, "#include <vector>\n" );
  if (m_language == CC && m_slave)
    fprintf(f,
	    "#include <inttypes.h>\n"
	    "#include <../OcpiApi.h>\n"
	    "#include <../OcpiOsDebug.h>\n");
  const char *last;
  unsigned in = 0, out = 0;
  if (m_ports.size()) {
    fprintf(f,
	    "/*\n"
	    " * Enumeration of port ordinals for worker %s\n"
	    " */\n"
	    "typedef enum {\n",
	    m_implName);
    last = "";
    for (unsigned n = 0; n < m_ports.size(); n++) {
      Port *port = m_ports[n];
      fprintf(f, "%s  %s_%s", last, upper, upperdup(port->name()));
      // FIXME TWO WAY
      last = ",\n";
      if (port->isDataProducer())
	out++;
      else
	in++;
    }
    fprintf(f, "\n} %c%sPort;\n", toupper(m_implName[0]), m_implName+1);
  }
  if (m_language == C)
    fprintf(f, "#define %s_N_INPUT_PORTS %u\n"
	    "#define %s_N_OUTPUT_PORTS %u\n",
	    upper, in, upper, out);
  if (m_ctl.nRunProperties < m_ctl.properties.size()) {
    fprintf(f,
	    "/*\n"
	    " * Definitions for default values of parameter properties.\n");
    if (m_language == C)
      fprintf(f,
	      " * Parameters are defined as macros with no arguments to catch spelling errors.\n");
    fprintf(f,
	    " */\n");
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      OU::Property &p = **pi;
      if (p.m_isParameter) {
	std::string value;
#if 0
	if (m_language == C)
	  fprintf(f,
		  "#ifndef PARAM_%s\n"
		  "#define PARAM_%s() (%s)\n"
		  "#endif\n",
		  p.m_name.c_str(), p.m_name.c_str(), rccPropValue(p, value));
	else
#endif
	  {
	  std::string typeDef, type, pretty;
	  rccPropType(p, typeDef, type, pretty);
	  fprintf(f,
		  "/* The constant value of the parameter property named: %s */\n"
		  "static const ", p.m_name.c_str());
	  size_t offset = 0;
	  unsigned pad = 0;
	  bool isLast = false;
	  printRccType(f, p, 0, offset, pad, m_implName, true, isLast, false, UINT_MAX-1);
	  std::string param;
	  upperconstant(param, m_implName, p.m_name.c_str());
	  fprintf(f, " %s", param.c_str());
	  printRccArray(f, p, true, isLast, false, false);
	  fprintf(f, " = PARAM_%s();\n", p.m_name.c_str());
	}
      }
    }
  }
  if (m_language == CC) {
    bool notifiers = false, writeNotifiers = false, readNotifiers = false;
    std::string s, stypes;
    camel(stypes, m_implName, "WorkerTypes", NULL);
    fprintf(f, "\nnamespace %s {\n", stypes.c_str());
    if (m_ctl.nRunProperties) {
      m_maxLevel = 0;
      unsigned pad = 0;
      size_t offset = 0;
      bool isLastDummy = false;
      // First pass, determine max depth
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
	if (!(*pi)->m_isParameter &&
	    ((*pi)->m_baseType == OA::OCPI_Enum || (*pi)->m_baseType == OA::OCPI_Struct)) {
	  printRccType(f, **pi, 0, offset, pad, NULL, true, isLastDummy, false, UINT_MAX);
	}
      fprintf(f,
	      "  /*\n"
	      "   * Property types for worker %s\n"
	      "   */\n",
	      m_implName);
      // Second path, define types bottom up
      for (unsigned l = m_maxLevel+1; l--; ) {
	pad = 0;
	offset = 0;
	isLastDummy = false;
	for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
	  if (!(*pi)->m_isParameter &&
	      ((*pi)->m_baseType == OA::OCPI_Enum || (*pi)->m_baseType == OA::OCPI_Struct)) {
	    printRccType(f, **pi, 0, offset, pad, NULL, true, isLastDummy, false, l);
	  }
      }
      fprintf(f,
	      "  /*\n"
	      "   * Property structure for worker %s\n"
	      "   */\n",
	      m_implName);
      fprintf(f, "  struct Properties {\n"); //, toupper(m_implName[0]), m_implName + 1);
      offset = 0;
      pad = 0;
      isLastDummy = false;
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
	if (!(*pi)->m_isParameter)
	  printRccMember(f, **pi, 1, offset, pad, NULL, true, isLastDummy, false, 0);
      fprintf(f, "  };\n");
    }
    fprintf(f, "}\n");
    camel(s, m_implName, "WorkerBase", NULL);
    fprintf(f,
	    "/*\n"
	    " * This is the customized class that is inherited by the actual\n"
	    " * derived class implemented by the worker author.  That class\n"
	    " * which inherits this one is declared in the skeleton/implementation file.\n"
	    " */\n"
	    "class %s : public OCPI::RCC::RCCUserWorker {\n"
	    "protected:\n",
	    s.c_str());

    if (m_scalable) {
      fprintf(f,
	      "\n"
	      "  unsigned getRank() const;                        // My rank within my crew \n"
	      "  unsigned getCrewSize() const ;                   // Number of members in my crew\n"
	      "  std::vector<unsigned> &getOtherCrewSize() const; // Number of members in crew connected to other end of port, vector len = number of fan in/out\n"
	      "  unsigned getNearestNeighbor( unsigned next ) const;   // next=0 is nearest, next=1 next nearest etc.\n\n"
	      );
    }

    if (m_slave) {
      // This worker is a proxy.  Give it access to its slave
      fprintf(f,
	      "  /*\n"
	      "   * This class defines the properties of the slave for convenient access.\n"
	      "   */\n"
	      "  class Slave : OCPI::RCC::RCCUserSlave {\n"
	      "  public:\n");
      if (m_slave->m_ctl.nRunProperties)
	for (PropertiesIter pi = m_slave->m_ctl.properties.begin();
	     pi != m_slave->m_ctl.properties.end(); pi++) {
	  OU::Property &p = **pi;
	  if (!p.m_isParameter) {
	    std::string typeDef, type, pretty;
	    rccPropType(p, typeDef, type, pretty);
	    size_t *offsets = NULL;
	    if (p.m_arrayRank) {
	      offsets = new size_t[p.m_arrayRank];
	      size_t n = p.m_arrayRank - 1;
	      offsets[n] = 1;
	      while (n > 0) {
		offsets[n-1] = offsets[n] * p.m_arrayDimensions[n];
		n--;
	      }
	    }
	    if (p.m_isReadable) {
	      fprintf(f, "    inline %s get_%s(", type.c_str(), p.m_name.c_str());
	      for (unsigned n = 0; n < p.m_arrayRank; n++)
		fprintf(f, "%sunsigned idx%u", n ? ", " : "", n);
	      fprintf(f, ") { return m_worker.get%sProperty(%u", pretty.c_str(), p.m_ordinal);
	      if (p.m_arrayRank)
		for (unsigned n = 0; n < p.m_arrayRank; n++)
		  fprintf(f, "%sidx%u*%zu", n ? " + " : ", ", n, offsets[n]);
	      else
		fprintf(f, ", 0");
	      fprintf(f,"); }\n");
	    }
	    if (p.m_isWritable) {
	      fprintf(f, "    inline void set_%s(", p.m_name.c_str());
	      for (unsigned n = 0; n < p.m_arrayRank; n++)
		fprintf(f, "unsigned idx%u, ", n);
	      fprintf(f, "%s val) {\n", type.c_str());
	      if (p.m_arrayRank) {
		fprintf(f, "      unsigned idx = ");
		for (unsigned n = 0; n < p.m_arrayRank; n++)
		  fprintf(f, "%sidx%u*%zu", n ? " + " : "", n, offsets[n]);
		fprintf(f,
			";\n"
			"      m_worker.set%sProperty(%u, val, idx);\n",
			pretty.c_str(), p.m_ordinal);
	      } else
		fprintf(f,
			"      m_worker.set%sProperty(%u, val, 0);\n",
			pretty.c_str(), p.m_ordinal);
	      fprintf(f,
		      "#if !defined(NDEBUG)\n"
		      "    OCPI::OS::logPrint(OCPI_LOG_DEBUG, \"Setting slave.set_%s",
		      p.m_name.c_str());
	      if (p.m_arrayRank)
		fprintf(f,
			" at index %%u(0x%%x): 0x%%llx\", idx, idx, (unsigned long long)val);\n");
	      else
		fprintf(f,
			": 0x%%llx\", (unsigned long long)val);\n");
	      fprintf(f,
		      "#endif\n"
		      "    }\n");
	    }
	    delete [] offsets;
	  }
	}
      fprintf(f,
	      "  } slave;\n");
    }
    if (m_ctl.nRunProperties) {
      fprintf(f,
	      "  %s::Properties m_properties;\n"
	      "  uint8_t *rawProperties(size_t &size) const {\n"
	      "    size = sizeof(m_properties);\n"
	      "    return (uint8_t*)&m_properties;\n"
	      "  }\n"
	      "  inline %s::Properties &properties() { return m_properties; }\n",
	      stypes.c_str(), stypes.c_str());
      // toupper(m_implName[0]), m_implName + 1,
      // toupper(m_implName[0]), m_implName + 1);
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
	if ((**pi).m_writeSync || (**pi).m_readSync) {
	  if (!notifiers)
	    fprintf(f, "  typedef OCPI::RCC::RCCResult (%s::*Notification)();\n", s.c_str());
	  notifiers = true;
	  if ((**pi).m_writeSync) {
	    if (!writeNotifiers)
	      fprintf(f, 
		      "  static Notification s_write_notifiers[];\n"
		      "  void propertyWritten(unsigned ordinal) {\n"
		      "    Notification n = s_write_notifiers[ordinal];\n"
		      "    if (n) (this->*n)(); \n"
		      "  }\n");
	    writeNotifiers = true;
	    fprintf(f,
		    "  virtual OCPI::RCC::RCCResult %s_written() = 0;\n", (**pi).m_name.c_str());
	  }
	  if ((**pi).m_readSync) {
	    if (!readNotifiers)
	      fprintf(f, 
		      "  static Notification s_read_notifiers[];\n"
		      "  void propertyRead(unsigned ordinal) {\n"
		      "    Notification n = s_read_notifiers[ordinal];\n"
		      "    if (n) (this->*n)(); \n"
		      "  }\n");
	    readNotifiers = true;
	    fprintf(f,
		    "  virtual OCPI::RCC::RCCResult %s_read() = 0;\n", (**pi).m_name.c_str());
	  }
	}
    }
    for (unsigned n = 0; n < m_ports.size(); n++)
      m_ports[n]->emitRccCppImpl(f);
    fprintf(f,
	    "};\n\n"
	    "#define %s_START_INFO \\\n"
	    "  extern \"C\" {\\\n"
	    "    OCPI::RCC::RCCConstruct %s;\\\n"
	    "    OCPI::RCC::RCCUserWorker *\\\n"
	    "    %s(void *place, OCPI::RCC::RCCWorkerInfo &info) {\\\n"
	    "      info.size = sizeof(%c%sWorker);\\\n",
	    upper, m_implName, m_implName,
	    toupper(m_implName[0]), m_implName + 1);
    fprintf(f,
	    "\n"
	    "#define %s_END_INFO \\\n"
	    "      return place ? new /*((%c%sWorker *)place)*/ %c%sWorker : NULL;\\\n"
	    "    }\\\n"
	    "  }\\\n",
	    upper,
	    toupper(m_implName[0]), m_implName + 1,
	    toupper(m_implName[0]), m_implName + 1);
    if (writeNotifiers) {
      fprintf(f, "  %s::Notification %s::s_write_notifiers[] = {\\\n", s.c_str(), s.c_str());
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
	OU::Property &p = **pi;
	if (!p.m_isParameter && p.m_writeSync)
	  fprintf(f, "  &%c%sWorkerBase::%s_written,\\\n", toupper(m_implName[0]),
		  m_implName + 1, p.m_name.c_str());
	else
	  fprintf(f, "  NULL,\\\n");
      }
      fprintf(f, "  };\\\n");
    }
    if (readNotifiers) {
      fprintf(f, "  %s::Notification %s::s_read_notifiers[] = {\\\n", s.c_str(), s.c_str());
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
	OU::Property &p = **pi;
	if (!p.m_isParameter && p.m_readSync)
	  fprintf(f, "  &%c%sWorkerBase::%s_read,\\\n", toupper(m_implName[0]),
		  m_implName + 1, p.m_name.c_str());
	else
	  fprintf(f, "  NULL,\\\n");
      }
      fprintf(f, "  };\\\n");
    }
    fprintf(f, "\n");
  } else {
    if (m_ctl.nRunProperties) {
      fprintf(f,
	      "/*\n"
	      " * Property structure for worker %s\n"
	      " */\n",
	      m_implName);
      fprintf(f, "typedef struct {\n");
      unsigned pad = 0;
      size_t offset = 0;
      bool isLastDummy = false;
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
	if (!(*pi)->m_isParameter)
	  printRccMember(f, **pi, 0, offset, pad, m_implName, true, isLastDummy, false, UINT_MAX-1);
      fprintf(f, "} %c%sProperties;\n\n", toupper(m_implName[0]), m_implName + 1);
    }
    const char *mName;
    fprintf(f,
	    "/*\n"
	    " * Use this macro, followed by a semicolon, to declare methods before\n"
	    " * implementing them, and before defining the RCCDispatch for the worker\n"
	    " * e.g.:\n"
	    " * %s_METHOD_DECLARATIONS;\n"
	    " */\n"
	    "#define %s_METHOD_DECLARATIONS \\\n",
	    upper, upper);
    if (m_ctl.controlOps) {
      last = "";
      fprintf(f, " %s RCCMethod ", m_pattern ? "extern" : "static");
      unsigned op = 0;
      for (const char **cp = OU::Worker::s_controlOpNames; *cp; cp++, op++)
	if (m_ctl.controlOps & (1 << op)) {
	  if ((err = rccMethodName(*cp, mName)))
	    return err;
	  fprintf(f, "%s%s", last, mName);
	  last = ", ";
	}
      fprintf(f, ";\\\n");
    }
    if ((err = rccMethodName("run", mName)))
      return err;
    fprintf(f,
	    " %s RCCRunMethod %s\\\n",
	    m_pattern ? "extern" : "static", mName);
    fprintf(f, "/**/\n");
    fprintf(f,
	    "/*\n"
	    " * This macro defines the initialization of the RCCDispatch structure\n"
	    " * for the %s worker. Insert it AFTER any customized initializations\n"
	    " * of members: memSizes, runCondition.\n"
	    " * E.g.: \n"
	    " * static RCCRunCondition myRunCondition { ... };\n"
	    " * %s_METHOD_DECLARATIONS;\n"
	    " * RCCDispatch xyz_dispatch = {\n"
	    " * .runCondition = &myRunCondition,\n"
	    " * %s_DISPATCH\n"
	    " * };\n"
	    " */\n"
	    "#define %s_DISPATCH \\\n"
	    " .version = RCC_VERSION,\\\n"
	    " .numInputs = %s_N_INPUT_PORTS,\\\n"
	    " .numOutputs = %s_N_OUTPUT_PORTS,\\\n"
	    " .threadProfile = %u,\\\n",
	    m_implName, upper, upper, upper, upper, upper, m_isThreaded ? 1 : 0);
    if (m_ctl.nRunProperties)
      fprintf(f, " .propertySize = sizeof(%c%sProperties),\\\n",
	      toupper(m_implName[0]), m_implName + 1);
    unsigned op = 0;
    for (const char **cp = OU::Worker::s_controlOpNames; *cp; cp++, op++)
      if (m_ctl.controlOps & (1 << op)) {
	if ((err = rccMethodName(*cp, mName)))
	  return err;
	fprintf(f, " .%s = %s,\\\n", *cp, mName);
      }
    if ((err = rccMethodName("run", mName)))
      return err;
    fprintf(f, " .run = %s,\\\n", mName);
    uint32_t optionals = 0;
    for (unsigned n = 0; n < m_ports.size(); n++) {
      Port *port = m_ports[n];
      if (port->isDataOptional())
	optionals |= 1 << n;
    }
    if (optionals)
      fprintf(f, " .optionallyConnectedPorts = 0x%x,\\\n", optionals);
    fprintf(f, "/**/\n");
    for (unsigned n = 0; n < m_ports.size(); n++)
      m_ports[n]->emitRccCImpl(f);
    for (unsigned n = 0; n < m_ports.size(); n++)
      m_ports[n]->emitRccCImpl1(f);
  }
  fprintf(f, "#endif /* ifndef RCC_WORKER_%s_H__ */\n", upper);
  fclose(f);
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "", RCCMAP, RCC_C_HEADER, NULL, f)))
    return err;
  fprintf(f, "#define RCC_FILE_WORKER_%s %s\n", m_fileName.c_str(), m_implName);
  fclose(f);
  return 0;
}

const char *Worker::
emitSkelRCC() {
  const char *err;
  FILE *f;
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "", "-skel",
			m_language == C ? ".c" : ".cc", NULL, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", m_file.c_str(), true);
  fprintf(f,
	  " *\n"
	  " * This file contains the implementation skeleton for the %s worker in %s\n"
	  " */\n\n"
	  "#include \"%s%s%s\"\n\n",
	  m_implName, m_language == C ? "C" : "C++", m_implName,
	  m_language == C ? RCC_C_IMPL : RCC_CC_IMPL,
	  m_language == C ? RCC_C_HEADER : RCC_CC_HEADER);
  const char *upper = upperdup(m_implName);
#if 0
  if (m_language == CC) {
    if (m_ctl.controlOps) {
      fprintf(f,"  RCCResult ");
      const char *last = "";
      unsigned op = 0;
      for (const char **cp = OU::Worker::s_controlOpNames; *cp; cp++, op++)
	if (m_ctl.controlOps & (1 << op)) {
	  fprintf(f, "%s    %s", last, *cp);
	  last = ", ";
	}
      fprintf(f, ";\n");
    }
  }
#endif
  if (m_language == C) {
    fprintf(f,
	    "%s_METHOD_DECLARATIONS;\n"
	    "RCCDispatch %s = {\n"
	    " /* insert any custom initializations here */\n"
	    " %s_DISPATCH\n"
	    "};\n\n"
	    "/*\n"
	    " * Methods to implement for worker %s, based on metadata.\n"
	    " */\n",
	    upper, m_implName,
	    upper, m_implName);
  } else {
    fprintf(f,
	    "using namespace OCPI::RCC; // for easy access to RCC data types and constants\n"
	    "using namespace %c%sWorkerTypes;\n"
	    "\n"
	    "class %c%sWorker : public %c%sWorkerBase {\n",
	    toupper(m_implName[0]), m_implName + 1,
	    toupper(m_implName[0]), m_implName + 1,
	    toupper(m_implName[0]), m_implName + 1);
  }
  unsigned op = 0;
  const char **cp;
  const char *mName;
  for (cp = OU::Worker::s_controlOpNames; *cp; cp++, op++)
    if (m_ctl.controlOps & (1 << op)) {
      if ((err = rccMethodName(*cp, mName)))
	return err;
      if (m_language == C)
	fprintf(f,
		"\n"
		"%s RCCResult\n%s(RCCWorker *self) {\n"
		" return RCC_OK;\n"
		"}\n",
		m_pattern ? "extern" : "static", mName);
      else
	fprintf(f,
		"  RCCResult %s() {\n"
		"    return RCC_OK;\n"
		"  }\n", mName);
    }
  if ((err = rccMethodName("run", mName)))
    return err;
  if (m_language == C)
    fprintf(f,
	    "\n"
	    "%s RCCResult\n%s(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {\n"
	    " (void)self;(void)timedOut;(void)newRunCondition;\n"
	    " return RCC_ADVANCE;\n"
	    "}\n",
	    m_pattern ? "extern" : "static", mName);
  else {
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      if ((**pi).m_writeSync)
	fprintf(f,
		"  // notification that %s property has been written\n"
		"  RCCResult %s_written() {\n"
		"    return RCC_OK;\n"
		"  }\n",
		(**pi).m_name.c_str(), (**pi).m_name.c_str());
      if ((**pi).m_readSync)
	fprintf(f,
		"  // notification that %s property will be read\n"
		"  RCCResult %s_read() {\n"
		"    return RCC_OK;\n"
		"  }\n",
		(**pi).m_name.c_str(), (**pi).m_name.c_str());
    }
    fprintf(f,
	    "  RCCResult run(bool /*timedout*/) {\n"
	    "    return RCC_ADVANCE;\n"
	    "  }\n");
  }
  if (m_language == CC) {
    fprintf(f,
	    "};\n\n"
	    "%s_START_INFO\n"
	    "// Insert any static info assignments here (memSize, memSizes, portInfo)\n"
            "// e.g.: info.memSize = sizeof(MyMemoryStruct);\n"
	    "%s_END_INFO\n",
	    upper, upper);
  }
  fclose(f);
  return 0;
}


/*
 * What implementation-specific attributes does an RCC worker have?
 * And which are not available at runtime?
 * And if they are indeed available at runtime, do we really retreive them from the
 * container or just let the container use what it knows?
 */
const char *Worker::
parseRccImpl(const char *package) {
  const char *err;
  if ((err = OE::checkAttrs(m_xml, IMPL_ATTRS, "ExternMethods", "StaticMethods", "Threaded",
			    "Language", "Slave", (void*)0)) ||
      (err = OE::checkElements(m_xml, IMPL_ELEMS, "port", (void*)0)))
    return err;
  // We use the pattern value as the method naming for RCC
  // and its presence indicates "extern" methods.
  m_pattern = ezxml_cattr(m_xml, "ExternMethods");
  m_staticPattern = ezxml_cattr(m_xml, "StaticMethods");
  ezxml_t xctl;
  if ((err = parseSpec(package)) ||
      (err = parseImplControl(xctl)) ||
      (xctl && (err = OE::checkAttrs(xctl, GENERIC_IMPL_CONTROL_ATTRS, "Threaded", (void *)0))) ||
      (err = OE::getBoolean(m_xml, "Threaded", &m_isThreaded)))
    return err;
  // Parse data port implementation metadata: maxlength, minbuffers.
  Port *sp;
  for (ezxml_t x = ezxml_cchild(m_xml, "Port"); x; x = ezxml_next(x))
    if ((err = checkDataPort(x, sp)) || !createPort<RccPort>(*this, x, sp, -1, err))
      return err;
  for (unsigned i = 0; i < m_ports.size(); i++)
    m_ports[i]->finalizeRccDataPort();

#if 0
    {
    if ((err = OE::checkAttrs(x, "Name", "MinBuffers", "MinBufferCount", "BufferSize", (void*)0)))
      return err;
    const char *name = ezxml_cattr(x, "Name");
    if (!name)
      return "Missing \"Name\" attribute on Port element if RccWorker";
    Port *p = 0; // kill warning
    unsigned n;
    for (n = 0; n < m_ports.size(); n++) {
      p = m_ports[n];
      if (!strcasecmp(p->name(), name))
        break;
    }
    if (n >= m_ports.size())
      return OU::esprintf("No DataInterface named \"%s\" from Port element", name);
    if ((err = OE::getNumber(x, "MinBuffers", &p->u.wdi.minBufferCount, 0, 0)) || // backward compat
        (err = OE::getNumber(x, "MinBufferCount", &p->u.wdi.minBufferCount, 0, p->u.wdi.minBufferCount)) ||
        (err = OE::getNumber(x, "Buffersize", &p->u.wdi.bufferSize, 0,
			     p->m_protocol ? p->m_protocol->m_defaultBufferSize : 0)))
      return err;
  }
#endif
  std::string slave;
  if (OE::getOptionalString(m_xml, slave, "slave")) {
    // The slave attribute is the name of an implementation including the model.
    // Thus the search is in the same library
    std::string sw;
    const char *dot = strrchr(slave.c_str(), '.');
    if (!dot)
      return OU::esprintf("slave attribute: '%s' has no authoring model suffix", slave.c_str());
    OU::format(sw, "../%s/%.*s.xml", slave.c_str(), (int)(dot - slave.c_str()), slave.c_str());
    if (!(m_slave = Worker::create(sw.c_str(), m_file.c_str(), NULL, m_outDir, NULL, NULL, 0,
				   err)))
      return OU::esprintf("for slave worker %s: %s", slave.c_str(), err);
  }
  m_model = RccModel;
  m_modelString = "rcc";
  m_baseTypes = rccTypes;
  return 0;
}

// RCC assemblies are collections of reusable instances with no connections.
const char *Worker::
parseRccAssy() {
  const char *err;
  ::Assembly *a = m_assembly = new ::Assembly(*this);
  static const char
    *topAttrs[] = {IMPL_ATTRS, RCC_TOP_ATTRS, RCC_IMPL_ATTRS, NULL},
    *instAttrs[] = {INST_ATTRS, "reusable", NULL};
  // Do the generic assembly parsing, then to more specific to RCC
  if ((err = a->parseAssy(m_xml, topAttrs, instAttrs, true, m_outDir)))
    return err;
  return NULL;
}

// This is an RCC file, and perhaps an assembly or a platform
const char *Worker::
parseRcc(const char *package) {
  const char *lang = ezxml_cattr(m_xml, "Language");
  if (!lang)
    m_language = C;
  else if (!strcasecmp(lang, "C"))
    m_language = C;
  else if (!strcasecmp(lang, "C++"))
    m_language = CC;
  else
    return OU::esprintf("Language attribute \"%s\" is not \"C\" or \"C++\""
			" in RccWorker xml file: '%s'", lang, m_file.c_str());
#if 0
  if (strcmp(m_implName, m_fileName.c_str()))
    return OU::esprintf("File name (%s) and implementation name in XML (%s) don't match",
			m_fileName.c_str(), m_implName);
#endif
  const char *err;
  // Here is where there is a difference between a implementation and an assembly
  if (!strcasecmp(m_xml->name, "RccWorker") || !strcasecmp(m_xml->name, "RccImplementation")) {
    if ((err = parseRccImpl(package)))
      return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  } else if (!strcasecmp(m_xml->name, "RccAssembly")) {
    if ((err = parseRccAssy()))
      return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  } else
      return "file contains neither an RccWorker nor an RccAssembly";
  m_model = RccModel;
  m_modelString = "rcc";
  m_reusable = true;
  return NULL;
}

RccAssembly *RccAssembly::
create(ezxml_t xml, const char *xfile, const char *&err) {
  RccAssembly *ha = new RccAssembly(xml, xfile, err);
  if (err) {
    delete ha;
    ha = NULL;
  }
  return ha;
}

RccAssembly::
RccAssembly(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, "", Worker::Assembly, this, NULL, err) {
  if (!(err = OE::checkAttrs(xml, IMPL_ATTRS, RCC_TOP_ATTRS, (void*)0)) &&
      !(err = OE::checkElements(xml, IMPL_ELEMS, RCC_IMPL_ELEMS, ASSY_ELEMS, (void*)0)))
    err = parseRcc();
}

RccAssembly::
~RccAssembly() {
}

void DataPort::
emitRccCppImpl(FILE *f) {
  // Define the union of structures for messages for operations
  fprintf(f,
	  "  class %c%sPort : public OCPI::RCC::RCCUserPort {\n",
	  toupper(name()[0]), name()+1);
  // Now emit structs for messages
  OU::Operation *o = m_protocol->operations();
  if (o) {
    std::string ops;
    OU::format(ops, "%c%sOperations", toupper(name()[0]), name()+1);
    bool first = true;
    for (unsigned nn = 0; nn < m_protocol->nOperations(); nn++, o++)
      if (o->nArgs()) {
	std::string s;
	camel(s, o->name().c_str());
	fprintf(f,
		"    // Structure for the '%s' operation on port '%s'\n"
		"    struct __attribute__ ((__packed__)) %s {\n",
		o->name().c_str(), name(), s.c_str());
	bool isLast = false;
	m_worker->emitRccStruct(f, o->nArgs(), o->args(), 8, s.c_str(), false, isLast,
				o->isTopFixedSequence(), 0);
	fprintf(f, "    };\n" );
      }


    // Start union
    fprintf(f, "\n\n    union %s {\n", ops.c_str());
    o = m_protocol->operations();
    for (unsigned nn = 0; nn < m_protocol->nOperations(); nn++, o++)
      if (o->nArgs()) {
	std::string s;
	camel(s, o->name().c_str());
	fprintf(f,"      %s m_%s;\n",s.c_str(),  o->name().c_str() );
      }
    fprintf(f,
	    "    };\n"
	    "    %s %s &message() %s { return *(%s *)RCCUserPort::data(); }\n",
	    !m_isProducer ? "const " : "", ops.c_str(),
	    !m_isProducer ? "const " : "", ops.c_str());
    // End union

    // Operation enums
    {
      o = m_protocol->operations();
      fprintf(f, "    enum { \n" );
      for (unsigned nn = 0; nn < m_protocol->nOperations(); nn++, o++) {
	std::string s;
	camel(s, o->name().c_str() );	
	fprintf(f,"      %s_OPERATION\n",  s.c_str());	  
      }
      fprintf(f, "    }; \n" );
    }

    // Start Op class

    // Now we need a class for each operation
    o = m_protocol->operations();
    for (unsigned nn = 0; nn < m_protocol->nOperations(); nn++, o++) {
      std::string s;
      camel(s, o->name().c_str());	
      if (o->nArgs()) {
	fprintf(f,
		"    class %sOp : public OCPI::RCC::RCCPortOperation { \n"
		"    private:\n"
		//		"       RCCUserBuffer * m_buffer;\n"
		"       %s * m_%s; \n"

		"    public:\n"
		"       %sOp(){}\n"
		"       %sOp( RCCUserBuffer & buffer ){ setBuffer( &buffer ); }\n"
		"       %sOp( %sOp& rhs  ){setBuffer( rhs.m_buffer );}\n"
		"       void setBuffer( RCCUserBuffer* buffer ){m_buffer=buffer; m_%s=(%s *)m_buffer;}\n\n"
		,
		s.c_str(), 
		s.c_str(),o->name().c_str(),
		s.c_str(),
		s.c_str(),
		s.c_str(),s.c_str(),
		o->name().c_str(),s.c_str()
		);

	// Arg enums
	{
	  OU::Member *m = o->args();
	  fprintf(f, "       enum { \n" );
	  for (unsigned n = 0; n < o->nArgs(); n++, m++) {
	    std::string s;
	    camel(s, m->m_name.c_str() );	
	    fprintf(f,"         %s_ARG\n",  s.c_str());	  
	  }
	  fprintf(f, "       }; \n" );
	}


	// And a class for each arg in the operation
	OU::Member *m = o->args();
	for (unsigned n = 0; n < o->nArgs(); n++, m++) {
	  std::string s;
	  camel(s, m->m_name.c_str() );	
	  std::string p;
	  camel(p, name() );	
	  std::string on;
	  camel(on, o->name().c_str() );

	  if ( m->m_isSequence ) {
	    fprintf(f,
		    "       class %sArg  { \n"
		    "       private:\n"
		    "          void * m_myptr;\n"
		    "          void * getArgAddress( unsigned int, unsigned int);\n"
		    "       public:\n"
		    "          %sArg() : m_myptr(NULL){}\n"
		    "          inline void * value() { return m_myptr ? m_myptr : (m_myptr = getArgAddress((unsigned)%sPort::%s_OPERATION, (unsigned)%s_ARG)); }\n",
		    s.c_str(), 
		    s.c_str(),
		    p.c_str(),on.c_str(),s.c_str()
		    );

	    if ( m_opScaling[n] != NULL )
	      fprintf(f,
		      "          bool   endOfWhole() const; \n"
		      "          void partSize( OCPI::RCC::RCCPartInfo & part ) const;\n"
		      );

	    fprintf(f,
		    "       } m_%sArg;\n",
		    s.c_str()
		    );

	  }
	  else if ( m->m_arrayRank ) {
	    fprintf(f,
		    "       class %sArg  { \n"
		    "       private:\n"
		    "          void * m_myptr;\n"
		    "          void * getArgAddress( unsigned int, unsigned int);\n"
		    "       public:\n"
		    "          %sArg() : m_myptr(NULL){}\n"
		    "          inline void * value() { return m_myptr ? m_myptr : (m_myptr = getArgAddress((unsigned)%sPort::%s_OPERATION, (unsigned)%s_ARG)); }\n",
		    s.c_str(), 
		    s.c_str(),
		    p.c_str(),on.c_str(),s.c_str()
		    );

	    if ( m_opScaling[n] != NULL )
	      fprintf(f,
		      "          size_t dimensions() const;  // Number of dimensions\n"
		      "          bool   endOfWhole() const; \n"
		      "          void partSize( unsigned dimension, OCPI::RCC::RCCPartInfo & part ) const;\n"
		      );

	    fprintf(f,
		    "       } m_%sArg;\n",
		    s.c_str()
		    );

	  }
	  else {
	    fprintf(f,
		    "       class %sArg  { \n"
		    "       private:\n"
		    "          void * m_myptr;\n"
		    "       public:\n"
		    "          %sArg() : m_myptr(NULL){}\n"
		    "          inline void * value() { return m_myptr ? m_myptr : (m_myptr = getArgAddress((unsigned)%sPort::%s_OPERATION, (unsigned)%s_ARG)); }\n"
		    "       } m_%sArg;\n"
		    ,s.c_str(), 
		    s.c_str(),
		    p.c_str(),on.c_str(),s.c_str(),
		    s.c_str()
		    );

	  }

	}
	// End args

      }

      fprintf(f, "    } m_%sOp;\n", s.c_str() );

      fprintf(f, 
	      "    // Conversion operators\n"
	      "    inline operator %sOp& () {m_%sOp.setBuffer(this); return m_%sOp;}\n"
	      "    // Factories, used to take messages\n" 
	      "    inline %sOp* take(%sOp& rhs) const\n "
	      "    {\n"
	      "       %sOp * so = new %sOp( rhs );\n"
	      "       return so;\n"
	      "    }\n"
	      , 
	      s.c_str(),s.c_str(),s.c_str(), 
	      s.c_str(),s.c_str(),
	      s.c_str(),s.c_str()
	      );
   
    // End Op class


    }



    first = true;
    o = m_protocol->operations();
    for (unsigned nn = 0; nn < m_protocol->nOperations(); nn++, o++)
      if (o->nArgs()) {
	std::string s;
	camel(s, o->name().c_str());
	if (first) {
	  fprintf(f, "  public:\n");
	  first = false;
	}

	fprintf(f, "    %s %s &get%s() %s { return message().m_%s; }\n",
		!m_isProducer ? "const " : "",
		s.c_str(), o->name().c_str(),
		!m_isProducer ? "const " : "",
		o->name().c_str());

	if (o->isTopFixedSequence()) {
	  std::string name;
	  topTypeName(name, o->args(), m_worker->m_baseTypes);
	  fprintf(f,
		  "    size_t %s_length() { return topLength(sizeof(%s)); }\n",
		  o->name().c_str(), name.c_str());
	}
      }
  }
  fprintf(f, "  } %s;\n", name());
}
void DataPort::
emitRccCImpl(FILE *f) {
  if (m_protocol->operations()) {
    unsigned nn;
    for (nn = 0; nn < m_ordinal; nn++)
      if (m_worker->m_ports[nn]->isData()) {
	DataPort *dp = static_cast<DataPort*>(m_worker->m_ports[nn]);

	if (dp->m_protocol->operations() &&
	    !strcasecmp(dp->m_protocol->m_name.c_str(), m_protocol->m_name.c_str()))
	  break;
      }
    if (nn >= m_ordinal) {
      fprintf(f,
	      "/*\n"
	      " * Enumeration of operations for protocol %s (%s)\n"
	      " */\n"
	      "typedef enum {\n",
	      m_protocol->m_name.c_str(), m_protocol->m_qualifiedName.c_str());
      OU::Operation *o = m_protocol->operations();
      char *puName = upperdup(m_protocol->m_name.c_str());
      for (unsigned no = 0; no < m_protocol->nOperations(); no++, o++) {
	char *ouName = upperdup(o->name().c_str());
	fprintf(f, " %s_%s,\n", puName, ouName);
	free((void*)ouName);
      }
      free(puName);
      fprintf(f, "} %c%sOperation;\n",
	      toupper(*m_protocol->m_name.c_str()),
	      m_protocol->m_name.c_str() + 1);
    }
  }
}

void DataPort::
emitRccCImpl1(FILE *f) {
  if (m_protocol->operations()) {
    char *upper = upperdup(m_worker->m_implName);
    fprintf(f,
	    "/*\n"
	    " * Enumeration of operations on port %s of worker %s\n"
	    " */\n"
	    "typedef enum {\n",
	    name(), m_worker->m_implName);
    OU::Operation *o = m_protocol->operations();
    char *puName = upperdup(name());
    for (unsigned nn = 0; nn < m_protocol->nOperations(); nn++, o++) {
      char *ouName = upperdup(o->name().c_str());
      fprintf(f, "  %s_%s_%s,\n", upper, puName, ouName);
      free(ouName);
    }
    free(puName);
    fprintf(f, "} %c%s%c%sOperation;\n",
	    toupper(m_worker->m_implName[0]), m_worker->m_implName+1,
	    toupper(name()[0]), name()+1);
    // Now emit structs for messages
    o = m_protocol->operations();
    for (unsigned nn = 0; nn < m_protocol->nOperations(); nn++, o++)
      if (o->nArgs()) {
	fprintf(f,
		"/*\n"
		" * Structure for the \"%s\" operation on port \"%s\"\n"
		" */\n"
		"typedef struct __attribute__ ((__packed__)) {\n",
		o->name().c_str(), name());
	std::string s;
	camel(s, m_worker->m_implName, name(), o->name().c_str());
	bool isLast = false;
	m_worker->emitRccStruct(f, o->nArgs(), o->args(), 0, s.c_str(), false, isLast,
				o->isTopFixedSequence(), UINT_MAX-1);
	fprintf(f, "} %s;\n", s.c_str());
	OpScaling *os = m_opScaling[nn];
	if (os && os->m_isPartitioned) {
	  OU::Member *arg = o->m_args;
	  fprintf(f,
		  "/*\n"
		  " * PartInfo Structure for the %s operation on port %s\n"
		  " */\n"
		  "typedef struct __attribute__ ((__packed__)) {\n",
		  o->name().c_str(), name());
	  for (unsigned a = 0; a < o->m_nArgs; a++, arg++) {
	    Partitioning *ap = os->m_partitioning[a];
	    if (ap && ap->m_scaling.m_min) {
	      fprintf(f, "  %sPartInfo %s_info[%zu];\n",
		      upperdup(m_worker->m_modelString), arg->m_name.c_str(),
		      arg->m_isSequence ? 1 : arg->m_arrayRank);
	      size_t dummy1 = 0;
	      unsigned dummy2 = 0;
	      bool dummy3;
	      m_worker->printRccBaseType(f, *arg, 2, dummy1, dummy2, "", false, dummy3, false);
	      fprintf(f, " *%s;\n", arg->m_name.c_str());
	    }
	  }
	  fprintf(f,
		  "} %c%s%c%sPartInfo;\n",
		  toupper(m_worker->m_implName[0]), m_worker->m_implName+1,
		  toupper(o->name().c_str()[0]), o->name().c_str() + 1);
	}
      }
  }
}

const char *DataPort::
finalizeRccDataPort() {
  const char *err = NULL;
  if (type == WDIPort)
    createPort<RccPort>(*m_worker, NULL, this, -1, err);
  return err;
}
RccPort::
RccPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : DataPort(w, x, sp, ordinal, RCCPort, err) {
  if (x && !err &&
      !(err = OE::checkAttrs(x, "Name", "implname",
			     "minbuffers", "minbuffercount", "buffersize",
			     DISTRIBUTION_ATTRS, PARTITION_ATTRS, (void*)0)))
    err = OE::checkElements(x, "operation" ,(void*)0);
  
}
