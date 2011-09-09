/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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
 *  OpenCPI is free software: you can redistribute it and/or modify
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <sys/time.h>
#include "OcpiUtilCppMacros.h"
#include "wip.h"
// Generate the readonly implementation file.
// What implementations must explicitly (verilog) or implicitly (VHDL) include.
#define HEADER ".h"
#define OCLIMPL "_Worker"
#define SOURCE ".cl"
#define OCLENTRYPOINT "_entry_point"

static const char *
upperdup(const char *s) {
  char *upper = (char *)malloc(strlen(s) + 1);
  for (char *u = upper; (*u++ = toupper(*s)); s++)
    ;
  return upper;
}

static const char* emitEntryPointOCL ( Worker* w,
                                       const char* outDir );

static const char *oclTypes[] = {"none",
  "OCLBoolean", "OCLChar", "OCLDouble", "OCLFloat", "int16_t", "int32_t", "uint8_t",
  "uint32_t", "uint16_t", "int64_t", "uint64_t", "OCLChar" };

static void
printMember(FILE *f, CP::Member *t, const char *prefix, unsigned &offset, unsigned &pad)
{
  unsigned rem = offset & (t->align - 1);
  if (rem)
    fprintf(f, "%s  char         pad%u_[%u];\n",
	    prefix, pad++, t->align - rem);
  offset = roundup(offset, t->align);
  if (t->type.isSequence) {
    fprintf(f, "%s  uint32_t     %s_length;\n", prefix, t->name);
    if (t->align > sizeof(uint32_t))
      fprintf(f, "%s  char         pad%u_[%u];\n",
	      prefix, pad++, t->align - (unsigned)sizeof(uint32_t));
    offset += t->align;
  }
  fprintf(f, "%s  %-12s %s", prefix, oclTypes[t->type.scalar], t->name);
  if (t->type.scalar == CP::Scalar::OCPI_String)
    fprintf(f, "[%lu]", roundup(t->type.stringLength + 1, 4));
  if (t->type.isSequence || t->type.isArray)
    fprintf(f, "[%u]", t->type.length);
  fprintf(f, "; // offset %u, 0x%x\n", offset, offset);
  offset += t->nBytes;
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
	return esprintf("Invalid pattern rule: %s", w->pattern);
      }
  }
  *s++ = 0;
  return 0;
}

/*
  FIXME
  1. add support for local memory.
     a. Get local memory sizes from metadata
     b. Add local memory structures to the worker context

*/
static void
emitStructOCL(FILE *f, unsigned nMembers, CP::Member *members, const char *indent) {
  unsigned align = 0, pad = 0;
  for (unsigned n = 0; n < nMembers; n++, members++)
    printMember(f, members, indent, align, pad);
}

const char *
emitImplOCL(Worker *w, const char *outDir, const char *library) {
  (void)library;
  const char *err;
  FILE *f;
  if ((err = openOutput(w->fileName, outDir, "", OCLIMPL, HEADER, w->implName, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", w->file);
  fprintf(f, " */\n");
  const char *upper = upperdup(w->implName);
  fprintf(f,
          "\n"
          "/* This file contains the implementation declarations for worker %s */\n\n"
          "#ifndef OCL_WORKER_%s_H__\n"
          "#define OCL_WORKER_%s_H__\n\n"
          "#include <OCL_Worker.h>\n\n"
          "#if defined (__cplusplus)\n"
          "extern \"C\" {\n"
          "#endif\n\n",
          w->implName, upper, upper);

  if (w->ctl.nProperties) {
    fprintf(f,
            "/*\n"
            " * Property structure for worker %s\n"
            " */\n"
            "typedef struct {\n",
            w->implName);
    Property *p = w->ctl.properties;
    unsigned pad = 0, align = 0;
    for (unsigned n = 0; n < w->ctl.nProperties; n++, p++) {
      if (p->isStructSequence) {
        fprintf(f, "  uint32_t %s_length;\n", p->m_name);
        if (p->m_maxAlign > sizeof(uint32_t))
          fprintf(f, "  char pad%u_[%u];\n",
                  pad++, p->m_maxAlign - (unsigned)sizeof(uint32_t));
        align += p->m_maxAlign;
      }
      if (p->isStruct) {
        fprintf(f, "  struct %c%s%c%s {\n",
                toupper(w->implName[0]), w->implName+1,
                toupper(p->m_name[0]), p->m_name + 1);
        emitStructOCL(f, p->nMembers, p->members, "  ");
        fprintf(f, "  } %s", p->m_name);
        if (p->isStructSequence)
          fprintf(f, "[%u]", p->nStructs);
        fprintf(f, ";\n");
      } else
        printMember(f, p->members, "", align, pad);
    }
    fprintf(f,
            "\n} %c%sProperties;\n\n",
            toupper(w->implName[0]), w->implName + 1);

    fprintf(f,
            "/*\n"
            " * Worker context structure for worker %s\n"
            " */\n"
            "typedef struct {\n",
            w->implName);
    fprintf(f,"  __global %c%sProperties* properties;\n",
                  toupper(w->implName[0]), w->implName + 1);
    fprintf(f,"  OCLRunCondition runCondition;\n");

    if (w->localMemories.size()) {
      for (unsigned n = 0; n < w->localMemories.size(); n++) {
        LocalMemory* mem = w->localMemories[n];
        fprintf(f, "  __global void* %s;\n", mem->name );
      }
    }

    if (w->ports.size()) {
      for (unsigned n = 0; n < w->ports.size(); n++) {
        Port *port = w->ports[n];
        fprintf(f, "  OCLPort %s;\n", port->name );
        /* FIXME how do we deal with two-way ports */
      }
    }

    fprintf(f,
            "\n} OCLWorker%c%s;\n\n",
            toupper(w->implName[0]), w->implName + 1);
  }

  fprintf(f,
          "\n"
          "#if defined (__cplusplus)\n"
          "}\n"
          "#endif\n"
          "#endif /* ifndef OCL_WORKER_%s_H__ */\n",
          upper);
  fclose(f);

  return 0;
}

const char*
emitSkelOCL(Worker *w, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->fileName, outDir, "", "_skel", ".cl", NULL, f)))
    return err;
  fprintf(f, "/*\n");
  printgen(f, " *", w->file, true);
  fprintf(f, " *\n");
  fprintf(f,
	  " * This file contains the OCL implementation skeleton for worker: %s\n"
	  " */\n\n"
	  "#include \"%s_Worker.h\"\n\n"
	  "/*\n"
	  " * Required work group size for worker %s run() function.\n"
	  " */\n"
	  "#define OCL_WG_X 1\n"
	  "#define OCL_WG_Y 1\n"
	  "#define OCL_WG_Z 1\n\n"
	  "/*\n"
	  " * Methods to implement for worker %s, based on metadata.\n"
	  " */\n",
	  w->implName, w->implName, w->implName, w->implName);
  unsigned op = 0;
  const char **cp;
  const char *mName;
  for (cp = controlOperations; *cp; cp++, op++)
    if (w->ctl.controlOps & (1 << op)) {
      if ((err = methodName(w, *cp, mName)))
	return err;
      fprintf(f,
	      "\n"
	      "OCLResult %s_%s ( __local OCLWorker%c%s* self )\n{\n"
	      "\n  (void)self;\n"
	      "  return OCL_OK;\n"
	      "}\n",
	      w->implName,
	      mName,
        toupper(w->implName[0]),
        w->implName + 1 );
    }

  const size_t pad_len = 10 + strlen ( w->implName ) + strlen ( mName );
  char pad [ pad_len + 1 ];
  memset ( pad, ' ', pad_len );
  pad [ pad_len ] = '\0';

  if ((err = methodName(w, "run", mName)))
    return err;
  fprintf(f,
	  "\n"
	  "OCLResult %s_%s ( __local OCLWorker%c%s* self,\n"
	  "%sOCLBoolean timedOut,\n"
	  "%s__global OCLBoolean* newRunCondition )\n{\n"
	  "  (void)self;(void)timedOut;(void)newRunCondition;\n"
	  "  return OCL_ADVANCE;\n"
	  "}\n",
	  w->implName,
	  mName,
    toupper(w->implName[0]),
    w->implName + 1,
    pad,
    pad );

  fclose(f);

  return emitEntryPointOCL ( w, outDir );
}
/*
  FIXME
  1. Add local memory to metadata.
*/
const char *
emitArtOCL(Worker *aw, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(aw->implName, outDir, "", "_art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", aw->file);
  fprintf(f,
	  " This file contains the artifact descriptor XML for the application assembly\n"
	  " named \"%s\". It must be attached (appended) to the shared object file\n",
	  aw->implName);
  fprintf(f, "  -->\n");
  // This assumes native compilation of course
  fprintf(f,
	  "<artifact os=\"%s\" osVersion=\"%s\" platform=\"%s\" "
	  "runtime=\"%s\" runtimeVersion=\"%s\" "
	  "tool=\"%s\" toolVersion=\"%s\">\n",
	  OCPI_CPP_STRINGIFY(OCPI_OS) + strlen("OCPI"),
	  OCPI_CPP_STRINGIFY(OCPI_OS_VERSION),
	  OCPI_CPP_STRINGIFY(OCPI_PLATFORM),
	  "", "", "", "");
  // Define all workers
  Worker *w;
  unsigned n;
  for (w = aw->assembly.workers, n = 0; n < aw->assembly.nWorkers; n++, w++)
    emitWorker(f, w);
  fprintf(f, "</artifact>\n");
  if (fclose(f))
    return "Could close output file. No space?";
  return 0;
}

static const char* emitEntryPointOCL ( Worker* w,
                                       const char* outDir )
{
  const char* err;
  FILE* f;
  if ((err = openOutput(w->fileName, outDir, "", OCLENTRYPOINT, SOURCE, w->implName, f)))
    return err;
  fprintf(f, "\n\n/* ---- Generated code that dispatches to the worker's functions --------- */\n\n");

  fprintf ( f,
            "#ifndef DEFINED_OCPI_OCL_OPCODES\n"
            "#define DEFINED_OCPI_OCL_OPCODES 1\n"
            "typedef enum\n"
            "{\n"
            "  OCPI_OCL_INITIALIZE = 0,\n"
            "  OCPI_OCL_START,\n"
            "  OCPI_OCL_STOP,\n"
            "  OCPI_OCL_RELEASE,\n"
            "  OCPI_OCL_BEFORE_QUERY,\n"
            "  OCPI_OCL_AFTER_CONFIGURE,\n"
            "  OCPI_OCL_TEST,\n"
            "  OCPI_OCL_RUN\n\n"
            "} OcpiOclOpcodes_t;\n"
            "#endif\n\n" );

  fprintf ( f, "/* ----- Single function to dispatch both run() and control operations. -- */\n\n" );

  const size_t pad_len = 29 + strlen ( w->implName );
  char pad [ pad_len + 1 ];
  memset ( pad, ' ', pad_len );
  pad [ pad_len ] = '\0';

  fprintf ( f,
            "__kernel __attribute__((reqd_work_group_size(OCL_WG_X, OCL_WG_Y, OCL_WG_Z)))\n"
            "void %s_entry_point ( OcpiOclOpcodes_t opcode,\n"
            "%sOCLBoolean timedOut,\n"
            "%s__global void* properties,\n"
            "%s__global OCLRunCondition* runCondition,\n"
            "%s__global OCLBoolean* newRunCondition,\n",
            w->implName,
            pad,
            pad,
            pad,
            pad );

  if ( w->localMemories.size() )
  {
    for ( size_t n = 0; n < w->localMemories.size(); n++ )
    {
      fprintf ( f,
                "%s__global void* local_mem_%zu_data,\n",
                pad,
                n );
    }
  }

  if ( w->ports.size() )
  {
    for ( size_t n = 0; n < w->ports.size(); n++ )
    {
      Port* port = w->ports [ n ];
      fprintf ( f,
                "%s__global void* port_%s_data,\n"
                "%sunsigned int port_%s_max_length,\n"
                "%s__global OCLPortAttr* port_%s_attrs,\n",
                pad,
                port->name,
                pad,
                port->name,
                pad,
                port->name );
    }
  }

  fprintf ( f, "%s__global OCLResult* result )\n", pad );
  fprintf ( f, "{\n" );

  fprintf ( f, "  barrier ( CLK_GLOBAL_MEM_FENCE );\n\n" );

  fprintf ( f, "  /* ---- Only one worker runs control operations ------------------------ */\n\n" );

  fprintf ( f, "    if ( opcode != OCPI_OCL_RUN )\n" );
  fprintf ( f, "    {\n" );
  fprintf ( f, "      if ( get_global_id ( 0 ) != 0 )\n" );
  fprintf ( f, "      {\n" );
  fprintf ( f, "        return;\n" );
  fprintf ( f, "      }\n" );
  fprintf ( f, "    }\n\n" );

  fprintf ( f, "  /* ---- Aggregate the flattened arguments ------------------------------ */\n\n" );

  fprintf ( f, "  __local OCLWorker%c%s self;\n\n",
            toupper(w->implName[0]), w->implName + 1);

  fprintf ( f, "  /* ---- Initialize the property pointer -------------------------------- */\n\n" );

  fprintf ( f, "  self.properties = ( __global %c%sProperties* ) properties;\n\n",
            toupper ( w->implName [ 0 ] ),
            w->implName + 1 );

  fprintf ( f, "  /* ---- Initialize the run condition ----------------------------------- */\n\n" );

  fprintf ( f, "  self.runCondition = *runCondition;\n\n" );

  if ( w->localMemories.size() )
  {
    fprintf ( f, "  /* ---- Initialize the local memory structures ------------------------- */\n\n" );
    for ( size_t n = 0; n < w->localMemories.size(); n++ )
    {
      fprintf ( f,
                "  self.%s = local_mem_%zu_data;\n",
                w->localMemories [ n ]->name,
                n );
    }
    fprintf ( f, "\n" );
  }

  fprintf ( f, "  /* ---- Initialize the port structures --------------------------------- */\n\n" );

  if ( w->ports.size() )
  {
    for ( size_t n = 0; n < w->ports.size(); n++ )
    {
      Port* port = w->ports [ n ];
      fprintf ( f,
                "  self.%s.current.data = port_%s_data;\n"
                "  self.%s.current.maxLength = port_%s_max_length;\n"
                "  self.%s.attr.length = port_%s_attrs->length;\n"
                "  self.%s.attr.connected = port_%s_attrs->connected;\n"
                "  self.%s.attr.u.operation = port_%s_attrs->u.operation;\n\n",
                port->name,
                port->name,
                port->name,
                port->name,
                port->name,
                port->name,
                port->name,
                port->name,
                port->name,
                port->name );
    }
  }

  fprintf ( f, "  /* ---- Perform the actual operation ----------------------------------- */\n\n" );

  fprintf ( f, "  barrier ( CLK_LOCAL_MEM_FENCE );\n\n" );

  fprintf ( f, "  OCLResult rc = 0;\n\n" );

  fprintf ( f, "  switch ( opcode )\n" );
  fprintf ( f, "  {\n" );

  fprintf ( f, "    case OCPI_OCL_RUN:\n" );
  fprintf ( f, "      rc = %s_run ( &self, timedOut, newRunCondition );\n", w->implName );
  fprintf ( f, "      break;\n" );
  unsigned op = 0;
  const char* mName;
  for ( const char** cp = controlOperations; *cp; cp++, op++ )
  {
    if ( w->ctl.controlOps & (1 << op ) )
    {
      if ( ( err = methodName ( w, *cp, mName  ) ) )
      {
        return err;
      }
      const char* mUname = upperdup ( mName );
      fprintf ( f, "    case OCPI_OCL_%s:\n", mUname );
      fprintf ( f, "      rc = %s_%s ( &self );\n",
                   w->implName,
                   mName );
      fprintf ( f, "      break;\n" );
    }
  }
  fprintf ( f, "  }\n\n" );

  fprintf ( f, "  /* ---- Update the run condition --------------------------------------- */\n\n" );

  fprintf ( f, "  if ( *newRunCondition )\n" );
  fprintf ( f, "  {\n" );
  fprintf ( f, "    *runCondition = self.runCondition;\n" );
  fprintf ( f, "  }\n\n" );

  fprintf ( f, "  /* ---- Update the output ports ---------------------------------------- */\n\n" );

  if ( w->ports.size() )
  {
    for ( size_t n = 0; n < w->ports.size(); n++ )
    {
      Port* port = w->ports [ n ];

      if ( port->u.wdi.isProducer ) {
        fprintf ( f,
                  "  port_%s_attrs->length = self.%s.attr.length;\n"
                  "  port_%s_attrs->u.operation = self.%s.attr.u.operation;\n\n",
                  port->name,
                  port->name,
                  port->name,
                  port->name );
      }
    }
  }

  fprintf ( f, "  /* ---- Return worker function result (ok, done, advance) -------------- */\n\n" );

  fprintf ( f, "  *result = rc;\n\n" );

  fprintf ( f, "  barrier ( CLK_LOCAL_MEM_FENCE );\n" );
  fprintf ( f, "  barrier ( CLK_GLOBAL_MEM_FENCE );\n\n" );

  fprintf ( f, "}\n\n" );

  if (fclose(f))
    return "Could close output file. No space?";
}
