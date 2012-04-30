
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <uuid/uuid.h>
// FIXME: integrate this into our UUID utility properly
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[50]; // darwin has 37 - lousy unsafe interface
#endif
#include "HdlOCCP.h"
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
	sprintf(s, "%d", wn + (pat[-1] - '0'));
	while (*s) s++;
	break;
#if 1
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



// Emit the file that can be used to instantiate the worker
 const char *
emitDefsHDL(Worker *w, const char *outDir, bool wrap) {
  const char *err;
  FILE *f;
  Language lang = wrap ? (w->language == VHDL ? Verilog : VHDL) : w->language;
  if ((err = openOutput(w->implName, outDir, "", DEFS, lang == VHDL ? VHD : ".vh", NULL, f)))
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
  OcpSignalDesc *osd;
  if (lang == VHDL)
    for (unsigned i = 0; i < w->ports.size(); i++) {
      Port *p = w->ports[i];
      bool mIn = p->masterIn();
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
	    "`ifndef NOT_EMPTY_%s\n"
	    "(* box_type=\"user_black_box\" *)\n"
	    "`endif\n"
	    "module %s (\n", w->implName, w->implName);
  // port name is scoped by entity, just has to avoid record name
  // _in or _out
  // type conventions _t or _Typ or T or..
  // port signal (wsi consumer port foo):
  //  foo_s_out: out spec_impl.foo_s_t;
  Clock *c = w->clocks;
  bool first = true;
  for (unsigned i = 0; i < w->nClocks; i++, c++) {
    if (!c->port) {
      if (first)
	fprintf(f,
		"  // Clocks not associated with one specific interface:\n");
      first = false;
      fprintf(f, "  %s, %*s// input\n",
	      c->signal, (int)(22 - strlen(c->signal)), "");
    }
  }
  char *last = 0;
  for (unsigned i = 0; i < w->ports.size(); i++) {
    Port *p = w->ports[i];
    if (last) {
      fprintf(f, last, ',');
      last = 0;
    }
    bool mIn = p->masterIn();
    char *nbuf;
    asprintf(&nbuf, " %d", p->count);
    fprintf(f,
	    "\n  %s%s The%s %s %sinterface%s named \"%s\", with \"%s\" acting as OCP %s:\n",
	    lang == VHDL ? "  " : "", comment, p->count > 1 ? nbuf : "", wipNames[p->type],
	    p->type == WMIPort || p->type == WSIPort ?
	    (p->u.wdi.isProducer ? "producer " : "consumer ") : "",
	    p->count > 1 ? "s" : "", p->name, w->implName, mIn ? "slave" : "master");
    fprintf(f, "  // WIP attributes for this %s interface are:\n", wipNames[p->type]);
    if (p->clockPort)
      fprintf(f, "  //   Clock: uses the clock from interface named \"%s\"\n",
	      p->clockPort->name);
    else if (p->myClock)
      fprintf(f, "  //   Clock: this interface has its own clock, named \"%s\"\n",
	      p->clock->signal);
    else
      fprintf(f, "  //   Clock: this interface uses the worker's clock named \"%s\"\n",
	      p->clock->signal);
    switch (p->type) {
      case WCIPort:
	fprintf(f, "  //   SizeOfConfigSpace: %llu (0x%llx)\n",
		(unsigned long long)w->ctl.sizeOfConfigSpace,
		(unsigned long long)w->ctl.sizeOfConfigSpace);
	fprintf(f, "  //   WritableConfigProperties: %s\n", BOOL(w->ctl.writableConfigProperties));
	fprintf(f, "  //   ReadableConfigProperties: %s\n", BOOL(w->ctl.readableConfigProperties));
	fprintf(f, "  //   Sub32BitConfigProperties: %s\n", BOOL(w->ctl.sub32BitConfigProperties));
	fprintf(f, "  //   ControlOperations (in addition to the required \"start\"): ");
	{
	  bool first = true;
	  for (unsigned op = 0; op < NoOp; op++, first = false)
	    if (op != ControlOpStart &&
		w->ctl.controlOps & (1 << op))
	      fprintf(f, "%s%s", first ? "" : ",", controlOperations[op]);
	}
	fprintf(f, "\n");
	fprintf(f, "  //   ResetWhileSuspended: %s\n", BOOL(p->u.wci.resetWhileSuspended));
	break;
    case WSIPort:
    case WMIPort:
      // Do common WDI attributes first
      fprintf(f, "  //   Protocol: \"%s\"\n", p->protocol->name().c_str());
      fprintf(f, "  //   DataValueWidth: %u\n", p->protocol->m_dataValueWidth);
      fprintf(f, "  //   DataValueGranularity: %u\n", p->protocol->m_dataValueGranularity);
      fprintf(f, "  //   DiverseDataSizes: %s\n", BOOL(p->protocol->m_diverseDataSizes));
      fprintf(f, "  //   MaxMessageValues: %u\n", p->protocol->m_maxMessageValues);
      fprintf(f, "  //   NumberOfOpcodes: %u\n", p->u.wdi.nOpcodes);
      fprintf(f, "  //   Producer: %s\n", BOOL(p->u.wdi.isProducer));
      fprintf(f, "  //   VariableMessageLength: %s\n", BOOL(p->protocol->m_variableMessageLength));
      fprintf(f, "  //   ZeroLengthMessages: %s\n", BOOL(p->protocol->m_zeroLengthMessages));
      fprintf(f, "  //   Continuous: %s\n", BOOL(p->u.wdi.continuous));
      fprintf(f, "  //   DataWidth: %u\n", p->dataWidth);
      fprintf(f, "  //   ByteWidth: %u\n", p->byteWidth);
      fprintf(f, "  //   ImpreciseBurst: %s\n", BOOL(p->impreciseBurst));
      fprintf(f, "  //   Preciseburst: %s\n", BOOL(p->preciseBurst));
      if (p->type == WSIPort) {
	fprintf(f, "  //   Abortable: %s\n", BOOL(p->u.wsi.abortable));
	fprintf(f, "  //   EarlyRequest: %s\n", BOOL(p->u.wsi.earlyRequest));
      } else if (p->type == WMIPort)
	fprintf(f, "  //   TalkBack: %s\n", BOOL(p->u.wmi.talkBack));
      break;
    case WTIPort:
      break;
    case WMemIPort:
      fprintf(f, "  //   DataWidth: %u\n", p->dataWidth);
      fprintf(f, "  //   ByteWidth: %u\n", p->byteWidth);
      fprintf(f, "  //   ImpreciseBurst: %s\n", BOOL(p->impreciseBurst));
      fprintf(f, "  //   Preciseburst: %s\n", BOOL(p->preciseBurst));
      fprintf(f, "  //   MemoryWords: %llu\n", (unsigned long long )p->u.wmemi.memoryWords);
      fprintf(f, "  //   MaxBurstLength: %u\n", p->u.wmemi.maxBurstLength);
      fprintf(f, "  //   WriteDataFlowControl: %s\n", BOOL(p->u.wmemi.writeDataFlowControl));
      fprintf(f, "  //   ReadDataFlowControl: %s\n", BOOL(p->u.wmemi.readDataFlowControl));
    default:
      ;
    }
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
		  p->clock->signal, (int)(22 - strlen(p->clock->signal)), "");
	p->ocp.Clk.signal = p->clock->signal;
      } else if (n == 0)
	fprintf(f,
		"  %s%s No Clk signal here. The \"%s\" interface uses \"%s\" as clock,\n",
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
	    fprintf(f, "  %s, %*s// input  ", name, (int)(22 - strlen(name)), "");
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
		i < w->ports.size() - 1 || p->count > n+1 ? ";" : "");
      else {
	osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	  if ((osd->master != mIn && strcmp(osd->name, "Clk")) && os->value) {
	    if (last)
	      fprintf(f, last, ',');
	    char *name;
	    asprintf(&name, os->signal, n);
	    asprintf(&last, "  %s%%c %*s// output ", name,
		     (int)(22 - strlen(name)), "");
	    if (osd->vector)
	      asprintf(&last, "%s[%3u:0]\n", last, os->width - 1);
	    else
	      asprintf(&last, "%s\n", last);
	  }
      }
    }
  }
  if (w->nSignals) {
    if (last)
      fprintf(f, last, ',');
    last = 0;
    fprintf(f, "  // Extra signals not part of any WIP interface:\n");
    Signal *s = w->signals;
    for (unsigned n = 0; n < w->nSignals; n++, s++) {
      if (last)
	fprintf(f, last, ',');
      asprintf(&last, "  %s%%c %*s// %s ", s->name,
	       (int)(22 - strlen(s->name)), " ",
	       s->direction == Signal::IN ? "input " :
	       (s->direction == Signal::OUT ? "output" : "inout "));
      if (s->width)
	asprintf(&last, "%s[%3u:0]\n", last, s->width - 1);
      else
	asprintf(&last, "%s\n", last);
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
    // Now we emit parameters.
    for (PropertiesIter pi = w->ctl.properties.begin(); pi != w->ctl.properties.end(); pi++)
      if ((*pi)->m_isParameter) {
	OU::Property *pr = *pi;
	if (w->language == VHDL) {
	  fprintf(f, "VHDL PARAMETERS HERE\n");
	} else {
	  int64_t i64 = 0;
	  if (pr->m_defaultValue)
	    switch (pr->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
	      case OA::OCPI_##pretty:	   \
		i64 = (int64_t)pr->m_defaultValue->m_##pretty; break;
	      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	  default:;
	  }
	  unsigned bits =
	    pr->m_baseType == OA::OCPI_Bool ?
	    1 : pr->m_nBits;
	  fprintf(f, "  parameter [%u:0] %s = %u'h%llx;\n",
		  bits - 1, pr->m_name.c_str(), bits, (long long)i64);
	}
      }
    // Now we emit the declarations (input, output, width) for each module port
    c = w->clocks;
    for (unsigned i = 0; i < w->nClocks; i++, c++)
      if (!c->port)
	fprintf(f, "  input          %s;\n", c->signal);
    for (unsigned i = 0; i < w->ports.size(); i++) {
      Port *p = w->ports[i];
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
    if (w->nSignals) {
      fprintf(f, "  // Extra signals not part of any WIP interface:\n");
      Signal *s = w->signals;
      for (unsigned n = 0; n < w->nSignals; n++, s++) {
	const char *dir =
	  s->direction == Signal::IN ? "input " :
	  (s->direction == Signal::OUT ? "output" : "inout ");
	if (s->width)
	  fprintf(f, "  %s [%3u:0] %s;\n", dir, s->width - 1, s->name);
	else
	  fprintf(f, "  %s         %s;\n", dir, s->name);
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
 static void
emitOpcodes(Port *p, FILE *f, const char *pName, const char *comment) {
   Protocol *prot = p->protocol;
   if (prot && prot->nOperations()) {
     OU::Operation *op = prot->operations();
     fprintf(f,
	     "  %s Opcode/operation value declarations for protocol \"%s\" on interface \"%s\"\n",
	     comment, prot->m_name.c_str(), p->name);
     for (unsigned n = 0; n < prot->nOperations(); n++, op++)
       fprintf(f, "  localparam [%sOpCodeWidth - 1 : 0] %s%s_Op = %u;\n", pName, pName, op->name().c_str(), n);
   }
}

// Generate the readonly implementation file.
// What implementations must explicitly (verilog) or implicitly (VHDL) include.
 const char *
emitImplHDL(Worker *w, const char *outDir, const char *library) {
  const char *err;
  FILE *f;
  if ((err = openOutput(w->implName, outDir, "", IMPL, w->language == VHDL ? VHD : ".vh", NULL, f)))
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
	    "`include \"ocpi_wip_defs%s\"\n",
	    w->implName, w->implName, DEFS, VERH, w->implName, DEFS, VERH, VERH);

  // For VHDL we need to basically replicate the port declarations that are part of the
  // component: there is no way to just use what the component decl says.
  // For Verilog we just include the module declaration so don't need anything here.
  if (w->language == VHDL) {
    // port name is scoped by entity, just has to avoid record name
    // _in or _out
    // type conventions _t or _Typ or T or..
    // port signal (wsi consumer port foo):
    //  foo_s_out: out spec_impl.foo_s_t;
    for (unsigned i = 0; i < w->ports.size(); i++) {
      Port *p = w->ports[i];
      bool mIn = p->masterIn();
      char *nbuf;
      asprintf(&nbuf, " %d", p->count);
      fprintf(f,
	      "\n    %s For the %s%s %sinterface%s named \"%s\", with %s acting as OCP %s\n",
	      comment, p->count > 1 ? nbuf : "", wipNames[p->type],
	      p->type == WMIPort || p->type == WSIPort ?
	      (p->u.wdi.isProducer ? "producer " : "consumer ") : "",
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
		i < w->ports.size() - 1 || p->count > n+1 ? ";" : "");
      }
    }
    fprintf(f, "  );\n");
  }
  // Aliases for OCP signals
  for (unsigned i = 0; i < w->ports.size(); i++) {
    Port *p = w->ports[i];
    bool mIn = p->masterIn();
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
                  "  wire [2:0] %sControlOp = %sMAddr[4:2];\n",
		  pin, pin, pin, pin, pin, pin);
	  if (w->ctl.sizeOfConfigSpace)
	    fprintf(f,
		    "  wire %sConfig    = %sMAddrSpace[0];\n"
		    "  wire %sIsCfgWrite = %sMCmd == OCPI_OCP_MCMD_WRITE &&\n"
		    "                           %sMAddrSpace[0] == OCPI_WCI_CONFIG;\n"
		    "  wire %sIsCfgRead = %sMCmd == OCPI_OCP_MCMD_READ &&\n"
		    "                          %sMAddrSpace[0] == OCPI_WCI_CONFIG;\n"
		    "  wire %sIsControlOp = %sMCmd == OCPI_OCP_MCMD_READ &&\n"
		    "                            %sMAddrSpace[0] == OCPI_WCI_CONTROL;\n",
		  pin, pin, pin, pin, pin, pin, pin, pin, pin,
		  pin, pin);
	  else
	    fprintf(f,
		    "  wire %sIsControlOp = %sMCmd == OCPI_OCP_MCMD_READ;\n", pin, pin);
	  fprintf(f,
		  "  assign %sSFlag[1] = 1; // indicate that this worker is present\n"
		  "  // This assignment requires that the %sAttention be used, not SFlag[0]\n"
		  "  reg %sAttention; assign %sSFlag[0] = %sAttention;\n",
		  pin, pout, pout, pout, pout);
	}
	if (w->ctl.properties.size() && n == 0) {
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
	  for (PropertiesIter pi = w->ctl.properties.begin(); pi != w->ctl.properties.end(); pi++)
	    if (!(*pi)->m_isParameter) {
	      OU::Property *pr = *pi;
	      if (w->language == VHDL) {
		fprintf(f,
			"  constant %-20s : Property_t := b\"", pr->m_name.c_str());
		for (int b = p->ocp.MAddr.width-1; b >= 0; b--)
		  fprintf(f, "%c", pr->m_offset & (1 << b) ? '1' : '0');
		fprintf(f, "\"; -- 0x%0*x\n",
			(int)roundup(p->ocp.MAddr.width, 4)/4, pr->m_offset);
	      } else
		fprintf(f, "  localparam [%d:0] %sAddr = %d'h%0*x;\n",
			p->ocp.MAddr.width - 1, pr->m_name.c_str(), p->ocp.MAddr.width,
			(int)roundup(p->ocp.MAddr.width, 4)/4, pr->m_offset);
	    }
	}
	break;
      case WSIPort:
	if (p->u.wsi.regRequest) {
	  fprintf(f,
		  "  %s Register declarations for request phase signals for interface \"%s\"\n",
		  comment, p->name);
	  OcpSignalDesc *osd = ocpSignals;
	  for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	    if (osd->request && p->u.wdi.isProducer && p->u.wsi.regRequest && os->value &&
		strcmp("MReqInfo", osd->name)) { // fixme add "aliases" attribute somewhere
	      if (osd->vector)
		fprintf(f, "  reg [%3u:0] %s%s;\n", os->width - 1, pout, osd->name);
	      else
		fprintf(f, "  reg %s%s;\n", pout, osd->name);
	    }
	}
	fprintf(f,
		"  %s Aliases for interface \"%s\"\n", comment, p->name);
	if (p->ocp.MReqInfo.width) {
	  if (n == 0) {
	    if (w->language == VHDL)
	      fprintf(f,
		      "  subtype %s%s_OpCode_t is std_logic_vector(%d downto 0);\n",
		      p->name, mIn ? pin : pout, p->ocp.MReqInfo.width - 1);
	    else
	      fprintf(f,
		      "  localparam %sOpCodeWidth = %d;\n",
		      mIn ? pin : pout, p->ocp.MReqInfo.width);
          }
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
		    p->u.wsi.regRequest ? "reg" : "wire", p->ocp.MReqInfo.width - 1, pout, pout, pout);
	  emitOpcodes(p, f, mIn ? pin : pout, comment);
	}
	if (p->ocp.MFlag.width) {
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
        }
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
	if (p->u.wdi.nOpcodes > 1) {
	  if (w->language == VHDL) {
	    if (n == 0)
	      fprintf(f,
		      "  subtype %s%sOpCode_t is std_logic_vector(7 downto 0);\n",
		      p->name, p->u.wdi.isProducer ? pout : pin);
	    fprintf(f,
		    "  alias %sOpcode: %s%sOpCode_t is %s.%cFlag(7 downto 0);\n",
		    p->u.wdi.isProducer ? pout : pin,
		    p->name, p->u.wdi.isProducer ? pout : pin,
		    p->u.wdi.isProducer ? pout : pin,
		    p->u.wdi.isProducer ? 'M' : 'S');
	  } else {
	    if (p->u.wdi.isProducer) // opcode is an output
	      fprintf(f,
		      "  wire [7:0] %sOpcode; assign %s%cFlag[7:0] = %sOpcode;\n",
		      pout, pout, p->master ? 'M' : 'S', pout);
	    else
	      fprintf(f,
		      "  wire [7:0] %sOpcode = %s%cFlag[7:0];\n",
		      pin, pin, p->master ? 'S' : 'M');
	    fprintf(f,
		    "  localparam %sOpCodeWidth = 7;\n",
		    mIn ? pin : pout);
	  }
	  emitOpcodes(p, f, mIn ? pin : pout, comment);
	}
	if (p->protocol->m_variableMessageLength) {
	  if (w->language == VHDL) {
	    if (n == 0)
	      fprintf(f,
		      "  subtype %s%sLength_t is std_logic_vector(%d downto 8);\n",
		      p->name, p->u.wdi.isProducer ? pout : pin,
		      (p->u.wdi.isProducer ? p->ocp.MFlag.width : p->ocp.MFlag.width) - 1);
	    fprintf(f,
		    "  alias %sLength: %s%s_Length_t is %s.%cFlag(%d downto 0);\n",
		    p->u.wdi.isProducer ? pout : pin,
		    p->name, p->u.wdi.isProducer ? pout : pin,
		    p->u.wdi.isProducer ? pout : pin,
		    p->u.wdi.isProducer ? 'M' : 'S',
		    (p->u.wdi.isProducer ? p->ocp.MFlag.width : p->ocp.MFlag.width) - 9);
	  } else {
	    if (p->u.wdi.isProducer) { // length is an output
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
  if (w->language == VHDL)
    fprintf(f,
	    "end entity %s;\n",	w->implName);
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
	    "`include \"%s_impl%s\"\n\n",
	    w->implName, w->implName, VERH);
    for (unsigned i = 0; i < w->ports.size(); i++) {
      Port *p = w->ports[i];
      switch (p->type) {
      case WSIPort:
	if (p->u.wsi.regRequest)
	  fprintf(f,
		  "// GENERATED: OCP request phase signals for interface \"%s\" are registered\n",
		  p->name);
      default:
	;
      }
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
  if (prod->protocol->m_dataValueWidth != cons->protocol->m_dataValueWidth)
    return "dataValueWidth incompatibility for connection";
  if (prod->protocol->m_dataValueGranularity < cons->protocol->m_dataValueGranularity ||
      prod->protocol->m_dataValueGranularity % cons->protocol->m_dataValueGranularity)
    return "dataValueGranularity incompatibility for connection";
  if (prod->protocol->m_maxMessageValues > cons->protocol->m_maxMessageValues)
    return "maxMessageValues incompatibility for connection";
  if (prod->u.wdi.nOpcodes > cons->u.wdi.nOpcodes)
    return "numberOfOpcodes incompatibility for connection";
  if (prod->protocol->m_variableMessageLength && !cons->protocol->m_variableMessageLength)
    return "variable length producer vs. fixed length consumer incompatibility";
  if (prod->protocol->m_zeroLengthMessages && !cons->protocol->m_zeroLengthMessages)
    return "zero length message incompatibility";
  if (prod->type != cons->type)
    return "profile incompatibility";
  if (cons->u.wdi.continuous && !prod->u.wdi.continuous)
    return "continuous incompatibility";
  if (prod->dataWidth != prod->dataWidth)
    return "dataWidth incompatibility";
  if (cons->u.wdi.continuous && !prod->u.wdi.continuous)
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
    if (cons->u.wsi.abortable) {
      if (!prod->u.wsi.abortable) {
	oa = &consumer->ocp[OCP_MFlag];
	oa->expr = "0";
	oa->comment = "Tell consumer no frames are ever aborted";
      }
    } else if (prod->u.wsi.abortable)
      return "consumer cannot handle aborts from producer";
    // EarlyRequest compatibility and adaptation
    if (cons->u.wsi.earlyRequest) {
      if (!prod->u.wsi.earlyRequest) {
	oa = &consumer->ocp[OCP_MDataLast];
	oa->other = OCP_MReqLast;
	oa->expr = "%s";
	oa->comment = "Tell consumer last data is same as last request";
	oa = &consumer->ocp[OCP_MDataValid];
	oa->other = OCP_MCmd;
	oa->expr = "%s == OCPI_OCP_MCMD_WRITE ? 1 : 0";
	oa->comment = "Tell consumer data is valid when request is MCMD_WRITE";
      }
    } else if (prod->u.wsi.earlyRequest)
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
	  w->implName, w->implName, w->implName, DEFS, VERH, w->implName, DEFS, VERH);
  Assembly *a = &w->assembly;
  unsigned n;
  Connection *c;
  OcpSignalDesc *osd;
  OcpSignal *os;
  InstancePort *ip;
  fprintf(f, "// Define signals for connections that are not externalized\n\n");
  fprintf(f, "wire[255: 0] nowhere; // for passing output ports\n");
  // Define the inside-the-assembly signals, and also figure out the necessary tieoffs or
  // simple expressions when there is a simple adaptation.
  for (n = 0, c = a->connections; n < a->nConnections; n++, c++)
    if (c->nExternals == 0) {
      InstancePort *master = 0, *slave = 0, *producer = 0, *consumer = 0;
      for (ip = c->ports; ip; ip = ip->nextConn) {
	if (ip->port->master)
	  master = ip;
	else
	  slave = ip;
	if (ip->port->u.wdi.isProducer)
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
      // Generate signals when both sides has the signal configured.
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
  // Create the instances
  Instance *i;
  for (n = 0, i = a->instances; n < a->nInstances; n++, i++) {
    fprintf(f, "%s", i->worker->implName);
    if (i->nValues) {
      bool any = false;
      unsigned n = 0;
      // Emit the compile-time properties (a.k.a. parameter properties).
      for (InstanceProperty *pv = i->properties; n < i->nValues; n++, pv++) {
	OU::Property *pr = pv->property;
	if (pr->m_isParameter) {
	  if (w->language == VHDL) {
	    fprintf(f, "VHDL PARAMETERS HERE\n");
	  } else {
	    fprintf(f, "%s", any ? ", " : " #(");
	    any = true;
	    int64_t i64 = 0;
	    switch (pr->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			\
	    case OA::OCPI_##pretty:				\
	      i64 = (int64_t)pv->value.m_##pretty;			\
	      break;
OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
            default:;
	    }
	    unsigned bits =
	      pr->m_baseType == OA::OCPI_Bool ?
	      1 : pr->m_nBits;
	    fprintf(f, ".%s(%u'b%lld)",
		    pr->m_name.c_str(), bits, (long long)i64);
	  }
        }
      }
      if (any)
	fprintf(f, ")");
    }
    fprintf(f, " %s (\n",i->name);
    unsigned nn;
    // For the instance, define the clock signals that are defined separate from
    // any interface/port.
    Clock *c = i->worker->clocks;
    for (nn = 0; nn < i->worker->nClocks; nn++, c++) {
      if (!c->port) {
	fprintf(f, "  .%s(%s)\n", c->signal,
		i->clocks[c - i->worker->clocks]->signal);
      }
    }
    const char *last = "", *comment = "";
    OcpAdapt *oa;
    for (ip = i->ports, nn = 0; nn < i->worker->ports.size(); nn++, ip++)
      for (osd = ocpSignals, os = ip->port->ocp.signals, oa = ip->ocp;
	   osd->name; os++, osd++, oa++)
	// If the signal is in the interface
	if (os->value) {
	  char *signal = 0; // The signal to connect this port to.
	  const char *thisComment = "";
	  if (os == &ip->port->ocp.Clk)
	    signal = strdup(i->clocks[ip->port->clock - i->worker->clocks]->signal);
	  else if (ip->connection)
	    // If the signal is attached to a connection
	    if (ip->connection->nExternals == 0) {
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
	      if ((err = pattern(w, p, 0, 0, p->u.wdi.isProducer, p->master, &suff)))
		return err;
	      asprintf((char **)&signal, "%s%s", suff, osd->name);
	    }
	  else if (ip->externalPort) {
	    // This port is indeed connected to an external, non-data-plane wip interface
	    OcpSignal *exf =
	      // was: &i->ports[ip->port - i->worker->ports].external->ocp.signals[os - ip->port->ocp.signals];
	      &ip->externalPort->ocp.signals[os - ip->port->ocp.signals];
	    const char *externalName;

	    asprintf((char **)&externalName, exf->signal, n);
	    // Ports with no data connection
	    switch (ip->port->type) {
	    case WCIPort:
	      switch (osd - ocpSignals) {
	      case OCP_MAddr:
		if (os->width < exf->width)
		  thisComment = "worker is narrower than external, which is OK";
		//asprintf((char **)&signal, "%s[%u:0]", externalName, os->width - 1);
		break;
	      }
	      break;
	    default:
	      ;
	    }
	    if (!signal)
	      signal = strdup(externalName);
	  } else {
	    // A truly unconnected port.  All we want is a tieoff if it is an input
	    // We can always use zero since that will assert reset
	    if (osd->master != ip->port->master)
	      asprintf(&signal,"%u'b0", os->width);
	  }
	  if (signal) {
	    fprintf(f, "%s%s%s%s  .%s(%s",
		    last, comment[0] ? " // " : "", comment, last[0] ? "\n" : "", os->signal, signal);
#if 0
	    if (osd->vector && !isdigit(*signal))
	      fprintf(f, "[%u:0]", width - 1);
#endif
	    fprintf(f, ")");
	    last = ",";
	    comment = oa->comment ? oa->comment : thisComment;
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
	  "import OCWip::*; // Include the OpenOCPI BSV WIP package\n\n"
	  "import Vector::*;\n"
	  "// Define parameterized types for each worker port\n"
	  "//  with parameters derived from WIP attributes\n\n",
	  w->implName);
  unsigned n, nn;
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
      fprintf(f, "// SizeOfConfigSpace: %llu (0x%llx)\fn",
	      (unsigned long long)w->ctl.sizeOfConfigSpace,
	      (unsigned long long)w->ctl.sizeOfConfigSpace);
      break;
    case WSIPort:
      fprintf(f, "// DataValueWidth: %u\n", p->protocol->m_dataValueWidth);
      fprintf(f, "// MaxMessageValues: %u\n", p->protocol->m_maxMessageValues);
      fprintf(f, "// ZeroLengthMessages: %s\n",
	      p->protocol->m_zeroLengthMessages ? "true" : "false");
      fprintf(f, "// NumberOfOpcodes: %u\n", p->u.wdi.nOpcodes);
      fprintf(f, "// DataWidth: %u\n", p->dataWidth);
      break;
    case WMIPort:
      fprintf(f, "// DataValueWidth: %u\n", p->protocol->m_dataValueWidth);
      fprintf(f, "// MaxMessageValues: %u\n", p->protocol->m_maxMessageValues);
      fprintf(f, "// ZeroLengthMessages: %s\n",
	      p->protocol->m_zeroLengthMessages ? "true" : "false");
      fprintf(f, "// NumberOfOpcodes: %u\n", p->u.wdi.nOpcodes);
      fprintf(f, "// DataWidth: %u\n", p->dataWidth);
      break;
    case WMemIPort:
      fprintf(f, "// DataWidth: %u\n// MemoryWords: %llu (0x%llx)\n// ByteWidth: %u\n",
	      p->dataWidth, (unsigned long long)p->u.wmemi.memoryWords,
	      (unsigned long long)p->u.wmemi.memoryWords, p->byteWidth);
      fprintf(f, "// MaxBurstLength: %u\n", p->u.wmemi.maxBurstLength);
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
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
    if (p->clock->port == p) {
      fprintf(f, "%sClock i_%sClk", last, p->name);
      last = ", ";
    }
  }
  // Now we must enumerate the various reset inputs as parameters
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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

  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
    if (p->clock->port == p)
      fprintf(f, "  input_clock  i_%sClk(%s) = i_%sClk;\n",
	      p->name, p->clock->signal, p->name);
    else
      fprintf(f, "  // Interface \"%s\" uses clock on interface \"%s\"\n", p->name, p->clock->port->name);
  }
  fprintf(f, "\n  // Reset inputs for worker interfaces that have one\n");
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
    if (p->clock->port == p) {
      fprintf(f, "%sClock i_%sClk", last, p->name);
      last = ", ";
    }
  }
  // Now we must enumerate the various reset inputs as parameters
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
    if (p->clock->port == p) {
      fprintf(f, "%si_%sClk", last, p->name);
      last = ", ";
    }
  }
  for (n = 0; n < w->ports.size(); n++) {
    Port *p = w->ports[n];
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
  fprintf(f, "<worker name=\"%s\" model=\"%s\"", w->implName, w->modelString);
  if (strcmp(w->specName, w->implName))
    fprintf(f, " specname=\"%s\"", w->specName);
  if (w->ctl.sizeOfConfigSpace)
    fprintf(f, " sizeOfConfigSpace=\"%llu\"", (unsigned long long)w->ctl.sizeOfConfigSpace);
  if (w->ctl.controlOps) {
    bool first = true;
    for (unsigned op = 0; op < NoOp; op++)
      if (op != ControlOpStart &&
	  w->ctl.controlOps & (1 << op)) {
	fprintf(f, "%s%s", first ? " controlOperations=\"" : ",",
		controlOperations[op]);
	first = false;
      }
    if (!first)
      fprintf(f, "\"");
  }
  if (w->ports[0]->type == WCIPort && w->ports[0]->u.wci.timeout)
    fprintf(f, " Timeout=\"%u\"", w->ports[0]->u.wci.timeout);
  fprintf(f, ">\n");
  unsigned nn;
  for (PropertiesIter pi = w->ctl.properties.begin(); pi != w->ctl.properties.end(); pi++) {
    OU::Property *prop = *pi;
    if (prop->m_isParameter)
      continue;
    prop->printAttrs(f, "property", 1);
    if (prop->m_isReadable)
      fprintf(f, " readable=\"true\"");
    if (prop->m_isWritable)
      fprintf(f, " writable=\"true\"");
    if (prop->m_readSync)
      fprintf(f, " readSync=\"true\"");
    if (prop->m_writeSync)
      fprintf(f, " writeSync=\"true\"");
    if (prop->m_readError)
      fprintf(f, " readError=\"true\"");
    if (prop->m_writeError)
      fprintf(f, " writeError=\"true\"");
    prop->printChildren(f, "property");
  }
  for (nn = 0; nn < w->ports.size(); nn++) {
    Port *p = w->ports[nn];
    if (p->isData) {
      fprintf(f, "  <port name=\"%s\" numberOfOpcodes=\"%u\"", p->name, p->u.wdi.nOpcodes);
      if (p->u.wdi.isBidirectional)
	fprintf(f, " bidirectional=\"true\"");
      else if (!p->u.wdi.isProducer)
	fprintf(f, " provider=\"true\"");
      if (p->u.wdi.minBufferCount)
	fprintf(f, " minBufferCount=\"%u\"", p->u.wdi.minBufferCount);
      if (p->u.wdi.bufferSize)
	fprintf(f, " bufferSize=\"%u\"", p->u.wdi.bufferSize);
      fprintf(f, ">\n");
      p->protocol->printXML(f, 2);
      fprintf(f, "  </port>\n");
    }
  }
  for (nn = 0; nn < w->localMemories.size(); nn++) {
    LocalMemory* m = w->localMemories[nn];
    fprintf(f, "  <localMemory name=\"%s\" size=\"%u\"/>\n", m->name, m->sizeOfLocalMemory);
  }
  fprintf(f, "</worker>\n");
}

 static void
emitInstance(Instance *i, FILE *f)
{
  fprintf(f, "<%s name=\"%s\" worker=\"%s\" occpIndex=\"%u\"",
	  i->iType == Instance::Application ? "instance" :
	  i->iType == Instance::Interconnect ? "interconnect" :
	  i->iType == Instance::IO ? "io" : "adapter",
	  i->name, i->worker->implName, i->index);
#if 0
  bool any = false;
  Port *p = i->worker->ports;
  for (unsigned n = 0; n < i->worker->nPorts; n++, p++)
    if (p->isData && !p->u.wdi.isProducer && p->u.wdi.isProducer) {
      fprintf(f, "%s%s", any ? "," : "inputs=\"", p->name);
      any = true;
    }
  if (any)
    fprintf(f, "\"");
  any = false;
  p = i->worker->ports;
  for (unsigned n = 0; n < i->worker->nPorts; n++, p++)
    if (p->isData && i->ports[n].isProducer && !p->u.wdi.isProducer) {
      fprintf(f, "%s%s", any ? "," : "outputs=\"", p->name);
      any = true;
    }
  if (any)
    fprintf(f, "\"");
#endif
  if (i->attach)
    fprintf(f, " attachment=\"%s\"", i->attach);
  if (i->iType == Instance::Interconnect) // FIXME!!!! when shep puts regions in the bitstream
    fprintf(f, " ocdpOffset=\"%d\"", !strcmp(i->name, "dp0") ? 0 : 32*1024);
  if (i->hasConfig)
    fprintf(f, " configure=\"%#lx\"", (unsigned long)i->config);
  fprintf(f, "/>\n");
}

static const char *
emitUuidHDL(const Worker *aw, const char *outDir, const uuid_t &uuid) {
  const char *err;
  FILE *f;
  if ((err = openOutput(aw->implName, outDir, "", "_UUID", aw->language == VHDL ? VHD : VER, NULL, f)))
    return err;
  const char *comment = aw->language == VHDL ? "--" : "//";
  printgen(f, comment, aw->file, true);
  OCPI::HDL::HdlUUID uuidRegs;
  memcpy(uuidRegs.uuid, uuid, sizeof(uuidRegs.uuid));
  uuidRegs.birthday = time(0);
  strncpy(uuidRegs.platform, platform, sizeof(uuidRegs.platform));
  strncpy(uuidRegs.device, device, sizeof(uuidRegs.device));
  strncpy(uuidRegs.load, load ? load : "", sizeof(uuidRegs.load));
  assert(sizeof(uuidRegs) * 8 == 512);
  fprintf(f,
	  "module mkUUID(uuid);\n"
	  "output [511 : 0] uuid;\nwire [511 : 0] uuid = 512'h");
  for (unsigned n = 0; n < sizeof(uuidRegs); n++)
    fprintf(f, "%02x", ((char*)&uuidRegs)[n]&0xff);
  fprintf(f, ";\nendmodule // mkUUID\n");
  if (fclose(f))
    return "Could close output file. No space?";
  return NULL;
}

// Emit the artifact XML.
const char *
emitArtHDL(Worker *aw, const char *outDir) {
  const char *err;
  uuid_t uuid;
  uuid_generate(uuid);
  if ((err = emitUuidHDL(aw, outDir, uuid)))
    return err;
  FILE *f;
  char *cname = strdup(container);
  char *cp = strrchr(cname, '/');
  if (cp)
    cname = cp+1;
  char *dot = strchr(cname, '.');
  if (dot)
    *dot = '\0';
  if ((err = openOutput(cname, outDir, "", "_art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", container);
  fprintf(f,
	  " This file contains the artifact descriptor XML for the application assembly\n"
	  " named \"%s\". It must be attached (appended) to the bitstream\n",
	  aw->implName);
  fprintf(f, "  -->\n");
  ezxml_t dep;
  Worker *dw = new Worker;
  if ((err = parseFile(container, 0, "HdlContainer", &dep, &dw->file)) ||
      (err = parseHdlAssy(dep, dw)))
    return err;
  uuid_string_t uuid_string;
  uuid_unparse_lower(uuid, uuid_string);
  fprintf(f, "<artifact platform=\"%s\" device=\"%s\" uuid=\"%s\">\n",
	  platform, device, uuid_string);
  // Define all workers
  for (WorkersIter wi = aw->assembly.workers.begin();
       wi != aw->assembly.workers.end(); wi++)
    emitWorker(f, *wi);
  for (WorkersIter wi = dw->assembly.workers.begin();
       wi != dw->assembly.workers.end(); wi++)
    emitWorker(f, *wi);
  Instance *i, *di;
  unsigned nn, n;
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
  // and within the container (adapters).
  Connection *cc, *ac;
  for (cc = dw->assembly.connections, n = 0; n < dw->assembly.nConnections; n++, cc++) {
    // Application connections are those that match by connection name, which means
    // that the name is meaningful/relative to the application (e.g. "in" is input to the app).
    for (ac = aw->assembly.connections, nn = 0; nn < aw->assembly.nConnections; nn++, ac++)
      if (ac->external && cc->external && !strcmp(ac->name, cc->name)) {
	if (ac->external->port->u.wdi.isProducer) {
	  if (cc->external->port->u.wdi.isProducer)
	    return esprintf("container connection \"%s\" has same direction (is producer) as application connection",
			    cc->name);
	} else if (!ac->external->port->u.wdi.isBidirectional &&
		   !cc->external->port->u.wdi.isProducer &&
		   !cc->external->port->u.wdi.isBidirectional)
	    return esprintf("container connection \"%s\" has same direction (is consumer) as application connection",
			    cc->name);
	break;
      }
    if (nn >= aw->assembly.nConnections)
      ac = NULL; // indicate no application connection
    InstancePort *otherp = NULL, *cip = NULL;
    for (cip = cc->ports; cip; cip = cip->nextConn)
      if (cip != cc->external)
	break;
    if (!cip)
      return esprintf("container connecction \"%s\" connects to no ports in the container", cc->name);
    if (ac) {
      for (otherp = ac->ports; otherp; otherp = otherp->nextConn)
	if (otherp != ac->external)
	  break;
    } else {
      // Internal connection
      for (otherp = cc->ports; otherp; otherp = otherp->nextConn)
	if (otherp != cip)
	  break;
    }
    assert(otherp != NULL);
    if (otherp->port->u.wdi.isProducer)
      // Application is producing to an external consumer
      fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
	      otherp->instance->name, otherp->port->name,
	      cip->instance->name, cip->port->name);
    else
      // Application is consuming from an external producer
      fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
	      cip->instance->name, cip->port->name,
	      otherp->instance->name, otherp->port->name);
  }
  // Emit the connections inside the application
  for (ac = aw->assembly.connections, nn = 0; nn < aw->assembly.nConnections; nn++, ac++)
    if (!ac->external) {
      InstancePort *aip;
      InstancePort *from = 0, *to = 0;
      for (aip = ac->ports; aip; aip = aip->nextConn)
	if (aip->port->u.wdi.isProducer)
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
