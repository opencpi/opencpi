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
#include <ctype.h>
#include "data.h"
#include "hdl.h"
#include "hdl-device.h"

//bool hdlAssy = false;

// This is an HDL file, and perhaps an assembly or a platform or a container
const char *Worker::
parseHdl(const char *a_package) {
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
    if ((err = parseHdlImpl(a_package)))
      return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  } else if (!strcasecmp(m_xml->name, "HdlAssembly") ||
	     !strcasecmp(m_xml->name, "HdlPlatformAssembly") ||
	     !strcasecmp(m_xml->name, "HdlContainerAssembly")) {
    if ((err = parseHdlAssy()))
      return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  } else
    return "file contains neither an HdlImplementation nor an HdlAssembly nor an HdlPlatform";
  if (m_ports.size() > 32)
    return "worker has more than 32 ports";
  m_model = HdlModel;
  m_modelString = "hdl";
  return NULL;
}
// FIXME: get the basic HDL workers in their own class
const char *Worker::
finalizeHDL() {
  // This depends on the final property processing based on parameters etc.
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++) {
    OU::Property &p = **pi;
    // For HDL, we need the string length for string properties, so we compute it here
    if (p.m_isParameter && p.m_baseType == OA::OCPI_String && p.m_stringLength == 0) {
      assert(p.m_default);
      assert(p.m_default->m_vt == &p);
      p.m_stringLength = p.m_default->maxStringLength();
    }
    if (!p.m_isParameter || p.m_isReadable) { // if worker has a value beyond generics
      if (p.m_isRaw) {
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
	if (p.m_isVolatile || (p.m_isReadable && !p.m_isWritable && !p.m_isParameter))
	  (p.m_isBuiltin ? m_ctl.builtinReadbacks : m_ctl.nonRawReadbacks) = true;
	m_ctl.nNonRawRunProperties++;
	if (p.m_isSub32)
	  m_ctl.nonRawSub32Bits = true;
      }
    }
  }
  if (m_ctl.sub32Bits)
      m_needsEndian = true;
  // Finalize endian default
  if (m_endian == NoEndian)
    m_endian = m_needsEndian ? Little : Neutral;
  const char *err;
  if ((err = deriveOCP()))
    return OU::esprintf("in %s for %s: %s", m_xml->name, m_implName, err);
  unsigned wipN[NWIPTypes][2] = {{0}};
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    if ((err = p->doPatterns(wipN[p->m_type][p->masterIn()], m_maxPortTypeName)))
      return err;
    wipN[p->m_type][p->masterIn()]++;
  }
  return NULL;
}

const char *endians[] = {ENDIANS, NULL};

const char *Worker::
parseHdlImpl(const char *a_package) {
  const char *err;
  ezxml_t xctl;
  size_t dw;
  bool dwFound;
  if (!strcasecmp(OE::ezxml_tag(m_xml),"hdldevice"))
    m_isDevice = true;
  // Since we will steal properties from the device (worker) being emulated,
  // we actually need to know that worker before parsing this one.
  const char *emulate = ezxml_cattr(m_xml, "emulate");
  if (emulate) {
    const char *dot = strrchr(emulate, '.');
    if (!dot)
      return OU::esprintf("'emulate' attribute: '%s' has no authoring model suffix", emulate);
    if (!(m_emulate = HdlDevice::get(emulate, NULL, m_file.c_str(), this, err)))
      return OU::esprintf("for emulated device worker %s: %s", emulate, err);
    for (PropertiesIter pi = m_emulate->m_ctl.properties.begin();
	 pi != m_emulate->m_ctl.properties.end(); ++pi) {
      OU::Property &p = **pi;
      if (p.m_isParameter || (p.m_isWritable && !p.m_isRaw)) {
	// Roughly like addProperty.  FIXME: make a cloning version of addProperty and use it here
	ocpiDebug("Cloning property %s from emulatee ", p.cname());
	OU::Property *np = new OU::Property(p);
	np->m_ordinal = m_ctl.ordinal++;
	if (&p == m_emulate->m_debugProp)
	  m_debugProp = np;
	m_ctl.properties.push_back(np);
	m_ctl.summarizeAccess(*np);
      }
    }
    // roughly a copy-constructor that replaces the worker reference
    m_paramConfigs.resize(m_emulate->m_paramConfigs.size());
    for (unsigned n = 0; n < m_paramConfigs.size(); n++) {
      m_paramConfigs[n] = new ParamConfig(*this);
      m_paramConfigs[n]->clone(*m_emulate->m_paramConfigs[n]);
    }
  }
  // This must be here so that when the properties are parsed,
  // the first raw one is properly aligned.
  if ((err = parseSpec(a_package)) ||
      (err = parseImplControl(xctl)) ||
      (err = OE::getNumber(m_xml, "datawidth", &dw, &dwFound)) ||
      (err = OE::getBoolean(m_xml, "outer", &m_outer)))
    return err;
  if (dwFound)
    m_defaultDataWidth = dw; // override the default if set
  if (m_noControl) {
    // Devices always get wci reset/control
    if (m_isDevice)
      addWciClockReset();
  } else {
    if (!createPort<WciPort>(*this, xctl, NULL, -1, err))
      return err;
    if (!m_wci->m_count)
      m_wci->m_count = 1;
  }
  if ((err = parseClocks()))
    return err;
  // Now that we have clocks roughly set up, we process the wci clock
  //  if (wci && (err = checkClock(xctl, wci)))
  //    return err;
  // End of control interface/wci processing (except OCP signal config)
  //  size_t oldSize = m_ports.size(); // remember the base of extra ports
  // This ordering is repeated below
  if ((err = initImplPorts(m_xml, "MemoryInterface", createPort<WmemiPort>)) ||
      (err = initImplPorts(m_xml, "TimeInterface", createPort<WtiPort>)) ||
      (err = initImplPorts(m_xml, "timeservice", createPort<TimeServicePort>)) ||
      (err = initImplPorts(m_xml, "timebase", createPort<TimeBasePort>)) ||
      (err = initImplPorts(m_xml, "CPMaster", createPort<CpPort>)) ||
      (err = initImplPorts(m_xml, "uNOC", createPort<NocPort>)) ||
      (err = initImplPorts(m_xml, "SDP", createPort<SdpPort>)) ||
      (err = initImplPorts(m_xml, "Metadata", createPort<MetaDataPort>)) ||
      (err = initImplPorts(m_xml, "Control", createPort<WciPort>)) ||
      (err = initImplPorts(m_xml, "DevSignal", createPort<DevSignalsPort>)) ||
      (err = initImplPorts(m_xml, "DevSignals", createPort<DevSignalsPort>)) ||
      (err = initImplPorts(m_xml, "rawprop", createPort<RawPropPort>)))
    return err;

  // Prepare to process data plane port implementation info
  // Now lets look at the implementation-specific data interface info
  DataPort *sp;
  for (ezxml_t s = ezxml_cchild(m_xml, "StreamInterface"); s; s = ezxml_cnext(s))
    if ((err = checkDataPort(s, sp)) || !createDataPort<WsiPort>(*this, s, sp, -1, err))
    return err;
  for (ezxml_t m = ezxml_cchild(m_xml, "MessageInterface"); m; m = ezxml_cnext(m))
    if ((err = checkDataPort(m, sp)) || !createDataPort<WmiPort>(*this, m, sp, -1, err))
    return err;
  // Final passes over all data ports for defaulting and checking
  // 1. Convert any data ports to WSI if they were not mentioned and determine if a wci clk is
  //    needed.
  for (unsigned i = 0; i < m_ports.size(); i++)
    m_ports[i]->finalizeHdlDataPort(); // This will convert to a concrete impl type if not one yet
  // 2. Resolve clock references between ports
  if (!m_wciClock)
    addWciClockReset();
  for (unsigned i = 0; i < m_ports.size(); i++)
    if ((err = m_ports[i]->checkClock()))
      return err;
  // Now check that all clocks have a home and all ports have a clock
  for (auto ci = m_clocks.begin(); ci != m_clocks.end(); ci++) {
    Clock *c = *ci;
    if (!c->m_port && c->m_signal.empty())
      c->m_signal = c->m_name;
  }
  if (emulate) {
    //    addWciClockReset();
    if (ezxml_cchild(m_xml, "signal") || ezxml_cchild(m_xml, "signals"))
      return OU::esprintf("Can't have both \"emulate\" attributed and \"signal\" elements");
    //    std::string ew;
    //    OU::format(ew, "../%s/%.*s.xml", emulate, (int)(dot - emulate), emulate);
    for (SignalsIter si = m_emulate->m_signals.begin();
	 si != m_emulate->m_signals.end(); si++) {
      Signal *s = (*si)->reverse();
      m_signals.push_back(s);
      m_sigmap[s->m_name.c_str()] = s;
    }
    // For device-type ports, create the mirror image for the emulator
    for (unsigned i = 0; i < m_emulate->m_ports.size(); i++) {
      Port &p = *m_emulate->m_ports[i];
      if (p.m_type == DevSigPort || p.m_type == PropPort) {
	bool master = false;
	ocpiCheck(!OE::getBoolean(p.m_xml, "master", &master));
	char *copy = ezxml_toxml(p.m_xml);
	ezxml_t nx = ezxml_parse_str(copy, strlen(copy));
	ezxml_set_attr_d(nx, "master", master ? "0" : "1");
	if (!ezxml_cattr(nx, "name"))
	  ezxml_set_attr_d(nx, "name", p.pname());
	if (p.m_type == DevSigPort)
	  new DevSignalsPort(*this, nx, NULL, -1, err);
	else
	  new RawPropPort(*this, nx, NULL, -1, err);
      }
    }
  } else if ((err = Signal::parseSignals(m_xml, m_file, m_signals, m_sigmap, this)))
    return err;
  // now make sure clockPort references are sorted out and counts are non-zero
  for (unsigned i = 0; i < m_ports.size(); i++) {
    Port *p = m_ports[i];
    if (p->m_clockPort != SIZE_MAX)
      p->m_clock = m_ports[p->m_clockPort]->m_clock;
    if (p->m_count == 0)
      p->m_count = 1;
  }
  return 0;
}


Signal::
Signal()
  : m_direction(NONE), m_width(0), m_differential(false), m_pin(false), m_type(NULL) {
}

Signal * Signal::
reverse() {
  Signal *s = new Signal(*this);
  switch(m_direction) {
  case IN: s->m_direction = OUT; break;
  case OUT:s->m_direction = IN; break;
  case INOUT:s->m_direction = OUTIN; break;
  default:
    break;
  }
  return s;
}

static void ucase(std::string &s) {
  for (unsigned n = 0; n < s.length(); ++n)
    if (s[n] == '%')
      ++n;
    else if (islower(s[n]))
      s[n] = (char)toupper(s[n]);
}

const char *Signal::directions[] = { DIRECTIONS, NULL };

const char *Signal::
parseDirection(const char *direction, std::string *expr, OCPI::Util::IdentResolver &ir) {
  static OU::Member dirType;
  if (dirType.m_baseType == OA::OCPI_none) {
    std::string enums;
    for (const char **ap = directions; *ap; ap++)
      OU::formatAdd(enums, "%s%s", enums.empty() ? "" : ",", *ap);
    char *xml;
    asprintf(&xml, "<member type='enum' enums='%s'/>", enums.c_str());
    ezxml_t tx;
    ocpiCheck(!OE::ezxml_parse_str(xml, 0, tx));
    ocpiCheck(!dirType.parse(tx, false, false, NULL, "direction", 0, NULL));
    free(xml);
  }
  OU::Value dirValue(dirType);
  bool isVariable;
  const char *err;
  if ((err = dirValue.parse(direction, NULL, false, &ir, &isVariable)))
    return OU::esprintf("error parsing direction attribute for \"%s\" signal: %s", cname(), err);
  if ((Direction)dirValue.m_ULong >= NONE)
    return OU::esprintf("direction attribute for \"%s\" invalid:  numeric value is %zu",
			cname(), (size_t)dirValue.m_ULong);
  m_direction = (Direction)dirValue.m_ULong;
  if (isVariable && expr)
    *expr = direction;
  return NULL;
}

const char *Signal::
parseWidth(const char *width, std::string *expr, OCPI::Util::IdentResolver &ir) {
    OU::Value widthValue(OA::OCPI_ULong);
    bool isVariable;
    const char *err;
    if ((err = widthValue.parse(width, NULL, false, &ir, &isVariable)))
      return OU::esprintf("error parsing width attribute for \"%s\" signal: %s", cname(), err);
    m_width = widthValue.m_ULong;
    if (isVariable && expr)
      *expr = width;
     return NULL;
}

const char *Signal::
parse(ezxml_t x, Worker *w) {
  const char *err;
  if ((err =
       OE::checkAttrs(x, "input", "inout", "bidirectional", "output", "width", "direction",
		      "name", "differential", "type", "pos", "neg", "in", "out", "oe",
		      "description", "pin", "platform", (void*)0)))
    return err;
  const char
    *name = ezxml_cattr(x, "name"),
    *nameIn = ezxml_cattr(x, "Input"),
    *nameOut = ezxml_cattr(x, "Output"),
    *nameInOut = ezxml_cattr(x, "InOut"),
    *nameOutIn = ezxml_cattr(x, "OutIn"),
    *nameBiDir = ezxml_cattr(x, "Bidirectional"),
    *direction = ezxml_cattr(x, "direction"),
    *width = ezxml_cattr(x, "width");
  if (name) {
    if (nameIn || nameOut || nameInOut || nameBiDir)
      return "Signal directions must be specified using the \"direction\" attribute when the "
	"\"name\" attribute is used";
    if (!direction)
      return "The \"direction\" attribute is required when the \"name\" attribute is present";
    if ((err = parseDirection(direction, &m_directionExpr, *w)))
      return err;
  } else {
    if (direction)
      return "Signal directions must be specified using the "
	"input/output/inout/bidirectional attributes if the name attribute is "
	"not used.  Using \"name\" and \"direction\" is recommended";
    switch ((nameIn ? 1 : 0) + (nameOut ? 1 : 0) + (nameInOut ? 1 : 0) + (nameBiDir ? 1 : 0)) {
    default:
      return "Only one of the input/output/inout/bidirectional attributes can be specified";
    case 0:
      return "If the \"name\" attribute is unspecified, one of input/output/inout/bidirectional "
	"must be specified";
    case 1:
      if ((name = nameIn))
	m_direction = IN;
      else if ((name = nameOut))
	m_direction = OUT;
      else if ((name = nameInOut))
	m_direction = INOUT;
      else if ((name = nameBiDir))
	m_direction = BIDIRECTIONAL;
      else if ((name = nameOutIn))
	m_direction = OUTIN;
    }
  }
  if (width && (err = parseWidth(width, &m_widthExpr, *w)))
    return err;
  if ((err = OE::getBoolean(x, "pin", &m_pin)) ||
      (err = OE::getBoolean(x, "differential", &m_differential)))
    return err;
  if (m_direction == INOUT && m_differential)
    return OU::esprintf("Signals that are both \"inout\" and differential are not supported");
  m_type = ezxml_cattr(x, "type");
  assert(!m_type); // catch any usage since this is obsolete and never documented?
  m_name = name;
  OE::getOptionalString(x, m_pos, "pos", "%sp");
  OE::getOptionalString(x, m_neg, "neg", "%sn");
  OE::getOptionalString(x, m_in, "in", "%s_i");
  OE::getOptionalString(x, m_out, "out", "%s_o");
  OE::getOptionalString(x, m_oe, "oe", "%s_oe");
  bool anyLower = false;
  for (const char *cp = name; *cp; ++cp)
    if (islower(*cp))
      anyLower = true;

  if (!anyLower) {
    ucase(m_pos);
    ucase(m_neg);
    ucase(m_in);
    ucase(m_out);
    ucase(m_oe);
  }
  return NULL;
}

const char *Signal::
parseSignals(ezxml_t xml, const std::string &parent, Signals &signals, SigMap &sigmap,
	     Worker *w) {
  const char *err = NULL;
  std::string sigattr;
  if (OE::getOptionalString(xml, sigattr, "signals")) {
    ezxml_t sigx;
    std::string sigFile;
    if ((err = parseFile(sigattr.c_str(), parent, "Signals", &sigx, sigFile, false)) ||
	(err = parseSignals(sigx, sigFile, signals, sigmap, w)))
      return err;
  }
  // process ad hoc signals
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); !err && xs; xs = ezxml_cnext(xs)) {
    Signal *s = new Signal;
    if (!(err = s->parse(xs, w)))  {
      if (sigmap.find(s->m_name.c_str()) == sigmap.end()) {
	signals.push_back(s);
	sigmap[s->m_name.c_str()] = s;
      } else {
	err = OU::esprintf("Duplicate signal: '%s'", s->m_name.c_str());
	delete s;
      }
    }
  }
  return err;
}

const Signal *Signal::
find(const SigMap &sigmap, const char *name) {
  SigMap::const_iterator si = sigmap.find(name);
  return si == sigmap.end() ? NULL : si->second;
}

void Signal::
deleteSignals(Signals &signals) {
  for (SignalsIter si = signals.begin(); si != signals.end(); si++)
    delete *si;
}

Signal *SigMap::
findSignal(const char *name, std::string *suffixed) const {
  SigMap_::const_iterator i = find(name);
  if (suffixed)
    suffixed->clear();
  if (i != end())
    return i->second;
  // No match, perhaps a suffixed match, look the slow way
  if (!suffixed)
    return NULL;
  for (i = begin(); i != end(); i++) {
    Signal &s = *i->second;
    if (s.m_differential) {
      OU::format(*suffixed, s.m_pos.c_str(), s.m_name.c_str());
      if (!strcasecmp(name, suffixed->c_str()))
	return &s;
      OU::format(*suffixed, s.m_neg.c_str(), s.m_name.c_str());
      if (!strcasecmp(name, suffixed->c_str()))
	return &s;
    } else if (s.m_direction == Signal::INOUT || s.m_direction == Signal::OUTIN) {
      OU::format(*suffixed, s.m_in.c_str(), s.m_name.c_str());
      if (!strcasecmp(name, suffixed->c_str()))
	return &s;
      OU::format(*suffixed, s.m_out.c_str(), s.m_name.c_str());
      if (!strcasecmp(name, suffixed->c_str()))
	return &s;
      OU::format(*suffixed, s.m_oe.c_str(), s.m_name.c_str());
      if (!strcasecmp(name, suffixed->c_str()))
	return &s;
    } else
      continue;
  }
  suffixed->clear();
  return NULL;
}

const char *SigMap::
findSignal(Signal *s) {
  for (SigMap_::const_iterator si = begin(); si != end(); si++)
    if ((*si).second == s)
      return (*si).first;
  return NULL;
}

const char *SigMapIdx::
findSignal(Signal *sig, size_t idx) const {
  for (SigMapIdxIter i = begin(); i != end(); i++)
    if ((*i).second.first == sig && (*i).second.second == idx)
      return (*i).first;
  return NULL;
}

// emit one side of differential, or only side..
void Signal::
emitConnectionSignal(FILE *f, const char *iname, const char *pattern, bool single,
		     Language lang) {
  std::string name;
  OU::format(name, pattern, m_name.c_str());
  if (lang == VHDL) {
    fprintf(f, "  signal %s%s%s : std_logic", iname ? iname : "", iname ? "_" : "", name.c_str());
    if (m_width && !single)
      fprintf(f, "_vector(%zu downto 0)", m_width-1);
    fprintf(f, ";\n");
  } else {
    fprintf(f, "wire ");
    if (m_width && !single)
      fprintf(f, "[%3zu:0] ", m_width-1);
    fprintf(f, "%s%s%s;\n", iname ? iname : "", iname ? "_" : "", name.c_str());
  }
}


// emit the "signal pseudo constants" for the worker.
// We record "macros" that define the directions of the ad-hoc signals for device workers.
// They are recorded in the generics file since they are similar in spirit to the
// constants that are defined there even though they are basically macros embedded in comments.
// The macro definition is of the form:
// <comment-prefix>__name=value
// The underscores are in fact part of the name of the macro
// This is inching toward VHDL template programming (cheetah etc.), but not enough to go all the
// way there.
void Worker::
emitSignalMacros(FILE *f, Language lang) {
  std::string prefix, last;
  OU::format(prefix, "  %s__decl", hdlComment(lang));
  bool first = true, anyNonExpr = false;
  // First pass - are there any non-parameterized signals?
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    if (s.m_directionExpr.empty() && s.m_widthExpr.empty() &&
	(s.m_direction != Signal::UNUSED || m_type == Container))
      anyNonExpr = true;
  }
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    if ((s.m_directionExpr.length() && (s.m_direction != Signal::UNUSED || m_type == Container)) ||
         s.m_widthExpr.length()) {
      last += prefix;
      if (first) {
	fprintf(f, "  %s Define signals that are parameterized\n", hdlComment(lang));
	if (anyNonExpr)
	  last += "    ;\n" + prefix;
      }
      first = false;
      emitDeviceSignal(f, lang, last, s, prefix.c_str());
    }
  }
  if (!first)
    emitLastSignal(f, last, lang, true);
  OU::format(prefix, "  %s__map", hdlComment(lang));
  last = "";
  first = true;
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    if ((s.m_directionExpr.length() && s.m_direction != Signal::UNUSED) ||
       s.m_widthExpr.length()) {
      if (first) {
	fprintf(f, "  %s Map signals that are parameterized\n", hdlComment(lang));
	last += prefix + "     ,\n";
      }
      first = false;
      emitDeviceSignalMapping(f, last, s, prefix.c_str());
      last = ",\n";
    }
  }
  fprintf(f, "\n");
}
