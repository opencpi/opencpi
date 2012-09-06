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

static const char *
upperdup(const char *s) {
  char *upper = (char *)malloc(strlen(s) + 1);
  for (char *u = upper; (*u++ = toupper(*s)); s++)
    ;
  return upper;
}

static const char *rccTypes[] = {"none",
				 "RCCBoolean", "RCCChar", "RCCDouble", "RCCFloat", "int16_t", "int32_t", "uint8_t",
				 "uint32_t", "uint16_t", "int64_t", "uint64_t", "RCCChar" };

static void camel(std::string &s, const char *s1, const char *s2 = NULL, const char *s3 = NULL) {
  s = toupper(s1[0]);
  s += s1 + 1;
  if (s2) {
    s += toupper(s2[0]);
    s += s2 + 1;
  }
  if (s3) {
    s += toupper(s3[0]);
    s += s3 + 1;
  }
}

static void
emitStructRCC(FILE *f, unsigned nMembers, OU::Member *members, unsigned indent,
	      const char *parent, bool isFixed, bool &isLast, bool topSeq);
static void
printMember(FILE *f, OU::Member *m, unsigned indent, unsigned &offset, unsigned &pad,
	    const char *parent, bool isFixed, bool &isLast, bool topSeq);

static void printArray(FILE *f, OU::Member *m, bool isFixed, bool &isLast, bool topSeq) {
  if (m->m_arrayRank)
    for (unsigned n = 0; n < m->m_arrayRank; n++)
      fprintf(f, "[%u]", m->m_arrayDimensions[n]);
  if (topSeq) {
    if (m->m_sequenceLength)
      fprintf(f, "[%u]", m->m_sequenceLength);
    else
      fprintf(f, "[1]");
  }
  // Although we align strings on a 4 byte boundary, we don't pad them to anything.
  if (m->m_baseType == OA::OCPI_String) {
    fprintf(f, "[%u]", isFixed ? m->m_stringLength + 1 : 1);
    if (m->m_stringLength == 0)
      isLast = true;
  }
  // End of declarator. If we're a sequence we close off the struct.
  if (topSeq)
    fprintf(f, "; /* this is a top level sequence of fixed size elements */ \n");
  else
    fprintf(f, ";\n");
}

// Print type, including sequence type etc.
static void
printType(FILE *f, OU::Member *m, unsigned indent, unsigned &offset, unsigned &pad,
	  const char *parent, bool isFixed, bool &isLast, bool topSeq);
// Just print the data type, not the "member", with names or arrays etc.
static void
printBaseType(FILE *f, OU::Member *m, unsigned indent, unsigned &offset, unsigned &pad,
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
printType(FILE *f, OU::Member *m, unsigned indent, unsigned &offset, unsigned &pad,
	  const char *parent, bool isFixed, bool &isLast, bool topSeq) {
  if (m->m_isSequence && !topSeq) {
    fprintf(f,
	    "%*sstruct {\n"
	    "%*s uint32_t length; /* offset %4u, 0x%x */\n",
	    indent, "", indent, "", offset, offset);
    offset += 4;
    if (m->m_dataAlign > sizeof(uint32_t)) {
      unsigned align = m->m_dataAlign - (unsigned)sizeof(uint32_t);
      fprintf(f, "%*s char pad%u_[%u];\n", indent, "", pad++, align);
      offset += align;
    }
    printBaseType(f, m, indent + 2, offset, pad, parent, isFixed, isLast);
    fprintf(f, " data");
    if (m->m_sequenceLength && isFixed)
      fprintf(f, "[%u]", m->m_sequenceLength);
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
printMember(FILE *f, OU::Member *m, unsigned indent, unsigned &offset, unsigned &pad,
	    const char *parent, bool isFixed, bool &isLast, bool topSeq)
{
  if (offset < m->m_offset) {
    fprintf(f, "%*schar pad%u_[%u];\n",
	    indent, "", pad++, m->m_offset - offset);
    offset = m->m_offset;
  }
  printType(f, m, indent, offset, pad, parent, isFixed, isLast, topSeq);
  fprintf(f, " %s", m->m_name.c_str());
  printArray(f, m, isFixed, isLast, topSeq);
  offset += m->m_nBytes;
}

static const char *
methodName(Worker *w, const char *method, const char *&mName) {
  const char *pat = w->pattern ? w->pattern : w->staticPattern;
  if (!pat) {
    mName = method;
    return 0;
  }
  unsigned length =
    strlen(w->implName) + strlen(method) + strlen(pat) + 1;
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
      *s++ = toupper(method[0]);
      strcpy(s, method + 1);
      while (*s)
	s++;
      break;
    case 'w':
      strcpy(s, w->implName);
      while (*s)
	s++;
      break;
    case 'W':
      *s++ = toupper(w->implName[0]);
      strcpy(s, w->implName + 1);
      while (*s)
	s++;
      break;
    default:
      return OU::esprintf("Invalid pattern rule: %s", w->pattern);
    }
  }
  *s++ = 0;
  return 0;
}

static void
emitStructRCC(FILE *f, unsigned nMembers, OU::Member *members, unsigned indent,
	      const char *parent, bool isFixed, bool &isLast, bool topSeq) {
  unsigned offset = 0, pad = 0;
  for (unsigned n = 0; !isLast && n < nMembers; n++, members++)
    printMember(f, members, indent, offset, pad, parent, isFixed, isLast, topSeq);
}

const char *
emitImplRCC(Worker *w, const char *outDir, const char *library) {
  (void)library;
  const char *err;
  FILE *f;
  if ((err = openOutput(w->fileName, outDir, "", RCCIMPL, HEADER, w->implName, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", w->file);
  fprintf(f, " */\n");
  const char *upper = upperdup(w->implName);
  fprintf(f,
	  "\n"
	  "/* This file contains the implementation declarations for worker %s */\n\n"
	  "#ifndef RCC_WORKER_%s_H__\n"
	  "#define RCC_WORKER_%s_H__\n"
	  "#include <RCC_Worker.h>\n"
	  "#if defined (__cplusplus)\n"
	  "extern \"C\" {\n"
	  "#endif\n",
	  w->implName, upper, upper);
  const char *last;
  unsigned in = 0, out = 0;
  if (w->ports.size()) {
    fprintf(f,
	    "/*\n"
	    " * Enumeration of port ordinals for worker %s\n"
	    " */\n"
	    "typedef enum {\n",
	    w->implName);
    last = "";
    for (unsigned n = 0; n < w->ports.size(); n++) {
      Port *port = w->ports[n];
      fprintf(f, "%s %s_%s", last, upper, upperdup(port->name));
      // FIXME TWO WAY
      last = ",\n";
      if (port->u.wdi.isProducer)
	out++;
      else
	in++;
    }
    fprintf(f, "\n} %c%sPort;\n", toupper(w->implName[0]), w->implName+1);
  }
  fprintf(f, "#define %s_N_INPUT_PORTS %u\n"
	  "#define %s_N_OUTPUT_PORTS %u\n",
	  upper, in, upper, out);
  if (w->ctl.properties.size()) {
    fprintf(f,
"/*\n"
" * Property structure for worker %s\n"
	    " */\n"
	    "typedef struct {\n",
	    w->implName);
    unsigned pad = 0, offset = 0;
    bool isLastDummy = false;
    for (PropertiesIter pi = w->ctl.properties.begin(); pi != w->ctl.properties.end(); pi++)
      printMember(f, *pi, 2, offset, pad, w->implName, true, isLastDummy, false);
    fprintf(f, "} %c%sProperties;\n\n", toupper(w->implName[0]), w->implName + 1);
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
  if (w->ctl.controlOps) {
    last = "";
    fprintf(f, " %s RCCMethod ", w->pattern ? "extern" : "static");
    for (cp = controlOperations; *cp; cp++, op++)
      if (w->ctl.controlOps & (1 << op)) {
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
	  w->pattern ? "extern" : "static", mName);
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
	  w->implName, upper, upper, upper, upper, upper, w->isThreaded ? 1 : 0);
  if (w->ctl.properties.size())
    fprintf(f, " .propertySize = sizeof(%c%sProperties),\\\n",
	    toupper(w->implName[0]), w->implName + 1);
  for (op = 0, cp = controlOperations; *cp; cp++, op++)
    if (w->ctl.controlOps & (1 << op)) {
      if ((err = methodName(w, *cp, mName)))
	return err;
      fprintf(f, " .%s = %s,\\\n", *cp, mName);
    }
  if ((err = methodName(w, "run", mName)))
    return err;
  fprintf(f, " .run = %s,\\\n", mName);
  uint32_t optionals = 0;
  for (unsigned n = 0; n < w->ports.size(); n++) {
    Port *port = w->ports[n];
    if (port->u.wdi.isOptional)
      optionals |= 1 << n;
  }
  if (optionals)
    fprintf(f, " .optionallyConnectedPorts = 0x%x,\\\n", optionals);
  fprintf(f, "/**/\n");
  if (w->ports.size()) {
    for (unsigned n = 0; n < w->ports.size(); n++) {
      Port *port = w->ports[n];
      if (port->protocol->operations()) {
	fprintf(f,
"/*\n"
" * Enumeration of operations on port %s of worker %s\n"
		" */\n"
		"typedef enum {\n",
		port->name, w->implName);
	OU::Operation *o = port->protocol->operations();
	const char *puName = upperdup(port->name);
	for (unsigned nn = 0; nn < port->protocol->nOperations(); nn++, o++)
	  fprintf(f, " %s_%s_%s,\n", upper, puName, upperdup(o->name().c_str()));
	fprintf(f, "} %c%s%c%sOperation;\n",
		toupper(w->implName[0]), w->implName+1,
		toupper(port->name[0]), port->name+1);
	// Now emit structs for messages
	o = port->protocol->operations();
	for (unsigned nn = 0; nn < port->protocol->nOperations(); nn++, o++)
	  if (o->nArgs()) {
	    fprintf(f,
"/*\n"
" * Structure for the %s operation on port %s\n"
		    " */\n"
		    "typedef struct {\n",
		    o->name().c_str(), port->name);
	    std::string s;
	    camel(s, w->implName, port->name, o->name().c_str());
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
  if ((err = openOutput(w->fileName, outDir, "", RCCMAP, HEADER, NULL, f)))
    return err;
  fprintf(f, "#define RCC_FILE_WORKER_%s %s\n", w->fileName, w->implName);
  fclose(f);
  return 0;
}

const char*
emitSkelRCC(Worker *w, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->fileName, outDir, "", "_skel", ".c", NULL, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", w->file, true);
  fprintf(f, " *\n");
  const char *upper = upperdup(w->implName);
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
	  w->implName, w->implName, upper, w->implName, upper, w->implName);
  unsigned op = 0;
  const char **cp;
  const char *mName;
  for (cp = controlOperations; *cp; cp++, op++)
    if (w->ctl.controlOps & (1 << op)) {
      if ((err = methodName(w, *cp, mName)))
	return err;
      fprintf(f,
"\n"
"%s RCCResult\n%s(RCCWorker *self) {\n"
" return RCC_OK;\n"
	      "}\n",
	      w->pattern ? "extern" : "static", mName);
    }
  if ((err = methodName(w, "run", mName)))
    return err;
  fprintf(f,
"\n"
"%s RCCResult\n%s(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {\n"
" (void)self;(void)timedOut;(void)newRunCondition;\n"
" return RCC_ADVANCE;\n"
	  "}\n",
	  w->pattern ? "extern" : "static", mName);
  // FIXME PortMemberMacros?
  // FIXME Compilable - any initial functionality??? cool.
  fclose(f);
  return 0;
}
const char *
emitArtRCC(Worker *aw, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(aw->fileName, outDir, "", "_art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", aw->file);
  fprintf(f,
" This file contains the artifact descriptor XML for the application assembly\n"
	  " named \"%s\". It must be attached (appended) to the shared object file\n",
	  aw->implName);
  fprintf(f, " -->\n");
  // This assumes native compilation of course
  fprintf(f,
"<artifact os=\"%s\" osVersion=\"%s\" platform=\"%s\" "
"runtime=\"%s\" runtimeVersion=\"%s\" "
	  "tool=\"%s\" toolVersion=\"%s\">\n",
	  OCPI_CPP_STRINGIFY(OCPI_OS) + strlen("OCPI"),
	  OCPI_CPP_STRINGIFY(OCPI_OS_VERSION),
	  OCPI_CPP_STRINGIFY(OCPI_PLATFORM),
	  "", "", "", "");
#if 0
  OCPI_CPP_STRINGIFY(OCPI_RUNTIME),
    OCPI_CPP_STRINGIFY(OCPI_RUNTIME_VERSION),
    OCPI_CPP_STRINGIFY(OCPI_TOOL),
    OCPI_CPP_STRINGIFY(OCPI_TOOL_VERSION));
#endif
// Define all workers
for (WorkersIter wi = aw->assembly.workers.begin();
     wi != aw->assembly.workers.end(); wi++)
  emitWorker(f, *wi);
fprintf(f, "</artifact>\n");
if (fclose(f))
  return "Could close output file. No space?";
return 0;
}

