#include <assert.h>
#include "wip.h"

// Property handling is not implemented here, but is still a worker-level issue.
// FIXME: move properties into the realm of WCI (big)

WciPort::
WciPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : OcpPort(w, x, sp, ordinal, WCIPort, "ctl", err) {
  assert(master || !m_worker->m_wci);
  // WCI ports implicitly a clock to the worker in all cases, master or slave
  if (x && ezxml_cattr(x, "clock")) {
    err = "A control interface can not specify a separate clock";
    return;
  }
  myClock = true;
  addMyClock();
  if (!master) {
    m_worker->m_wci = this;
    m_worker->m_wciClock = clock;
  }
  if (x &&
      ((err = OE::checkAttrs(x, GENERIC_IMPL_CONTROL_ATTRS, "ResetWhileSuspended",
			     "Clock", "MyClock", "Timeout", "Count", "Name", "Pattern",
			     "master",
			     (void *)0)) ||
       (err = OE::getNumber(x, "Timeout", &m_timeout, 0, 0)) ||
       (err = OE::getBoolean(x, "ResetWhileSuspended", &m_resetWhileSuspended))))
    return;
  w.m_ctl.controlOps |= 1 << OU::Worker::OpStart;
  if (w.m_language == VHDL)
    w.m_ctl.controlOps |= 1 << OU::Worker::OpStop;
}

bool WciPort::
needsControlClock() const {
  return !master;
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
	  m_worker->m_ctl.controlOps & (1 << op))
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
  if (master) {
    ocp.MAddr.width = 32;
    ocp.MAddrSpace.value = s;
    ocp.MByteEn.width = 4;
    ocp.MData.width = 32;
    ocp.SData.width = 32;
    ocp.MFlag.width = 19;
  } else {
    // 0: abort, 1: endian, 2: barrier, 10:3 me, 18:11 total
    ocp.MFlag.width = m_worker->m_scalable || m_worker->m_assembly ? 19 : 2;
    if (m_worker->m_ctl.sizeOfConfigSpace <= 32)
      ocp.MAddr.width = 5;
    else
      ocp.MAddr.width = ceilLog2(m_worker->m_ctl.sizeOfConfigSpace);
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
	  comment, typeName(), name());
  if (lang != VHDL) {
    fprintf(f,
	    "  wire %sTerminate = %sMFlag[0];\n"
	    "  wire %sEndian    = %sMFlag[1];\n"
	    "  wire %sBarrier   = %sMFlag[2];\n"
	    "  wire [2:0] %sControlOp = %sMAddr[4:2];\n",
	    pin, pin, pin, pin, pin, pin, pin, pin);
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
      if (!(*pi)->m_isParameter) {
	OU::Property *pr = *pi;
	if (lang != VHDL)
	  fprintf(f, "  localparam [%zu:0] %sAddr = %zu'h%0*zx;\n",
		  ocp.MAddr.width - 1, pr->m_name.c_str(), ocp.MAddr.width,
		  (int)OU::roundUp(ocp.MAddr.width, 4)/4,
		  pr->m_isIndirect ? pr->m_indirectAddr : pr->m_offset);
      }
  }
}

void WciPort::
emitImplSignals(FILE *f) {
  Control &ctl = m_worker->m_ctl;
  // Record for property-related inputs to the worker - writable values and strobes, readable strobes
  if (ctl.nonRawWritables || ctl.nonRawReadables || ctl.rawProperties)
    fprintf(f, "  signal props_to_worker   : worker_props_in_t;\n");
  if (ctl.nonRawReadbacks || ctl.rawReadables)
    fprintf(f, "  signal props_from_worker : worker_props_out_t;\n");
  fprintf(f,
	  "  -- wci information into worker\n");
  if (m_worker->m_endian == Dynamic)
    fprintf(f, "  signal wci_is_big_endian    : Bool_t;\n");
  fprintf(f,
	  "  signal wci_control_op       : wci.control_op_t;\n"
	  "  signal raw_offset           : unsigned(work.%s_worker_defs.worker.decode_width-1 downto 0);\n"
	  "  signal wci_state            : wci.state_t;\n"
	  "  -- wci information from worker\n"
	  "  signal wci_attention        : Bool_t;\n"
	  "  signal wci_abort_control_op : Bool_t;\n"
	  "  signal wci_done             : Bool_t;\n"
	  "  signal wci_error            : Bool_t;\n"
	  "  signal wci_finished         : Bool_t;\n"
	  "  signal wci_is_read          : Bool_t;\n"
	  "  signal wci_is_write         : Bool_t;\n", m_worker->m_implName);
  if (m_worker->m_scalable)
    fprintf(f,
	    "  signal wci_crew             : UChar_t;\n"
	    "  signal wci_rank             : UChar_t;\n"
	    "  signal wci_barrier          : Bool_t;\n"
	    "  signal wci_waiting          : Bool_t;\n");

}

void WciPort::
emitRecordInputs(FILE *f) {
  fprintf(f,
	  "    control_op       : wci.control_op_t; -- control op in progress, or no_op_e\n"
	  "    state            : wci.state_t;      -- wci state: see state_t\n"
	  "    is_operating     : Bool_t;           -- shorthand for state = operating_e\n"
	  "    abort_control_op : Bool_t;           -- demand that slow control op finish now\n");
  if (m_worker->m_endian == Dynamic)
    fprintf(f, "    is_big_endian    : Bool_t;           -- for endian-switchable workers\n");
  if (m_worker->m_scalable)
    fprintf(f,
	    "    crew             : UChar_t;          -- crew size\n"
	    "    rank             : UChar_t;          -- rank in crew\n"
	    "    barrier          : Bool_t;           -- barrier in progress\n");
}
void WciPort::
emitRecordOutputs(FILE *f) {
  fprintf(f,
	  "    done             : Bool_t;           -- the pending prop access/config op is done\n"
	  "    error            : Bool_t;           -- the pending prop access/config op is erroneous\n"
	  "    finished         : Bool_t;           -- worker is finished\n"
	  "    attention        : Bool_t;           -- worker wants attention\n");
  if (m_worker->m_scalable)
    fprintf(f,
	    "    waiting          : Bool_t;           -- worker is waiting at barrier\n");
}

void WciPort::
emitWorkerEntitySignals(FILE *f, std::string &last, unsigned maxPropName) {
  if (master)
    return;
  Worker &w = *m_worker;
  fprintf(f,
	  "    -- Signals for control and configuration.  See record types above.\n"
	  "    %-*s : in  worker_%s_t;\n"
	  "    %-*s : out worker_%s_t := (btrue, bfalse, bfalse, bfalse)",
	  (int)maxPropName, typeNameIn.c_str(), typeNameIn.c_str(),
	  (int)maxPropName, typeNameOut.c_str(), typeNameOut.c_str());
  last = ";\n";
  if (w.m_ctl.writables || w.m_ctl.readbacks || w.m_ctl.rawProperties) {
    fprintf(f, 
	    "%s"
	    "    -- Input values and strobes for this worker's writable properties\n"
	    "    %-*s : in  worker_props_in_t",
	    last.c_str(), (int)maxPropName, "props_in");
  }
  if (w.m_ctl.readbacks || w.m_ctl.rawReadables) {
    fprintf(f, 
	    "%s"
	    "    -- Outputs for this worker's volatile, readable properties\n"
	    "    %-*s : out worker_props_out_t",
	    last.c_str(), maxPropName, "props_out");
  }
}

void WciPort::
emitRecordInterface(FILE *f, const char *implName) {
  if (!master && !m_worker->m_assembly)
    OcpPort::emitRecordInterface(f, implName);
}
void WciPort::
emitRecordArray(FILE *f) {
  if (!master && !m_worker->m_assembly)
    OcpPort::emitRecordArray(f);
}

void WciPort::
emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inWorker) {
  Worker &w = *m_worker;
  if (master || m_worker->m_assembly) {
    if (last.size())
      fprintf(f, last.c_str(), ";");
    std::string in, out;
    OU::format(in, typeNameIn.c_str(), "");
    OU::format(out, typeNameOut.c_str(), "");
    std::string index;
    if (count > 1)
      OU::format(index, "(0 to %zu)", count-1);
    OU::format(last,
	       "  %-*s : in  platform.platform_pkg.wci_%s%s_t%s;\n"
	       "  %-*s : out platform.platform_pkg.wci_%s%s_t%s%%s",
	       (int)w.m_maxPortTypeName, in.c_str(), master ? "s2m" : "m2s",
	       count > 1 ? "_array" : "", index.c_str(),
	       (int)w.m_maxPortTypeName, out.c_str(), master ? "m2s" : "s2m",
	       count > 1 ? "_array" : "", index.c_str());
  } else {
    OcpPort::emitRecordSignal(f, last, prefix, inWorker);
    if (inWorker) {
      if (w.m_ctl.writables || w.m_ctl.readbacks || w.m_ctl.rawProperties) {
	emitLastSignal(f, last, VHDL, false);
	OU::format(last,
		   "    -- Input values and strobes for this worker's writable properties\n"
		   "    %-*s : in  worker_props_in_t%%s",
		   (int)w.m_maxPortTypeName, "props_in");
      }      
      if (w.m_ctl.readbacks || w.m_ctl.rawReadables) {
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
	  last.c_str(), name(), in.c_str(), name(), name(),
	  name(), name(), name());
  if (m_worker->m_scalable)
    fprintf(f,
	    "    %s_in.barrier => wci_barrier,\n"
	    "    %s_in.crew => wci_crew,\n"
	    "    %s_in.rank => wci_rank,\n", name(), name(), name());
  if (m_worker->m_endian == Dynamic)
    fprintf(f, "    %s_in.is_big_endian => wci_is_big_endian,\n", name());
  fprintf(f,
	  "    %s_out.done => wci_done,\n"
	  "    %s_out.error => wci_error,\n"
	  "    %s_out.finished => wci_finished,\n"
	  "    %s_out.attention => wci_attention",
	  name(), name(), name(), name());
  if (m_worker->m_scalable)
    fprintf(f,
	    ",\n"
	    "    %s_out.waiting => wci_waiting", name());
  last = ",\n";
}

void WciPort::
emitPortSignals(FILE *f, Attachments &atts, Language lang, const char *indent,
		bool &any, std::string &comment, std::string &last, const char *myComment,
		OcpAdapt *adapt) {
  if (master || m_worker->m_assembly)
    Port::emitPortSignals(f, atts, lang, indent, any, comment, last, myComment, adapt);
  else
    OcpPort::emitPortSignals(f, atts, lang, indent, any, comment, last, myComment, adapt);
}
