#include "assembly.h"
#include "hdl.h"

WtiPort::
WtiPort(Worker &w, ezxml_t x, Port *sp, int ordinal, const char *&err)
  : OcpPort(w, x, sp, ordinal, WTIPort, "wti", err) {
  if ((err = OE::checkAttrs(x, "Name", "Clock", "SecondsWidth", "FractionWidth",
			    "AllowUnavailable", "Pattern", "master", "myclock", (void*)0)) ||
      (err = OE::getNumber(x, "SecondsWidth", &m_secondsWidth, 0, 32)) ||
      (err = OE::getNumber(x, "FractionWidth", &m_fractionWidth, 0, 0)) ||
      (err = OE::getBoolean(x, "AllowUnavailable", &m_allowUnavailable)))
    return;
  for (unsigned i = 0; i < w.m_ports.size(); i++) {
    Port *p = w.m_ports[i];
    if (p->type == WTIPort && p != this) {
      err = "More than one WTI specified, which is not permitted";
      return;
    }
  }
  m_dataWidth = m_secondsWidth + m_fractionWidth;
}

// Our special copy constructor
WtiPort::
WtiPort(const WtiPort &other, Worker &w , std::string &name, const char *&err)
  : OcpPort(other, w, name, 1, err) {
  if (err)
    return;
  m_secondsWidth = other.m_secondsWidth;
  m_fractionWidth = other.m_fractionWidth;
  m_allowUnavailable = other.m_allowUnavailable;
}

// Virtual constructor: the concrete instantiated classes must have a clone method,
// which calls the corresponding specialized copy constructor
Port &WtiPort::
clone(Worker &w, std::string &name, size_t count, OCPI::Util::Assembly::Role */*role*/,
      const char *&err) const {
  assert(count <= 1);
  return *new WtiPort(*this, w, name, err);
}

bool WtiPort::
haveWorkerOutputs() const {
  return m_allowUnavailable;
}

void WtiPort::
emitPortDescription(FILE *f, Language lang) const {
  OcpPort::emitPortDescription(f, lang);
  const char *comment = hdlComment(lang);
  fprintf(f, "  %s   SecondsWidth: %zu\n", comment, m_secondsWidth);
  fprintf(f, "  %s   FractionWidth: %zu\n", comment, m_fractionWidth);
  fprintf(f, "  %s   AllowUnavailable: %s\n", comment, BOOL(m_allowUnavailable));
  fprintf(f, "  %s   DataWidth: %zu\n", comment, m_dataWidth);
}

const char *WtiPort::
deriveOCP() {
  static uint8_t s[1]; // a non-zero string pointer
  OcpPort::deriveOCP();
  ocp.Clk.master = false; //  FIXME. this should be smart...
  ocp.Clk.value = s;
  ocp.MCmd.width = 3;
  ocp.MData.width = m_dataWidth;
  // Note no MReset is present.  OCP says either reset must be present.
  // Thus MCmd is qualified by SReset
  ocp.SReset_n.value = s;
  ocp.SThreadBusy.value = s;
  fixOCP();
  return NULL;
}

void WtiPort::
emitImplSignals(FILE *f) {
  fprintf(f,
	  "  -- Signals from the outer WTI converted to the inner worker ones\n"
	  "  signal worker_wti_in : worker_wti_in_t;\n");
  if (m_allowUnavailable)
    fprintf(f,
	    "  signal worker_wti_out : worker_wti_out_t;\n");
}
void WtiPort::
emitVhdlShell(FILE *f, Port *wci) {
  // FIXME: use a common clock and reset retrieval here
  fprintf(f,
	  "  -- The WTI interface conversion between OCP and inner worker interfaces\n"
	  "  wti_out.Clk <= %s;\n"
	  "  wti_out.SReset_n <= from_bool(not wci_reset)(0);\n",
	  wci ? "ctl_in.Clk" : "wci_Clk");
  if (m_allowUnavailable)
    fprintf(f,
	    "  wti_out.SThreadBusy <= from_bool(not worker_wti_out.request);\n"
	    "  worker_wti_in.valid <= to_bool(wci_reset and wti_in.MCmd = ocp.MCmd_WRITE);\n");
  else
    fprintf(f,
	    "  wti_out.SThreadBusy <= from_bool(wci_reset);\n");
  if (m_secondsWidth)
    fprintf(f, "  worker_wti_in.seconds <= unsigned(wti_in.MData(%zu downto 32));\n",
	    m_secondsWidth + 31);
  if (m_fractionWidth)
    fprintf(f, "  worker_wti_in.fraction <= unsigned(wti_in.MData(31 downto %zu));\n",
	    32 - m_fractionWidth);
}

void WtiPort::
emitVHDLShellPortMap(FILE *f, std::string &last) {
  std::string in, out;
  OU::format(in, typeNameIn.c_str(), "");
  OU::format(out, typeNameOut.c_str(), "");
  fprintf(f,
	  "%s    %s_in => worker_wti_in",
	  last.c_str(), name());
  if (m_allowUnavailable)
    fprintf(f, ",\n    %s_out => worker_wti_out", name());
  last = ",\n";
}

void WtiPort::
emitRecordInputs(FILE *f) {
  if (clock != m_worker->m_wciClock)
    fprintf(f,
	    "    clk        : std_logic; -- this ports clk, different from wci_clk");
  if (m_allowUnavailable)
    fprintf(f, "    valid    : Bool_t;\n");
  if (m_secondsWidth)
    fprintf(f, "    seconds  : unsigned(%zu downto 0);\n", m_secondsWidth-1);
  if (m_fractionWidth)
    fprintf(f, "    fraction : unsigned(%zu downto 0);\n", m_fractionWidth-1);
}

void WtiPort::
emitRecordOutputs(FILE *f) {
  if (m_allowUnavailable)
    fprintf(f,
	    "    request    : Bool_t; -- worker wants the clock to be valid\n");
}

#if 0
void WtiPort::
emitRecordSignal(FILE *f, std::string &last, const char *prefix, bool inRecord, bool inPackage,
		 bool inWorker) {
  ocpiAssert(!master);
  fprintf(f, "%s    -- Signals for %s %s port named \"%s\".  See record type(s) above.\n",
	  last.c_str(), typeName(), masterIn() ? "slave" : "master", name());

  fprintf(f,
	  "    %-*s : in  worker_%s_t",
	  maxPropName, typeNameIn.c_str(), typeNameIn.c_str());
  if (m_allowUnavailable)
    fprintf(f,
	    ";\n    %-*s : out worker_%s_t",
	    maxPropName, typeNameOut.c_str(), typeNameOut.c_str());
  last = ";\n";
}
#endif
const char *WtiPort::
finalizeExternal(Worker &aw, Worker &/*iw*/, InstancePort &ip,
		 bool &/*cantDataResetWhileSuspended*/) {
  // We don't share ports since the whole point of WTi is to get
  // intra-chip accuracy via replication of the time clients.
  // We could have an option to use wires instead to make things smaller
  // and less accurate...
  const char *err;
  if (!master && ip.m_attachments.empty() &&
      (err = aw.m_assembly->externalizePort(ip, "wti", aw.m_assembly->m_nWti)))
    return err;
  return NULL;
}
