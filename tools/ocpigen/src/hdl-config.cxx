#include <strings.h>
#include <stdint.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "assembly.h"
#include "hdl-config.h"
#include "hdl.h"

namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;

DevInstance::
DevInstance(const Device &d, const Card *c, const Slot *s, bool control,
	    const DevInstance *parent)
  : device(d), card(c), slot(s), control(control), parent(parent) {
  m_connected.resize(d.m_deviceType.m_ports.size(), 0);
  if (slot) {
    m_name = slot->name();
    m_name += "_";
    m_name += d.name();
  } else
    m_name = d.name();
}

const DevInstance *HdlHasDevInstances::
findDevInstance(const Device &dev, const Card *card, const Slot *slot,
		DevInstances *baseInstances, bool *inBase) {
  if (inBase)
    *inBase = false;
  if (baseInstances)
    for (DevInstancesIter dii = baseInstances->begin(); dii != baseInstances->end(); dii++) {
      const DevInstance &di = *dii;
      if (&di.device == &dev && di.slot == slot && di.card == card) {
	if (inBase)
	  *inBase = true;
	return &di;
      }
    }
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const DevInstance &di = *dii;
    if (&di.device == &dev && di.slot == slot && di.card == card)
      return &di;
  }
  return NULL;
}

const char *HdlHasDevInstances::
addDevInstance(const Device &dev, const Card *card, const Slot *slot,
	       bool control, const DevInstance *parent, DevInstances *baseInstances,
	       const DevInstance *&devInstance) {
  const char *err;
  m_devInstances.push_back(DevInstance(dev, card, slot, control, parent));
  assert(card && slot || !card && !slot);
  assert(!slot || !m_plugged[slot->m_ordinal] || card == m_plugged[slot->m_ordinal]);
  if (slot && !m_plugged[slot->m_ordinal])
    m_plugged[slot->m_ordinal] = card;
  devInstance = &m_devInstances.back();
  // See which (sub)devices on the same board support this added device, 
  // and make sure they are present.
  const Board &bd =
    card ? static_cast<const Board&>(*card) : static_cast<const Board&>(m_platform);
  for (DevicesIter bi = bd.m_devices.begin(); bi != bd.m_devices.end(); bi++)
    for (SupportsIter si = (*bi)->m_deviceType.m_supports.begin();
	 si != (*bi)->m_deviceType.m_supports.end(); si++)
      // FIXME: use the package name here...
      //      if (&(*si).m_type == &dev.m_deviceType && // the sdev supports this TYPE of device
      if (!strcasecmp((*si).m_type.m_implName, dev.m_deviceType.m_implName) &&
	  (*bi)->m_ordinal == dev.m_ordinal) { // the ordinals match. FIXME allow mapping
	const DevInstance *sdi = findDevInstance(**bi, card, slot, baseInstances, NULL);
	if (!sdi && (err = addDevInstance(**bi, card, slot, control, devInstance, NULL, sdi)))
	  return err;
      }
  return NULL;
}

const char *HdlHasDevInstances::
parseDevInstance(const char *device, ezxml_t x, const char *parentFile, Worker *parent,
		 bool control, DevInstances *baseInstances, const DevInstance **result,
		 bool *inBase) {
  const char *err;
  std::string s;
  const Slot *slot = NULL;
  if (OE::getOptionalString(x, s, "slot") &&
      !(slot = m_platform.findSlot(s.c_str(), err)))
    return err;
  const Card *card = NULL;
  if (OE::getOptionalString(x, s, "card") &&
      !(card = Card::get(s.c_str(), parentFile, parent, err)))
    return err;
  // Card and slots have been checked individually)
  if (slot) {
    const Card *plug = m_plugged[slot->m_ordinal];
    if (card) {
      if (plug && card != plug)
	return
	  OU::esprintf("Conflicting cards (\"%s\" vs. \"%s\") specified in slot \"%s\"",
		       plug->name(), card->name(), slot->name());
    } else if (plug)
      card = plug;
    else
      OU::esprintf("No card specified for slot \"%s\"", slot->name());
  } else if (card) {
    // Find a slot...
    switch (m_platform.slots().size()) {
    case 0:
      return OU::esprintf("Card \"%s\" specified when platform has no slots",
			  card->name());
    case 1:
      slot = m_platform.slots().begin()->second;
      if (slot->type() != card->type())
	return OU::esprintf("Card \"%s\" has slot type \"%s\", which is not in the platform",
			    card->name(), slot->type()->name());
      break;
    default:
      for (SlotsIter si = m_platform.slots().begin(); si != m_platform.slots().end(); si++)
	if ((*si).second->type() == card->type())
	  if (slot)
	    return OU::esprintf("Multiple slots are possible for card \"%s\"",
				card->name());
	  else
	    slot = (*si).second;
    }
  }
  const Device *dev;
  if (card) {
    if (!(dev = card->findDevice(device)))
      return OU::esprintf("There is no device named \"%s\" on \"%s\" cards",
			  device, card->name());
  } else if (!(dev = m_platform.findDevice(device)))
    return OU::esprintf("There is no device named \"%s\" on this platform",
			device);
      
  const DevInstance *di;
  assert(card && slot || !card && !slot);
  if (control && !dev->m_deviceType.m_canControl)
    return OU::esprintf("Device '%s' cannot have control since its type (%s) cannot",
			dev->name(), dev->deviceType().name());
  if ((di = findDevInstance(*dev, card, slot, baseInstances, inBase))) {
    if (result) {
      *result = di;
      return NULL;
    }
    // So a container is finding duplicate devices
    if (card || slot)
      return
	OU::esprintf("Device '%s' on card '%s' in slot '%s' is "
		     "already in the platform configuration",
		     di->device.name(), di->card->name(), di->slot->name());
    else
      return OU::esprintf("Platform device '%s' is already in the platform configuration",
			  di->device.name());
  }
  if ((err = addDevInstance(*dev, card, slot, control, NULL, baseInstances, di)))
    return err;
  if (result)
    *result = di;
  return NULL;
}

// Parse references to devices for instantiation
// Used for platform configurations and containers
// for EXPLICIT instantiation
const char *HdlHasDevInstances::
parseDevInstances(ezxml_t xml, const char *parentFile, Worker *parent,
		  DevInstances *baseInstances) {
  // Now we have a platform to work from.  Here we parse the extra info needed to
  // generate this platform configuration.
  const char *err = NULL;
  for (ezxml_t xd = ezxml_cchild(xml, "Device"); !err && xd; xd = ezxml_next(xd)) {
    std::string name;
    bool control = false;
    if ((err = OE::checkAttrs(xd, "name", "control", "slot", "card", (void*)0)) ||
	(err = OE::getBoolean(xd, "control", &control)) ||
	(err = OE::getRequiredString(xd, name, "name", "device")))
      return err;
#if 0
    // FIXME: Change the "control" to an attribute of the platform config which indicates
    // which device is performing control
    if (control && baseInstances)
      return "It is invalid to specify a 'control' attribute to a device in a container";
    if (!strcasecmp(m_platform.name(), name.c_str())) {
      m_platform.setControl(control); // FIXME make the platform a device...
      continue;
    }
#endif
    if ((err = parseDevInstance(name.c_str(), xd, parentFile, parent, control, baseInstances,
				NULL, NULL)))
      return err;
  }
  return NULL;
}

void HdlHasDevInstances::
emitSubdeviceConnections(std::string &assy,  DevInstances *baseInstances) {
  // Connect top down.  For any device that is supported, connect to the support modules
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const Device &d = (*dii).device;
    // Search for other instances that support this device instances
    for (DevInstancesIter sii = m_devInstances.begin(); sii != m_devInstances.end(); sii++) {
      bool inConfig;
      const Device &s = (*sii).device;
      const DevInstance *sdi = NULL;
      const Support *sup = NULL;
      if (&*sii != &*dii)
	for (SupportsIter si = s.m_deviceType.m_supports.begin();
	     si != s.m_deviceType.m_supports.end(); si++)
	  //	  if (&(*si).m_type == &d.m_deviceType && // the subdevice supports this TYPE of device
	  if (!strcasecmp((*si).m_type.m_implName, d.m_deviceType.m_implName) &&
	      s.m_ordinal == d.m_ordinal) {  // and the ordinals match. FIXME allow mapping
	    // Find whether it is in the platform config or not.
	    sdi = findDevInstance(s, (*dii).card, (*dii).slot, baseInstances, &inConfig);
	    assert(sdi);
	    sup = &*si;
	    break;
	  }
      if (!sup)
	continue; // this (sub)dev instance does not support this device;
      for (SupportConnectionsIter sci = sup->m_connections.begin();
	   sci != sup->m_connections.end(); sci++) {
	// A port connection
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "    <port instance='%s' name='%s%s%s'",
		      (*dii).name(), (*sci).m_port->cname(),
		      inConfig ? "pfconfig" : sdi->name(),
		      inConfig ? sdi->name() : "", inConfig ? "_" : "",
		      (*sci).m_sup_port->cname());
	if ((*sci).m_indexed) {
	  size_t
	    supOrdinal = (*sci).m_sup_port->m_ordinal,
	    supIndex = (*sci).m_index,
	    unconnected = 0,
	    index = supIndex;
	  // If we are in a container and the subdevice is in the config,
	  // we may need to index relative to what is NOT connected in the config,
	  // and thus externalized.
	  if (inConfig) {
	    for (size_t i = 0; i < (*sci).m_sup_port->count; i++)
	      if (sdi->m_connected[supOrdinal] & (1 << i)) {
		assert(i != supIndex);
		if (i < supIndex)
		  index--;
	      } else
		unconnected++; // count how many were unconnected in the config
	    assert(unconnected > 0 && index < unconnected);
	  } else // the subdevice is where the device is, so record the connection
	    sdi->m_connected[supOrdinal] |= 1 << supIndex;
	  OU::formatAdd(assy, " index='%zu'", index);
	}
	OU::formatAdd(assy,
		      "/>\n"
		      "  </connection>\n");
      }
    }
  }
}

HdlConfig *HdlConfig::
create(ezxml_t xml, const char *knownPlatform, const char *xfile, Worker *parent,
       const char *&err) {
  err = NULL;
  std::string myPlatform;
  OE::getOptionalString(xml, myPlatform, "platform");
  // Note that we generate the name of the platform file here to be findable
  // in the hdl/platforms directory since:
  // 1. The platform config might be remote from the platform.
  // 2. The platform config is parsed during container processing elsewhere.
  if (myPlatform.empty())
    if (knownPlatform)
      myPlatform = knownPlatform;
    else if (::platform)
      myPlatform = ::platform;
    else {
	err = "No platform specified in HdlConfig nor on command line";
	return NULL;
    }
      
#if 0
    else {
      const char *slash = xfile ? strrchr(xfile, '/') : NULL;
      if (slash) {
	std::string pfdir(xfile, slash - xfile);
	const char *sl2 = strrchr(pfdir.c_str(), '/');
	if (sl2)
	  if (!strcmp(sl2 + 1, "gen")) {
	    pfdir.resize(sl2 - pfdir.c_str());
	    sl2 = strrchr(pfdir.c_str(), '/');
	    myPlatform = sl2 ? sl2 + 1 : pfdir.c_str();
	    myPlatform += "/";
	    myPlatform += sl2 ? sl2 + 1 : pfdir.c_str();
	  } else
	    myPlatform = sl2 + 1;
      } else {
	err = "No platform specified in HdlConfig nor on command line";
	return NULL;
      }
    }
#endif
  std::string pfile;
  ezxml_t pxml;
  HdlPlatform *pf;
  // 
  if ((err = parseFile(myPlatform.c_str(), xfile, "HdlPlatform", &pxml, pfile)) ||
      !(pf = HdlPlatform::create(pxml, pfile.c_str(), NULL, err)))
    return NULL;
  HdlConfig *p = new HdlConfig(*pf, xml, xfile, parent, err);
  if (err) {
    delete p;
    p = NULL;
  }
  return p;
}

HdlConfig::
HdlConfig(HdlPlatform &pf, ezxml_t xml, const char *xfile, Worker *parent, const char *&err)
  : Worker(xml, xfile, "", Worker::Configuration, parent, NULL, err),
    HdlHasDevInstances(pf, m_plugged),
    m_platform(pf) {
  if (err ||
      (err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONFIG_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, HDL_CONFIG_ELEMS, (void*)0)))
    return;
  pf.setParent(this);
  //hdlAssy = true;
  m_plugged.resize(pf.m_slots.size());
  ezxml_t tx = ezxml_add_child(xml, "device", 0);
  ezxml_set_attr(tx, "name", "time_server");
  if ((err = parseDevInstances(xml, xfile, this, NULL)))
    return;
  std::string assy;
  OU::format(assy, "<HdlPlatformAssembly name='%s'>\n", m_name.c_str());
  // Add the platform instance
  // We make the worker name platform/platform so it is findable from the platforms
  // directory.
  OU::formatAdd(assy, "  <instance worker='%s'/>\n", // index='%zu'/>\n",
		m_platform.m_name.c_str()); //, index++);
  // Add all the device instances
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const ::Device &d = (*dii).device;
    const DeviceType &dt = d.m_deviceType;
    OU::formatAdd(assy, "  <instance worker='%s' name='%s'%s>\n",
		  d.m_deviceType.name(), (*dii).name(), dt.m_instancePVs ? "" : "/");
    if (dt.m_instancePVs) {
      OU::Assembly::Property *ap = &(*dt.m_instancePVs)[0];
      for (size_t n = dt.m_instancePVs->size(); n; n--, ap++)
	OU::formatAdd(assy, "    <property name='%s' value='%s'/>\n",
		      ap->m_name.c_str(), ap->m_value.c_str());
      OU::formatAdd(assy, "  </instance>\n");
    }
    // Add a time client instance as needed by device instances
    for (PortsIter pi = d.m_deviceType.ports().begin();
	 pi != d.m_deviceType.ports().end(); pi++)
      if ((*pi)->type == WTIPort)
	OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
		      (*dii).name());
  }
  // Internal connections:
  // 1. Control plane master to OCCP
  // 2. WCI connections to platform and device workers
  // 3. To and from time clients
  // 4. Between devices and required subdevices

  // So: 1. Add the internal connection from the platform and/or device workers to occp
  if ((err = addControlConnection(assy)))
    return;
  // 2. Connect the time service to the platform worker
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='timebase'/>\n"
		"    <port instance='time_server' name='timebase'/>\n"
		"  </connection>\n",
		m_platform.m_name.c_str());
  // 3. To and from time clients
  unsigned tIndex = 0;
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const ::Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().ports().begin();
	 pi != d.deviceType().ports().end(); pi++)
      if ((*pi)->type == WTIPort) {
	// connection from platform worker's time service to the client
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='time_server' name='time'/>\n"
		      "    <port instance='time_client%u' name='time'/>\n"
		      "  </connection>\n",
		      tIndex);
	// connection from the time client to the device worker
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='time_client%u' name='wti'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      tIndex++, (*dii).name(), (*pi)->cname());
      }
  }
  // 4. To and from subdevices
  emitSubdeviceConnections(assy, NULL);
  // End of internal connections.
  // Start of external connections (not signals)
  //  1. WCI master
  //  2. Time service
  //  3. Metadata
  //  4. Any data ports from device worker
  //  5. Any unocs from device workers
  OU::formatAdd(assy,
		"  <external instance='time_server' port='time'/>\n"
		"  <external instance='%s' port='metadata'/>\n",
		m_platform.m_name.c_str());
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const ::Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().ports().begin(); pi != d.deviceType().ports().end(); pi++) {
      Port &p = **pi;
      if (p.isData() || p.type == NOCPort ||
	  (!p.master && (p.type == PropPort || p.type == DevSigPort))) {
	size_t unconnected = 0, first = 0;
	for (size_t i = 0; i < p.count; i++)
	  if (!((*dii).m_connected[p.m_ordinal] & (1 << i))) {
	    if (!unconnected++)
	      first = i;
	  }
	// FIXME: (hard) this will not work if the connectivity is not simply contiguous.
	// (at one end or the other).
	if (unconnected)
	  OU::formatAdd(assy,
			"  <external name='%s_%s' instance='%s' port='%s' "
			"index='%zu' count='%zu'/>\n",
			(*dii).name(), p.cname(),
			(*dii).name(), p.cname(),
			first, unconnected);
      }
    }
  }
  for (PortsIter pi = m_platform.m_ports.begin(); pi != m_platform.m_ports.end(); pi++) {
      Port &p = **pi;
      if (p.type == NOCPort) {
	// Port names of noc ports are interconnect names on the platform
	OU::formatAdd(assy,
		      "  <external name='%s' instance='%s' port='%s'/>\n",
		      p.cname(), m_platform.m_name.c_str(), p.cname());
      }
  }
  
  OU::formatAdd(assy, "</HdlPlatformAssembly>\n");
  // The assembly will automatically inherit all the signals, prefixed by instance.
  //  if (!attribute)
    ocpiInfo("=======Begin generated platform configuration assembly=======\n"
	     "%s"
	     "=======End generated platform configuration assembly=======\n",
	     assy.c_str());
  // Now we hack the (inherited) worker to have the xml for the assembly we just generated.
  char *copy = strdup(assy.c_str());
  ezxml_t x;
  if ((err = OE::ezxml_parse_str(copy, strlen(copy), x)))
    err = OU::esprintf("XML Parsing error on generated platform configuration: %s", err);
  else {
    m_xml = x;
    err = parseHdl();
  }
  if (err)
    return;
  // Externalize all the device signals.
  unsigned n = 0;
  for (Instance *i = m_assembly->m_instances; n < m_assembly->m_nInstances; i++, n++) {
    for (SignalsIter si = i->worker->m_signals.begin(); si != i->worker->m_signals.end(); si++) {
      Signal *s = new Signal(**si);
      if (i->worker->m_type != Worker::Platform)
	OU::format(s->m_name, "%s_%s", i->name, (**si).m_name.c_str());
      m_signals.push_back(s);
      m_sigmap[s->name()] = s;
      ocpiDebug("Externalizing device signal '%s' for device '%s'", s->name(), i->worker->m_implName);
    }
  }
}

HdlConfig::
~HdlConfig() {
  delete &m_platform;
}


// Add the internal control connection, either on the platform worker or on a device worker.
// if there is only one possible control port, use it.
// if more than one, then the "control" attribute will be used to identify the
// control ports, which will be multiplexed.
const char *HdlConfig::
addControlConnection(std::string &assy) {
  const char *cpInstanceName = NULL, *cpPortName = NULL;
  unsigned nCpPorts = 0;
  bool multiple = false;
  for (PortsIter pi = m_platform.m_ports.begin(); pi != m_platform.m_ports.end(); pi++)
    if ((*pi)->type == CPPort) {
      cpInstanceName = m_platform.m_name.c_str();
      cpPortName = (*pi)->cname();
      if (m_platform.m_control)
	nCpPorts++;
      break;
    }
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const ::Device &d = (*dii).device;
    for (PortsIter pi = d.m_deviceType.ports().begin(); pi != d.m_deviceType.ports().end(); pi++)
      if ((*pi)->type == CPPort) {
	if (cpInstanceName)
	  multiple = true;
	else {
	  cpInstanceName = d.m_name.c_str();
	  cpPortName = (*pi)->cname();
	}
	if ((*dii).control)
	  nCpPorts++;
      }  
  }
  if (multiple)
    if (nCpPorts  > 1)
      return "Multiple control ports are not yet supported";
    else
      return "No control-capable port designated among the multiple possibilities";
  else if (!cpInstanceName)
    return "No feasible control port was found";
  // Connect the control master port of the platform or device/interconnect worker to
  // the control plane worker
  OU::formatAdd(assy,
		"  <external instance='%s' port='%s'/>\n",
		cpInstanceName, cpPortName);
  return NULL;
}

const char *Worker::
emitConfigImplHDL(FILE *f) {
  const char *comment = hdlComment(m_language);
  fprintf(f,
	  "%s This file contains the implementation declarations for platform configuration %s\n"
	  "%s Interface definition signal names are defined with pattern rule: \"%s\"\n\n",
	  comment, m_implName, comment, m_pattern);
  fprintf(f,
	  "Library IEEE; use IEEE.std_logic_1164.all;\n"
	  "Library ocpi; use ocpi.all, ocpi.types.all;\n"
          "use work.%s_defs.all, work.%s_constants.all;\n",
	  m_implName, m_implName);
  emitVhdlLibraries(f);
  fprintf(f,
	  "\nentity %s_rv is\n", m_implName);
  emitParameters(f, m_language);
  emitSignals(f, VHDL, true, true, false);
  fprintf(f, "end entity %s_rv;\n", m_implName);
  return NULL;
}

