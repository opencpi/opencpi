#include <assert.h>
#include "hdl.h"

// This is an HDL file, and perhaps an assembly or a platform
const char *Worker::
parseHdl(const char *package) {
  const char *err;
  if (strcmp(m_implName, m_fileName.c_str()))
    return OU::esprintf("File name (%s) and implementation name in XML (%s) don't match",
			m_fileName.c_str(), m_implName);
  m_pattern = ezxml_cattr(m_xml, "Pattern");
  m_portPattern = ezxml_cattr(m_xml, "PortPattern");
  if (!m_pattern)
    m_pattern = "%s_";
  if (!m_portPattern)
    m_portPattern = "%s_%n";
  // Here is where there is a difference between a implementation and an assembly
  if (!strcasecmp(m_xml->name, "HdlImplementation") || !strcasecmp(m_xml->name, "HdlWorker") ||
      !strcasecmp(m_xml->name, "HdlPlatform") || !strcasecmp(m_xml->name, "HdlDevice") ||
      !strcasecmp(m_xml->name, "HdlConfig") || !strcasecmp(m_xml->name, "HdlContainer")) {
    if ((err = parseHdlImpl(package)))
      return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  } else if (!strcasecmp(m_xml->name, "HdlAssembly") ||
	     !strcasecmp(m_xml->name, "HdlPlatformAssembly") ||
	     !strcasecmp(m_xml->name, "HdlContainerAssembly")) {
    if ((err = parseHdlAssy()))
      return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  } else
    return "file contains neither an HdlImplementation nor an HdlAssembly nor an HdlPlatform";
  // Whether a worker or an assembly, we derive the external OCP signals, etc.
  if ((err = deriveOCP()))
    return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  unsigned wipN[NWIPTypes][2] = {{0}};
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    if ((err = p->doPatterns(wipN[p->type][p->masterIn()], m_maxPortTypeName)))
      return err;
    wipN[p->type][p->masterIn()]++;
  }
  if (m_ports.size() > 32)
    return "worker has more than 32 ports";
  m_model = HdlModel;
  m_modelString = "hdl";
  return 0;
}


Clock *Worker::
addWciClockReset() {
  // If there is no control port, then we synthesize the clock as wci_clk
  for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++)
    if (!strcasecmp("wci_Clk", (*ci)->name))
      return *ci;
  Clock *clock = addClock();
  clock->name = strdup("wci_Clk");
  clock->signal = strdup("wci_Clk");
  clock->reset = "wci_Reset_n";
  m_wciClock = clock;
  return clock;
}

Clock *Worker::
findClock(const char *name) const {
  for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!strcasecmp(name, c->name))
      return c;
  }
  return NULL;
}

const char *Worker::
parseHdlImpl(const char *package) {
  const char *err;
  ezxml_t xctl;
  size_t dw;
  bool dwFound;
  if (!strcasecmp(OE::ezxml_tag(m_xml),"hdldevice"))
    m_isDevice = true;
  if ((err = parseSpec(package)) ||
      (err = parseImplControl(xctl)) ||
      (err = OE::getNumber(m_xml, "datawidth", &dw, &dwFound)) ||
      (err = OE::getBoolean(m_xml, "outer", &m_outer)))
    return err;
  if (dwFound)
    m_defaultDataWidth = (int)dw; // override the -1 default if set
  // Parse the optional endian attribute.
  // If not specified, it will be defaulted later based on protocols
  const char *myendian = ezxml_cattr(m_xml, "endian");
  if (myendian) {
    static const char *endians[] = {ENDIANS, NULL};
    for (const char **ap = endians; *ap; ap++)
      if (!strcasecmp(myendian, *ap)) {
	m_endian = (Endian)(ap - endians);
	break;
      }
  }
  if (!m_noControl) {
#if 1
    if (!createPort<WciPort>(*this, xctl, NULL, -1, err))
      return err;
#else
    // Insert the control port at the beginning of the port list since we want
    // To always process the control port first if we have one
    wci = new Port(ezxml_cattr(xctl, "Name"), this, WCIPort, xctl);
    m_ports.insert(m_ports.begin(), wci);
    // Finish HDL-specific control parsing
    m_ctl.controlOps |= 1 << OU::Worker::OpStart;
    if (m_language == VHDL)
      m_ctl.controlOps |= 1 << OU::Worker::OpStop;
    size_t timeout = 0;
    bool resetWhileSuspended = false;
    if (xctl) {
      if ((err = OE::checkAttrs(xctl, GENERIC_IMPL_CONTROL_ATTRS, "ResetWhileSuspended",
				"Clock", "MyClock", "Timeout", "Count", "Name", "Pattern",
				(void *)0)) ||
          (err = OE::getNumber(xctl, "Timeout", &timeout, 0, 0)) ||
	  (err = getNumber(xctl, "Count", &wci->count, 0, 0)) ||
          (err = OE::getBoolean(xctl, "RawProperties", &m_ctl.rawProperties)) ||
          (err = OE::getBoolean(xctl, "ResetWhileSuspended",
				&resetWhileSuspended)))
        return err;
      m_wci->setTimeout(timeout);
      m_wci->setResetWhileSuspended(resetWhileSuspended);
      wci->pattern = ezxml_cattr(xctl, "Pattern");
    }
#endif
    const char *firstRaw = ezxml_cattr(m_xml, "FirstRawProperty");
    if ((err = OE::getBoolean(m_xml, "RawProperties", &m_ctl.rawProperties)))
      return err;
    if (firstRaw) {
      for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
	if (!strcasecmp((*pi)->m_name.c_str(), firstRaw))
	  m_ctl.firstRaw = *pi;
      if (!m_ctl.firstRaw)
	return OU::esprintf("FirstRawProperty: '%s' not found as a property", firstRaw);
      m_ctl.rawProperties = true;
    }
    bool raw = false;
    for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
      OU::Property &p = **pi;
      if (!p.m_isParameter) {
	// Determine when the raw properties start
	if (m_ctl.rawProperties &&
	    (!m_ctl.firstRaw ||
	     !strcasecmp(m_ctl.firstRaw->m_name.c_str(), p.m_name.c_str())))
	  raw = true;
	if (raw) {
	  if (p.m_isWritable)
	    m_ctl.rawWritables = true;
	  if (p.m_isReadable)
	    m_ctl.rawReadables = true;
	} else {
	  // These control attributes are only set for non-raw properties.
	  if (p.m_isReadable)
	    m_ctl.nonRawReadables = true;
	  if (p.m_isWritable)
	    m_ctl.nonRawWritables = true;
	  if (p.m_isVolatile)
	    m_ctl.nonRawVolatiles = true;
	  if (p.m_isVolatile || p.m_isReadable && !p.m_isWritable)
	    m_ctl.nonRawReadbacks = true;
	  if (!p.m_isParameter || p.m_isReadable)
	    m_ctl.nNonRawRunProperties++;
	  if (p.m_isSub32)
	    m_ctl.nonRawSub32Bits = true;
	}
      }
    }
    if (!m_wci->count)
      m_wci->count = 1;
    // clock processing depends on the name so it must be defaulted here
    if (m_ctl.sub32Bits)
      m_needsEndian = true;
  }
  // Now we do clocks before interfaces since they may refer to clocks
  for (ezxml_t xc = ezxml_cchild(m_xml, "Clock"); xc; xc = ezxml_next(xc)) {
    if ((err = OE::checkAttrs(xc, "Name", "Signal", "Home", (void*)0)))
      return err;
    Clock *c = addClock();
    c->name = ezxml_cattr(xc, "Name");
    if (!c->name)
      return "Missing Name attribute in Clock subelement of HdlWorker";
    c->signal = ezxml_cattr(xc, "Signal");
  }
  // Now that we have clocks roughly set up, we process the wci clock
  //  if (wci && (err = checkClock(xctl, wci)))
  //    return err;
  // End of control interface/wci processing (except OCP signal config)
  //  size_t oldSize = m_ports.size(); // remember the base of extra ports
  // This ordering is repeated below
  if ((err = initImplPorts(m_xml, "MemoryInterface", createPort<WmemiPort>)) ||
      (err = initImplPorts(m_xml, "TimeInterface", createPort<WtiPort>)) ||
      (err = initImplPorts(m_xml, "timeservice", createPort<TimeServicePort>)) ||
      (err = initImplPorts(m_xml, "CPMaster", createPort<CpPort>)) ||
      (err = initImplPorts(m_xml, "uNOC", createPort<NocPort>)) ||
      (err = initImplPorts(m_xml, "Metadata", createPort<MetaDataPort>)) ||
      (err = initImplPorts(m_xml, "Control", createPort<WciPort>)) ||
      (err = initImplPorts(m_xml, "rawprop", createPort<RawPropPort>)))
    return err;

  // Prepare to process data plane port implementation info
  // Now lets look at the implementation-specific data interface info
  Port *sp;
  for (ezxml_t s = ezxml_cchild(m_xml, "StreamInterface"); s; s = ezxml_next(s))
    if ((err = checkDataPort(s, sp)) || !createPort<WsiPort>(*this, s, sp, -1, err))
    return err;
  for (ezxml_t m = ezxml_cchild(m_xml, "MessageInterface"); m; m = ezxml_next(m))
    if ((err = checkDataPort(m, sp)) || !createPort<WmiPort>(*this, m, sp, -1, err))
    return err;
  // Final passes over all data ports for defaulting and checking
  // 1. Convert any data ports to WSI if they were not mentioned and determine if a wci clk is
  //    needed.
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port &p = *m_ports[i];
    p.finalizeHdlDataPort(); // This will convert to a concrete impl type if not one yet
    if ((err = p.checkClock()))
      return err;
  }
#if 0
  {
    Port *p = m_ports[i];
    
    if (p->type == WDIPort) {
      
      // FIXME: a data port method...
      // Do it via XML so we aren't duplicating other code
      char *wsi;
      asprintf(&wsi, "<streaminterface name='%s' dataWidth='%zu' impreciseburst='true'/>",
	       p->name(), 
	       m_defaultDataWidth >= 0 ?
	       m_defaultDataWidth : p->m_protocol->m_dataValueWidth);
      ezxml_t wsix = ezxml_parse_str(wsi, strlen(wsi));
      if (!(p = createPort<WsiPort>(*this, wsix, p, err)))
	return err;
    }
    if ((err = p->checkClock()))
      return err;
  }
#endif
  // Now check that all clocks have a home and all ports have a clock
  for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!c->port && !c->signal)
      return OU::esprintf("Clock %s is owned by no port and has no signal name",
			  c->name);
  }
  // now make sure clockPort references are sorted out
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    if (p->clockPort)
      p->clock = p->clockPort->clock;
    if (p->count == 0)
      p->count = 1;
  }
  // process ad hoc signals
  if ((err = Signal::parseSignals(m_xml, m_signals)))
    return err;
  // Parse submodule requirements - note that this information is only used
  // for platform configurations and containers, but we do a bit of error checking here
  for (ezxml_t rqx = ezxml_cchild(m_xml, "requires"); rqx; rqx = ezxml_next(rqx)) {
    std::string worker;
    if ((err = OE::getRequiredString(rqx, worker, "worker")))
      return err;
    std::string rqFile;
    OU::format(rqFile, "../%s.hdl/%s.xml", worker.c_str(),  worker.c_str());
    Worker *rqw = Worker::create(rqFile.c_str(), m_file.c_str(), NULL, m_outDir, NULL, 0, err);
    if (rqw) {
      m_requireds.push_back(Required(*rqw));
      m_requireds.back().parse(rqx, *this);
    } else
      return OU::esprintf("for required worker %s: %s", worker.c_str(), err);
  }
  // Finalize endian default
  if (m_endian == NoEndian)
    m_endian = m_needsEndian ? Little : Neutral;
  return 0;
}

ReqConnection::
ReqConnection()
  : m_port(NULL), m_rq_port(NULL), m_signal(NULL), m_rq_signal(NULL), m_index(0) {
}
const char *ReqConnection::
parse(ezxml_t cx, Worker &w, Required &r) {
  const char *err;
  if ((err = OE::checkAttrs(cx, "port", "signal", "to", "index", (void *)0)))
    return err;
  const char
    *port = ezxml_cattr(cx, "port"),
    *signal = ezxml_cattr(cx, "signal"),
    *to = ezxml_cattr(cx, "to");
  if ((err = OE::getNumber(cx, "index", &m_index)))
    return err;
  if (port) {
      if ((err = w.getPort(port, m_port)))
	return err;
      if (signal)
	return OU::esprintf("Using both \"port\" and \"signal\" is invalid");
      if (to) {
	if ((err = r.m_worker.getPort(to, m_rq_port)))
	  return err;
	if (m_rq_port->type != m_port->type)
	  return OU::esprintf("Required worker port \"%s\" is not the same type",
			      to);
	if (m_rq_port->master == m_port->master)
	  return OU::esprintf("Required worker port \"%s\" has the same role (master) as port \"%s\"",
			      to, port);
      } else {
	for (unsigned i = 0; i < r.m_worker.m_ports.size(); i++)
	  if (r.m_worker.m_ports[i]->type == m_port->type &&
	      r.m_worker.m_ports[i]->master != m_port->master) {
	    if (m_rq_port)
	      return OU::esprintf("A \"to\" attribute is required since there are multiple "
				  "possible suitable ports on the required worker");
	    m_rq_port = r.m_worker.m_ports[i];
	  }
	if (!m_rq_port)
	  return OU::esprintf("There are no suitable ports on the required worker");
      }
    } else if (signal) {
      if (!to)
	return OU::esprintf("When the \"signal\" attribute is present, "
			    "the \"to\" attribute must also be present");
      if (!(m_signal = Signal::find(w.m_signals, signal)))
	return OU::esprintf("No signal named \"%s\" exists", signal);
      if (!(m_rq_signal = Signal::find(r.m_worker.m_signals, to)))
	return OU::esprintf("No signal named \"%s\" exists on the required worker", to);
    } else
      return OU::esprintf("One of either \"port\" or \"signal\" is required");
  return NULL;
}

Required::
Required(Worker &w)
  : m_worker(w) {
}

const char *Required::
parse(ezxml_t rqx, Worker &w) {
  const char *err;
  if ((err = OE::checkAttrs(rqx, "worker", (void *)0)) ||
      (err = OE::checkElements(rqx, "connect", (void*)0)))
    return err;
  for (ezxml_t cx = ezxml_cchild(rqx, "connect"); cx; cx = ezxml_next(cx)) {
    m_connections.push_back(ReqConnection());
    if ((err = m_connections.back().parse(cx, w, *this)))
      return err;
  }
  return NULL;
}

Signal::
Signal()
  : m_direction(IN), m_width(0), m_differential(false), m_pos("%sp"), m_neg("%sn"),
    m_type(NULL) {
}

const char *Signal::
parse(ezxml_t x) {
  const char *err;
  if ((err = OE::checkAttrs(x, "input", "inout", "bidirectional", "output",
			    "width", "differential", "type", (void*)0)))
    return err;
  const char *name;
  if ((name = ezxml_cattr(x, "Input")))
    m_direction = IN;
  else if ((name = ezxml_cattr(x, "Output")))
    m_direction = OUT;
  else if ((name = ezxml_cattr(x, "Inout")))
    m_direction = INOUT;
  else if ((name = ezxml_cattr(x, "bidirectional")))
    m_direction = INOUT;
  else
    return "Missing input, output, or inout attribute for signal element";
  if ((err = OE::getNumber(x, "Width", &m_width, 0, 0)) ||
      (err = OE::getBoolean(x, "differential", &m_differential)))
    return err;
  m_type = ezxml_cattr(x, "type");
  m_name = name;
  return NULL;
}

const char *Signal::
parseSignals(ezxml_t xml, Signals &signals) {
  const char *err = NULL;
  // process ad hoc signals
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); !err && xs; xs = ezxml_next(xs)) {
    Signal *s = new Signal;
    if (!(err = s->parse(xs)))
      if (!Signal::find(signals, s->m_name.c_str()))
	signals.push_back(s);
      else {
	err = OU::esprintf("Duplicate signal: '%s'", s->m_name.c_str());
	delete s;
      }
  }
  return err;
}

Signal *Signal::
find(Signals &signals, const char *name) {
  for (SignalsIter si = signals.begin(); si != signals.end(); si++)
    if ((*si)->m_name == name)
      return *si;
  return NULL;
}

void Signal::
deleteSignals(Signals &signals) {
  for (SignalsIter si = signals.begin(); si != signals.end(); si++)
    delete *si;
}
