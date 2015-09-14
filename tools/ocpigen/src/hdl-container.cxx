#include <stdint.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "HdlOCCP.h"
#include "assembly.h"
#include "hdl.h"
#include "hdl-container.h"

static void
emitTimeClient(std::string &assy, const char *instance, const char *port) {
  OU::formatAdd(assy,
		"  <instance worker='time_client' name='%s_%s_time_client'/>\n"
		"  <connection>\n"
		"    <port instance='%s_%s_time_client' name='wti'/>\n"
		"    <port instance='%s' name='%s'/>\n"
		"  </connection>\n"
		"  <connection>\n"
		"    <port instance='pfconfig' name='time'/>\n"
		"    <port instance='%s_%s_time_client' name='time'/>\n"
		"  </connection>\n",
		instance, port,
		instance, port,
		instance, port,
		instance, port);
}

HdlContainer *HdlContainer::
create(ezxml_t xml, const char *xfile, const char *&err) {
  err = NULL;
  if ((err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONTAINER_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, HDL_CONTAINER_ELEMS, (void*)0)))
    return NULL;
  std::string myConfig, myPlatform, myAssy;
  // Process the configuration name.
  // It might have a slash between platform and config, or just be a platform.
  // The configuration might be auto-generated (in the gen subdir) or not.
  // So first we split things, and then check for hand-authored (in the platform dir),
  // then try the "gen" subdir.
  OE::getOptionalString(xml, myConfig, "config");
  OE::getOptionalString(xml, myPlatform, "platform");
  if (myConfig.length() && myPlatform.length()) {
    if (strchr(myConfig.c_str(), '/') ||
	strchr(myPlatform.c_str(), '/')) {
      err = "Slashes not allowed when both platform and config attributes specified";
      return NULL;
    }
  } else {
    // one or the other
    if (myConfig.length())
      myPlatform = myConfig;
    else if (myPlatform.empty())
      if (platform)
	myPlatform = platform;
      else {
	err = "No platform or platform configuration specified in HdlContainer";
	return NULL;
      }
    // assume only platform is specified
    const char *slash = strchr(myPlatform.c_str(), '/');
    if (slash) {
      myConfig = slash + 1;
      myPlatform.resize(slash - myPlatform.c_str());
    } else
      myConfig = "base";
  }
  OE::getOptionalString(xml, myAssy, "assembly");
  if (myAssy.empty())
    if (assembly)
      myAssy = assembly;
    else {
      err = OU::esprintf("No assembly specified for container specified in %s", xfile);
      return NULL;
    }
  std::string configFile, assyFile, configName;
  HdlConfig *config;
  HdlAssembly *appAssembly;
  ezxml_t x;
  // base configurations are by definition empty.
  // This is done by hand here, and there is also a base.xml file in platforms/specs
  if (myConfig == "base") {
    char *xml = strdup("<HdlConfig/>");
    // Base config has not been generated...
    ocpiCheck(OE::ezxml_parse_str(xml, strlen(xml), x) == NULL);
    configFile = "base.xml"; // where is this really used?
  } else {
    OU::format(configName, "%s/hdl/%s", ::platformDir, myConfig.c_str());
    if ((err = parseFile(configName.c_str(), xfile, "HdlConfig", &x, configFile))) {
      configName = myPlatform + "/gen/" + myConfig;
      if (parseFile(configName.c_str(), xfile, "HdlConfig", &x, configFile))
	return NULL;
    }
  }
  if (!(config = HdlConfig::create(x, myPlatform.c_str(), configFile.c_str(), NULL, err)) ||
      (err = parseFile(myAssy.c_str(), xfile, "HdlAssembly", &x, assyFile)) ||
      !(appAssembly = HdlAssembly::create(x, assyFile.c_str(), NULL, err)))
    return NULL;
  HdlContainer *p = new HdlContainer(*config, *appAssembly, xml, xfile, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}

HdlContainer::
HdlContainer(HdlConfig &config, HdlAssembly &appAssembly, ezxml_t xml, const char *xfile,
	     const char *&err)
  : Worker(xml, xfile, "", Worker::Container, NULL, NULL, err),
    HdlHasDevInstances(config.m_platform, config.m_plugged),
    m_appAssembly(appAssembly), m_config(config) {
  appAssembly.setParent(this);
  config.setParent(this);
  if (!platform)
    platform = m_config.platform().m_name.c_str();
  bool doDefault = false;
  if ((err = OE::getBoolean(xml, "default", &doDefault)))
    return;
  switch (m_endian) {
  case NoEndian:
    m_endian = Little;
  case Little:
  case Big:
    break;
  default:
    err = OU::esprintf("The endian setting \"%s\" is not allowed in containers",
		       endians[m_endian]);
    return;
  }
  // Set the fixed elements that would normally be parsed
  m_noControl = true;
  // Prepare to build (and terminate) the uNocs for interconnects
  // We remember the last instance for each uNoc
  // Establish the NOC usage, if there is any.
  // An interconnect can be on any device worker, but for now it is on the config.
  UNocs uNocs;
  Port *sdp = NULL;
  unsigned *unoc = NULL;
  for (PortsIter pi = m_config.m_ports.begin(); pi != m_config.m_ports.end(); pi++) {
    Port &p = **pi;
    Port *slave = NULL;
    if (p.master && (p.type == NOCPort || p.type == SDPPort)) {
      size_t len = p.m_name.length();
      // Find the slave port for this master just for error checking
      for (PortsIter si = m_config.m_ports.begin(); si != m_config.m_ports.end(); si++) {
	Port &sp = **si;
	if (!sp.master && (sp.type == NOCPort || sp.type == SDPPort) &&
	    !strncasecmp(p.name(), sp.name(), len) && !strcasecmp(sp.name() + len, "_slave")) {
	  slave = &sp;
	  break;
	}
      }
      ocpiAssert(slave);
      uNocs[p.name()] = 0;
      if (p.type == SDPPort) {
	unoc = &uNocs.at(p.name());
	sdp = &p;
      }
    }
  }
  // Preinstall device instances.  These may be devices in the platform OR may be
  // random devices that are just standalone workers.
  for (ezxml_t dx = ezxml_cchild(m_xml, "device"); dx; dx = ezxml_next(dx)) {
    bool floating = false;
    if ((err = OE::getBoolean(dx, "floating", &floating)))
      return;
    std::string name;
    if (floating) {
      // This device is not part of the platform.
      // FIXME:  Apologies for this gross unconsting, but its the least of various evils
      // Fixing would involve allowing containers to own devices...
      HdlPlatform &pf = *(HdlPlatform *)&m_platform;
      err = pf.addFloatingDevice(dx, xfile, this, name);
    } else
      err = OE::getRequiredString(dx, name, "name");
    // We have a device to add to the container that exists on the platform or on a card
    if (err || (err = parseDevInstance(name.c_str(), dx, m_file.c_str(), this, false,
				       &m_config.devInstances(), NULL, NULL)))
      return;
  }
  // Establish connections, WHICH MAY ALSO IMPLICITLY CREATE DEVICE INSTANCES
  ContConnects connections;
  for (ezxml_t cx = ezxml_cchild(m_xml, "connection"); cx; cx = ezxml_next(cx)) {
    ContConnect c;
    if ((err = parseConnection(cx, c)))
      return;
    connections.push_back(c);
  }
  size_t nWCIs = 0; // count control ports as we go, to scale the ocscp at the end
  std::string assy;
  OU::format(assy, "<HdlContainerAssembly name='%s' language='vhdl'>\n", m_name.c_str());
  // The platform configuration instance
  OU::formatAdd(assy, "  <instance name='pfconfig' worker='%s'>\n", m_config.m_file.c_str());
  // We must map any signals from card-based device workers to the slot signals.
  // Devices on platforms will already have the right default external signal names.
  for (DevInstancesIter di = m_config.m_devInstances.begin();
       di != m_config.m_devInstances.end(); di++)
    mapDevSignals(assy, *di, false);
  // We must force platform signals to be mapped to themselves.
  // FIXME: when platforms are devices, we could remap those signals too.
  for (SignalsIter s = m_config.m_platform.Worker::m_signals.begin();
       s != m_config.m_platform.Worker::m_signals.end(); s++) {
#if 0
    OU::formatAdd(assy, "    <signal name='%s' external='%s'/>\n",
		  (*s)->name(), (*s)->name());
#endif
    Signal *es = new Signal(**s);
    m_signals.push_back(es);
    m_sigmap[(*s)->cname()] = es;
  }
  OU::formatAdd(assy, "  </instance>\n");
  // Connect the platform configuration to the control plane
  if (!m_config.m_noControl) {
    Port &p = *m_config.m_ports[0];
    OU::formatAdd(assy,
		  "  <connection count='%zu'>\n"
		  "    <port instance='ocscp' name='wci'/>\n"
		  "    <port instance='pfconfig' name='%s'/>\n"
		  "  </connection>\n",
		  p.count, p.name());
    nWCIs += p.count;
  }
  if (m_appAssembly.m_assembly && m_appAssembly.m_assembly->m_nInstances != 0) {
    // Instance the assembly and connect its wci
    OU::formatAdd(assy, "  <instance worker='%s'/>\n", m_appAssembly.m_implName);
    // Connect the assembly to the control plane
    if (!m_appAssembly.m_noControl) {
      Port &p = *m_appAssembly.m_ports[0];
      OU::formatAdd(assy,
		    "  <connection count='%zu'>\n"
		    "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		    "    <port instance='%s' name='%s'/>\n"
		    "  </connection>\n",
		    p.count, nWCIs,
		    m_appAssembly.m_implName, p.name());
      nWCIs += p.count;
    }
  }
  if (doDefault) {
    if (ezxml_cchild(m_xml, "connection")) {
      err = "Connections are not allowed in default containers";
      return;
    }
    if (ezxml_cchild(m_xml, "device")) {
      err = "Devices are not allowed in default containers";
      return;
    }
    for (PortsIter pi = m_appAssembly.m_ports.begin(); pi != m_appAssembly.m_ports.end(); pi++)
      if ((*pi)->isData()) {
	if (uNocs.empty() || uNocs.size() > 1) {
	  if (!attribute)
	    err = OU::esprintf("No single interconnect in platform configuration for assembly port %s",
			       (*pi)->name());
	  return;
	}
	for (PortsIter ii = m_config.m_ports.begin(); ii != m_config.m_ports.end(); ii++) {
	  Port &i = **ii;
	  if (i.master && i.type == NOCPort) {
	    ContConnect c;
	    c.external = *pi;
	    c.interconnect = &i;
	    if ((err = emitUNocConnection(assy, uNocs, nWCIs, c)))
	      return;
	    break;
	  }
	}
      }
  } else {
    for (DevInstancesIter di = m_devInstances.begin(); di != m_devInstances.end(); di++) {
      // Decide whether to map the signals or not, based on whether we are an emulator
      // or are paired with an emulator
      const DevInstance *emulator = NULL;
      for (DevInstancesIter edi = m_devInstances.begin(); edi != m_devInstances.end(); edi++)
	if ((*edi).device.deviceType().m_emulate &&
	    !strcasecmp((*edi).device.deviceType().m_emulate->m_implName,
			(*di).device.deviceType().m_implName)) {
	  emulator = &*edi;
	  break;
	}
      // Instance the device and connect its wci
      OU::formatAdd(assy, "  <instance name='%s' worker='%s'%s>\n",
		    (*di).cname(),
		    (*di).device.deviceType().cname(),
		    emulator ? " emulated='1'" : "");
      if (!emulator && !(*di).device.deviceType().m_emulate)
	mapDevSignals(assy, *di, true);
      assy += "  </instance>\n";
      if (!(*di).device.deviceType().m_noControl) {
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      nWCIs, (*di).cname(),
		      (*di).device.deviceType().ports()[0]->name());
      
	nWCIs++;
      }
      // Instance time clients for the assembly
      const Ports &ports = (*di).device.deviceType().ports();
      for (PortsIter pi = ports.begin(); pi != ports.end(); pi++)
	if ((*pi)->type == WTIPort)
	  emitTimeClient(assy, (*di).cname(), (*pi)->name());
    }
    for (ContConnectsIter ci = connections.begin(); ci != connections.end(); ci++)
      if ((err = emitConnection(assy, uNocs, nWCIs, *ci)))
	return;
    emitSubdeviceConnections(assy, &m_config.m_devInstances);
  }
  // Instance the scalable control plane and adapter to SDP if present.
  OU::formatAdd(assy,
		"  <instance worker='ocscp'>\n"
		"    <property name='nworkers' value='%zu'/>\n"
		"    <property name='ocpi_endian' value='%s'/>\n"
		"  </instance>\n", nWCIs,
		endians[m_endian]);
  if (sdp) {
    OU::formatAdd(assy,
		  "  <instance name='%s_unoc%u' worker='sdp_node'>\n"
		  "    <property name='sdp_width' value='%zu'/>\n"
		  "  </instance>\n"
		  "  <instance name='%s_sdp2cp' worker='sdp2cp'>\n"
		  "    <property name='sdp_width' value='%zu'/>\n"
		  "  </instance>\n"
		  "  <connection>\n"
		  "    <port instance='%s_unoc%u' name='up'/>\n"
		  "    <port instance='pfconfig' name='sdp'/>\n"
		  "  </connection>\n"
		  "  <connection>\n"
		  "    <port instance='%s_unoc%u' name='client'/>\n"
		  "    <port instance='%s_sdp2cp' name='sdp'/>\n"
		  "  </connection>\n"
		  "  <connection>\n"
		  "    <port instance='%s_sdp2cp' name='cp'/>\n"
		  "    <port instance='ocscp' name='cp'/>\n"
		  "  </connection>\n",
		  sdp->name(), *unoc, m_config.sdpWidth(), sdp->name(), m_config.sdpWidth(),
		  sdp->name(), *unoc, sdp->name(), *unoc, sdp->name(), sdp->name());
    (*unoc)++;
  } else
    // Connect it to the pf config's cpmaster
    for (PortsIter ii = m_config.m_ports.begin(); ii != m_config.m_ports.end(); ii++) {
      Port &i = **ii;
      if (i.master && i.type == CPPort) {
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='pfconfig' name='%s'/>\n"
		      "    <port instance='ocscp' name='cp'/>\n"
		      "  </connection>\n",
		      i.name());
	break;
      }
    }
  // Terminate the uNocs
  for (UNocsIter ii = uNocs.begin(); ii != uNocs.end(); ii++) {
    std::string prevInstance, prevPort;
    if (ii->second == 0) {
      prevInstance = "pfconfig";
      prevPort = ii->first;
    } else {
      OU::format(prevInstance, "%s_unoc%u", ii->first, ii->second - 1);
      prevPort = "down";
    }
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <port instance='pfconfig' name='%s_slave'/>\n"
		  "  </connection>\n",
		  prevInstance.c_str(), prevPort.c_str(), ii->first);
  }
  // Instance time clients for the assembly
  for (PortsIter pi = m_appAssembly.m_ports.begin(); pi != m_appAssembly.m_ports.end(); pi++)
    if ((*pi)->type == WTIPort)
      emitTimeClient(assy, m_appAssembly.m_implName, (*pi)->name());
  OU::formatAdd(assy,
		"  <instance worker='metadata'/>\n"
		"    <connection>\n"
		"     <port instance='metadata' name='metadata'/>\n"
		"     <port instance='pfconfig' name='metadata'/>\n"
		"    </connection>\n");
  OU::formatAdd(assy, "</HdlContainerAssembly>\n");
  // The assembly will automatically inherit all the signals, prefixed by instance.
  //  if (!attribute)
  ocpiInfo("=======Begin generated container assembly=======\n"
	   "%s"
	   "=======End generated container assembly=======\n",
	   assy.c_str());
  // Now we hack the (inherited) worker to have the xml for the assembly we just generated.
  char *copy = strdup(assy.c_str());
  ezxml_t x;
  if ((err = OE::ezxml_parse_str(copy, strlen(copy), x))) {
    err = OU::esprintf("XML Parsing error on generated container assembly: %s", err);
    return;
  }
  // Make all slot signals external whether they are used or not.
  for (SlotsIter sli = m_platform.slots().begin(); sli != m_platform.slots().end(); sli++) {
    const Slot &sl = *(*sli).second;
    const SlotType &t = sl.m_type;
    unsigned n = 0;
    for (SignalsIter si = t.m_signals.begin(); si != t.m_signals.end(); si++, n++) {
      Slot::SignalsIter ssi = sl.m_signals.find(*si);
      if (ssi != sl.m_signals.end() && ssi->second.empty())
	continue;
      Signal &sig = *new Signal(**si);
      sig.m_name = sl.m_name + "_" + (ssi == sl.m_signals.end() ? (*si)->cname() :
				      ssi->second.c_str());
      if (!m_sigmap.findSignal(sig.m_name)) {
	m_signals.push_back(&sig);
	m_sigmap[sig.cname()] = &sig;
	ocpiDebug("Creating container signal from slot signal: %s", sig.cname());
      }
    }
  }
  m_xml = x;
#if 0 // try this below
  // During the parsing of the container assembly we KNOW what the platform is,
  // but the platform config XML that might be parsed might think it is defaulting
  // from the platform where it lives, so we temporarily set the global to the
  // platform we know.
  const char *save = platform;
  platform = m_platform.m_name.c_str();
  if ((err = parseHdl()))
    return;
  platform = save;
#endif
#if 0 // this is now done in mapDevSignals
  // Make all device instances signals external that are not mapped already
  unsigned n = 0;
  for (Instance *i = m_assembly->m_instances; n < m_assembly->m_nInstances; i++, n++) {
#if 0
    Instance *emulator = NULL, *ii = m_assembly->m_instances;
    for (unsigned nn = 0; nn < m_assembly->m_nInstances; nn++, ii++)
      if (ii->worker->m_emulate &&
	  !strcasecmp(ii->worker->m_emulate->m_implName, i->worker->m_implName) {
	emulator = ii;
	break;
      }
#endif
    if (i->worker->m_emulate || i->m_emulated)
      continue;
    for (SignalsIter si = i->worker->m_signals.begin(); si != i->worker->m_signals.end(); si++) {
      Signal &s = **si;
      for (unsigned n = 0; s.m_width ? n < s.m_width : n == 0; n++) {
	bool single;
	const char *external = i->m_extmap.findSignal(s, n, single);
	if (external) {
	  assert(!*external || m_sigmap.find(external) != m_sigmap.end());
	  if (!single) // if mapped whole, we're done with this (maybe vector) signal
	    break;
	} else {
	  Signal *ns = new Signal(s);
	  if (!i->worker->m_assembly)
	    OU::format(ns->m_name, "%s_%s", i->name, s.cname());
	  m_signals.push_back(ns);
	  m_sigmap[ns->m_name.c_str()] = ns;
	  break;
	}
      }
    }
  }
#endif
  // For platform devices that are not instanced:
  //    Make all device signals external, and cause the outputs to be tied to zero.
  //    EXCEPT for device signals that are explicitly mapped to NULL, meaning they are
  //    not present in this platform
  //    This is necessary here because external signals are normally just created
  //    as a side effect of instances with signals.
  for (DevicesIter di = m_platform.devices().begin(); di != m_platform.devices().end(); di++) {
    if (!findDevInstance(**di, NULL, NULL, &m_config.devInstances(), NULL)) {
      const DeviceType &dt = (*di)->deviceType();
      // We have an uninstanced device
      for (SignalsIter si = dt.m_signals.begin(); si != dt.m_signals.end(); si++) {
	for (unsigned n = 0; (*si)->m_width ? n < (*si)->m_width : n == 0; n++) {
	  Signal *cs = NULL; // The container signal we will create
	  bool isSingle;
	  const char *boardName;
	  if ((boardName = (*di)->m_dev2bd.findSignal(**si, n, isSingle))) {
	    // There is a mapping, but it might be NULL if the signal is not on the platform
	    if (*boardName) {
	      Signal *bs = (**di).m_board.m_extmap.findSignal(boardName);
	      assert(bs);
	      assert((**di).m_board.m_bd2dev.findSignal(boardName));
	      if (m_sigmap.find(boardName) == m_sigmap.end()) {
		cs = new Signal(*bs); // clone the board signal for the container signal
		// If the board signal is bidirectional (can be anything), it should inherit
		// the direction of the device's signal
		if (bs->m_direction == Signal::BIDIRECTIONAL)
		  cs->m_direction = (*si)->m_direction;
	      }
	    }
	  } else {
	    std::string name;
	    OU::format(name, "%s_%s", (*di)->m_name.c_str(), (*si)->m_name.c_str());
	    if (m_sigmap.find(name.c_str()) == m_sigmap.end()) {
	      // No mapping - the device signal has the default mapping - clone the device signal
	      cs = new Signal(**si);
	      cs->m_name = name;
	    }
	  }
	  if (cs) {
	    m_signals.push_back(cs);
	    m_sigmap[cs->cname()] = cs;
	  }
	  if (!isSingle)
	    break;
	}
      }
    }
  }
#if 1
  // During the parsing of the container assembly we KNOW what the platform is,
  // but the platform config XML that might be parsed might think it is defaulting
  // from the platform where it lives, so we temporarily set the global to the
  // platform we know.
  const char *save = platform;
  platform = m_platform.m_name.c_str();
  if ((err = parseHdl()))
    return;
  platform = save;
#endif
}

HdlContainer::
~HdlContainer() {
  delete &m_config;
}

// Establish and parse connection - THIS MAY IMPLICITLY CREATE DEVICE INSTANCES
const char *HdlContainer::
parseConnection(ezxml_t cx, ContConnect &c) {
  const char *err;
  if ((err = OE::checkAttrs(cx, "external", "instance", "device", "interconnect", "port",
			    "otherdevice", "otherport", "card", "slot", NULL)))
    return err;
  const char *attr;
  c.external = NULL;
  if ((attr = ezxml_cattr(cx, "external"))) {
    // Instance time clients for the assembly
    for (PortsIter pi = m_appAssembly.m_ports.begin(); pi != m_appAssembly.m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name(), attr)) {
	c.external = *pi;
	break;
      }
    if (!c.external)
      return OU::esprintf("External assembly port '%s' not found in assembly", attr);
  }
  // Devices are complicated.  Two orthogonal issues:
  // 1. Is it a platform device or a card device?
  // 2. Is it:
  //    A. Already instantiated in the config
  //    B. Is it instantiated explicitly here
  //    C. Is it instantiated implicitly here
  c.devInConfig = false;
  if ((attr = ezxml_cattr(cx, "device")) &&
      (err = parseDevInstance(attr, cx, m_file.c_str(), this, false, &m_config.devInstances(),
			      &c.devInstance, &c.devInConfig)))
    return err;
  if ((attr = ezxml_cattr(cx, "port"))) {
    if (!c.devInstance)
      return OU::esprintf("Port '%s' specified without specifying a device", attr);
    const ::Device &d = c.devInstance->device;
    for (PortsIter pi = d.deviceType().ports().begin();
	 pi != d.deviceType().ports().end(); pi++)
      if (!strcasecmp((*pi)->name(), attr)) {
	c.port = *pi;
	break;
      }
    if (!c.port)
      return OU::esprintf("Port '%s' not found for device '%s'", attr, d.cname());
    if (!c.port->isData())
      return OU::esprintf("Port '%s' for device '%s' is not a data port", attr, d.cname());
  } else if (c.devInstance) {
    const ::Device &d = c.devInstance->device;
    // Find the one data port
    for (PortsIter pi = d.deviceType().ports().begin();
	 pi != d.deviceType().ports().end(); pi++)
      if ((*pi)->isData()) {
	if (c.port)
	  return OU::esprintf("There are multiple data ports for device '%s'; you must specify one",
			      d.cname());
	c.port = *pi;
      }
    if (!c.port)
      return OU::esprintf("There are no data ports for device '%s'", d.cname());
  }
  if ((attr = ezxml_cattr(cx, "interconnect"))) {
    // An interconnect can be on any device worker, but for now it is on the config.
    for (PortsIter pi = m_config.m_ports.begin(); pi != m_config.m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name(), attr)) {
	c.interconnect = *pi;
	break;
      }
    if (!c.interconnect ||
	(c.interconnect->type != NOCPort && c.interconnect->type != SDPPort) ||
	!c.interconnect->master)
      return OU::esprintf("Interconnect '%s' not found for platform '%s'", attr,
			   m_config.platform().m_name.c_str());
  }
  return NULL;
}

// Make a connection to an interconnect
const char *HdlContainer::
emitSDPConnection(std::string &assy, unsigned &unoc, size_t &index, const ContConnect &c) {
  const char *iname = c.interconnect->name();
  Port *port = c.external ? c.external : c.port;
  // Create the two instances:
  // 1. A sdp node to use the interconnect sdp
  // 2. A sender or receiver DP/DMA module to stream to/from another place on the interconnect
  OU::formatAdd(assy,
		"  <instance name='%s_unoc%u' worker='sdp_node'>\n"
		"    <property name='sdp_width' value='%zu'/>\n"
		"  </instance>\n",
		iname, unoc, m_config.sdpWidth());

  const char *dir = port->isDataProducer() ? "send" : "receive";
  // instantiate sdp send or receive, and connect its wci
  OU::formatAdd(assy,
		"  <instance name='%s_sdp_%s%u' worker='sdp_%s' interconnect='%s'>\n"
		"    <property name='sdp_width' value='%zu'/>\n"
		"  </instance>\n"
		"  <connection>\n"
		"    <port instance='%s_sdp_%s%u' name='ctl'/>\n"
		"    <port instance='ocscp' name='wci' index='%zu'/>\n"
		"  </connection>\n"
		"  <connection>\n"
		"    <port instance='%s_unoc%u' name='client'/>\n"
		"    <port instance='%s_sdp_%s%u' name='client'/>\n"
		"  </connection>\n",
		iname, dir, unoc, dir, iname, m_config.sdpWidth(),
		iname, dir, unoc, index,
		iname, unoc, iname, dir, unoc); 
  index++;
  // Connect to the port
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s_sdp_%s%u' %s='%s'/>\n"
		"    <port instance='%s' %s='%s'/>\n"
		"  </connection>\n",
		iname, dir, unoc,
		port->isDataProducer() ? "to" : "from", port->isDataProducer() ? "in" : "out",
		c.external ? m_appAssembly.m_implName : c.devInstance->cname(),
		port->isDataProducer() ? "from" : "to", port->name());
  return NULL;
}
// Make a connection to an interconnect
const char *HdlContainer::
emitUNocConnection(std::string &assy, UNocs &uNocs, size_t &index, const ContConnect &c) {
    // Find uNoc
  const char *iname = c.interconnect->name();
  unsigned &unoc = uNocs.at(iname);
  Port *port = c.external ? c.external : c.port;
  if (port->type != WSIPort ||
      (c.interconnect->type != NOCPort && c.interconnect->type != SDPPort) ||
      !c.interconnect->master)
    return OU::esprintf("unsupported container connection between "
			"port %s of %s%s and interconnect %s",
			port->name(), iname,
			c.external ? "assembly" : "device",
			c.external ? "" : c.devInstance->device.cname());
  const char *err;
  if (c.interconnect->type == SDPPort) {
    if ((err = emitSDPConnection(assy, unoc, index, c)))
      return err;
  } else {
    // Create the three instances:
    // 1. A unoc node to use the interconnect unoc
    // 2. A DP/DMA module to stream to/from another place on the interconnect
    // 3. An SMA to adapt the WMI on the DP to the WSI that is needed (for now).
    OU::formatAdd(assy,
		  "  <instance name='%s_unoc%u' worker='unoc_node'>\n"
		  "    <property name='control' value='false'/>\n"
		  "    <property name='position' value='%u'/>\n"
		  "  </instance>\n",
		  iname, unoc, unoc);
    // instantiate dp, and connect its wci
    OU::formatAdd(assy,
		  "  <instance name='%s_ocdp%u' worker='ocdp' interconnect='%s' configure='%u'>\n"
		  "    <property name='includePull' value='%u'/>\n"
		  "    <property name='includePush' value='%u'/>\n"
		  "  </instance>\n"
		  "  <connection>\n"
		  "    <port instance='%s_ocdp%u' name='ctl'/>\n"
		  "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		  "  </connection>\n",
		  iname, unoc, iname, unoc,
		  1, // port->u.wdi.isProducer ? 0 : 1,
		  1, // port->u.wdi.isProducer ? 1 : 0,
		  iname, unoc,
		  index);
    index++;
    // instantiate sma, and connect its wci
    OU::formatAdd(assy,
		  "  <instance name='%s_sma%u' worker='sma' adapter='%s' configure='%u'/>\n"
		  "  <connection>\n"
		  "    <port instance='%s_sma%u' name='ctl'/>\n"
		  "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		  "  </connection>\n",
		  iname, unoc, iname, 
		  port->isDataProducer() ? 2 : 1,
		  iname, unoc,
		  index);
    index++;
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s_unoc%u' name='client'/>\n"
		  "    <port instance='%s_ocdp%u' name='client'/>\n"
		  "  </connection>\n",
		  iname, unoc, iname, unoc);
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s_ocdp%u' %s='data'/>\n"
		  "    <port instance='%s_sma%u' %s='message'/>\n"
		  "  </connection>\n",
		  iname, unoc, port->isDataProducer() ? "to" : "from",
		  iname, unoc, port->isDataProducer() ? "from" : "to");
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s_sma%u' %s='%s'/>\n"
		  "    <port instance='%s' %s='%s'/>\n"
		  "  </connection>\n",
		  iname, unoc,
		  port->isDataProducer() ? "to" : "from",
		  port->isDataProducer() ? "in" : "out",
		  c.external ? m_appAssembly.m_implName : c.devInstance->cname(),
		  port->isDataProducer() ? "from" : "to",
		  port->name());
    std::string tc;
    OU::format(tc, "%s_ocdp%u", iname, unoc);
    emitTimeClient(assy, tc.c_str(), "wti");
  }
  // Connect the new unoc node to the unoc
  std::string prevInstance, prevPort;
  if (unoc == 0) {
    prevInstance = "pfconfig";
    prevPort = iname;
  } else {
    OU::format(prevInstance, "%s_unoc%u", iname, unoc - 1);
    prevPort = "down";
  }
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='%s'/>\n"
		"    <port instance='%s_unoc%u' name='up'/>\n"
		"  </connection>\n",
		prevInstance.c_str(), prevPort.c_str(), iname, unoc);
  unoc++;
  return NULL;
}

// Emit the connection XML for a container-level connection.
// The possibilities are:
// interconnect <-> external
// interconnect <-> device - the device can be in the config or specifically instanced in the container
// external <-> device - ditto
// There is currently no provision for same->same yet.
// interconnect <-> interconnect might be useful for bridging
// device <-> device might be useful for testing
// external <-> external?
const char *HdlContainer::
emitConnection(std::string &assy, UNocs &uNocs, size_t &index, const ContConnect &c) {
  const char *err;
  if (c.interconnect) {
    if ((err = emitUNocConnection(assy, uNocs, index, c)))
      return err;
  } else if (c.external && c.devInstance) {
    // We need to connect an external port to a port of a device instance.
    std::string devport;
    if (c.devInConfig)
      OU::format(devport, "%s_%s", c.devInstance->cname(), c.port->name());
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "  </connection>\n",
		  m_appAssembly.m_implName, c.external->name(),
		  c.devInConfig ? "pfconfig" : c.devInstance->cname(),
		  c.devInConfig ? devport.c_str() : c.port->name());
  } else
    return "unsupported container connection";
  return NULL;
}

const char *HdlContainer::
emitAttribute(const char *attr) {
  if (!strcasecmp(attr, "language"))
    printf(m_language == VHDL ? "VHDL" : "Verilog");
  else if (!strcasecmp(attr, "platform"))
    puts(m_config.platform().m_implName);
  else if (!strcasecmp(attr, "configuration"))
    puts(m_config.m_implName);
  else
    return OU::esprintf("Unknown container attribute: %s", attr);
  return NULL;
}

void HdlContainer::
emitXmlWorkers(FILE *f) {
  m_config.emitXmlWorkers(f);
  m_appAssembly.emitXmlWorkers(f);
  Worker::emitXmlWorkers(f);
}

void HdlContainer::
emitXmlInstances(FILE *f) {
  size_t index = 0;
  m_config.emitInstances(f, "p", index);
  m_appAssembly.emitInstances(f, "a", index);
  emitInstances(f, "c", index);
}
void HdlContainer::
emitXmlConnections(FILE *f) {
  m_config.emitInternalConnections(f, "p");
  m_appAssembly.emitInternalConnections(f, "a");
  // The only internal data connections in a container might be between
  // an adapter and a device worker or conceivably between two adapters.
  emitInternalConnections(f, "c");
  // What is left is data connections that go through the container to the app.
  // 1. pf config to container (e.g. to an adapter in the container).
  // 2. pf config to app (e.g. to a dev wkr in the pf config)
  // 3. container to app (e.g. a dev wkr or adapter in the container)
  for (ConnectionsIter cci = m_assembly->m_connections.begin();
       cci != m_assembly->m_connections.end(); cci++) {
    Connection &cc = **cci;
    if (cc.m_attachments.front()->m_instPort.m_port->isData()) {
      InstancePort *ap = NULL, *pp = NULL, *ip = NULL; // instance ports for the app, for pf, for internal
      for (AttachmentsIter ai = cc.m_attachments.begin(); ai != cc.m_attachments.end(); ai++) {
	Attachment &a = **ai;
	(!strcasecmp(a.m_instPort.m_port->m_worker->m_implName, m_appAssembly.m_implName) ? ap :
	 !strcasecmp(a.m_instPort.m_port->m_worker->m_implName, m_config.m_implName) ? pp : ip)
	  = &a.m_instPort;
      }
      assert(ap || pp || ip);
      if (!ap && !pp)
	continue; // internal connection already dealt with
      // find the corresponding instport inside each side.
      InstancePort
	*aap = ap ? m_appAssembly.m_assembly->findInstancePort(ap->m_port->name()) : NULL,
        *ppp = pp ? m_config.m_assembly->findInstancePort(pp->m_port->name()) : NULL,
	*producer = (aap && aap->m_port->isDataProducer() ? aap :
		     ppp && ppp->m_port->isDataProducer() ? ppp : ip),
	*consumer = (aap && !aap->m_port->isDataProducer() ? aap :
		     ppp && !ppp->m_port->isDataProducer() ? ppp : ip);
      // Application is producing to an external consumer
      fprintf(f, "<connection from=\"%s/%s\" out=\"%s\" to=\"%s/%s\" in=\"%s\"/>\n",
	      producer == ip ? "c" : producer == aap ? "a" : "p",
	      producer->m_instance->name, producer->m_port->name(),
	      consumer == ip ? "c" : consumer == aap ? "a" : "p",
	      consumer->m_instance->name, consumer->m_port->name());
    }
  }
}

// inContainer means the device is instanced in the container as opposed to
// in the platform configuration.  If in a platform configuration the device signals
// are actually signals of the platform configuration worker.
void HdlContainer::
mapDevSignals(std::string &assy, const DevInstance &di, bool inContainer) {
  const Signals &devsigs = di.device.deviceType().m_signals;
  for (SignalsIter s = devsigs.begin(); s != devsigs.end(); s++)
    for (unsigned n = 0; (*s)->m_width ? n < (*s)->m_width : n == 0; n++) {
      // (Re)create the signal name of the pf_config signal
      const char *boardName;
      bool isSingle;
      if ((boardName = di.device.m_dev2bd.findSignal(**s, n, isSingle))) {
#if 0
	if (*boardName) {
	  Signal *boardSig = di.device.m_board.m_extmap.findSignal(boardName);
	  assert(boardSig);
	  assert(di.device.m_board.m_bd2dev.findSignal(boardName));
	}
#endif
	std::string devSig = (*s)->cname();
	if ((*s)->m_width && isSingle)
	  OU::formatAdd(devSig, "(%u)", n);
	std::string dname, ename;
	if (di.slot && !inContainer)
	  OU::format(dname, "%s_%s_%s", di.slot->cname(), di.device.cname(), devSig.c_str());
	else
	  dname = devSig.c_str();
	if (*boardName && di.slot) {
	  Signal *slotSig = di.device.m_board.m_extmap.findSignal(boardName);
	  assert(slotSig);
	  Slot::SignalsIter ssi = di.slot->m_signals.find(slotSig);
	  OU::format(ename, "%s_%s", di.slot->cname(),
		     ssi == di.slot->m_signals.end()  ? slotSig->cname() : ssi->second.c_str());
	} else
	  ename = boardName;
	OU::formatAdd(assy, "    <signal name='%s' external='%s'/>\n",
		      dname.c_str(), ename.c_str());
      } else {
	Signal *ns = new Signal(**s);
	OU::format(ns->m_name, "%s_%s", di.device.cname(), ns->cname());
	m_signals.push_back(ns);
	m_sigmap[ns->m_name.c_str()] = ns;
	break;
      }
    }
}

const char *HdlContainer::
emitUuid(const OU::Uuid &uuid) {
  const char *err;
  FILE *f;
  if ((err = openOutput(m_implName, m_outDir, "", "_UUID", VER, NULL, f)))
    return err;
  const char *comment = hdlComment(Verilog);
  printgen(f, comment, m_file.c_str(), true);
  OCPI::HDL::HdlUUID uuidRegs;
  memcpy(uuidRegs.uuid, uuid, sizeof(uuidRegs.uuid));
  uuidRegs.birthday = (uint32_t)time(0);
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
    return "Could not close output file. No space?";
  return NULL;
}

const char *Worker::
emitContainerImplHDL(FILE *f) {
  const char *comment = hdlComment(m_language);
  fprintf(f,
	  "%s This file contains the implementation declarations for a container configuration %s\n"
	  "%s Interface definition signal names are defined with pattern rule: \"%s\"\n\n",
	  comment, m_implName, comment, m_pattern);
  fprintf(f,
	  "Library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;\n"
	  "Library ocpi; use ocpi.all, ocpi.types.all;\n"
          "use work.%s_defs.all, work.%s_constants.all;\n",
	  m_implName, m_implName);
  emitVhdlLibraries(f);
  fprintf(f,
	  "\nentity %s_rv is\n", m_implName);
  emitParameters(f, m_language);
  emitSignals(f, VHDL, true, true, false);
  fprintf(f, "end entity %s_rv;\n", m_implName);
  emitVhdlSignalWrapper(f, "ftop");
  return NULL;
}

void HdlContainer::
recordSignalConnection(Signal &s, const char *from) {
  // A signal may be connected more than once if it is an input
  //  assert(m_connectedSignals.find(&s) == m_connectedSignals.end());
  m_connectedSignals[&s] = from;
  //  m_connectedSignals.insert(&s);
}
void HdlContainer::
emitDeviceSignalMapping(FILE *f, std::string &last, Signal &s) {
  std::string name;
  if (s.m_direction == Signal::INOUT)
    fprintf(f, "%s      %s => %s", last.c_str(), s.m_name.c_str(), s.m_name.c_str());
  else
    Worker::emitDeviceSignalMapping(f, last, s);
}
void HdlContainer::
emitDeviceSignal(FILE *f, Language lang, std::string &last, Signal &s) {
  if (s.m_direction == Signal::INOUT) {
    // Inouts are different for containers since the tristate IOBUF is inserted.
    emitSignal(s.m_name.c_str(), f, lang, s.m_direction, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
  } else
    Worker::emitDeviceSignal(f, lang, last, s);
}

void HdlContainer::
emitTieoffSignals(FILE *f) {
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    ConnectedSignalsIter csi = m_connectedSignals.find(*si);
    if (csi == m_connectedSignals.end()) {
      // Assign device signal tieoffs for signals with no connections.
      std::string name;
      switch (s.m_direction) {
      case Signal::IN:
	fprintf(f, "  -- Input signal \"%s\" is unused and unconnected in this module\n",
		s.cname());
	break;
      case Signal::OUT:
	fprintf(f,
		"  -- Output signal \"%s\" is not connected to any instance.\n", s.cname());
	if (s.m_differential) {
	  OU::format(name, s.m_pos.c_str(), s.cname());
	  fprintf(f, "  %s => %s,\n", name.c_str(), s.m_width ? "(others => '0')" : "'0'");
	  OU::format(name, s.m_neg.c_str(), s.cname());
	  fprintf(f, "  %s => %s,\n", name.c_str(), s.m_width ? "(others => '0')" : "'0'");
	} else
	  fprintf(f, "  %s <= %s;\n", s.cname(), s.m_width ? "(others => '0')" : "'0'");
	break;
      case Signal::OUTIN:
	assert("Emulated INOUT signals must be connected"==0);
	break;
      case Signal::INOUT:
	assert(s.m_differential == false);
	fprintf(f,
		"  -- Inout signal \"%s\" is not connected to any instance.\n"
		"  %s_ts: util.util.TSINOUT_",
		s.cname(), s.cname());
	if (s.m_width)
	  fprintf(f,
		  "N\n"
		  "    generic map(width => %zu)\n"
		  "    port map(I => (others => '0')", s.m_width);
	else
	  fprintf(f,
		  "1\n"
		  "    port map(I => '0'");
	fprintf(f, ", IO => %s, O => open, OE => '0');\n", s.cname());
	break;
      case Signal::BIDIRECTIONAL: // not really supported properly
	break;
      default:
	assert("Unexpected signal type for assembly tieoff" == 0);
      }
    } else if (s.m_direction == Signal::INOUT) {
      std::string in, out, oe;
      OU::format(in, s.m_in.c_str(), csi->second.c_str());
      OU::format(out, s.m_out.c_str(), csi->second.c_str());
      OU::format(oe, s.m_oe.c_str(), csi->second.c_str());
      // Signal has a connection and is INOUT - generate the top level tristate
      fprintf(f,
	      "  -- Inout signal \"%s\" needs a tristate buffer.\n"
		"  %s_ts: util.util.TSINOUT_",
	      s.cname(), s.cname());
      if (s.m_width)
	fprintf(f, "N\n    generic map(width => %zu)\n", s.m_width);
      else
	fprintf(f, "1\n");
      fprintf(f,
	      "    port map(I => %s, IO => %s, O => %s, OE => %s);\n",
	      out.c_str(), s.cname(), in.c_str(), oe.c_str());
    }
  }
}
