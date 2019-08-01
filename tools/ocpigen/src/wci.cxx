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

#include <assert.h>
#include "hdl.h"

// Property handling is not implemented here, but is still a worker-level issue.
// FIXME: move properties into the realm of WCI (big)

WciPort::
WciPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : OcpPort(w, x, sp, ordinal, WCIPort, "ctl", err), m_timeout(0), m_resetWhileSuspended(false) {
  if (err)
    return;
  OU::format(m_addrWidthExpr, "ocpi_port_%s_MAddr_width", pname());
  assert(m_master || !m_worker->m_wci);
  // WCI ports implicitly a clock to the worker in all cases, master or slave
  if (x && ezxml_cattr(x, "clock")) {
    err = "A control interface can not specify a separate clock";
    return;
  }
  m_myClock = true;
  addMyClock(m_master);
  m_worker->m_wciClock = m_clock;
  if (!m_master)
    m_worker->m_wci = this;
  if (x &&
      ((err = OE::checkAttrs(x, GENERIC_IMPL_CONTROL_ATTRS, "ResetWhileSuspended", "Timeout",
			     "Count", "Name", "Pattern", "master", (void *)0)) ||
       (err = OE::getNumber(x, "Timeout", &m_timeout, 0, 0)) ||
       (err = OE::getBoolean(x, "ResetWhileSuspended", &m_resetWhileSuspended))))
    return;
  w.m_ctl.controlOps |= 1 << OU::Worker::OpStart;
  if (w.m_language == VHDL)
    w.m_ctl.controlOps |= 1 << OU::Worker::OpStop;
  m_dataWidth = 32;
  m_byteWidth = 8;
}

bool WciPort::
needsControlClock() const {
  return !m_master;
}

void WciPort::
emitPortDescription(FILE *f, Language lang) const {
  OcpPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, "  %s   SizeOfConfigSpace: %llu (0x%llx)\n", comment,
	  (unsigned long long)m_worker->m_ctl.sizeOfConfigSpace,
	  (unsigned long long)m_worker->m_ctl.sizeOfConfigSpace);
  fprintf(f, "  %s   WritableConfigProperties: %s\n",
	  comment, BOOL(m_worker->m_ctl.writables));
  fprintf(f, "  %s   ReadableConfigProperties: %s\n",
	  comment, BOOL(m_worker->m_ctl.readables));
  fprintf(f, "  %s   Sub32BitConfigProperties: %s\n",
	  comment, BOOL(m_worker->m_ctl.sub32Bits));
  fprintf(f, "  %s   ControlOperations (in addition to the required \"start\"): ",
	  comment);
  {
    bool first = true;
    for (unsigned op = 0; op < OU::Worker::OpsLimit; op++, first = false)
      if (op != OU::Worker::OpStart &&
	  m_worker->m_ctl.controlOps & (1u << op))
	fprintf(f, "%s%s", first ? "" : ",", OU::Worker::s_controlOpNames[op]);
  }
  fprintf(f, "\n");
  fprintf(f, "  %s   ResetWhileSuspended: %s\n",
	  comment, BOOL(m_resetWhileSuspended));
}

const char *WciPort::
deriveOCP() {
  static uint8_t s[1]; // a non-zero string pointer
  OcpPort::deriveOCP();
  // Do the WCI port
  ocp.MCmd.width = 3;
  ocp.MReset_n.value = s;
  ocp.SFlag.width = 3; // 0: attention, 1: waiting (was present), 2: finished
  ocp.SResp.value = s;
  ocp.SThreadBusy.value = s;
  // From here down things are dependent on properties
  if (m_master) {
    ocp.MAddr.width = 32;
    ocp.MAddrSpace.value = s;
    ocp.MData.width = m_dataWidth;
    ocp.MByteEn.width = m_dataWidth /m_byteWidth;
    ocp.SData.width = 32;
    ocp.MFlag.width = 19;
  } else {
    // 0: abort, 1: endian, 2: barrier, 10:3 me, 18:11 total
    ocp.MFlag.width = m_worker->m_scalable || m_worker->m_assembly ? 19 : 2;
    if (m_worker->m_ctl.sizeOfConfigSpace <= 32)
      ocp.MAddr.width = 5;
    else
      ocp.MAddr.width = OU::ceilLog2(m_worker->m_ctl.sizeOfConfigSpace);
    if (m_worker->m_ctl.sizeOfConfigSpace != 0)
      ocp.MAddrSpace.value = s;
    if (m_worker->m_ctl.sub32Bits)
      ocp.MByteEn.width = 4;
    if (m_worker->m_ctl.writables)
      ocp.MData.width = 32;
    if (m_worker->m_ctl.readables)
      ocp.SData.width = 32;
  }
  fixOCP();
  return NULL;
}

void WciPort::
emitImplAliases(FILE *f, unsigned n, Language lang) {
  const char *pin = fullNameIn.c_str();
  const char *pout = fullNameOut.c_str();
  const char *comment = hdlComment(lang);
  fprintf(f,
	  "  %s Aliases for %s interface \"%s\"\n",
	  comment, typeName(), pname());
  if (lang != VHDL) {
    fprintf(f,
	    "  wire %sTerminate = %sMFlag[0];\n"
	    "  wire %sEndian    = %sMFlag[1];\n"
	    "  wire [2:0] %sControlOp = %sMAddr[4:2];\n",
	    pin, pin, pin, pin, pin, pin);
    if (m_worker->m_scalable)
      fprintf(f,
	      "  wire %sBarrier   = %sMFlag[2];\n",
	      pin, pin);
    if (m_worker->m_ctl.sizeOfConfigSpace)
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
	    "  assign %sSFlag[1] = 0; // no barrier support here\n"
	    "  // This assignment requires that the %sAttention be used, not SFlag[0]\n"
	    "  reg %sAttention; assign %sSFlag[0] = %sAttention;\n",
	    pin, pout, pout, pout, pout);
  }
  if (m_worker->m_ctl.nRunProperties && n == 0) {
    fprintf(f,
	    "  %s Constants for %s's property addresses\n",
	    comment, m_worker->m_implName);
    if (lang != VHDL)
      fprintf(f,
	      "  localparam %sPropertyWidth = %zu;\n", pin, ocp.MAddr.width);
    for (PropertiesIter pi = m_worker->m_ctl.properties.begin();
	 pi != m_worker->m_ctl.properties.end(); pi++)
      if (!(*pi)->m_isParameter && !(*pi)->m_isReadable) {
	OU::Property *pr = *pi;
	if (lang != VHDL)
	  fprintf(f, "  localparam [%zu:0] %sAddr = %zu'h%0*zx;\n",
		  ocp.MAddr.width - 1, pr->m_name.c_str(), ocp.MAddr.width,
		  (int)OU::roundUp(ocp.MAddr.width, 4)/4,
		  pr->m_isIndirect ? pr->m_indirectAddr : pr->m_offset);
      }
  }
}

#if 0
void WciPort::
emitVerilogPortParameters(FILE *f) {
  // FIXME: This will not work with Verilog workers with multiple configurations with differing
  // property space sizes.
  fprintf(f, "  localparam ocpi_port_%s_addr_width = %zu;\n", pname(), ocp.MAddr.width);
}
#endif
void WciPort::
emitImplSignals(FILE *f) {
  Control &ctl = m_worker->m_ctl;
  // Record for property-related inputs to the worker - writable values and strobes, readable strobes
  fprintf(f, "  signal props_to_worker   : worker_props_in_t;\n");
  if (ctl.nonRawReadbacks || ctl.rawReadables)
    fprintf(f, "  signal props_from_worker : worker_props_out_t;\n");
  if (ctl.nonRawReadbacks || ctl.rawReadables || ctl.builtinReadbacks)
    fprintf(f,
	    "  signal internal_props_out : internal_props_out_t; -- this includes builtin volatiles\n");
  if (ctl.builtinReadbacks)
    for (PropertiesIter pi = ctl.properties.begin(); pi != ctl.properties.end(); pi++)
      if ((*pi)->m_isReadable && (*pi)->m_isBuiltin) {
	std::string type;
	m_worker->prType(**pi, type);
	fprintf(f, "  signal props_builtin_%s : %s;\n", (**pi).cname(), type.c_str());
      }
  fprintf(f,
	  "  -- wci information into worker\n"
	  "  signal wci_is_big_endian    : Bool_t;\n"
	  "  signal wci_control_op       : wci.control_op_t;\n"
	  "  signal wci_state            : wci.state_t;\n"
	  "  -- wci information from worker\n"
	  "  signal wci_attention        : Bool_t;\n"
	  "  signal wci_abort_control_op : Bool_t;\n"
	  "  signal wci_done             : Bool_t;\n"
	  "  signal wci_error            : Bool_t;\n"
	  "  signal wci_finished         : Bool_t;\n"
	  );
  if (m_worker->m_scalable)
    fprintf(f,
	    "  signal wci_crew             : UChar_t;\n"
	    "  signal wci_rank             : UChar_t;\n"
	    "  signal wci_barrier          : Bool_t;\n"
	    "  signal wci_waiting          : Bool_t;\n");

}

void WciPort::
emitRecordInputs(FILE *f) {
  OcpPort::emitRecordInputs(f);
  fprintf(f,
	  "    control_op       : wci.control_op_t; -- control op in progress, or no_op_e\n"
	  "    state            : wci.state_t;      -- wci state: see state_t\n"
	  "    is_operating     : Bool_t;           -- shorthand for state = operating_e\n"
	  "    abort_control_op : Bool_t;           -- demand that slow control op finish now\n"
	  "    is_big_endian    : Bool_t;           -- for endian-switchable workers\n");
  if (m_worker->m_scalable)
    fprintf(f,
	    "    crew             : UChar_t;          -- crew size\n"
	    "    rank             : UChar_t;          -- rank in crew\n"
	    "    barrier          : Bool_t;           -- barrier in progress\n");
}
void WciPort::
emitRecordOutputs(FILE *f) {
  OcpPort::emitRecordOutputs(f);
  fprintf(f,
	  "    done             : Bool_t;           -- the pending prop access/config op is done\n"
	  "    error            : Bool_t;           -- the pending prop access/config op is erroneous\n"
	  "    finished         : Bool_t;           -- worker is finished\n"
	  "    attention        : Bool_t;           -- worker wants attention\n");
  if (m_worker->m_scalable)
    fprintf(f,
	    "    waiting          : Bool_t;           -- worker is waiting at barrier\n");
}

#if 0
void WciPort::
emitWorkerEntitySignals(FILE *f, std::string &last, unsigned maxPropName) {
  if (m_master)
    return;
  Worker &w = *m_worker;
  fprintf(f,
	  "    -- Signals for control and configuration.  See record types above.\n"
	  "    %-*s : in  worker_%s_t;\n"
	  "    %-*s : out worker_%s_t := (btrue, bfalse, bfalse, bfalse)",
	  (int)maxPropName, typeNameIn.c_str(), typeNameIn.c_str(),
	  (int)maxPropName, typeNameOut.c_str(), typeNameOut.c_str());
  last = ";\n";
    {
    fprintf(f, 
	    "%s"
	    "    -- Input values and strobes for this worker's writable properties\n"
	    "    %-*s : in  worker_props_in_t",
	    last.c_str(), (int)maxPropName, "props_in");
  }
  if (w.m_ctl.nonRawReadbacks || w.m_ctl.rawReadables) {
    fprintf(f, 
	    "%s"
	    "    -- Outputs for this worker's volatile, readable properties\n"
	    "    %-*s : out worker_props_out_t",
	    last.c_str(), maxPropName, "props_out");
  }
}
#endif

void WciPort::
emitRecordInterface(FILE *f, const char *implName) {
  if (!m_master && !m_worker->m_assembly)
    OcpPort::emitRecordInterface(f, implName);
}
#if 0
void WciPort::
emitRecordInterfaceConstants(FILE *f) {
  fprintf(f, "  constant ocpi_port_%s_addr_width : positive;\n", pname());
}
#endif
void
emitConstant(FILE *f, const std::string &prefix, const char *name, size_t val, Language lang, bool ieee) {
  if (lang == VHDL) {
    if (ieee)
      fprintf(f, "  constant %s_%s : unsigned(31 downto 0) := X\"%08zx\";\n", prefix.c_str(), name, val);
    else
      fprintf(f, "  constant %s_%s : natural := %zu;\n", prefix.c_str(), name, val);
  } else
    fprintf(f, "  localparam %s_%s = %zu;\n", prefix.c_str(), name, val);
}
#if 0

void WciPort::
emitInterfaceConstants(FILE *f, Language lang) {
  Port::emitInterfaceConstants(f, lang);
  std::string pref("ocpi_port_" + m_name);
  emitConstant(f, pref, "addr_width", ocp.MAddr.width, lang);
}
#endif
// This cannot be a WCI port method since it is needed when there are parameters
// with NO CONTROL INTERFACE
void Worker::
emitPropertyAttributeConstants(FILE *f, Language lang) {
  bool first = true;
  size_t last_end = 0;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
    OU::Property &pr = **pi;
    if (first &&
	(pr.m_stringLengthExpr.length() ||
	 pr.m_sequenceLengthExpr.length() ||
	 (pr.m_arrayRank && pr.m_arrayDimensionsExprs[0].length()))) {
      fprintf(f, "  %s Attributes of properties determined by parameter expressions\n",
	      hdlComment(lang));
      first = false;
    }
    if (!pr.m_isRaw && (!pr.m_isParameter || pr.m_isReadable)) {
      emitConstant(f, pr.m_name, "offset", pr.m_offset, lang, true);
      emitConstant(f, pr.m_name, "nbytes_1", pr.m_nBytes - 1 - (pr.m_isSequence ? pr.m_align : 0), lang);
      last_end = pr.m_offset + pr.m_nBytes;
    }
    if (pr.m_baseType == OA::OCPI_String)
      emitConstant(f, pr.m_name, "string_length", pr.m_stringLength, lang);
    if (pr.m_isSequence && pr.m_sequenceLengthExpr.length())
      emitConstant(f, pr.m_name, "sequence_length", pr.m_sequenceLength, lang);
    if (pr.m_arrayRank && pr.m_arrayDimensionsExprs[0].length()) {
      if (lang == VHDL) {
	fprintf(f, "  constant %s_array_dimensions : dimensions_t(0 to %zu) := (",
	      pr.m_name.c_str(), pr.m_arrayRank - 1);
	for (unsigned n = 0; n < pr.m_arrayRank; n++)
	  fprintf(f, "%s%u => %zu", n ? ", " : "", n, pr.m_arrayDimensions[n]);
	fprintf(f, ");\n");
      } else
	for (unsigned n = 0; n < pr.m_arrayRank; n++)
	  fprintf(f, "  localparam %s_array_dimension_%u = %zu;\n", pr.m_name.c_str(), n,
		  pr.m_arrayDimensions[n]);
    }
  }
  emitConstant(f, "ocpi", "sizeof_non_raw_properties", OU::roundUp(last_end, 4), lang);
}

void WciPort::
emitRecordArray(FILE *f) {
  if (!m_master && !m_worker->m_assembly)
    OcpPort::emitRecordArray(f);
}

void WciPort::
emitRecordSignal(FILE *f, std::string &last, const char *aprefix, bool inRecord,
		 bool inPackage, bool inWorker,
		 const char */*defaultIn*/, const char */*defaultOut*/) {
  Worker &w = *m_worker;
  if (m_master || m_worker->m_assembly) {
    if (last.size())
      fprintf(f, last.c_str(), ";");
    std::string in, out;
    OU::format(in, typeNameIn.c_str(), "");
    OU::format(out, typeNameOut.c_str(), "");
    std::string index;
    if (m_arrayCount)
      OU::format(index, "(0 to %zu)", m_arrayCount-1);
    OU::format(last,
	       "  %-*s : in  wci.wci_%s%s_t%s;\n"
	       "  %-*s : out wci.wci_%s%s_t%s%%s",
	       (int)w.m_maxPortTypeName, in.c_str(), m_master ? "s2m" : "m2s",
	       m_arrayCount ? "_array" : "", index.c_str(),
	       (int)w.m_maxPortTypeName, out.c_str(), m_master ? "m2s" : "s2m",
	       m_arrayCount ? "_array" : "", index.c_str());
  } else {
    OcpPort::emitRecordSignal(f, last, aprefix, inRecord, inPackage, inWorker, NULL,
			      inWorker ? "(done=>btrue, others=>bfalse)" : NULL);
    if (inWorker) {
      {
	emitLastSignal(f, last, VHDL, false);
	OU::format(last,
		   "\n"
		   "    -- Input values and strobes for this worker's writable properties\n"
		   "    %-*s : in  worker_props_in_t%%s",
		   (int)w.m_maxPortTypeName, "props_in");
      }      
      if (w.m_ctl.nonRawReadbacks || w.m_ctl.rawReadables) {
	emitLastSignal(f, last, VHDL, false);
	OU::format(last,
		   "    -- Outputs for this worker's volatile, readable properties\n"
		   "    %-*s : out worker_props_out_t%%s",
		   (int)w.m_maxPortTypeName, "props_out");
      }
    }
  }
}

void WciPort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "%s    %s_in.clk => %s.Clk,\n"
	  "    %s_in.reset => wci_reset,\n"
	  "    %s_in.control_op => wci_control_op,\n"
	  "    %s_in.state => wci_state,\n"
	  "    %s_in.is_operating => wci_is_operating,\n"
	  "    %s_in.abort_control_op => wci_abort_control_op,\n",
	  last.c_str(), pname(), in.c_str(), pname(), pname(),
	  pname(), pname(), pname());
  if (m_worker->m_scalable)
    fprintf(f,
	    "    %s_in.barrier => wci_barrier,\n"
	    "    %s_in.crew => wci_crew,\n"
	    "    %s_in.rank => wci_rank,\n", pname(), pname(), pname());
  fprintf(f,
	  "    %s_in.is_big_endian => wci_is_big_endian,\n"
	  "    %s_out.done => wci_done,\n"
	  "    %s_out.error => wci_error,\n"
	  "    %s_out.finished => wci_finished,\n"
	  "    %s_out.attention => wci_attention",
	  pname(), pname(), pname(), pname(), pname());
  if (m_worker->m_scalable)
    fprintf(f,
	    ",\n"
	    "    %s_out.waiting => wci_waiting", pname());
  last = ",\n";
}

void WciPort::
emitPortSignals(FILE *f, const InstancePort &ip, Language lang, const char *indent,
		bool &any, std::string &comment, std::string &last, const char *myComment, std::string &exprs) {
  if (m_master || m_worker->m_assembly)
    Port::emitPortSignals(f, ip, lang, indent, any, comment, last, myComment, exprs);
  else
    OcpPort::emitPortSignals(f, ip, lang, indent, any, comment, last, myComment, exprs);
}

void WciPort::
emitSkelSignals(FILE *f) {
  if (!m_worker->m_noControl)
    fprintf(f,"  ctl_out.finished <= btrue; "
	      "-- remove or change this line for worker to be finished when appropriate\n"
	      "                             "
	      "-- workers that are never \"finished\" need not drive this signal\n");
  // A skeleton should set every volatile property
  if (m_worker->m_ctl.nonRawVolatiles) {
    fprintf(f,
	    "  %s Skeleton assignments for %s's volatile properties\n",
	    hdlComment(m_worker->m_language), m_worker->m_implName);
    if (m_worker->m_language != VHDL)
      return;
    for (PropertiesIter pi = m_worker->m_ctl.properties.begin();
	 pi != m_worker->m_ctl.properties.end(); pi++) {
      const OU::Property &pr = **pi;
      if (!pr.m_isBuiltin && !pr.m_isParameter &&
	  (pr.m_isVolatile || (pr.m_isReadable && !pr.m_isWritable))) {
	if (pr.m_isSequence)
	  fprintf(f,
		  "  -- zero length sequence output, data values are not driven here yet\n"
		  "  props_out.%s_length <= (others => '0');\n", pr.m_name.c_str());
	else {
	  std::string value;
	  OU::Value v(pr); // This constructor creates a zero value
	  vhdlValue(NULL, pr.m_name.c_str(), v, value, false);
	  fprintf(f, "  props_out.%s <= %s;\n", pr.m_name.c_str(), value.c_str());
	}
      }
    }
  }
}

const char *WciPort::
finalizeExternal(Worker &aw, Worker &iw, InstancePort &/*ip*/,
		 bool &cantDataResetWhileSuspended) {
  // slave ports that are connected are ok as is.
  assert(m_master || this == m_worker->m_wci);
  if (!m_master && !aw.m_noControl) {
    // Make assembly WCI the union of all inside, with a replication count
    // We make it easier for CTOP, hoping that wires dissolve appropriately
    // FIXME: when we generate containers, these might be customized, but not now
    //if (iw->m_ctl.sizeOfConfigSpace > aw->m_ctl.sizeOfConfigSpace)
    //            aw->m_ctl.sizeOfConfigSpace = iw->m_ctl.sizeOfConfigSpace;
    aw.m_ctl.sizeOfConfigSpace = (1ll<<32) - 1;
    if (iw.m_ctl.writables)
      aw.m_ctl.writables = true;
#if 0
    // FIXME: until container automation we must force this
    if (iw.m_ctl.readables)
#endif
      aw.m_ctl.readables = true;
#if 0
    // FIXME: Until we have container automation, we force the assembly level
    // WCIs to have byte enables.  FIXME
    if (iw.m_ctl.sub32Bits)
#endif
      aw.m_ctl.sub32Bits = true;
    aw.m_ctl.controlOps |= iw.m_ctl.controlOps; // needed?  useful?
    // Reset while suspended: This is really only interesting if all
    // external data ports are only connected to ports of workers where this
    // is true.  And the use-case is just that you can reset the
    // infrastructure while maintaining worker state.  BUT resetting the
    // CP could clearly reset anything anyway, so this is only relevant to
    // just reset the dataplane infrastructure.
    if (!m_worker->m_wci->resetWhileSuspended())
      cantDataResetWhileSuspended = true;
  }
  return NULL;
}
