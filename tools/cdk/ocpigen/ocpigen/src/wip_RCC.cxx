#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <sys/time.h>
#include "wip.h"
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

static void
printSimple(FILE *f, Simple *t, const char *prefix, unsigned &offset, unsigned &pad)
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
  fprintf(f, "%s  %-12s %s", prefix, rccTypes[t->type.scalar], t->name);
  if (t->type.scalar == CP::Scalar::CPI_String)
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

static void
emitStructRCC(FILE *f, unsigned nMembers, Simple *types, const char *indent) {
  Simple *t = types;
  unsigned align = 0, pad = 0;
  for (unsigned n = 0; n < nMembers; n++, t++)
    printSimple(f, t, indent, align, pad);
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
  if (w->nPorts) {
    fprintf(f,
	    "/*\n"
	    " * Enumeration of port ordinals for worker %s\n"
	    " */\n"
	    "typedef enum {\n",
	    w->implName);
    Port *port = w->ports;
    unsigned in = 0, out = 0;
    last = "";
    for (unsigned n = 0; n < w->nPorts; n++, port++) {
      fprintf(f, "%s  %s_%s", last, upper, upperdup(port->name));
      // FIXME TWO WAY
      last = ",\n";
      if (port->wdi.isProducer)
	out++;
      else
	in++;
    }
    fprintf(f, "\n} %c%sPort;\n", toupper(w->implName[0]), w->implName+1);
    fprintf(f,
	    "#define %s_N_INPUT_PORTS %u\n"
	    "#define %s_N_OUTPUT_PORTS %u\n",
	    upper, in, upper, out);
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
	  fprintf(f, "  uint32_t     %s_length;\n", p->name);
	  if (p->maxAlign > sizeof(uint32_t))
	    fprintf(f, "  char   pad%u_[%u];\n",
		    pad++, p->maxAlign - (unsigned)sizeof(uint32_t));
	  align += p->maxAlign;
	}
	if (p->isStruct) {
	  fprintf(f, "  struct %c%s%c%s {\n",
		  toupper(w->implName[0]), w->implName+1,
		  toupper(p->name[0]), p->name + 1);
	  emitStructRCC(f, p->nMembers, p->members, "  ");
	  fprintf(f, "  } %s", p->name);
	  if (p->isStructSequence)
	    fprintf(f, "[%u]", p->nStructs);
	  fprintf(f, ";\n");
	} else
	  printSimple(f, p->members, "", align, pad);
      }
      fprintf(f,
	      "} %c%sProperties;\n\n",
	      toupper(w->implName[0]), w->implName + 1);
    }
    const char *mName;
    if ((err = methodName(w, "run", mName)))
      return err;
    fprintf(f,
	    "/*\n"
	    " * Use this macro, followed by a semicolon, to declare methods before\n"
	    " * implementing them, and before defining the RCCDispatch for the worker\n"
	    " * e.g.:\n"
	    " *  %s_METHOD_DECLARATIONS;\n"
	    " */\n"
	    "#define %s_METHOD_DECLARATIONS \\\n",
	    upper, upper);
    unsigned op = 0;
    const char **cp;
    if (w->ctl.controlOps) {
      last = "";
      fprintf(f, "  %s RCCMethod ", w->pattern ? "extern" : "static");
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
	    "  %s RCCRunMethod %s\\\n",
	    w->pattern ? "extern" : "static", mName);
    fprintf(f, "/**/\n");
    fprintf(f,
	    "/*\n"
	    " * This macro defines the initialization of the RCCDispatch structure\n"
	    " * for the %s worker.  Insert it AFTER any customized initializations\n"
	    " * of members: memSizes, runCondition.\n"
	    " * E.g.: \n"
	    " *  static RCCRunCondition myRunCondition { ... };\n"
	    " *  %s_METHOD_DECLARATIONS;\n"
	    " *  RCCDispatch xyz_dispatch = {\n"
	    " *     .runCondition = &myRunCondition,\n"
	    " *     %s_DISPATCH\n"
	    " *  };\n"
	    " */\n"
	    "#define %s_DISPATCH \\\n"
	    "  .version = RCC_VERSION,\\\n"
	    "  .numInputs = %s_N_INPUT_PORTS,\\\n"
	    "  .numOutputs = %s_N_OUTPUT_PORTS,\\\n"
	    "  .threadProfile = %u,\\\n",
	    w->implName, upper, upper, upper, upper, upper, w->rcc.isThreaded ? 1 : 0);
    if (w->ctl.nProperties)
      fprintf(f, "  .propertySize = sizeof(%c%sProperties),\\\n",
	      toupper(w->implName[0]), w->implName + 1);
    for (op = 0, cp = controlOperations; *cp; cp++, op++)
      if (w->ctl.controlOps & (1 << op)) {
	if ((err = methodName(w, *cp, mName)))
	  return err;
	fprintf(f, "  .%s = %s,\\\n", *cp, mName);
      }
    if ((err = methodName(w, "run", mName)))
      return err;
    fprintf(f, "  .run = %s,\\\n", mName);
    port = w->ports;
    uint32_t optionals = 0;
    for (unsigned n = 0; n < w->nPorts; n++, port++)
      if (port->wdi.isOptional)
	optionals |= 1 << n;
    if (optionals)
      fprintf(f, "  .optionallyConnectedPorts = 0x%x,\\\n", optionals);
    fprintf(f, "/**/\n");
    if (w->nPorts) {
      port = w->ports;
      for (unsigned n = 0; n < w->nPorts; n++, port++)
	if (port->wdi.nOperations) {
	  fprintf(f,
		  "/*\n"
		  " * Enumeration of operations on port %s of worker %s\n"
		  " */\n"
		  "typedef enum {\n",
		  port->name, w->implName);
	  Operation *o = port->wdi.operations;
	  const char *puName = upperdup(port->name);
	  for (unsigned nn = 0; nn < port->wdi.nOperations; nn++, o++)
	    fprintf(f, "  %s_%s_%s,\n", upper, puName, upperdup(o->name));
	  fprintf(f, "} %c%s%c%sOperation;\n",
		  toupper(w->implName[0]), w->implName+1,
		  toupper(port->name[0]), port->name+1);
	  // Now emit structs for messages
	  o = port->wdi.operations;
	  for (unsigned nn = 0; nn < port->wdi.nOperations; nn++, o++)
	    if (o->nArgs) {
	      fprintf(f,
		      "/*\n"
		      " * Structure for the %s operation on port %s\n"
		      " */\n"
		      "typedef struct {\n",
		      o->name, port->name);
	      emitStructRCC(f, o->nArgs, o->args, "");
	      fprintf(f, "} %c%s%c%s%c%s;\n",
		      toupper(w->implName[0]), w->implName + 1,
		      toupper(port->name[0]), port->name + 1,
		      toupper(o->name[0]), o->name + 1);
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
  }  
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
	  "  /* insert any custom initializations here */\n"
	  "  %s_DISPATCH\n"
	  "};\n\n"
	  "/*\n"
	  " * Methods to implement for worker %s, based on metadata.\n"
	  "*/\n",
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
	      "%s RCCResult %s(RCCWorker *self) {\n"
	      "  return RCC_OK;\n"
	      "}\n",
	      w->pattern ? "extern" : "static", mName);
    }
  if ((err = methodName(w, "run", mName)))
    return err;
  fprintf(f,
	  "\n"
	  "%s RCCResult %s(RCCWorker *self,\n"
	  "                 %*s RCCBoolean timedOut,\n"
	  "                 %*s RCCBoolean *newRunCondition) {\n"
	  "  return RCC_ADVANCE;\n"
	  "}\n",
	  w->pattern ? "extern" : "static", mName,
	  (int)strlen(mName), "", (int)strlen(mName), "");
  // FIXME PortMemberMacros?
  // FIXME Compilable - any initial functionality??? cool.
  fclose(f);
  return 0;
}
const char *
emitArtRCC(Worker *aw, const char *outDir) {
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
  fprintf(f, "<artifact>\n");
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
