/*
 * Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 * Mercury Federal Systems, Incorporated
 * 1901 South Bell Street
 * Suite 402
 * Arlington, Virginia 22202
 * United States of America
 * Telephone 703-413-0781
 * FAX 703-413-0784
 *
 * This file is part of OpenCPI (www.opencpi.org).
 * ____ __________ ____
 * / __ \____ ___ ____ / ____/ __ \ / _/ ____ _________ _
 * / / / / __ \/ _ \/ __ \/ / / /_/ / / / / __ \/ ___/ __ `/
 * / /_/ / /_/ / __/ / / / /___/ ____/_/ / _/ /_/ / / / /_/ /
 * \____/ .___/\___/_/ /_/\____/_/ /___/(_)____/_/ \__, /
 * /_/ /____/
 *
 * OpenCPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenCPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenCPI. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <sys/time.h>
#include "OcpiUtilCppMacros.h"
#include "OcpiUtilMisc.h"
#include "wip.h"

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

const char *
emitImplRCC(Worker *w, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->m_fileName.c_str(), outDir, "", RCCIMPL, HEADER, w->m_implName, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", w->m_file.c_str());
  fprintf(f, " */\n");
  char *upper = upperdup(w->m_implName);
  fprintf(f,
	  "\n"
	  "/* This file contains the implementation declarations for worker %s */\n\n"
	  "#ifndef RCC_WORKER_%s_H__\n"
	  "#define RCC_WORKER_%s_H__\n"
	  "#include <RCC_Worker.h>\n"
	  "#if defined (__cplusplus)\n"
	  "extern \"C\" {\n"
	  "#endif\n",
	  w->m_implName, upper, upper);
  const char *last;
  unsigned in = 0, out = 0;
  if (w->m_ports.size()) {
    fprintf(f,
	    "/*\n"
	    " * Enumeration of port ordinals for worker %s\n"
	    " */\n"
	    "typedef enum {\n",
	    w->m_implName);
    last = "";
    for (unsigned n = 0; n < w->m_ports.size(); n++) {
      Port *port = w->m_ports[n];
      fprintf(f, "%s %s_%s", last, upper, upperdup(port->name));
      // FIXME TWO WAY
      last = ",\n";
      if (port->u.wdi.isProducer)
	out++;
      else
	in++;
    }
    fprintf(f, "\n} %c%sPort;\n", toupper(w->m_implName[0]), w->m_implName+1);
  }
  fprintf(f, "#define %s_N_INPUT_PORTS %u\n"
	  "#define %s_N_OUTPUT_PORTS %u\n",
	  upper, in, upper, out);
  if (w->m_ctl.properties.size()) {
    fprintf(f,
	    "/*\n"
	    " * Property structure for worker %s\n"
	    " */\n"
	    "typedef struct {\n",
	    w->m_implName);
    unsigned pad = 0;
    size_t offset = 0;
    bool isLastDummy = false;
    for (PropertiesIter pi = w->m_ctl.properties.begin(); pi != w->m_ctl.properties.end(); pi++)
      printMember(f, *pi, 2, offset, pad, w->m_implName, true, isLastDummy, false);
    fprintf(f, "} %c%sProperties;\n\n", toupper(w->m_implName[0]), w->m_implName + 1);
  }
  const char *mName;
  if ((err = methodName(w, "run", mName)))
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
  if (w->m_ctl.controlOps) {
    last = "";
    fprintf(f, " %s RCCMethod ", w->m_pattern ? "extern" : "static");
    for (cp = OU::controlOpNames; *cp; cp++, op++)
      if (w->m_ctl.controlOps & (1 << op)) {
	if ((err = methodName(w, *cp, mName)))
	  return err;
	fprintf(f, "%s%s", last, mName);
	last = ", ";
      }
    fprintf(f, ";\\\n");
  }
  if ((err = methodName(w, "run", mName)))
    return err;
  fprintf(f,
	  " %s RCCRunMethod %s\\\n",
	  w->m_pattern ? "extern" : "static", mName);
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
	  w->m_implName, upper, upper, upper, upper, upper, w->m_isThreaded ? 1 : 0);
  if (w->m_ctl.properties.size())
    fprintf(f, " .propertySize = sizeof(%c%sProperties),\\\n",
	    toupper(w->m_implName[0]), w->m_implName + 1);
  for (op = 0, cp = OU::controlOpNames; *cp; cp++, op++)
    if (w->m_ctl.controlOps & (1 << op)) {
      if ((err = methodName(w, *cp, mName)))
	return err;
      fprintf(f, " .%s = %s,\\\n", *cp, mName);
    }
  if ((err = methodName(w, "run", mName)))
    return err;
  fprintf(f, " .run = %s,\\\n", mName);
  uint32_t optionals = 0;
  for (unsigned n = 0; n < w->m_ports.size(); n++) {
    Port *port = w->m_ports[n];
    if (port->u.wdi.isOptional)
      optionals |= 1 << n;
  }
  if (optionals)
    fprintf(f, " .optionallyConnectedPorts = 0x%x,\\\n", optionals);
  fprintf(f, "/**/\n");
  if (w->m_ports.size()) {
    // First generate the protocol enumerations
    for (unsigned n = 0; n < w->m_ports.size(); n++) {
      Port *port = w->m_ports[n];
      if (port->protocol->operations()) {
	unsigned nn;
	for (nn = 0; nn < n; nn++) {
	  Port *pp = w->m_ports[nn];
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
    for (unsigned n = 0; n < w->m_ports.size(); n++) {
      Port *port = w->m_ports[n];
      if (port->protocol->operations()) {
	fprintf(f,
		"/*\n"
		" * Enumeration of operations on port %s of worker %s\n"
		" */\n"
		"typedef enum {\n",
		port->name, w->m_implName);
	OU::Operation *o = port->protocol->operations();
	char *puName = upperdup(port->name);
	for (unsigned nn = 0; nn < port->protocol->nOperations(); nn++, o++) {
	  char *ouName = upperdup(o->name().c_str());
	  fprintf(f, " %s_%s_%s,\n", upper, puName, ouName);
	  free(ouName);
	}
	free(puName);
	fprintf(f, "} %c%s%c%sOperation;\n",
		toupper(w->m_implName[0]), w->m_implName+1,
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
	    camel(s, w->m_implName, port->name, o->name().c_str());
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
  if ((err = openOutput(w->m_fileName.c_str(), outDir, "", RCCMAP, HEADER, NULL, f)))
    return err;
  fprintf(f, "#define RCC_FILE_WORKER_%s %s\n", w->m_fileName.c_str(), w->m_implName);
  fclose(f);
  return 0;
}

const char*
emitSkelRCC(Worker *w, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->m_fileName.c_str(), outDir, "", "-skel", ".c", NULL, f)))
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
  for (cp = OU::controlOpNames; *cp; cp++, op++)
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
const char *
emitArtRCC(Worker *aw, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(aw->m_fileName.c_str(), outDir, "", "_art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", aw->m_file.c_str());
  fprintf(f,
" This file contains the artifact descriptor XML for the application assembly\n"
	  " named \"%s\". It must be attached (appended) to the shared object file\n",
	  aw->m_implName);
  fprintf(f, " -->\n");
  // This assumes native compilation of course
  fprintf(f,
"<artifact os=\"%s\" osVersion=\"%s\" platform=\"%s\" "
"runtime=\"%s\" runtimeVersion=\"%s\" "
	  "tool=\"%s\" toolVersion=\"%s\">\n",
	  os, os_version, platform,
	  "", "", "", "");
#if 0
// Define all workers
for (WorkersIter wi = aw->m_assembly.m_workers.begin();
     wi != aw->m_assembly.m_workers.end(); wi++)
  emitWorker(f, *wi);
#else
 aw->emitWorkers(f);
#endif
fprintf(f, "</artifact>\n");
if (fclose(f))
  return "Could close output file. No space?";
return 0;
}

