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
    // Derive full names
    bool mIn = p->masterIn();
    // ordinal == -1 means insert "%u" into the name for using later
    if ((err = doPattern(p, -1, wipN[p->type][mIn], true, !mIn, p->fullNameIn)) ||
        (err = doPattern(p, -1, wipN[p->type][mIn], false, !mIn, p->fullNameOut)) ||
        (err = doPattern(p, -1, wipN[p->type][mIn], true, !mIn, p->typeNameIn, true)) ||
        (err = doPattern(p, -1, wipN[p->type][mIn], false, !mIn, p->typeNameOut, true)))
      return err;
    if (p->typeNameIn.length() > m_maxPortTypeName)
      m_maxPortTypeName = p->typeNameIn.length();
    if (p->typeNameOut.length() > m_maxPortTypeName)
      m_maxPortTypeName = p->typeNameOut.length();
    //    const char *pat = p->pattern ? p->pattern : w->pattern;
    
    if (p->clock && p->clock->port == p) {
      std::string sin;
      // ordinal == -2 means suppress ordinal
      if ((err = doPattern(p, p->count > 1 ? 0 : -2, wipN[p->type][mIn], true, !mIn, sin)))
        return err;
      asprintf((char **)&p->ocp.Clk.signal, "%s%s", sin.c_str(), "Clk");
      p->clock->signal = p->ocp.Clk.signal;
    }
    switch (p->type) {
    case WCIPort:
    case WSIPort:
    case WMIPort:
    case WMemIPort:
    case WTIPort:
      {
	OcpSignalDesc *osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	  if (os->master == mIn && /* strcasecmp(osd->name, "Clk") && */ os->value)
	    asprintf((char **)&os->signal, "%s%s", p->fullNameIn.c_str(), osd->name);
	osd = ocpSignals;
	for (OcpSignal *os = p->ocp.signals; osd->name; os++, osd++)
	  if (os->master != mIn && /* strcasecmp(osd->name, "Clk") && */os->value)
	    asprintf((char **)&os->signal, "%s%s", p->fullNameOut.c_str(), osd->name);
      }
    default:;
    }
    wipN[p->type][mIn]++;
  }
  if (m_ports.size() > 32)
    return "worker has more than 32 ports";
  m_model = HdlModel;
  m_modelString = "hdl";
  return 0;
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
  Port *wci;
  if (m_noControl) {
    wci = NULL;
  } else {
    // Insert the control port at the beginning of the port list since we want
    // To always process the control port first if we have one
    wci = new Port(ezxml_cattr(xctl, "Name"), this, false, WCIPort, xctl);
    m_ports.insert(m_ports.begin(), wci);
    // Finish HDL-specific control parsing
    m_ctl.controlOps |= 1 << OU::Worker::OpStart;
    if (m_language == VHDL)
      m_ctl.controlOps |= 1 << OU::Worker::OpStop;
    if (xctl) {
      if ((err = OE::checkAttrs(xctl, GENERIC_IMPL_CONTROL_ATTRS, "ResetWhileSuspended",
				"Clock", "MyClock", "Timeout", "Count", "Name", "Pattern",
				(void *)0)) ||
          (err = OE::getNumber(xctl, "Timeout", &wci->u.wci.timeout, 0, 0)) ||
          (err = getNumber(xctl, "Count", &wci->count, 0, 0)) ||
          (err = OE::getBoolean(xctl, "RawProperties", &m_ctl.rawProperties)) ||
          (err = OE::getBoolean(xctl, "ResetWhileSuspended",
				&wci->u.wci.resetWhileSuspended)))
        return err;
      wci->pattern = ezxml_cattr(xctl, "Pattern");
    }
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
      // Determine when the raw properties start
      if (!p.m_isParameter && m_ctl.rawProperties &&
	  (!m_ctl.firstRaw ||
	   !strcasecmp(m_ctl.firstRaw->m_name.c_str(), p.m_name.c_str())))
	raw = true;
      if (raw) {
	if (p.m_isWritable)
	  m_ctl.rawWritables = true;
	if (p.m_isReadable)
	  m_ctl.rawReadables = true;
      } else if (!p.m_isParameter) {
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
    if (!wci->count)
      wci->count = 1;
    // clock processing depends on the name so it must be defaulted here
    if (!wci->name)
      wci->name = "ctl";
    if (m_ctl.sub32Bits)
      m_needsEndian = true;
    
  }
  // Now we do clocks before interfaces since they may refer to clocks
#if 0
  m_nClocks = OE::countChildren(m_xml, "Clock");
  // add one to allow for adding the WCI clock later
  m_clocks = myCalloc(Clock, m_nClocks + 1 + m_ports.size());
#endif
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
  size_t oldSize = m_ports.size(); // remember the base of extra ports
  // This ordering is repeated below
  if ((err = initImplPorts(m_xml, "MemoryInterface", "mem", WMemIPort)) ||
      (err = initImplPorts(m_xml, "TimeInterface", "wti", WTIPort)) ||
      (err = initImplPorts(m_xml, "timeservice", "time", TimePort)) ||
      (err = initImplPorts(m_xml, "CPMaster", "cp", CPPort)) ||
      (err = initImplPorts(m_xml, "uNOC", "noc", NOCPort)) ||
      (err = initImplPorts(m_xml, "Metadata", "metadata", MetadataPort)) ||
      (err = initImplPorts(m_xml, "Control", "wci", WCIPort)))
    return err;

  // Prepare to process data plane port implementation info
  // Now lets look at the implementation-specific data interface info
  Port *dp;
  for (ezxml_t s = ezxml_cchild(m_xml, "StreamInterface"); s; s = ezxml_next(s))
    if ((err = OE::checkAttrs(s, "Name", "Clock", "DataWidth", "PreciseBurst",
                              "ImpreciseBurst", "Continuous", "Abortable",
                              "EarlyRequest", "MyClock", "RegRequest", "Pattern",
                              "NumberOfOpcodes", "MaxMessageValues",
			      "datavaluewidth", "zerolengthmessages",
			      "implname", "producer", "optional", (void*)0)) ||
        (err = checkDataPort(s, &dp, WSIPort)) ||
        (err = OE::getBoolean(s, "Abortable", &dp->u.wsi.abortable)) ||
        (err = OE::getBoolean(s, "RegRequest", &dp->u.wsi.regRequest)) ||
        (err = OE::getBoolean(s, "EarlyRequest", &dp->u.wsi.earlyRequest)))
      return err;
  for (ezxml_t m = ezxml_cchild(m_xml, "MessageInterface"); m; m = ezxml_next(m))
    if ((err = OE::checkAttrs(m, "Name", "Clock", "MyClock", "DataWidth", "master",
                              "PreciseBurst", "MFlagWidth", "ImpreciseBurst",
                              "Continuous", "ByteWidth", "TalkBack",
                              "Bidirectional", "Pattern",
                              "NumberOfOpcodes", "MaxMessageValues",
			      "datavaluewidth", "zerolengthmessages",
                              (void*)0)) ||
        (err = checkDataPort(m, &dp, WMIPort)) ||
	(err = OE::getBoolean(m, "master", &dp->master)) ||
        (err = OE::getNumber(m, "ByteWidth", &dp->byteWidth, 0, dp->dataWidth)) ||
        (err = OE::getBoolean(m, "TalkBack", &dp->u.wmi.talkBack)) ||
        (err = OE::getBoolean(m, "Bidirectional", &dp->u.wdi.isBidirectional)) ||
        (err = OE::getNumber(m, "MFlagWidth", &dp->u.wmi.mflagWidth, 0, 0)))
      return err;
  // Final pass over all data ports for defaulting and checking
  for (unsigned i = 0; i < m_ports.size(); i++) {
    dp = m_ports[i];
    if ((err = checkClock(dp)))
      return err;
    switch (dp->type) {
    case WDIPort:
      // For data ports that have not been specified as stream or message,
      // default to imprecise stream clocked by the WSI, with data width implied from protocol.
      dp->type = WSIPort;
      dp->dataWidth = m_defaultDataWidth >= 0 ? m_defaultDataWidth : dp->protocol->m_dataValueWidth;
      dp->impreciseBurst = true;
      if (m_ports[0]->type == WCIPort)
	dp->clockPort = m_ports[0];
      else
	return "A data port that defaults to WSI must be in a worker with a WCI";
      // fall into
    case WSIPort:
    case WMIPort:
      {
	// If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
	size_t granuleWidth =
	  dp->protocol->m_dataValueWidth * dp->protocol->m_dataValueGranularity;
	// If messages are always a multiple of datawidth and we don't have zlms, bytes are datawidth
	if (granuleWidth >= dp->dataWidth &&
	    (dp->dataWidth == 0 || (granuleWidth % dp->dataWidth) == 0) && 
	    !dp->protocol->m_zeroLengthMessages)
	  dp->byteWidth = dp->dataWidth;
	else
	  dp->byteWidth = dp->protocol->m_dataValueWidth;
	if (dp->byteWidth != 0 && dp->dataWidth % dp->byteWidth)
	  return "Specified ByteWidth does not divide evenly into specified DataWidth";
	// Check if this port requires endianness
	// Either the granule is smaller than or not a multiple of data path width
	if (granuleWidth < dp->dataWidth || dp->dataWidth && granuleWidth % dp->dataWidth)
	  m_needsEndian = true;
      }
      break;
    default:;
    }
  }
  // This is pretty lame, but there is no other heuristic now.
  // Presumably we could enumerate ports that imply we have some clock.
  if (m_ports.size() == 0)
    addWciClockReset();
  size_t nextPort = oldSize;
  for (ezxml_t m = ezxml_cchild(m_xml, "MemoryInterface"); m; m = ezxml_next(m), nextPort++) {
    Port *mp = m_ports[nextPort];
    bool memFound = false;
    if ((err = OE::checkAttrs(m, "Name", "Clock", "DataWidth", "PreciseBurst", "ImpreciseBurst",
                              "MemoryWords", "ByteWidth", "MaxBurstLength", "WriteDataFlowControl",
                              "ReadDataFlowControl", "Count", "Pattern", "master", "myclock", (void*)0)) ||
        (err = OE::getBoolean(m, "master", &mp->master)) ||
        (err = getNumber(m, "Count", &mp->count, 0, 0)) ||
        (err = OE::getNumber64(m, "MemoryWords", &mp->u.wmemi.memoryWords, &memFound, 0)) ||
        (err = OE::getNumber(m, "DataWidth", &mp->dataWidth, 0, 8)) ||
        (err = OE::getNumber(m, "ByteWidth", &mp->byteWidth, 0, 8)) ||
        (err = OE::getNumber(m, "MaxBurstLength", &mp->u.wmemi.maxBurstLength, 0, 0)) ||
        (err = OE::getBoolean(m, "ImpreciseBurst", &mp->impreciseBurst)) ||
        (err = OE::getBoolean(m, "PreciseBurst", &mp->preciseBurst)) ||
        (err = OE::getBoolean(m, "WriteDataFlowControl", &mp->u.wmemi.writeDataFlowControl)) ||
        (err = OE::getBoolean(m, "ReadDataFlowControl", &mp->u.wmemi.readDataFlowControl)))
      return err;
    if (!memFound || !mp->u.wmemi.memoryWords)
      return "Missing \"MemoryWords\" attribute in MemoryInterface";
    if (!mp->preciseBurst && !mp->impreciseBurst) {
      if (mp->u.wmemi.maxBurstLength > 0)
        return "MaxBurstLength specified when no bursts are enabled";
      if (mp->u.wmemi.writeDataFlowControl || mp->u.wmemi.readDataFlowControl)
        return "Read or WriteDataFlowControl enabled when no bursts are enabled";
    }
    if (mp->byteWidth < 8 || mp->dataWidth % mp->byteWidth)
      return "Bytewidth < 8 or doesn't evenly divide into DataWidth";
    mp->pattern = ezxml_cattr(m, "Pattern");
  }
  bool foundWTI = false;
  for (ezxml_t m = ezxml_cchild(m_xml, "TimeInterface"); m; m = ezxml_next(m), nextPort++) {
    Port *mp = m_ports[nextPort];
    if (foundWTI)
      return "More than one WTI specified, which is not permitted";
    if ((err = OE::checkAttrs(m,
			      "Name", "Clock", "SecondsWidth", "FractionWidth", "AllowUnavailable",
			      "Pattern", "master", "myclock",
			      (void*)0)) ||
        (err = OE::getNumber(m, "SecondsWidth", &mp->u.wti.secondsWidth, 0, 32)) ||
        (err = OE::getNumber(m, "FractionWidth", &mp->u.wti.fractionWidth, 0, 0)) ||
        (err = OE::getBoolean(m, "master", &mp->master)) ||
        (err = OE::getBoolean(m, "AllowUnavailable", &mp->u.wti.allowUnavailable)))
      return err;
    mp->dataWidth = mp->u.wti.secondsWidth + mp->u.wti.fractionWidth;
    foundWTI = true;
    mp->pattern = ezxml_cattr(m, "Pattern");
  }
  // Now check that all clocks have a home and all ports have a clock
  for (ClocksIter ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (c->port) {
#if 0
      if (c->signal)
        return OU::esprintf("Clock %s is owned by interface %s and has a signal name",
			    c->name, c->port->name);
      //asprintf((char **)&c->signal, "%s_Clk", c->port->fullNameIn);
#endif
    } else if (!c->signal)
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
  // Finalize endian default
  if (m_endian == NoEndian)
    m_endian = m_needsEndian ? Little : Neutral;
  return 0;
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
