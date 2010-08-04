#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include "wip.h"
/*
 * todo
 * generate WIP attribute constants for use by code (widths, etc.)
 * sthreadbusyu alias?
 */
static const char *wipNames[] = {"Unknown", "WCI", "WSI", "WMI", "WDI", "WMemI", 0};

const char *pattern(Worker *w, Port *p, int n, unsigned wn, bool in, bool master, char **suff) {
  const char *pat = p->pattern ? p->pattern : w->pattern;
  if (!pat) {
    *suff = strdup("");
    return 0;
  }
  char c, *s = (char *)malloc(strlen(p->name) + strlen(w->pattern) * 3 + 10);
  *suff = s;
  while ((c = *pat++)) {
    if (c != '%')
      *s++ = c;
    else if (!*pat)
      *s++ = '%';
    else {
      bool myMaster = master;
      if (*pat == '!') {
	myMaster = !master;
	*pat++;
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
	sprintf(s, "%d", wn + (pat[-1] - '0'));
	while (*s) s++;
	break;
#if 0
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
#endif
      case 's': // interface name as is
      case 'S': // capitalized interface name
	strcpy(s, p->name);
	if (pat[-1] == 'S')
	  *s = toupper(*s);
	while (*s)
	  s++;
	if (p->count > 1)
	  switch (n) {
	  case -1:
	    *s++ = '%';
	    *s++ = 'd';
	    break;
	  case -2:
	    break;
	  default:
	    sprintf(s, "%d", n);
	    while (*s)
	      s++;
	  }
	break;
      case 'W': // capitalized profile name
	strcpy(s, wipNames[p->type]);
	s++;
        while (*s) {
	  *s = tolower(*s);
	  s++;
	}
	break;
      case 'w': // lower case profile name
	strcpy(s, wipNames[p->type]);
        while (*s) {
	  *s = tolower(*s);
	  s++;
	}
	break;
      default:
	return esprintf("Invalid pattern rule: %s", w->pattern);
      }
    }
  }
  *s++ = 0;
  return 0;
}


void
printgen(FILE *f, const char *comment, const char *file, bool orig) {
  time_t now = time(0);
  char *ct = ctime(&now);
  ct[strlen(ct) - 1] = '\0';
  struct tm *local = localtime(&now);
  fprintf(f,
	  "%s THIS FILE WAS %sGENERATED ON %s %s\n"
	  "%s BASED ON THE FILE: %s\n"
	  "%s YOU %s EDIT IT\n",
	  comment, orig ? "ORIGINALLY " : "", ct, local->tm_zone,
	  comment, file,
	  comment, orig ? "ARE EXPECTED TO" : "PROBABLY SHOULD NOT");
}

const char *
openOutput(const char *name, const char *outDir, const char *prefix, const char *suffix,
	   const char *ext, const char *other, FILE *&f) {
  char *file = (char *)malloc(strlen(outDir) + 1 + strlen(prefix) +
			      strlen(name) + (other ? strlen(other) : 0) + strlen(suffix) + 1 +
			      strlen(ext) + 1);
  sprintf(file, "%s%s%s%s%s%s", outDir ? outDir : "", outDir ? "/" : "",
	  prefix, name, suffix, ext);
  if ((f = fopen(file, "w")) == NULL)
    return esprintf("Can't not open file %s for writing (%s)\n",
		    file, strerror(errno));
  dumpDeps(file);
  if (other && strcmp(other, name)) {
    char *otherFile = strdup(file);
    sprintf(otherFile, "%s%s%s%s%s%s", outDir ? outDir : "", outDir ? "/" : "",
	    prefix, other, suffix, ext);
    // Put all this junk in CpiOs
    char dummy;
    ssize_t length = readlink(otherFile, &dummy, 1);
    if (length != -1) {
      char *buf = (char*)malloc(length + 1);
      if (readlink(otherFile, buf, length) != length)
	return "Unexpected system error reading symlink";
      buf[length] = '\0';
      if (!strcmp(otherFile, buf))
	return 0;
      if (unlink(otherFile))
	return "Cannot remove symlink to replace it";
    } else if (errno != ENOENT)
      return "Unexpected error reading symlink";
    char *contents = strrchr(file, '/');
    contents = contents ? contents + 1 : file;
    if (symlink(contents, otherFile))
      return "Cannot create symlink";
  }
  return 0;
}
										
// Emit the file that can be used to instantiate the worker
 const char *
emitDefsHDL(Worker *w, const char *outDir, bool wrap) {
  const char *err;
  FILE *f;
  Language lang = wrap ? (w->language == VHDL ? Verilog : VHDL) : w->language;
  if ((err = openOutput(w->implName, outDir, "", DEFS, lang == VHDL ? VHD : VER, NULL, f)))
    return err;
  const char *comment = lang == VHDL ? "--" : "//";
  printgen(f, comment, w->file);
  fprintf(f,
	  "%s This file contains the %s declarations for the worker with\n"
	  "%s  spec name \"%s\" and implementation name \"%s\".\n"
	  "%s It is needed for instantiating the worker.\n"
	  "%s Interface signal names are defined with pattern rule: \"%s\"\n",
	  comment, lang == VHDL ? "VHDL" : "Verilog", comment, w->specName,
	  w->implName, comment, comment, w->pattern);
  if (lang == VHDL)
    fprintf(f,
	    "Library IEEE;\n"
	    "  use IEEE.std_logic_1164.all;\n"
	    "Library work;\n"
	    "  use work.wip_defs.all;\n"
	    "\n"
	    "package %s_defs is\n",
	    w->implName);
  // Generate record types for interfaces
  Port *p = w->ports;
  OcpSignalDesc *osd;
  if (lang == VHDL)
    for (unsigned i = 0; i < w->nPorts; i++, p++) {
      bool mIn = masterIn(p);
      fprintf(f,
	      "\n"
	      "-- These 2 records correspond to the input and output sides of the OCP bundle\n"
	      "-- for the \"%s\" worker's \"%s\" profile interface named \"%s\"\n",
	      w->implName, wipNames[p->type], p->name);
      fprintf(f,
	      "\n-- Record for the %s input (OCP %s) signals for port \"%s\" of worker \"%s\"\n",
	      wipNames[p->type], mIn ? "master" : "slave", p->name, w->implName);
      char *pin, *pout;
      if ((err = pattern(w, p, 0, 0, true, !mIn, &pin)) ||
	  (err = pattern(w, p, 0, 0, false, !mIn, &pout)))
	return err;
      fprintf(f, "type %s_t is record\n", pin);
      osd = ocpSignals;
      for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	if ((osd->master == mIn && strcmp(osd->name, "Clk")) && os->value) {
	  fprintf(f, "    %-20s: ", osd->name);
	  if (osd->type)
	    fprintf(f, "%s_t", osd->name);
	  else if (osd->vector)
	    fprintf(f, "std_logic_vector(%u downto 0)", os->width - 1);
	  else
	    fprintf(f, "std_logic");
	  fprintf(f, ";\n");
	}
      fprintf(f, "end record %s_t;\n", pin);
      fprintf(f,
	      "\n-- Record for the %s output (OCP %s) signals for port \"%s\" of worker \"%s\"\n"
	      "type %s%s_t is record\n",
	      wipNames[p->type], mIn ? "slave" : "master",
	      p->name, w->implName, p->name, pout);
      osd = ocpSignals;
      for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	if ((osd->master != mIn && strcmp(osd->name, "Clk")) && os->value) {
	  fprintf(f, "    %-20s: ", osd->name);
	  if (osd->type)
	    fprintf(f, "%s_t", osd->name);
	  else if (osd->vector)
	    fprintf(f, "std_logic_vector(%u downto 0)", os->width - 1);
	  else
	    fprintf(f, "std_logic");
	  fprintf(f, ";\n");
	}
      fprintf(f, "end record %s_t;\n", pout);
    }
  if (lang == VHDL)
    fprintf(f,
	    "\n\ncomponent %s is\n"
	    "  port (\n", w->implName);
  else
    fprintf(f,
	    "\n"
	    "`default_nettype none\n"
	    "module %s (\n",
	    w->isAssembly ? "ocpi_app" : w->implName);
  p = w->ports;
  // port name is scoped by entity, just has to avoid record name
  // _in or _out
  // type conventions _t or _Typ or T or..
  // port signal (wsi consumer port foo):
  //  foo_s_out: out spec_impl.foo_s_t;
  char *last = 0;
  for (unsigned i = 0; i < w->nPorts; i++, p++) {
    if (last) {
      fprintf(f, last, ',');
      last = 0;
    }
    bool mIn = masterIn(p);
    char *nbuf;
    asprintf(&nbuf, " %d", p->count);
    fprintf(f,
	    "\n  %s%s For the%s %s %sinterface%s named \"%s\", with \"%s\" acting as OCP %s\n",
	    lang == VHDL ? "  " : "", comment, p->count > 1 ? nbuf : "", wipNames[p->type],
	    p->type == WMIPort || p->type == WSIPort ?
	    (p->wdi.isProducer ? "producer " : "consumer ") : "",
	    p->count > 1 ? "s" : "", p->name, w->implName, mIn ? "slave" : "master");
    for (unsigned n = 0; n < p->count; n++) {
      if (last) {
	fprintf(f, last, ',');
	last = 0;
      }
      if (p->clock->port == p && n == 0) {
	if (lang == VHDL)
	  fprintf(f, "    %-20s: in  std_logic;\n", p->clock->signal);
	else
	  fprintf(f, "  %s, %*s// input\n",
		  p->clock->signal, (int)(20 - strlen(p->clock->signal)), "");
	p->ocp.Clk.signal = p->clock->signal;
      } else if (n == 0)
	fprintf(f,
		"  %s%s No Clk here. \"%s\" interface uses \"%s\" as clock,\n",
		lang == VHDL ? "  " : "", comment, p->name, p->clock->signal);
      if (lang == VHDL)
	fprintf(f, "    %-20s: in  %s_t;\n",
		p->fullNameIn, p->fullNameIn);
      else {
	osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	  if ((osd->master == mIn && strcmp(osd->name, "Clk")) && os->value) {
	    char *name;
	    asprintf(&name, os->signal, n);
	    fprintf(f, "  %s, %*s// input  ", name, (int)(20 - strlen(name)), "");
	    //if (osd->type)
	    //fprintf(f, "%s_t", osd->name);
	    //else
	    if (osd->vector)
	      fprintf(f, "[%3u:0]\n", os->width - 1);
	    else
	      fprintf(f, "\n");
	  }
      }
      if (lang == VHDL)
	fprintf(f, "    %-20s: out %s_t%s\n", p->fullNameOut, p->fullNameOut,
		i < w->nPorts-1 || p->count > n+1 ? ";" : "");
      else {
	osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	  if ((osd->master != mIn && strcmp(osd->name, "Clk")) && os->value) {
	    if (last)
	      fprintf(f, last, ',');
	    char *name;
	    asprintf(&name, os->signal, n);
	    asprintf(&last, "  %s%%c %*s// output ", name,
		     (int)(20 - strlen(name)), "");
	    if (osd->vector)
	      asprintf(&last, "%s[%3u:0]\n", last, os->width - 1);
	    else
	      asprintf(&last, "%s\n", last);
	  }
      }
    }
  }
  if (lang == VHDL) {
    fprintf(f, "  );\n"
	    "end component %s;\n"
	    "end package %s_defs;\n", w->implName, w->implName);
  } else {
    if (last)
	fprintf(f, last, ' ');
    fprintf(f, ");\n");
    // Now we emit paarameters.
    Property *pr = w->ctl.properties;
    for (unsigned i = 0; i < w->ctl.nProperties; i++, pr++)
      if (pr->isParameter)
	if (w->language == VHDL) {
	  fprintf(f, "VHDL PARAMETERS HERE\n");
	} else {
	  int64_t i64;
	  switch (pr->types->type) {
#define CPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
	    case CM::Property::CPI_##pretty: i64 = (int64_t)pr->types->defaultValue.v##pretty; break;
CPI_PROPERTY_DATA_TYPES
	  default:;
	  }
	  fprintf(f, "  parameter [%u:0] %s = %u'b%lld;\n",
		  pr->types->bits - 1, pr->name, pr->types->bits,
		  (long long)i64);
	}
    // Now we emit the declarations (input, output, width) for each module port
    p = w->ports;
    for (unsigned i = 0; i < w->nPorts; i++, p++) {
      bool mIn = masterIn(p);
      for (unsigned n = 0; n < p->count; n++) {
	if (p->clock->port == p && n == 0)
	  fprintf(f, "  input          %s;\n", p->clock->signal);
#if 0
	char *pin, *pout;
	if ((err = pattern(w, p, n, 0, true, !mIn, &pin)) ||
	    (err = pattern(w, p, n, 0, false, !mIn, &pout)))
	  return err;
#endif
	osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	  if ((osd->master == mIn && strcmp(osd->name, "Clk")) && os->value) {
	    char *name;
	    asprintf(&name, os->signal, n);
	    if (osd->vector)
	      fprintf(f, "  input  [%3u:0] %s;\n", os->width - 1, name);
	    else
	      fprintf(f, "  input          %s;\n", name);
	  }
	osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	  if ((osd->master != mIn && strcmp(osd->name, "Clk")) && os->value) {
	    char *name;
	    asprintf(&name, os->signal, n);
	    if (osd->vector)
	      fprintf(f, "  output [%3u:0] %s;\n", os->width - 1, name);
	    else
	      fprintf(f, "  output         %s;\n", name);
	  }
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
	    w->implName, w->implName, w->implName);
  }
  fclose(f);
  return 0;
}
// Generate the readonly implementation file.
// What implementations must explicitly (verilog) or implicitly (VHDL) include.
 const char *
emitImplHDL(Worker *w, const char *outDir, const char *library) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->implName, outDir, "", IMPL, w->language == VHDL ? VHD : VER, NULL, f)))
    return err;
  const char *comment = w->language == VHDL ? "--" : "//";
  printgen(f, comment, w->file);
  fprintf(f,
	  "%s This file contains the implementation declarations for worker %s\n"
	  "%s Interface definition signal names defined with pattern rule: \"%s\"\n\n",
	  comment, w->implName, comment, w->pattern);
  if (w->language == VHDL)
    fprintf(f,
	    "library IEEE;\n"
	    "  use IEEE.std_logic_1164.all;\n"
	    "library %s;\n"
	    "  use %s.%s%s.all;\n\n"
	    "entity %s is\n"
	    "  port (\n",
	    library, library, w->implName, DEFS, w->implName);
  else
    // Verilog just needs the module declaration and any other associate declarations
    // required for the module declaration.
    fprintf(f,
	    "`define NOT_EMPTY_%s // suppress the \"endmodule\" in %s%s%s\n"
	    "`include \"%s%s%s\"\n"
	    "`include \"ocpi_wip_defs.v\"\n",
	    w->implName, w->implName, DEFS, VER, w->implName, DEFS, VER);
	    
  // For VHDL we need to basically replicate the port declarations that are part of the 
  // component: there is no way to just use what the component decl says.
  // For Verilog we just include the module declaration so don't need anything here.
  Port *p = w->ports;
  if (w->language == VHDL) {
    // port name is scoped by entity, just has to avoid record name
    // _in or _out
    // type conventions _t or _Typ or T or..
    // port signal (wsi consumer port foo):
    //  foo_s_out: out spec_impl.foo_s_t;
    for (unsigned i = 0; i < w->nPorts; i++, p++) {
      bool mIn = masterIn(p);
      char *nbuf;
      asprintf(&nbuf, " %d", p->count);
      fprintf(f,
	      "\n    %s For the %s%s %sinterface%s named \"%s\", with %s acting as OCP %s\n",
	      comment, p->count > 1 ? nbuf : "", wipNames[p->type],
	      p->type == WMIPort || p->type == WSIPort ?
	      (p->wdi.isProducer ? "producer " : "consumer ") : "",
	      p->count > 1 ? "s" : "", p->name,
	      w->implName, mIn ? "slave" : "master");
      for (unsigned n = 0; n < p->count; n++) {
	if (p->clock->port == p && n == 0)
	  fprintf(f, "    %-20s: in  std_logic;\n", p->clock->signal);
	else if (n == 0)
	  fprintf(f,
		  "    -- No Clk here. \"%s\" interface uses \"%s\" as clock,\n",
		  p->name, p->clock->signal);
	char *pin, *pout;
	if ((err = pattern(w, p, n, 0, true, !mIn, &pin)) ||
	    (err = pattern(w, p, n, 0, false, !mIn, &pout)))
	  return err;
	fprintf(f, "    %-20s: in  %s_t;\n",
		pin, pin);
	fprintf(f, "    %-20s: out %s_t%s\n", pout, pout,
		i < w->nPorts-1 || p->count > n+1 ? ";" : "");
      }
    }
    fprintf(f, "  );\n");
  }
  // Aliases for OCP signals
  p = w->ports;
  for (unsigned i = 0; i < w->nPorts; i++, p++) {
    bool mIn = masterIn(p);
    for (unsigned n = 0; n < p->count; n++) {
      char *pin, *pout;
      if ((err = pattern(w, p, n, 0, true, !mIn, &pin)) ||
	  (err = pattern(w, p, n, 0, false, !mIn, &pout)))
	return err;
      switch (p->type) {
      case WCIPort:
	fprintf(f,
		"  %s Aliases for %s interface \"%s\"\n",
		comment, wipNames[p->type], p->name);
	if (w->language == VHDL)
	  fprintf(f,
		  "  alias %s_Terminate : std_logic is %s.MFlag(0);\n"
		  "  alias %s_Endian   : std_logic is %s.MFlag(1);\n"
		  "  alias %s_Config    : std_logic is %s.MAddrSpace(0);\n"
		  "  alias %s_Attention : std_logic is %s.SFlag(0);\n",
		  pin, pin, pin, pin, pin, pin, pout, pout);
	else {
	  fprintf(f,
		  "  wire %sTerminate = %sMFlag[0];\n"
		  "  wire %sEndian    = %sMFlag[1];\n"
		  "  wire %sConfig    = %sMAddrSpace[0];\n"
		  "  wire %sIsCfgWrite = %sMCmd == OCPI_OCP_MCMD_WRITE &&\n"
		  "                           %sMAddrSpace[0] == OCPI_WCI_CONFIG;\n"
		  "  wire %sIsCfgRead = %sMCmd == OCPI_OCP_MCMD_READ &&\n"
		  "                          %sMAddrSpace[0] == OCPI_WCI_CONFIG;\n"
		  "  wire %sIsControlOp = %sMCmd == OCPI_OCP_MCMD_READ &&\n"
		  "                            %sMAddrSpace[0] == OCPI_WCI_CONTROL;\n"
                  "  wire [2:0] %sControlOp = %sMAddr[4:2];\n"
		  "  assign %sSFlag[1] = 1; // indicate that this worker is present\n",
		  pin, pin, pin, pin, pin, pin,
		  pin, pin, pin, pin, pin, pin, pin, pin, pin,
		  pin, pin, pin);
	  fprintf(f,
		  "  // This assignment requires that the %sAttention be used, not SFlag[0]\n"
		  "  reg %sAttention; assign %sSFlag[0] = %sAttention;\n",
		  pout, pout, pout, pout);
	}
	if (w->ctl.nProperties) {
	  fprintf(f,
		  "  %s Constants for %s's property addresses\n",
		  comment, w->implName);
	  if (w->language == VHDL)
	    fprintf(f,
		  "  subtype Property_t is std_logic_vector(%d downto 0);\n",
		  p->ocp.MAddr.width - 1);
	  else
	    fprintf(f,
		    "  localparam %sPropertyWidth = %d;\n", pin, p->ocp.MAddr.width);
	  if (n == 0) {
	    Property *pr = w->ctl.properties;
	    for (unsigned i = 0; i < w->ctl.nProperties; i++, pr++) {
	      if (w->language == VHDL) {
		fprintf(f,
			"  constant %-20s : Property_t := b\"", pr->name);
		for (int b = p->ocp.MAddr.width-1; b >= 0; b--)
		  fprintf(f, "%c", pr->offset & (1 << b) ? '1' : '0');
		fprintf(f, "\"; -- 0x%0*x\n",
			(int)roundup(p->ocp.MAddr.width, 4)/4, pr->offset);
	      } else
		fprintf(f, "  localparam [%d:0] %sAddr = %d'h%0*x;\n",
			p->ocp.MAddr.width - 1, pr->name, p->ocp.MAddr.width,
			(int)roundup(p->ocp.MAddr.width, 4)/4, pr->offset);
	    }
	  }
	}
	break;
      case WSIPort:
	if (p->wsi.regRequest) {
	  fprintf(f,
		  "  %s Register declarations for request phase signals for interface \"%s\"\n",
		  comment, p->name);
	  OcpSignalDesc *osd = ocpSignals;
	  for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	    if (osd->request && p->wdi.isProducer && p->wsi.regRequest && os->value &&
		strcmp("MReqInfo", osd->name)) // fixme add "aliases" attribute somewhere
	      fprintf(f, "  reg %s%s;\n", pout, osd->name);
	}
	fprintf(f,
		"  %s Aliases for interface \"%s\"\n", comment, p->name);
	if (p->ocp.MReqInfo.width) {
	  if (n == 0)
	    if (w->language == VHDL)
	      fprintf(f, 
		      "  subtype %s%s_OpCode_t is std_logic_vector(%d downto 0);\n",
		      p->name, mIn ? pin : pout, p->ocp.MReqInfo.width - 1);
	    else
	      fprintf(f, 
		      "  localparam %sOpCodeWidth = %d;\n",
		      mIn ? pin : pout, p->ocp.MReqInfo.width);
	  if (w->language == VHDL)
	    fprintf(f, 
		    "  alias %s_Opcode: %sOpCode_t is %s.MReqInfo(%d downto 0);\n",
		    mIn ? pin : pout, mIn ? pin : pout,
		    mIn ? pin : pout, p->ocp.MReqInfo.width - 1);
	  else if (mIn)
	    fprintf(f, 
		    "  wire [%d:0] %sOpcode = %sMReqInfo;\n",
		    p->ocp.MReqInfo.width - 1, pin, pin);
	  else
	    fprintf(f, 
		    //"  wire [%d:0] %s_Opcode; always@(posedge %s) %s_MReqInfo = %s_Opcode;\n",
		    // p->ocp.MReqInfo.width - 1, pout, p->clock->signal, pout, pout);
		    "  %s [%d:0] %sOpcode; assign %sMReqInfo = %sOpcode;\n",
		    p->wsi.regRequest ? "reg" : "wire", p->ocp.MReqInfo.width - 1, pout, pout, pout);
	}
	if (p->ocp.MFlag.width)
	  if (w->language == VHDL)
	    fprintf(f, 
		    "  alias %sAbort : std_logic is %s.MFlag(0);\n",
		    mIn ? pin : pout, mIn ? pin : pout);
	  else if (mIn)
	    fprintf(f, 
		    "  wire %sAbort = %sMFlag[0];\n",
		    pin, pin);
	  else
	    fprintf(f, 
		    "  wire %sAbort; assign %sMFlag[0] = %sAbort;\n",
		    pout, pout, pout);
	break;
      case WMIPort:
	fprintf(f,
		"  %s Aliases for interface \"%s\"\n", comment, p->name);
	if (w->language == VHDL)
	  fprintf(f,
		  "  alias %sNodata  : std_logic is %s.MAddrSpace(0);\n"
		  "  alias %sDone    : std_logic is %s.MReqInfo(0);\n",
		  pout, pout, pout, pout);
	else if (p->master) // if we are app
	  fprintf(f,
		  "  wire %sNodata; assign %sMAddrSpace[0] = %sNodata;\n"
		  "  wire %sDone;   assign %sMReqInfo[0] = %sDone;\n",
		  pout, pout, pout, pout, pout, pout);
	else // we are infrastructure
	  fprintf(f,
		  "  wire %sNodata = %sMAddrSpace[0];\n"
		  "  wire %sDone   = %sMReqInfo[0];\n",
		  pin, pin, pin, pin);
	if (p->wdi.numberOfOpcodes) {
	  if (w->language == VHDL) {
	    if (n == 0)
	      fprintf(f,
		      "  subtype %s%sOpCode_t is std_logic_vector(7 downto 0);\n",
		      p->name, p->wdi.isProducer ? pout : pin);
	    fprintf(f,
		    "  alias %sOpcode: %s%sOpCode_t is %s.%cFlag(7 downto 0);\n",
		    p->wdi.isProducer ? pout : pin,
		    p->name, p->wdi.isProducer ? pout : pin,
		    p->wdi.isProducer ? pout : pin,
		    p->wdi.isProducer ? 'M' : 'S');
	  } else {
	    if (p->wdi.isProducer) // opcode is an output
	      fprintf(f,
		      "  wire [7:0] %sOpcode; assign %s%cFlag[7:0] = %sOpcode;\n",
		      pout, pout, p->master ? 'M' : 'S', pout);
	    else
	      fprintf(f,
		      "  wire [7:0] %sOpcode = %s%cFlag[7:0];\n",
		      pin, pin, p->master ? 'S' : 'M');
	  }
	}
	if (p->wdi.variableMessageLength) {
	  if (w->language == VHDL) {
	    if (n == 0)
	      fprintf(f,
		      "  subtype %s%sLength_t is std_logic_vector(%d downto 8);\n",
		      p->name, p->wdi.isProducer ? pout : pin,
		      (p->wdi.isProducer ? p->ocp.MFlag.width : p->ocp.MFlag.width) - 1);
	    fprintf(f,
		    "  alias %sLength: %s%s_Length_t is %s.%cFlag(%d downto 0);\n",
		    p->wdi.isProducer ? pout : pin,
		    p->name, p->wdi.isProducer ? pout : pin,
		    p->wdi.isProducer ? pout : pin,
		    p->wdi.isProducer ? 'M' : 'S',
		    (p->wdi.isProducer ? p->ocp.MFlag.width : p->ocp.MFlag.width) - 9);
	  } else {
	    if (p->wdi.isProducer) { // length is an output
	      unsigned width =
		(p->master ? p->ocp.MFlag.width : p->ocp.SFlag.width) - 8;
	      fprintf(f,
		    "  wire [%d:0] %sLength; assign %s%cFlag[%d:8] = %sLength;\n",
		      width - 1, pout, pout, p->master ? 'M' : 'S', width + 7, pout);
	    } else {
	      unsigned width =
		(p->master ? p->ocp.SFlag.width : p->ocp.MFlag.width) - 8;
	      fprintf(f,
		    "  wire [%d:0] %sLength = %s%cFlag[%d:8];\n",
		      width - 1, pin, pin, p->master ? 'S' : 'M', width + 7);
	    }
	  }
	}
	break;
      default:
	;
      }
    }
  }
  // Emit properties
  if (w->language == VHDL)
    fprintf(f,
	    "end entity %s;\n",	w->implName);
  // no close for verilog since its included
  fclose(f);
  return 0;
}

const char *
openSkelHDL(Worker *w, const char *outDir, const char *suff, FILE *&f) {
  const char *err;
  if ((err = openOutput(w->implName, outDir, "", suff, w->language == VHDL ? VHD : VER, NULL, f)))
    return err;
  const char *comment = w->language == VHDL ? "--" : "//";
  printgen(f, comment, w->file, true);
  return 0;
}
const char*
emitSkelHDL(Worker *w, const char *outDir) {
  FILE *f;
  const char *err = openSkelHDL(w, outDir, SKEL, f);
  if (err)
    return err;
  if (w->language == VHDL)
    fprintf(f,
	    "-- This file contains the architecture skeleton for worker: %s\n\n"
	    "library IEEE;\n"
	    "  use IEEE.std_logic_1164.all;\n\n"
	    "architecture rtl of %s is\n"
	    "begin -- rtl\n\n\n\n"
	    "end rtl;\n",
	    w->implName, w->implName);
  else {
    fprintf(f,
	    "// This file contains the implementation skeleton for worker: %s\n\n"
	    "`include \"%s_impl.v\"\n\n",
	    w->implName, w->implName);
    Port *p = w->ports;
    for (unsigned i = 0; i < w->nPorts; i++, p++)
      switch (p->type) {
      case WSIPort:
	if (p->wsi.regRequest)
	  fprintf(f,
		  "// GENERATED: OCP request phase signals for interface \"%s\" are registered\n",
		  p->name);
      default:
	;
      }
    fprintf(f, "\n\nendmodule //%s\n",  w->implName);
  }
  fclose(f);
  return 0;
}

const char *
adjustConnection(InstancePort *consumer, InstancePort *producer) {
  // Check WDI compatibility
  Port *prod = producer->port, *cons = consumer->port;
  if (prod->wdi.dataValueWidth != cons->wdi.dataValueWidth)
    return "dataValueWidth incompatibility for connection";
  if (prod->wdi.dataValueGranularity < cons->wdi.dataValueGranularity ||
      prod->wdi.dataValueGranularity % cons->wdi.dataValueGranularity)
    return "dataValueGranularity incompatibility for connection";
  if (prod->wdi.maxMessageValues > cons->wdi.maxMessageValues)
    return "maxMessageValues incompatibility for connection";
  if (prod->wdi.numberOfOpcodes > cons->wdi.numberOfOpcodes)
    return "numberOfOpcodes incompatibility for connection";
  if (prod->wdi.variableMessageLength && !cons->wdi.variableMessageLength)
    return "variable length producer vs. fixed length consumer incompatibility";
  if (prod->wdi.zeroLengthMessages && !cons->wdi.zeroLengthMessages)
    return "zero length message incompatibility";
  if (prod->type != cons->type)
    return "profile incompatibility";
  if (cons->wdi.continuous && !prod->wdi.continuous)
    return "continuous incompatibility";
  if (prod->dataWidth != prod->dataWidth)
    return "dataWidth incompatibility";
  if (cons->wdi.continuous && !prod->wdi.continuous)
    return "producer is not continuous, but consumer requires it";
  // Profile-specific error checks and adaptations
  OcpAdapt *oa;
  switch (prod->type) {
  case WSIPort:
    // Bursting compatibility and adaptation
    if (prod->impreciseBurst && !cons->impreciseBurst)
      return "consumer needs precise, and producer may produce imprecise";
    if (cons->impreciseBurst) {
      if (!cons->preciseBurst) {
	// Consumer accepts only imprecise bursts
	if (prod->preciseBurst) {
	  // Convert any precise bursts to imprecise
	  oa = &consumer->ocp[OCP_MBurstLength];
	  oa->expr= "%s ? 1 : 2";
	  oa->other = OCP_MReqLast;
	  oa->comment = "Convert precise to imprecise";
	  oa = &producer->ocp[OCP_MBurstLength];
	  oa->expr = "";
	  oa->comment = "MBurstLength ignored for imprecise consumer";
	  if (prod->impreciseBurst) {
	    oa = &producer->ocp[OCP_MBurstPrecise];
	    oa->expr = "";
	    oa->comment = "MBurstPrecise ignored for imprecise-only consumer";
	  }
	}
      } else { // consumer does both
	// Consumer accept both, has MPreciseBurst Signal
	oa = &consumer->ocp[OCP_MBurstPrecise];
	if (!prod->impreciseBurst) {
	  oa->expr = "1";
	  oa->comment = "Tell consumer all bursts are precise";
	} else if (!prod->preciseBurst) {
	  oa = &consumer->ocp[OCP_MBurstPrecise];
	  oa->expr = "0";
	  oa->comment = "Tell consumer all bursts are imprecise";
	  oa = &consumer->ocp[OCP_MBurstLength];
	  oa->other = OCP_MBurstLength;
	  asprintf((char **)&oa->expr, "{%u'b0,%%s}", cons->ocp.MBurstLength.width - 2);
	  oa->comment = "Consumer only needs imprecise burstlength (2 bits)";
	}	
      }
    }
    if (prod->preciseBurst && cons->preciseBurst &&
	prod->ocp.MBurstLength.width < cons->ocp.MBurstLength.width) {
      oa = &consumer->ocp[OCP_MBurstLength];
      asprintf((char **)&oa->expr, "{%u'b0,%%s}",
	       cons->ocp.MBurstLength.width - prod->ocp.MBurstLength.width);
      oa->comment = "Consumer takes bigger bursts than producer";
      oa->other = OCP_MBurstLength;
    }
    // Abortable compatibility and adaptation
    if (cons->wsi.abortable) {
      if (!prod->wsi.abortable) {
	oa = &consumer->ocp[OCP_MFlag];
	oa->expr = "0";
	oa->comment = "Tell consumer no frames are ever aborted";
      }
    } else if (prod->wsi.abortable)
      return "consumer cannot handle aborts from producer";
    // EarlyRequest compatibility and adaptation
    if (cons->wsi.earlyRequest) {
      if (!prod->wsi.earlyRequest) {
	oa = &consumer->ocp[OCP_MDataLast];
	oa->other = OCP_MReqLast;
	oa->expr = "%s";
	oa->comment = "Tell consumer last data is same as last request";
	oa = &consumer->ocp[OCP_MDataValid];
	oa->other = OCP_MCmd;
	oa->expr = "%s == OCPI_OCP_MCMD_WRITE ? 1 : 0";
	oa->comment = "Tell consumer data is valid when request is MCMD_WRITE";
      }
    } else if (prod->wsi.earlyRequest)
      return "producer emits early requests, but consumer doesn't support them";
    break;
  case WMIPort:
    break;
  default:
    return "unknown data port type";
  }
  return 0;
}


/*
 * Generate a file containing wrapped versions of all the workers in an assembly.
 * The wrapping is for normalization when the profiles don't match precisely.
 * The WCI connections are implicit.
 * The WDI and WMemI connections are explicit.
 * For WCI the issues are:
 * -- Config accesses may be disabled in the worker
 * -- Config accesses may have only one of read or write data paths
 * -- Byte enables may or may not be present
 * -- Address width of config properties may be different.
 * For WDI the issues are:
 * -- May have different profiles entirely (WSI vs. WMI).
 * -- May be generic infrastructure ports.
 * -- Basic protocol attributes must match.
 * -- May require repeaters
 * For WSI the issues are:
 * -- Width.
 */

// Verilog only for now
 const char *
emitAssyHDL(Worker *w, const char *outDir)
{
  FILE *f;
  const char *err = openSkelHDL(w, outDir, ASSY, f);
  if (err)
    return err;
  fprintf(f,
	  "// This confile contains the generated assembly implementation for worker: %s\n\n"
	  "`define NOT_EMPTY_%s // suppress the \"endmodule\" in %s%s%s\n"
	  "`include \"%s%s%s\"\n\n",
	  w->implName, w->implName, w->implName, DEFS, VER, w->implName, DEFS, VER);
  Assembly *a = &w->assembly;
  unsigned n;
  Connection *c;
  OcpSignalDesc *osd;
  OcpSignal *os;
  InstancePort *ip;
  fprintf(f, "// Define signals for connections that are not externalized\n\n");
  fprintf(f, "wire[255: 0] nowhere; // for passing output ports\n");
  // We define the internal-to-assembly signals, and also figure out the necessary tieoffs or 
  // simple expressions when there is a simple adaptation.
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    if (c->nExtConsumers == 0 && c->nExtProducers == 0) {
      InstancePort *master, *slave, *producer, *consumer;
      for (ip = c->ports; ip; ip = ip->nextConn) {
	if (ip->port->master)
	  master = ip;
	else
	  slave = ip;
	if (ip->port->wdi.isProducer)
	  producer = ip;
	else
	  consumer = ip;
      }
      assert(master && slave && producer && consumer);
      asprintf((char **)&c->masterName, "%s_%s_2_%s_%s_",
	       master->instance->name, master->port->name,
	       slave->instance->name, slave->port->name);
      asprintf((char **)&c->slaveName, "%s_%s_2_%s_%s_",
	       slave->instance->name, slave->port->name,
	       master->instance->name, master->port->name);
      if ((err = adjustConnection(consumer, producer)))
	return esprintf("for connection from %s/%s to %s/%s: %s",
			producer->instance->name, producer->port->name,
			consumer->instance->name, consumer->port->name, err);
      // Generate signals when both side has the signal configured.
      OcpSignal *osMaster, *osSlave;
      for (osd = ocpSignals, osMaster = master->port->ocp.signals, osSlave = slave->port->ocp.signals ;
	   osd->name; osMaster++, osSlave++, osd++)
	if (osMaster->value && osSlave->value) {
	  unsigned width = osMaster->width < osSlave->width ? osMaster->width : osSlave->width;
	  fprintf(f, "wire ");
	  if (osd->vector)
	    fprintf(f, "[%2d:%2d] ", width - 1, 0);
	  else
	    fprintf(f, "        ");
	  fprintf(f, "%s%s;\n", osd->master ? c->masterName : c->slaveName, osd->name);
	  // Fall through the switch to enable the signal
	}
    }
  Instance *i;
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++) {
    fprintf(f, "%s %s (\n", i->worker->implName, i->name);
    const char *last = "", *comment = "";
    unsigned nn;
    OcpAdapt *oa;
    for (ip = i->ports, nn = 0; nn < i->worker->nPorts; nn++, ip++)
      for (osd = ocpSignals, os = ip->port->ocp.signals, oa = ip->ocp; osd->name; os++, osd++, oa++)
	// If the signal is in the interface
	if (os->value) {
	  const char *signal = 0; // The signal to connect this port to.
	  if (os == &ip->port->ocp.Clk)
	    signal = i->clocks[ip->port->clock - i->worker->clocks]->signal;
	  else if (ip->connection)
	    // If the signal is attached to a connection
	    if (ip->connection->nExtConsumers == 0 && ip->connection->nExtProducers == 0) {
	      // If it is internal
	      if (oa->expr) {
		const char *other;
		if (oa->other) {
		  asprintf((char **)&other, "%s%s", 
			   ocpSignals[oa->other].master ?
			   ip->connection->masterName : ip->connection->slaveName,
			   ocpSignals[oa->other].name);
		} else
		  other = "";
		asprintf((char **)&signal, oa->expr, other);
	      } else
		asprintf((char **)&signal, "%s%s",
			 osd->master ?
			 ip->connection->masterName : ip->connection->slaveName,
			 osd->name);
	    } else {
	      // This port is connected to the external port of the connection
	      char *suff;
	      Port *p = ip->connection->external->port;
	      if ((err = pattern(w, p, 0, 0, p->wdi.isProducer, p->master, &suff)))
		return err;
	      asprintf((char **)&signal, "%s%s", suff, osd->name);
	    }
	  else if (ip->external) {
	    // This port is indeed connected to an external, non-data-plane wip interface
	    OcpSignal *exf =
	      // was: &i->ports[ip->port - i->worker->ports].external->ocp.signals[os - ip->port->ocp.signals];
	      &ip->external->ocp.signals[os - ip->port->ocp.signals];
	    const char *externalName;

	    asprintf((char **)&externalName, exf->signal, n);
	    // Ports with no data connection
	    switch (ip->port->type) {
	    case WCIPort:
	      switch (osd - ocpSignals) {
	      case OCP_MAddr:
		if (os->width < exf->width)
		  asprintf((char **)&signal, "%s[%u:0]", externalName, os->width - 1);
		break;
	      }
	      break;
	    default:
	      ;
	    }
	    if (!signal)
	      signal = externalName;
	  } else {
	    // A trule unconnected port.  All we want is a tieoff if it is an input
	    // We can always use zero since that will assert reset
	    if (osd->master != ip->port->master)
	      signal="0";
	  }
	  if (signal) {
	    fprintf(f, "%s%s%s%s  .%s(%s)",
		    last, comment[0] ? " // " : "", comment, last[0] ? "\n" : "", os->signal, signal);
	    last = ",";
	    comment = oa->comment ? oa->comment : "";
	  }
	}
    fprintf(f, ");%s%s\n", comment[0] ? " // " : "", comment);
  }
#if 0
  Worker *ww = a->workers;
  for (unsigned i = 0; i < a->nWorkers; i++, ww++) {
    fprintf(f, "instance: %s\n"
    // Define the signals that are not already established as external to the assembly.
    





    Port *p = ww->ports;
    for (unsigned i = 0; i < ww->nPorts; i++, p++) {
      if (!p->connection)
	continue;
      Port *other =
	p->connection->from == p ? p->connection->from : p->connection->to;
      bool otherMaster =
	other->worker->assembly ? !other->master : other->master;
      if (p->master == otherMaster)
	return "Connection with same master/slave role at both sides";

      switch (p->type) {
      case WCIPort:
	// MAddr
	// MAddrSpace
	// MByteEn
	// MData
	// SData
	// config at all? addrspace?
	// data per direction
	// byteen
	// config address width
      case WSIPort:
	// Deal with trivial adaptation cases, like precise->imprecise etc.
      case WMIPort:
      case WMemIPort:
      case WDIPort:
      case WTIPort:
      case NoPort:
	;
      }
    }
  }
#endif
  fprintf(f, "\n\nendmodule //%s\n",  w->implName);
  return 0;
}
#define BSV ".bsv"
const char *
emitBsvHDL(Worker *w, const char *outDir) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->implName, outDir, "I_", "", BSV, NULL, f)))
    return err;
  const char *comment = "//";
  printgen(f, comment, w->file);
  fprintf(f,
	  "%s This file contains the BSV declarations for the worker with\n"
	  "%s  spec name \"%s\" and implementation name \"%s\".\n"
	  "%s It is needed for instantiating the worker in BSV.\n"
	  "%s Interface signal names are defined with pattern rule: \"%s\"\n\n",
	  comment, comment, w->specName, w->implName, comment, comment, w->pattern);
  fprintf(f,
	  "package I_%s; // Package name is the implementation name of the worker\n\n"
	  "import OCWip::*; // Include the OpenCPI BSV WIP package\n\n"
	  "import Vector::*;\n"
	  "// Define parameterized types for each worker port\n"
	  "//  with parameters derived from WIP attributes\n\n",
	  w->implName);
  Port *p;
  unsigned n, nn;
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
    const char *num;
    if (p->count == 1) {
      fprintf(f, "// For worker interface named \"%s\"", p->name);
      num = "";
    } else
      fprintf(f, "// For worker interfaces named \"%s0\" to \"%s%u\"",
	      p->name, p->name, p->count - 1);
    fprintf(f, " WIP Attributes are:\n");
    switch (p->type) {
    case WCIPort:
      fprintf(f, "// SizeOfConfigSpace: %u (0x%x)\n", w->ctl.sizeOfConfigSpace,
	      w->ctl.sizeOfConfigSpace);
      break;
    case WSIPort:
      fprintf(f, "// DataValueWidth: %u\n", p->wdi.dataValueWidth);
      fprintf(f, "// MaxMessageValues: %u\n", p->wdi.maxMessageValues);
      fprintf(f, "// ZeroLengthMessages: %s\n",
	      p->wdi.zeroLengthMessages ? "true" : "false");
      fprintf(f, "// NumberOfOpcodes: %u\n", p->wdi.numberOfOpcodes);
      fprintf(f, "// DataWidth: %u\n", p->dataWidth);
      break;
    case WMIPort:
      fprintf(f, "// DataValueWidth: %u\n", p->wdi.dataValueWidth);
      fprintf(f, "// MaxMessageValues: %u\n", p->wdi.maxMessageValues);
      fprintf(f, "// ZeroLengthMessages: %s\n",
	      p->wdi.zeroLengthMessages ? "true" : "false");
      fprintf(f, "// NumberOfOpcodes: %u\n", p->wdi.numberOfOpcodes);
      fprintf(f, "// DataWidth: %u\n", p->dataWidth);
      break;
    case WMemIPort:
      fprintf(f, "// DataWidth: %u\n// MemoryWords: %llu (0x%llx)\n// ByteWidth: %u\n",
	      p->dataWidth, (unsigned long long)p->wmemi.memoryWords,
	      (unsigned long long)p->wmemi.memoryWords, p->byteWidth);
      fprintf(f, "// MaxBurstLength: %u\n", p->wmemi.maxBurstLength);
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
	fprintf(f, "typedef Wci_Es#(%u) ", p->ocp.MAddr.width);
	break;
      case WSIPort:
	fprintf(f, "typedef Wsi_E%c#(%u,%u,%u,%u,%u) ",
		p->master ? 'm' : 's',
		p->ocp.MBurstLength.width, p->dataWidth, p->ocp.MByteEn.width,
		p->ocp.MReqInfo.width, p->ocp.MDataInfo.width);
	break;
      case WMIPort:
	fprintf(f, "typedef Wmi_Em#(%u,%u,%u,%u,%u,%u) ",
		p->ocp.MAddr.width, p->ocp.MBurstLength.width, p->dataWidth,
		p->ocp.MDataInfo.width,p->ocp.MDataByteEn.width,
		p->ocp.MFlag.width ? p->ocp.MFlag.width : p->ocp.SFlag.width);
	break;
      case WMemIPort:
	fprintf(f, "typedef Wmemi_Em#(%u,%u,%u,%u) ",
		p->ocp.MAddr.width, p->ocp.MBurstLength.width, p->dataWidth, p->ocp.MDataByteEn.width);
	break;
      case WTIPort:
      default:
	;
      }
      fprintf(f, "I_%s%s;\n", p->name, num);
    }
  }
  fprintf(f,
	  "\n// Define the wrapper module around the real verilog module \"%s\"\n"
	  "interface V%sIfc;\n"
	  "  // First define the various clocks so they can be used in BSV across the OCP interfaces\n",
	  w->implName, w->implName);
#if 0
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
    if (p->clock->port == p)
      fprintf(f, "  Clock %s;\n", p->clock->signal);
  }
#endif
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      fprintf(f, "  interface I_%s%s i_%s%s;\n", p->name, num, p->name, num);
    }
  }
  fprintf(f,
	  "endinterface: V%sIfc\n\n", w->implName);
  fprintf(f,
	  "// Use importBVI to bind the signal names in the verilog to BSV methods\n"
	  "import \"BVI\" %s =\n"
	  "module vMk%s #(",
	  w->implName, w->implName);
  // Now we must enumerate the various input clocks and input resets as parameters
  const char *last = "";
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++)
    if (p->clock->port == p) {
      fprintf(f, "%sClock i_%sClk", last, p->name);
      last = ", ";
    }
  // Now we must enumerate the various reset inputs as parameters
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
    if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
			       !p->master && p->ocp.MReset_n.value)) {
      if (p->count > 1)
	fprintf(f, "%sVector#(%d,Reset) i_%sRst", last, p->count, p->name);
      else
	fprintf(f, "%sReset i_%sRst", last, p->name);
      last = ", ";
    }
  }
  fprintf(f,
	  ") (V%sIfc);\n\n"
	  "  default_clock no_clock;\n"
	  "  default_reset no_reset;\n\n"
	  "  // Input clocks on specific worker interfaces\n",
	  w->implName);
  
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++)
    if (p->clock->port == p)
      fprintf(f, "  input_clock  i_%sClk(%s) = i_%sClk;\n",
	      p->name, p->clock->signal, p->name);
    else
      fprintf(f, "  // Interface \"%s\" uses clock on interface \"%s\"\n", p->name, p->clock->port->name);
  fprintf(f, "\n  // Reset inputs for worker interfaces that have one\n");
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
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
		  p->name, num, signal, p->name, nn);
	else
	  fprintf(f, "  input_reset  i_%sRst(%s) = i_%sRst;\n",
		  p->name, signal, p->name);
      }
    }
  }
  unsigned en = 0;
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
    const char *num = "";
    for (nn = 0; nn < p->count; nn++) {
      if (p->count > 1)
	asprintf((char **)&num, "%u", nn);
      fprintf(f, "interface I_%s%s i_%s%s;\n", p->name, num, p->name, num);
      OcpSignalDesc *osd;
      OcpSignal *os;
      unsigned o;
      const char *reset;
      if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
				 !p->master && p->ocp.MReset_n.value)) {
	asprintf((char **)&reset, "i_%s%sRst", p->name, num); 
      } else
	reset = "no_reset";
      for (o = 0, os = p->ocp.signals, osd = ocpSignals; osd->name; osd++, os++, o++)
	if (os->value) {
	  char *signal;
	  asprintf(&signal, os->signal, nn);
	  
	  // Inputs
	  if (p->master != osd->master && o != OCP_Clk &&
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
		      p->clock->port->name, reset);
	    else
	      fprintf(f, "  method %c%s (%s) enable((*inhigh*)en%d) clocked_by(i_%sClk) reset_by(%s);\n",
		      tolower(osd->name[0]), osd->name + 1, signal, en++,
		      p->clock->port->name, reset);
	  }
	  if (p->master == osd->master) 
	    fprintf(f, "  method %s %c%s clocked_by(i_%sClk) reset_by(%s);\n",
		    signal, tolower(osd->name[0]), osd->name + 1,
		    p->clock->port->name, reset);
	}
      fprintf(f, "endinterface: i_%s%s\n\n", p->name, num);
    }
  }
  // warning suppression...
  fprintf(f, "schedule (\n");
  last = "";
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
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
	  fprintf(f, "%si_%s%s_%c%s", last, p->name, num, tolower(osd->name[0]), osd->name+1);
	  last = ", ";
	}
    }
  }
  fprintf(f, ")\n   CF  (\n");
  last = "";
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
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
	  fprintf(f, "%si_%s%s_%c%s", last, p->name, num, tolower(osd->name[0]), osd->name+1);
	  last = ", ";
	}
    }
  }
  fprintf(f, ");\n\n");
  fprintf(f, "\nendmodule: vMk%s\n", w->implName);
  fprintf(f,
	  "// Make a synthesizable Verilog module from our wrapper\n"
	  "(* synthesize *)\n"
	  "(* doc= \"Info about this module\" *)\n"
	  "module mk%s#(", w->implName);
  // Now we must enumerate the various input clocks and input resets as parameters
  last = "";
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++)
    if (p->clock->port == p) {
      fprintf(f, "%sClock i_%sClk", last, p->name);
      last = ", ";
    }
  // Now we must enumerate the various reset inputs as parameters
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
    if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
			       !p->master && p->ocp.MReset_n.value)) {
      if (p->count > 1)
	fprintf(f, "%sVector#(%d,Reset) i_%sRst", last, p->count, p->name);
      else
	fprintf(f, "%sReset i_%sRst", last, p->name);
      last = ", ";
    }
  }
  fprintf(f, ") (V%sIfc);\n", w->implName);
  fprintf(f,
	  "  let _ifc <- vMk%s(",
	  w->implName);
  last = "";
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++)
    if (p->clock->port == p) {
      fprintf(f, "%si_%sClk", last, p->name);
      last = ", ";
    }
  for (p = w->ports, n = 0; n < w->nPorts; n++, p++) {
    if (p->type == WCIPort && (p->master && p->ocp.SReset_n.value ||
			       !p->master && p->ocp.MReset_n.value)) {
      fprintf(f, "%si_%sRst", last, p->name);
      last = ", ";
    }
  }
  fprintf(f, ");\n"
	  "  return _ifc;\n"
	  "endmodule: mk%s\n\n"
	  "endpackage: I_%s\n",
	  w->implName, w->implName);
  return 0;
}

  void
emitWorker(FILE *f, Worker *w) 
{
  fprintf(f, "<worker name=\"%s\"", w->implName);
  if (w->ctl.controlOps) {
    fprintf(f, " controlOperations=\"");
    bool first = true;
    for (unsigned op = 0; op < NoOp; op++)
      if (op != ControlOpStart &&
	  w->ctl.controlOps & (1 << op)) {
	fprintf(f, "%s%s", first ? "" : ",", controlOperations[op]);
	first = false;
      }
    fprintf(f, "\"");
  }
  if (w->ports->type == WCIPort && w->ports->wci.timeout)
    fprintf(f, " Timeout=\"%u\"", w->ports->wci.timeout);
  fprintf(f, ">\n");
  unsigned nn;
  Property *prop;
  for (prop = w->ctl.properties, nn = 0; nn < w->ctl.nProperties; nn++, prop++) {
    fprintf(f, "<property name=\"%s\" type=\"%s\"",
	    prop->name, propertyTypes[prop->types->type]);
    if (prop->isReadable)
      fprintf(f, " readable=\"true\"");
    if (prop->isWritable)
      fprintf(f, " writable=\"true\"");
    if (prop->readSync)
      fprintf(f, " readSync=\"true\"");
    if (prop->writeSync)
      fprintf(f, " writeSync=\"true\"");
    if (prop->readError)
      fprintf(f, " readError=\"true\"");
    if (prop->writeError)
      fprintf(f, " writeError=\"true\"");
    if (prop->types->type == CM::Property::CPI_String)
      fprintf(f, " size=\"%u\"", prop->types->stringLength);
    if (prop->types->isSequence)
      fprintf(f, " sequenceSize=\"%u\"\n", prop->types->length);
    fprintf(f, "/>\n");
  }
  Port *p;
  for (p = w->ports, nn = 0; nn < w->nPorts; nn++, p++)
    if (p->isData) {
      fprintf(f, "<port name=\"%s\" provider=\"%s\" minBufferSize=\"%u\"",
	      p->name, p->wdi.isProducer ? "false" : "true",
	      (p->wdi.maxMessageValues * p->wdi.dataValueWidth + 7) / 8);
      if (p->wdi.minBuffers)
	fprintf(f, " minNumBuffers=\"%u\"", p->wdi.minBuffers);
      fprintf(f, "/>\n");
    }
  fprintf(f, "</worker>\n");
}

 static void
emitInstance(Instance *i, FILE *f)
{
  fprintf(f, "<%s name=\"%s\" worker=\"%s\" occpIndex=\"%u\"",
	  i->isInterconnect ? "interconnect" : "instance",
	  i->name, i->worker->implName, i->index);
  if (i->attach)
    fprintf(f, " attachment=\"%s\"", i->attach);
  fprintf(f, "/>\n");
}

// Emit the artifact XML.
const char *
emitArtHDL(Worker *aw, const char *outDir, const char *hdlDep) {
  const char *err;
  FILE *f;
  char *cname = strdup(hdlDep);
  char *cp = strrchr(cname, '/');
  if (cp)
    cname = cp+1;
  char *dot = strchr(cname, '.');
  if (dot)
    *dot = '\0';
  if ((err = openOutput(cname, outDir, "", "_art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", hdlDep);
  fprintf(f,
	  " This file contains the artifact descriptor XML for the application assembly\n"
	  " named \"%s\". It must be attached (appended) to the bitstream\n",
	  aw->implName);
  fprintf(f, "  -->\n");
  ezxml_t dep;
  Worker *dw = myCalloc(Worker, 1);
  if ((err = parseFile(hdlDep, 0, "HdlContainer", &dep, 0)) ||
      (err = parseHdlAssy(dep, hdlDep, dw)))
    return err;
  fprintf(f, "<artifact>\n");
  // Define all workers
  Worker *w;
  unsigned n;
  for (w = aw->assembly.workers, n = 0; n < aw->assembly.nWorkers; n++, w++)
    emitWorker(f, w);
  Instance *i, *di;
  unsigned nn;
  // For each app instance, we need to retrieve the index within the container
  // Then emit that app instance's info
  for (i = aw->assembly.instances, n = 0; n < aw->assembly.nInstances; n++, i++) {
    for (di = dw->assembly.instances, nn = 0; nn < dw->assembly.nInstances; nn++, di++) {
      if (di->name && !strcmp(di->name, i->name)) {
	i->index = di->index;
	break;
      }
    }
    if (!nn >= dw->assembly.nInstances)
      return esprintf("No instance in container assembly for assembly instance \"%s\"",
		      i->name);
    emitInstance(i, f);
  }
  // Now emit the container's instances
  for (di = dw->assembly.instances, nn = 0; nn < dw->assembly.nInstances; nn++, di++)
    if (di->worker)
      emitInstance(di, f);
  // Emit the connections between the container and the application
  Connection *cc, *ac;
  for (cc = dw->assembly.connections, n = 0; n < dw->assembly.nConnections; n++, cc++)
    for (ac = aw->assembly.connections, nn = 0; nn < aw->assembly.nConnections; nn++, ac++)
      if (!strcmp(ac->name, cc->name)) {
	if (ac->external->isProducer == cc->external->isProducer)
	  return "container connection same direction as application connection";
	InstancePort *aip, *cip;
	for (aip = ac->ports; aip; aip = aip->nextConn)
	  if (aip != ac->external)
	    break;
	for (cip = cc->ports; cip; cip = cip->nextConn)
	  if (cip != cc->external)
	    break;
	if (ac->external->isProducer)
	  // Application is consuming from an external producer
	  fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
		  cip->instance->name, cip->port->name,
		  aip->instance->name, aip->port->name);
	else
	  // Application is producing to an external consumer
	  fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
		  aip->instance->name, aip->port->name,
		  cip->instance->name, cip->port->name);
      }
  // Emit the connections inside the application
  for (ac = aw->assembly.connections, nn = 0; nn < aw->assembly.nConnections; nn++, ac++)
    if (!ac->external) {
      InstancePort *aip;
      InstancePort *from, *to;
      for (aip = ac->ports; aip; aip = aip->nextConn)
	if (aip->isProducer)
	  from = aip;
        else
	  to = aip;
      fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
	      from->instance->name, from->port->name,
	      to->instance->name, to->port->name);
    }
  fprintf(f, "</artifact>\n");
  if (fclose(f))
    return "Could close output file. No space?";
  return 0;
}
