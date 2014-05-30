#include "assembly.h"
#include "rcc.h"

namespace OU = OCPI::Util;


// Generate the readonly implementation file.
// What implementations must explicitly (verilog) or implicitly (VHDL) include.
#define HEADER ".h"
#define RCCIMPL "_Worker"
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
  s = (char)toupper(s1[0]);
  s += s1 + 1;
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
emitStructRCC(FILE *f, size_t nMembers, OU::Member *members, unsigned indent,
	      const char *parent, bool isFixed, bool &isLast, bool topSeq);
static void
printMember(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
	    const char *parent, bool isFixed, bool &isLast, bool topSeq);

static void
printArray(FILE *f, OU::Member *m, bool isFixed, bool &isLast, bool topSeq) {
  if (m->m_arrayRank)
    for (unsigned n = 0; n < m->m_arrayRank; n++)
      fprintf(f, "[%zu]", m->m_arrayDimensions[n]);
  if (topSeq) {
    if (m->m_sequenceLength)
      fprintf(f, "[%zu]", m->m_sequenceLength);
    else
      fprintf(f, "[1]");
  }
  // We align strings on a 4 byte boundary, and implicitly pad them to a 4 byte boundary too
  if (m->m_baseType == OA::OCPI_String) {
    fprintf(f, "[%zu]", isFixed ? (m->m_stringLength + 4) & ~3 : 4);
    if (m->m_stringLength == 0)
      isLast = true;
  }
  // End of declarator. If we're a sequence we close off the struct.
  fprintf(f, "; /* %8zxx", m->m_offset); 
  if (topSeq)
    fprintf(f, "this is a top level sequence of fixed size elements");
  fprintf(f, " */\n");
}

// Print type, including sequence type etc.
static void
printType(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
	  const char *parent, bool isFixed, bool &isLast, bool topSeq);
// Just print the data type, not the "member", with names or arrays etc.
static void
printBaseType(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
	      const char *parent, bool isFixed, bool &isLast) {
  if (m->m_baseType == OA::OCPI_Struct) {
    std::string s;
    camel(s, parent, m->m_name.c_str());
    fprintf(f, "%*sstruct %s {\n", indent, "", s.c_str());
    emitStructRCC(f, m->m_nMembers, m->m_members, indent + 2, s.c_str(), isFixed, isLast, false);
    fprintf(f, "%*s}", indent, "");
  } else if (m->m_baseType == OA::OCPI_Type)
    printType(f, m->m_type, indent, offset, pad, parent, isFixed, isLast, false);
  else
    fprintf(f, "%*s%-13s", indent, "", rccTypes[m->m_baseType]);
}
// Print type, including sequence type etc.
static void
printType(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
	  const char *parent, bool isFixed, bool &isLast, bool topSeq) {
  if (m->m_isSequence && !topSeq) {
    fprintf(f,
	    "%*sstruct {\n"
	    "%*s  uint32_t length;\n",
	    indent, "", indent, "");
    //    offset += 4;
    if (m->m_dataAlign > sizeof(uint32_t)) {
      size_t align = m->m_dataAlign - (unsigned)sizeof(uint32_t);
      fprintf(f, "%*s  char pad%u_[%zu];\n", indent, "", pad++, align);
      //      offset += align;
    }
    printBaseType(f, m, indent + 2, offset, pad, parent, isFixed, isLast);
    fprintf(f, " data");
    if (m->m_sequenceLength && isFixed)
      fprintf(f, "[%zu]", m->m_sequenceLength);
    else {
      fprintf(f, "[]");
      isLast = true;
    }
    printArray(f, m, isFixed, isLast, false);
    fprintf(f, "%*s}", indent, "");
  } else
    printBaseType(f, m, indent, offset, pad, parent, isFixed, isLast);
}
// FIXME: a tool-time member class should have this...OCPI::Tools::RCC::Member...
// Returns true when something is variable length.
// strings or sequences are like that unless then are bounded.
static void
printMember(FILE *f, OU::Member *m, unsigned indent, size_t &offset, unsigned &pad,
	    const char *parent, bool isFixed, bool &isLast, bool topSeq)
{
  //  printf("name %s offset %u m->m_offset %u bytes %u\n",
  //	 m->m_name.c_str(), offset, m->m_offset, m->m_nBytes);
  if (offset < m->m_offset) {
    fprintf(f, "%*schar pad%u_[%zu];\n",
	    indent, "", pad++, m->m_offset - offset);
    offset = m->m_offset;
  }
  printType(f, m, indent, offset, pad, parent, isFixed, isLast, topSeq);
  //  printf("1name %s offset %u m->m_offset %u bytes %u\n",
  //	 m->m_name.c_str(), offset, m->m_offset, m->m_nBytes);
  fprintf(f, " %s", m->m_name.c_str());
  printArray(f, m, isFixed, isLast, topSeq);
  offset += m->m_nBytes;
  //  printf("2name %s offset %u m->m_offset %u bytes %u\n",
  //	 m->m_name.c_str(), offset, m->m_offset, m->m_nBytes);
}

static const char *
methodName(Worker *w, const char *method, const char *&mName) {
  const char *pat = w->m_pattern ? w->m_pattern : w->m_staticPattern;
  if (!pat) {
    mName = method;
    return 0;
  }
  size_t length =
    strlen(w->m_implName) + strlen(method) + strlen(pat) + 1;
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
      strcpy(s, w->m_implName);
      while (*s)
	s++;
      break;
    case 'W':
      *s++ = (char)toupper(w->m_implName[0]);
      strcpy(s, w->m_implName + 1);
      while (*s)
	s++;
      break;
    default:
      return OU::esprintf("Invalid pattern rule: %s", w->m_pattern);
    }
  }
  *s++ = 0;
  return 0;
}

static void
emitStructRCC(FILE *f, size_t nMembers, OU::Member *members, unsigned indent,
	      const char *parent, bool isFixed, bool &isLast, bool topSeq) {
  size_t offset = 0;
  unsigned pad = 0;
  for (unsigned n = 0; !isLast && n < nMembers; n++, members++)
    printMember(f, members, indent, offset, pad, parent, isFixed, isLast, topSeq);
}

const char *Worker::
rccValue(OU::Value &v, std::string &value) {
  value = "(";
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
      v.unparse(value, true);
      break;
    case OA::OCPI_ULongLong:
      // This can be bad unless we force it to ull since decimal is assumed signed
      // FIXME: perhaps make this an option in the unparser? C++?
      v.unparse(value, true);
      value += "ull";
      break;
    case OA::OCPI_String:
      value += '\"';
      if (v.m_String && v.m_String[0])
	v.unparse(value, true);
      value += '\"';
      break;
    case OA::OCPI_Enum:
      // Should we define an enum here?
    default:
      ;
    }
  value += ")";
  return value.c_str();
}
const char *Worker::
rccPropValue(OU::Property &p, std::string &value) {
  if (p.m_default)
    return rccValue(*p.m_default, value);
  OU::format(value, "("); // , rccTypes[p.m_baseType]);
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
  value += ")";
  return value.c_str();
}

static void
cc_type(OU::Property &p, std::string &typeDef, std::string &type) {
  (void)p;(void)typeDef;(void)type;
}

const char *Worker::
emitImplRCC() {
  const char *err;
  FILE *f;
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "", RCCIMPL, HEADER, m_implName, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", m_file.c_str());
  fprintf(f, " */\n");
  char *upper = upperdup(m_implName);
  fprintf(f,
	  "\n"
	  "/* This file contains the implementation declarations for worker %s */\n\n"
	  "#ifndef RCC_WORKER_%s_H__\n"
	  "#define RCC_WORKER_%s_H__\n"
	  "#include <RCC_Worker.h>\n"
	  "#if defined (__cplusplus)\n"
	  "extern \"C\" {\n"
	  "#endif\n",
	  m_implName, upper, upper);
  const char *last;
  unsigned in = 0, out = 0;
  if (m_ports.size() && m_language == C) {
    fprintf(f,
	    "/*\n"
	    " * Enumeration of port ordinals for worker %s\n"
	    " */\n"
	    "typedef enum {\n",
	    m_implName);
    last = "";
    for (unsigned n = 0; n < m_ports.size(); n++) {
      Port *port = m_ports[n];
      fprintf(f, "%s %s_%s", last, upper, upperdup(port->name));
      // FIXME TWO WAY
      last = ",\n";
      if (port->u.wdi.isProducer)
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
	    " * Definitions for default values of parameter properties.\n"
	    " * Parameters are defined as macros with no arguments to catch spelling errors.\n" 
	    " */\n");
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      OU::Property &p = **pi;
      if (p.m_isParameter) {
	std::string value;
	if (m_language == C)
	  fprintf(f,
		  "#ifndef PARAM_%s\n"
		  "#define PARAM_%s() %s\n"
		  "#endif\n",
		  p.m_name.c_str(), p.m_name.c_str(), rccPropValue(p, value));
	else {
	  std::string typeDef, type;
	  cc_type(p, typeDef, type);
	  fprintf(f,
		  "%sstatic const %s PARAM_%s = %s",
		  typeDef.c_str(), type.c_str(), p.m_name.c_str(), rccPropValue(p, value));
	}
      }
    }
  }
  if (m_ctl.nRunProperties) {
    fprintf(f,
	    "/*\n"
	    " * Property structure for worker %s\n"
	    " */\n"
	    "typedef struct {\n",
	    m_implName);
    unsigned pad = 0;
    size_t offset = 0;
    bool isLastDummy = false;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
      if (!(*pi)->m_isParameter)
	printMember(f, *pi, 2, offset, pad, m_implName, true, isLastDummy, false);
    fprintf(f, "} %c%sProperties;\n\n", toupper(m_implName[0]), m_implName + 1);
  }
  const char *mName;
  if ((err = methodName(this, "run", mName)))
    return err;
  fprintf(f,
	  "/*\n"
	  " * Use this macro, followed by a semicolon, to declare methods before\n"
	  " * implementing them, and before defining the RCCDispatch for the worker\n"
	  " * e.g.:\n"
	  " * %s_METHOD_DECLARATIONS;\n"
	  " */\n"
	  "#define %s_METHOD_DECLARATIONS \\\n",
	  upper, upper);
  unsigned op = 0;
  const char **cp;
  if (m_ctl.controlOps) {
    last = "";
    fprintf(f, " %s RCCMethod ", m_pattern ? "extern" : "static");
    for (cp = OU::Worker::s_controlOpNames; *cp; cp++, op++)
      if (m_ctl.controlOps & (1 << op)) {
	if ((err = methodName(this, *cp, mName)))
	  return err;
	fprintf(f, "%s%s", last, mName);
	last = ", ";
      }
    fprintf(f, ";\\\n");
  }
  if ((err = methodName(this, "run", mName)))
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
  for (op = 0, cp = OU::Worker::s_controlOpNames; *cp; cp++, op++)
    if (m_ctl.controlOps & (1 << op)) {
      if ((err = methodName(this, *cp, mName)))
	return err;
      fprintf(f, " .%s = %s,\\\n", *cp, mName);
    }
  if ((err = methodName(this, "run", mName)))
    return err;
  fprintf(f, " .run = %s,\\\n", mName);
  uint32_t optionals = 0;
  for (unsigned n = 0; n < m_ports.size(); n++) {
    Port *port = m_ports[n];
    if (port->u.wdi.isOptional)
      optionals |= 1 << n;
  }
  if (optionals)
    fprintf(f, " .optionallyConnectedPorts = 0x%x,\\\n", optionals);
  fprintf(f, "/**/\n");
  if (m_ports.size()) {
    // First generate the protocol enumerations
    for (unsigned n = 0; n < m_ports.size(); n++) {
      Port *port = m_ports[n];
      if (port->protocol->operations()) {
	unsigned nn;
	for (nn = 0; nn < n; nn++) {
	  Port *pp = m_ports[nn];
	  if (pp->protocol->operations() &&
	      !strcasecmp(pp->protocol->m_name.c_str(),
			  port->protocol->m_name.c_str()))
	    break;
	}
	if (nn >= n) {
	  OU::Protocol *prot = port->protocol;
	  fprintf(f,
		  "/*\n"
		  " * Enumeration of operations for protocol %s (%s)\n"
		  " */\n"
		  "typedef enum {\n",
		  prot->m_name.c_str(), prot->m_qualifiedName.c_str());
	  OU::Operation *o = prot->operations();
	  char *puName = upperdup(prot->m_name.c_str());
	  for (unsigned no = 0; no < prot->nOperations(); no++, o++) {
	    char *ouName = upperdup(o->name().c_str());
	    fprintf(f, " %s_%s,\n", puName, ouName);
	    free((void*)ouName);
	  }
	  free(puName);
	  fprintf(f, "} %c%sOperation;\n",
		  toupper(*prot->m_name.c_str()),
		  prot->m_name.c_str() + 1);
	}
      }
    }
    for (unsigned n = 0; n < m_ports.size(); n++) {
      Port *port = m_ports[n];
      if (port->protocol->operations()) {
	fprintf(f,
		"/*\n"
		" * Enumeration of operations on port %s of worker %s\n"
		" */\n"
		"typedef enum {\n",
		port->name, m_implName);
	OU::Operation *o = port->protocol->operations();
	char *puName = upperdup(port->name);
	for (unsigned nn = 0; nn < port->protocol->nOperations(); nn++, o++) {
	  char *ouName = upperdup(o->name().c_str());
	  fprintf(f, " %s_%s_%s,\n", upper, puName, ouName);
	  free(ouName);
	}
	free(puName);
	fprintf(f, "} %c%s%c%sOperation;\n",
		toupper(m_implName[0]), m_implName+1,
		toupper(port->name[0]), port->name+1);
	// Now emit structs for messages
	o = port->protocol->operations();
	for (unsigned nn = 0; nn < port->protocol->nOperations(); nn++, o++)
	  if (o->nArgs()) {
	    fprintf(f,
		    "/*\n"
		    " * Structure for the %s operation on port %s\n"
		    " */\n"
		    "typedef struct __attribute__ ((__packed__)) {\n",
		    o->name().c_str(), port->name);
	    std::string s;
	    camel(s, m_implName, port->name, o->name().c_str());
	    bool isLast = false;
	    emitStructRCC(f, o->nArgs(), o->args(), 2, s.c_str(), false, isLast, o->isTopFixedSequence());
	    fprintf(f, "} %s;\n", s.c_str());
	  }
      }
    }
  }
  fprintf(f,
"\n"
"#if defined (__cplusplus)\n"
"}\n"
"#endif\n"
	  "#endif /* ifndef RCC_WORKER_%s_H__ */\n",
	  upper);
  fclose(f);
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "", RCCMAP, HEADER, NULL, f)))
    return err;
  fprintf(f, "#define RCC_FILE_WORKER_%s %s\n", m_fileName.c_str(), m_implName);
  fclose(f);
  return 0;
}

const char*
emitSkelRCC(Worker *w) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->m_fileName.c_str(), w->m_outDir, "", "-skel", ".c", NULL, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", w->m_file.c_str(), true);
  fprintf(f, " *\n");
  const char *upper = upperdup(w->m_implName);
  fprintf(f,
" * This file contains the RCC implementation skeleton for worker: %s\n"
	  " */\n"
"#include \"%s_Worker.h\"\n\n"

"%s_METHOD_DECLARATIONS;\n"
"RCCDispatch %s = {\n"
	  " /* insert any custom initializations here */\n"
" %s_DISPATCH\n"
"};\n\n"
"/*\n"
" * Methods to implement for worker %s, based on metadata.\n"
	  " */\n",
	  w->m_implName, w->m_implName, upper, w->m_implName, upper, w->m_implName);
  unsigned op = 0;
  const char **cp;
  const char *mName;
  for (cp = OU::Worker::s_controlOpNames; *cp; cp++, op++)
    if (w->m_ctl.controlOps & (1 << op)) {
      if ((err = methodName(w, *cp, mName)))
	return err;
      fprintf(f,
"\n"
"%s RCCResult\n%s(RCCWorker *self) {\n"
" return RCC_OK;\n"
	      "}\n",
	      w->m_pattern ? "extern" : "static", mName);
    }
  if ((err = methodName(w, "run", mName)))
    return err;
  fprintf(f,
"\n"
"%s RCCResult\n%s(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {\n"
" (void)self;(void)timedOut;(void)newRunCondition;\n"
" return RCC_ADVANCE;\n"
	  "}\n",
	  w->m_pattern ? "extern" : "static", mName);
  // FIXME PortMemberMacros?
  // FIXME Compilable - any initial functionality??? cool.
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
			    "ControlOperations", "Language", (void*)0)) ||
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
  if ((err = parseList(ezxml_cattr(m_xml, "ControlOperations"), parseControlOp, this)))
    return err;
  // Parse data port implementation metadata: maxlength, minbuffers.
  for (ezxml_t x = ezxml_cchild(m_xml, "Port"); x; x = ezxml_next(x)) {
    if ((err = OE::checkAttrs(x, "Name", "MinBuffers", "MinBufferCount", "BufferSize", (void*)0)))
      return err;
    const char *name = ezxml_cattr(x, "Name");
    if (!name)
      return "Missing \"Name\" attribute on Port element if RccWorker";
    Port *p = 0; // kill warning
    unsigned n;
    for (n = 0; n < m_ports.size(); n++) {
      p = m_ports[n];
      if (!strcasecmp(p->name, name))
        break;
    }
    if (n >= m_ports.size())
      return OU::esprintf("No DataInterface named \"%s\" from Port element", name);
    if ((err = OE::getNumber(x, "MinBuffers", &p->u.wdi.minBufferCount, 0, 0)) || // backward compat
        (err = OE::getNumber(x, "MinBufferCount", &p->u.wdi.minBufferCount, 0, p->u.wdi.minBufferCount)) ||
        (err = OE::getNumber(x, "Buffersize", &p->u.wdi.bufferSize, 0,
			     p->protocol ? p->protocol->m_defaultBufferSize : 0)))
      return err;
  }
  m_model = RccModel;
  m_modelString = "rcc";
  return 0;
}

// RCC assemblies are collections of reusable instances with no connections.
const char *Worker::
parseRccAssy() {
  const char *err;
  Assembly *a = m_assembly = new Assembly(*this);
  static const char
    *topAttrs[] = {IMPL_ATTRS, RCC_TOP_ATTRS, RCC_IMPL_ATTRS, NULL},
    *instAttrs[] = {INST_ATTRS, "reusable", NULL};
  // Do the generic assembly parsing, then to more specific to HDL
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
  : Worker(xml, xfile, NULL, NULL, err) {
  if (!(err = OE::checkAttrs(xml, IMPL_ATTRS, RCC_TOP_ATTRS, (void*)0)) &&
      !(err = OE::checkElements(xml, IMPL_ELEMS, RCC_IMPL_ELEMS, ASSY_ELEMS, (void*)0)))
    err = parseRcc();
}

RccAssembly::
~RccAssembly() {
}

const char *RccAssembly::
emitArtXML(const char */*wksfile*/) {
  const char *err;
  FILE *f;
  if ((err = openOutput(m_fileName.c_str(), m_outDir, "", "_art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", m_file.c_str());
  fprintf(f,
	  " This file contains the artifact descriptor XML for the application assembly\n"
	  " named \"%s\". It must be attached (appended) to the shared object file\n",
	  m_implName);
  fprintf(f, " -->\n");
  // This assumes native compilation of course
  fprintf(f,
	  "<artifact os=\"%s\" osVersion=\"%s\" platform=\"%s\" "
	  "runtime=\"%s\" runtimeVersion=\"%s\" "
	  "tool=\"%s\" toolVersion=\"%s\">\n",
	  os, os_version, platform,
	  "", "", "", "");
  emitWorkers(f);
  fprintf(f, "</artifact>\n");
  if (fclose(f))
    return "Could not close output file. No space?";
  return 0;
}
