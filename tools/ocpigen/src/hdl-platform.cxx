#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <map>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "hdl-platform.h"
#include "hdl-assembly.h"

namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;

bool hdlAssy = false;
DeviceType *DeviceType::
create(const char *name, const char *parent, DeviceTypes &types, const char *&err) {
  DeviceType *dt = NULL;
  for (DeviceTypesIter dti = types.begin(); !dt && dti != types.end(); dti++)
    if (!strcasecmp(name, (*dti)->m_name.c_str()))
      dt = *dti;
  if (!dt) {
    // New device type, which must be a file.
    ezxml_t xml;
    const char *xfile;
    if (!(err = parseFile(name, parent, NULL, &xml, &xfile))) {
      dt = new DeviceType(xml, xfile, parent, err);
      if (err) {
	delete dt;
	dt = NULL;
      } else
	types.push_back(dt);
    }
  }
  return dt;
}

Device *Device::
create(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single,
       const char *&err) {
  Device *d = new Device(xml, parent, deviceTypes, single, err);
  if (err) {
    delete d;
    d = NULL;
  }
  return d;
}

// A device is not in its own file (yet).
Device::
Device(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single, const char *&err) {
  err = NULL;
  std::string wname;
  if ((err = OE::getRequiredString(xml, wname, "worker")) ||
      !(m_deviceType = DeviceType::create(wname.c_str(), parent, deviceTypes, err)))
    return;
  const char *cp = strchr(wname.c_str(), '.');
  if (cp)
    wname.resize(cp - wname.c_str());
  if (single)
    m_name = wname;
  else {
    wname += "%u";
    OE::getNameWithDefault(xml, m_name, wname.c_str(), m_deviceType->m_count);
  }
}

DeviceType::
DeviceType(ezxml_t xml, const char *file, const char *parent, const char *&err)
  : Worker(xml, file, parent, err), m_count(0) {
  if (err ||
      (err = OE::checkTag(xml, "HdlDevice", "Expected 'HdlDevice' as tag in '%s'", file)) ||
      (err = OE::checkAttrs(xml, PARSED_ATTRS, HDL_TOP_ATTRS, HDL_IMPL_ATTRS,
			    "interconnect", "control", (void*)0)) ||
      (err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, (void*)0)) ||
      (err = OE::getBoolean(m_xml, "interconnect", &m_interconnect)) ||
      (err = OE::getBoolean(m_xml, "control", &m_canControl)))
    return;
  err = parseHdlImpl();
}

HdlPlatform *HdlPlatform::
create(ezxml_t xml, const char *xfile, const char *&err) {
  err = NULL;
  HdlPlatform *p = new HdlPlatform(xml, xfile, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}

HdlPlatform::
HdlPlatform(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, NULL, err), m_control(false) {
  if (err ||
      (err = OE::checkAttrs(xml, IMPL_ATTRS, GENERIC_IMPL_CONTROL_ATTRS, HDL_TOP_ATTRS,
			    HDL_IMPL_ATTRS, HDL_PLATFORM_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, HDL_PLATFORM_ELEMS, (void*)0)) ||
      (err = parseHdl("ocpi")) ||
      (err = OE::getBoolean(xml, "control", &m_control)))
    return;

  // These devices are declaring that they are part of the platform.
  for (ezxml_t xs = ezxml_cchild(xml, "Device"); xs; xs = ezxml_next(xs)) {
    const char *worker = ezxml_cattr(xs, "worker");
    bool single = true;
    for (ezxml_t x = ezxml_cchild(xml, "Device"); x; x = ezxml_next(x)) {
      const char *w = ezxml_cattr(x, "worker");
      if (x != xs && worker && w && !strcasecmp(worker, w))
	single = false;
    }
    Device *dev = Device::create(xs, xml->name, m_deviceTypes, single, err);
    if (!dev)
      return;
    m_devices.push_back(dev);
  }
  for (ezxml_t xs = ezxml_cchild(xml, "slot"); xs; xs = ezxml_next(xs)) {
    if (!Slot::create(xs, OE::ezxml_tag(xml), m_slotTypes, m_slots, err))
      return;
  }
}

HdlPlatform::
~HdlPlatform() {
  while (m_devices.size()) {
    Device *d = m_devices.front();
    m_devices.pop_front();
    delete d;
  }
}

HdlConfig *HdlConfig::
create(ezxml_t xml, const char *xfile, size_t &index, const char *&err) {
  err = NULL;
  HdlConfig *p = new HdlConfig(xml, xfile, index, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}

#if 0
static Worker *
createControlPlaneWorker(const char *parent, const char *&err) {
  err = NULL;
  Worker *w = new Worker(NULL, "occp", parent, err);
  if (err) {
    delete w;
    return NULL;
  }
  w->m_noControl = true;
  w->m_ports.push_back(new Port("cpmaster", w, false, CPPort));
  w->m_ports.push_back(new Port("wci", w, false, WCIPort, 15, true));
  return w;
}
#endif

// Add the internal control connection, either on the platform worker or on a device worker.
// if there is only one possible control port, use it.
// if more than one, then the "control" attribute will be used to identify the
// control ports, which will be multiplexed.
const char *HdlConfig::
addControlConnection(std::string &assy) {
  const char *cpInstanceName = NULL, *cpPortName = NULL;
  unsigned nCpPorts = 0;
  bool multiple = false;
  for (PortsIter pi = m_platform->m_ports.begin(); pi != m_platform->m_ports.end(); pi++)
    if ((*pi)->type == CPPort) {
      cpInstanceName = m_platform->m_name.c_str();
      cpPortName = (*pi)->name;
      if (m_platform->m_control)
	nCpPorts++;
      break;
    }
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    Device &d = (*dii).device;
    for (PortsIter pi = d.m_deviceType->m_ports.begin(); pi != d.m_deviceType->m_ports.end(); pi++)
      if ((*pi)->type == CPPort) {
	if (cpInstanceName)
	  multiple = true;
	else {
	  cpInstanceName = d.m_name.c_str();
	  cpPortName = (*pi)->name;
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
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port Instance='occp' name='cp'/>\n"
		"    <port Instance='%s' name='%s'/>\n"
		"  </connection>\n",
		cpInstanceName, cpPortName);
  return NULL;
}

static const DevInstance *
findDeviceInstance(const char *device, std::string &card, std::string &slot, DevInstances &devInstances) {
  for (DevInstancesIter dii = devInstances.begin(); dii != devInstances.end(); dii++) {
    const DevInstance &di = *dii;
    Device &d = (*dii).device;
    if (!strcasecmp(device, d.name().c_str()) &&
	(!di.card && card.empty() || !strcasecmp(card.c_str(), di.card->name().c_str())) &&
	(!di.slot && slot.empty() || !strcasecmp(slot.c_str(), di.slot->name().c_str())))
      return &di;
  }
  return NULL;
}

static const char *
addDevInstance(const char *name, const char *parent, std::string &card, std::string &slot, bool control,
	       HdlPlatform &platform, Cards &cards,
	       DevInstances &devInstances, const DevInstance *&devInstance) {
  Devices &pDevices = platform.devices();
  Device *dev = NULL;
  for (DevicesIter di = pDevices.begin(); di != pDevices.end(); di++)
    if (!strcasecmp(name, (*di)->m_name.c_str())) {
      dev = *di;
      break;
    }
  Card *c = NULL;
  Slot *s = NULL;
  if (dev) {
    if (!card.empty() || !slot.empty())
      return OU::esprintf("The device named '%s' is found in platform '%s', can't be a card in a slot",
			 name, platform.name().c_str());
  } else {
    if (card.empty())
      return OU::esprintf("No device named '%s' found in platform '%s'",
			  name, platform.name().c_str());
    if (platform.slots().empty())
      return OU::esprintf("A card (%s) is specified, but platform has no slots", card.c_str());
    // We need to retrieve the card object (could be for the first time here).
    const char *err;
    if (!(c = Card::create(card.c_str(), parent, platform.slotTypes(), cards, err)))
      return err;
    if (slot.empty()) {
      for (SlotsIter si = platform.slots().begin(); si != platform.slots().end(); si++)
	if (&c->m_type == &si->second->m_type) {
	  if (s != NULL)
	    return  OU::esprintf("A card (%s) is specified with no slot, "
				 "but platform multiple '%s' slots",
				 card.c_str(), c->m_type.m_name.c_str());
	  s = si->second;
	}
      if (!s)
	return OU::esprintf("For card '%s', no slot of type '%s' was found",
			    card.c_str(), c->m_type.m_name.c_str());				
    } else {
      SlotsIter si = platform.slots().find(slot.c_str());
      if (si == platform.slots().end())
	return OU::esprintf("Platform has no slot '%s' for card '%s'",
			    slot.c_str(), card.c_str());
      s = si->second;
    }
    for (DevicesIter di = c->devices().begin(); di != c->devices().end(); di++)
      if (!strcasecmp(name, (*di)->m_name.c_str())) {
	dev = *di;
	break;
      }
    if (!dev)
      return OU::esprintf("There is no device '%s' on '%s' cards",
			  name, c->name().c_str());
  }
  if (control && !dev->m_deviceType->m_canControl)
    return OU::esprintf("Device '%s' cannot have control since its type (%s) cannot",
			dev->name().c_str(), dev->deviceType().name().c_str());
  devInstances.push_back(DevInstance(*dev, c, s, control));
  devInstance = &devInstances.back();
  return NULL;
}

// Parse references to devices for instantiation
// Used for platform configurations and containers
// for EXPLICIT instantiation
static const char *
parseDevInstances(ezxml_t xml, HdlPlatform &platform, DevInstances *baseInstances, DevInstances &devInstances,
		     Cards &cards, bool container) {
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
    if (control && container)
      return "It is invalid to specify a 'control' attribute to a device in a container";
    std::string slot, card;
    OE::getOptionalString(xd, slot, "slot");
    OE::getOptionalString(xd, card, "card");
    const DevInstance *di;
    if (baseInstances && (di = findDeviceInstance(name.c_str(), card, slot, *baseInstances)))
      if (di->card)
	return OU::esprintf("Device '%s' on card '%s' in slot '%s' is already in the platform configuration",
			    di->device.name().c_str(), di->card->name().c_str(), di->slot->name().c_str());
      else
	return OU::esprintf("Platform Device '%s' is already in the platform configuration",
			    di->device.name().c_str());
    if ((di = findDeviceInstance(name.c_str(), card, slot, devInstances)))
      if (di->card)
	return OU::esprintf("Device '%s' on card '%s' in slot '%s' is already instantiated",
			    di->device.name().c_str(), di->card->name().c_str(), di->slot->name().c_str());
      else
	return OU::esprintf("Platform Device '%s' is already instantiated",
			    di->device.name().c_str());
    if (!strcasecmp(platform.name().c_str(), name.c_str())) {
      platform.setControl(control); // FIXME make the platform a device...
      continue;
    }
    if ((err = addDevInstance(name.c_str(), OE::ezxml_tag(xd), card, slot, control, platform,
			      cards, devInstances, di)))
      return err;
  }
  return NULL;
}

HdlConfig::
HdlConfig(ezxml_t xml, const char *xfile, size_t &index, const char *&err)
  : Worker(xml, xfile, NULL, err), m_platform(NULL) {
  if (err ||
      (err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONFIG_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, HDL_CONFIG_ELEMS, (void*)0)))
    return;
  hdlAssy = true;
  // Set the fixed elements that would normally be parsed
  m_noControl = true;
  std::string myPlatform;
  OE::getOptionalString(xml, myPlatform, "platform");
  if (myPlatform.empty()) {
    if (!::platform) {
      // how about using the dir?
      const char *slash = xfile ? strrchr(xfile, '/') : NULL;
      if (slash) {
	std::string pfdir(xfile, slash - xfile);
	const char *sl2 = strrchr(pfdir.c_str(), '/');
	if (sl2)
	  myPlatform = sl2 + 1;
	else
	  myPlatform = pfdir.c_str();
      } else {
	err = "No platform specified in HdlConfig nor on command line";
	return;
      }
    } else
      myPlatform = ::platform;
  }
  const char *pfile;
  ezxml_t pxml;
  if ((err = parseFile(myPlatform.c_str(), xfile, "HdlPlatform", &pxml, &pfile)) ||
      !(m_platform = HdlPlatform::create(pxml, pfile, err)) ||
      (err = parseDevInstances(xml, *m_platform, NULL, m_devInstances, m_cards, false)))
    return;
  std::string assy;
  OU::format(assy, "<HdlPlatformAssembly name='%s'>\n", m_name.c_str());
  // Add the platform instance
  OU::formatAdd(assy, "  <instance worker='%s' index='%zu'/>\n", m_platform->m_name.c_str(), index++);
  // Add the control plane instance
  OU::formatAdd(assy, "  <instance worker='occp'/>\n");
  // Add all the device instances
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    Device &d = (*dii).device;
    OU::formatAdd(assy, "  <instance worker='%s' name='%s' index='%zu'/>\n",
		  d.m_deviceType->m_implName, d.m_name.c_str(), index++);
    // Add a time client instance as needed by device instances
    for (PortsIter pi = d.m_deviceType->m_ports.begin(); pi != d.m_deviceType->m_ports.end(); pi++)
      if ((*pi)->type == WTIPort)
	OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
		      d.name().c_str());
  }
  // Internal connections:
  // 1. Control plane master to OCCP
  // 2. WCI connections to platform and device workers
  // 3. To and from time clients

  // So: 1. Add the internal connection from the platform and/or device workers to occp
  if ((err = addControlConnection(assy)))
    return;
  // 2. WCI connections to platform and device workers
  size_t nWCI = 0;
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='%s'/>\n"
		"    <port instance='occp' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		m_platform->m_name.c_str(), m_platform->m_ports[0]->name, nWCI++);
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    Device &d = (*dii).device;
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <port instance='occp' name='wci' index='%zu'/>\n"
		  "  </connection>\n",
		  d.name().c_str(), d.deviceType().m_ports[0]->name,
		  nWCI++);
  }
  // 3. To and from time clients
  unsigned tIndex = 0;
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().m_ports.begin(); pi != d.deviceType().m_ports.end(); pi++)
      if ((*pi)->type == WTIPort) {
	// connection from platform worker's time service to the client
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s' name='time'/>\n"
		      "    <port instance='time_client%u' name='time'/>\n"
		      "  </connection>\n",
		      m_platform->m_name.c_str(), tIndex);
	// connection from the time client to the device worker
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='time_client%u' name='wti'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      tIndex++, d.name().c_str(), (*pi)->name);
      }
  }
  // End of internal connections.
  // Start of external connections (not signals)
  //  1. WCI master
  //  2. Time service
  //  3. Metadata
  //  4. Any data ports from device worker
  //  5. Any unocs from device workers
  OU::formatAdd(assy,
		"  <external instance='occp' port='wci' index='%zu' count='%zu'/>\n"
		"  <external instance='%s' port='time'/>\n"
		"  <external instance='%s' port='metadata'/>\n",
		nWCI, 15 - nWCI,
		m_platform->m_name.c_str(),
		m_platform->m_name.c_str());
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().m_ports.begin(); pi != d.deviceType().m_ports.end(); pi++) {
      Port &p = **pi;
      if (p.isData || p.type == NOCPort)
	OU::formatAdd(assy,
		      "  <external name='%s_%s' instance='%s' port='%s'/>\n",
		      d.name().c_str(), p.name,
		      d.name().c_str(), p.name);
    }		
  }
  for (PortsIter pi = m_platform->m_ports.begin(); pi != m_platform->m_ports.end(); pi++) {
      Port &p = **pi;
      if (p.type == NOCPort) {
	// Port names of noc ports are interconnect names on the platform
	OU::formatAdd(assy,
		      "  <external name='%s' instance='%s' port='%s'/>\n",
		      p.name, m_platform->m_name.c_str(), p.name);
      }
  }
  
  OU::formatAdd(assy, "</HdlPlatformAssembly>\n");
  // The assembly will automatically inherit all the signals, prefixed by instance.
  if (!attribute)
    ocpiInfo("=======Begin generated platform configuration assembly=======\n"
	     "%s"
	     "=======End generated platform configuration assembly=======\n",
	     assy.c_str());
  // Now we hack the (inherited) worker to have the xml for the assembly we just generated.
  char *copy = strdup(assy.c_str());
  ezxml_t x = ezxml_parse_str(copy, strlen(copy));
  if (x && ezxml_error(x)[0]) {
      err = OU::esprintf("XML Parsing error on generated platform configuration: %s", ezxml_error(x));
      return;
  }
  m_xml = x;
  err = parseHdl(NULL);
}  

HdlConfig::
~HdlConfig() {
  delete m_platform;
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
          "use work.%s_defs.all;\n",
	  m_implName);
  emitVhdlLibraries(f);
  fprintf(f,
	  "\nentity %s_rv is\n", m_implName);
  emitParameters(f, m_language);
  emitSignals(f, VHDL, false, true, true);
  fprintf(f, "end entity %s_rv;\n", m_implName);
  return NULL;
}

HdlContainer *HdlContainer::
create(ezxml_t xml, const char *xfile, const char *&err) {
  err = NULL;
  HdlContainer *p = new HdlContainer(xml, xfile, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}

// Establish and parse connection - THIS MAY IMPLICIT DEVICE INSTANCES
const char *HdlContainer::
parseConnection(ezxml_t cx, ContConnect &c) {
  const char *err;
  if ((err = OE::checkAttrs(cx, "external", "instance", "device", "interconnect", "port",
			    "otherdevice", "otherport", NULL)))
    return err;
  const char *attr;
  c.external = NULL;
  if ((attr = ezxml_cattr(cx, "external"))) {
    // Instance time clients for the assembly
    for (PortsIter pi = m_appAssembly->m_ports.begin(); pi != m_appAssembly->m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name, attr)) {
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
  if ((attr = ezxml_cattr(cx, "device"))) {
    std::string card, slot;
    OE::getOptionalString(cx, card, "card");
    OE::getOptionalString(cx, slot, "slot");
    c.devInstance = findDeviceInstance(attr, card, slot, m_config->devInstances());
    if (c.devInstance)
      c.devInConfig = true;
    else if (!(c.devInstance = findDeviceInstance(attr, card, slot, m_devInstances)) &&
	     (err = addDevInstance(attr, OE::ezxml_tag(cx), card, slot, false, m_config->platform(),
				   m_cards, m_devInstances, c.devInstance)))
      return err;
    
  }
  if ((attr = ezxml_cattr(cx, "port"))) {
    if (!c.devInstance)
      return OU::esprintf("Port '%s' specified without specifying a device", attr);
    Device &d = c.devInstance->device;
    for (PortsIter pi = d.deviceType().m_ports.begin(); pi != d.deviceType().m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name, attr)) {
	c.port = *pi;
	break;
      }
    if (!c.port)
      return OU::esprintf("Port '%s' not found for device '%s'", attr, d.name().c_str());
    if (!c.port->isData)
      return OU::esprintf("Port '%s' for device '%s' is not a data port", attr, d.name().c_str());
  } else if (c.devInstance) {
    Device &d = c.devInstance->device;
    // Find the one data port
    for (PortsIter pi = d.deviceType().m_ports.begin(); pi != d.deviceType().m_ports.end(); pi++)
      if ((*pi)->isData) {
	if (c.port)
	  return OU::esprintf("There are multiple data ports for device '%s'; you must specify one",
			      d.name().c_str());
	c.port = *pi;
      }
    if (!c.port)
      return OU::esprintf("There are no data ports for device '%s'", d.name().c_str());
  }
  if ((attr = ezxml_cattr(cx, "interconnect"))) {
    // An interconnect can be on any device worker, but for now it is on the config.
    for (PortsIter pi = m_config->m_ports.begin(); pi != m_config->m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name, attr)) {
	c.interconnect = *pi;
	break;
      }
    if (!c.interconnect ||
	c.interconnect->type != NOCPort || !c.interconnect->master)
      return OU::esprintf("Interconnect '%s' not found for platform '%s'", attr,
			   m_config->platform().m_name.c_str());
  }
  return NULL;
}

static void
emitTimeClient(std::string &assy, const char *config, const char *instance, const char *port) {
  OU::formatAdd(assy,
		"  <instance worker='time_client' name='%s_%s_time_client'/>\n"
		"  <connection>\n"
		"    <port instance='%s_%s_time_client' name='wti'/>\n"
		"    <port instance='%s' name='%s'/>\n"
		"  </connection>\n"
		"  <connection>\n"
		"    <port instance='%s' name='time'/>\n"
		"    <port instance='%s_%s_time_client' name='time'/>\n"
		"  </connection>\n",
		instance, port,
		instance, port,
		instance, port,
		config,
		instance, port);
}

// Make a connection to an interconnect
const char *HdlContainer::
emitUNocConnection(std::string &assy, UNocs &uNocs, size_t &index, size_t baseIndex, const ContConnect &c) {
    // Find uNoc
   unsigned &unoc = uNocs.at(c.interconnect->name);
   Port *port = c.external ? c.external : c.port;
   if (port->type != WSIPort || c.interconnect->type != NOCPort || !c.interconnect->master)
     return OU::esprintf("unsupported container connection between port %s of %s%s and interconnect %s",
			 port->name, c.interconnect->name, c.external ? "assembly" : "device",
			 c.external ? "" : c.devInstance->device.name().c_str());
  // Create the three instances:
  // 1. A unoc node to use the interconnect unoc
  // 2. A DP/DMA module to stream to/from another place on the interconnect
  // 3. An SMA to adapt the WMI on the DP to the WSI that is needed (for now).
  OU::formatAdd(assy,
		"  <instance name='%s_unoc%u' worker='unoc_node'>\n"
		"    <property name='control' value='false'/>\n"
		"    <property name='position' value='%u'/>\n"
		"  </instance>\n",
		c.interconnect->name, unoc, unoc);
  // instantiate dp, and connect its wci
  OU::formatAdd(assy,
		"  <instance name='%s_ocdp%u' worker='ocdp' index='%zu' interconnect='%s' configure='%u'/>\n"
		"  <connection>\n"
		"    <port instance='%s_ocdp%u' name='ctl'/>\n"
		"    <port instance='%s' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		c.interconnect->name, unoc, index, c.interconnect->name, unoc,
		c.interconnect->name, unoc,
		m_config->m_implName, index - baseIndex);
  index++;
  // instantiate sma, and connect its wci
  OU::formatAdd(assy,
		"  <instance name='%s_sma%u' worker='sma' index='%zu' adapter='%s' configure='%u'/>\n"
		"  <connection>\n"
		"    <port instance='%s_sma%u' name='ctl'/>\n"
		"    <port instance='%s' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		c.interconnect->name, unoc, index, c.interconnect->name, 
		port->u.wdi.isProducer ? 2 : 1,
		c.interconnect->name, unoc,
		m_config->m_implName, index - baseIndex);
  index++;
  // Connect the new unoc node to the unoc
  std::string prevInstance, prevPort;
  if (unoc == 0) {
    prevInstance = m_config->m_implName;
    prevPort = c.interconnect->name;
  } else {
    OU::format(prevInstance, "%s_unoc%u", c.interconnect->name, unoc - 1);
    prevPort = "down";
  }
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='%s'/>\n"
		"    <port instance='%s_unoc%u' name='up'/>\n"
		"  </connection>\n",
		prevInstance.c_str(), prevPort.c_str(), c.interconnect->name, unoc);
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s_unoc%u' name='client'/>\n"
		"    <port instance='%s_ocdp%u' name='client'/>\n"
		"  </connection>\n",
		c.interconnect->name, unoc, c.interconnect->name, unoc);
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s_ocdp%u' %s='data'/>\n"
		"    <port instance='%s_sma%u' %s='message'/>\n"
		"  </connection>\n",
		c.interconnect->name, unoc, port->u.wdi.isProducer ? "to" : "from",
		c.interconnect->name, unoc, port->u.wdi.isProducer ? "from" : "to");
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s_sma%u' %s='%s'/>\n"
		"    <port instance='%s' %s='%s'/>\n"
		"  </connection>\n",
		c.interconnect->name, unoc,
		port->u.wdi.isProducer ? "to" : "from",
		port->u.wdi.isProducer ? "in" : "out",
		c.external ? m_appAssembly->m_implName : c.devInstance->device.name().c_str(),
		port->u.wdi.isProducer ? "from" : "to",
		port->name);
  OU::format(prevInstance, "%s_ocdp%u", c.interconnect->name, unoc);
  emitTimeClient(assy, m_config->m_implName, prevInstance.c_str(), "wti");
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
emitConnection(std::string &assy, UNocs &uNocs, size_t &index, size_t baseIndex, const ContConnect &c) {
  const char *err;
  if (c.interconnect) {
    if ((err = emitUNocConnection(assy, uNocs, index, baseIndex, c)))
      return err;
  } else if (c.external && c.devInstance) {
    // We need to connect an external port to a port of a device instance.
    OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='%s'/>\n"
		"    <port instance='%s' name='%s'/>\n"
		"  </connection>\n",
	       m_appAssembly->m_implName, c.external->name,
	       c.devInstance->device.name().c_str(),
	       c.port->name);
  } else
    return "unsupported container connection";
  return NULL;
}

HdlContainer::
HdlContainer(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, NULL, err), m_appAssembly(NULL), m_config(NULL) {
  bool doDefault = false;
  if (err ||
      (err = OE::checkAttrs(m_xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONTAINER_ATTRS, (void*)0)) ||
      (err = OE::checkElements(m_xml, HDL_CONTAINER_ELEMS, (void*)0)) ||
      (err = OE::getBoolean(m_xml, "default", &doDefault)))
    return;
  hdlAssy = true;
  // Set the fixed elements that would normally be parsed
  m_noControl = true;
  std::string myConfig, myAssy;
  OE::getOptionalString(m_xml, myConfig, "config");
  if (myConfig.empty()) {
    OE::getOptionalString(m_xml, myConfig, "platform");
    if (myConfig.empty()) {
      err = "No platform or platform configration specified in HdlContainer";
      return;
    }
  }
  OE::getOptionalString(m_xml, myAssy, "assembly");
  if (myAssy.empty())
    if (assembly)
      myAssy = assembly;
    else
      err = OU::esprintf("No assembly specified for container specified in %s", xfile);
  const char *configFile, *assyFile;
  size_t index = 0;
  if (err || (err = parseFile(myConfig.c_str(), xfile, "HdlConfig", &xml, &configFile)) ||
      !(m_config = HdlConfig::create(xml, configFile, index, err)))
    return;
  // Remember the index that corresponds to the base of the WCI array coming out of the
  // platform config (m_config).
  size_t baseIndex = index;
  if ((err = parseFile(myAssy.c_str(), xfile, "HdlAssembly", &xml, &assyFile)) ||
      !(m_appAssembly = HdlAssembly::create(xml, assyFile, index, err)) ||
      // This is inferred by a connection, but might have devide attributes someday
      (err = parseDevInstances(m_xml, m_config->platform(), &m_config->devInstances(), m_devInstances, m_cards, true)))
    return;
  // Prepare to build (and terminate) the uNocs for interconnects
  // We remember the last instance for each uNoc
  // Establish the NOC usage, if there is any.
  // An interconnect can be on any device worker, but for now it is on the config.
  UNocs uNocs;
  for (PortsIter pi = m_config->m_ports.begin(); pi != m_config->m_ports.end(); pi++) {
    Port &p = **pi;
    Port *slave = NULL;
    if (p.master && p.type == NOCPort) {
      size_t len = strlen(p.name);
      // Find the slave port for this master just for error checking
      for (PortsIter si = m_config->m_ports.begin(); si != m_config->m_ports.end(); si++) {
	Port &sp = **si;
	if (!sp.master && sp.type == NOCPort && !strncasecmp(p.name, sp.name, len) &&
	    !strcasecmp(sp.name + len, "_slave")) {
	  slave = &sp;
	  break;
	}
      }
      ocpiAssert(slave);
      uNocs[p.name] = 0;
    }
  }
  // Establish connections, WHICH MAY ALSO IMPLICITLY CREATE DEVICE INSTANCES
  ContConnects connections;
  for (ezxml_t cx = ezxml_cchild(m_xml, "connection"); cx; cx = ezxml_next(cx)) {
    ContConnect c;
    if ((err = parseConnection(cx, c)))
      return;
    connections.push_back(c);
  }
  std::string assy;
  OU::format(assy, "<HdlContainerAssembly name='%s' language='vhdl'>\n", m_name.c_str());
  // The platform configuration instance, which will have a WCI master
  OU::formatAdd(assy, "  <instance worker='%s'/>\n", configFile);
  // Instance the assembly and connect its wci
  OU::formatAdd(assy, "  <instance worker='%s'/>\n", assyFile);
  if (!m_appAssembly->m_noControl) {
    Port &p = *m_appAssembly->m_ports[0];
    OU::formatAdd(assy,
		  "  <connection count='%zu'>\n"
		  "    <port instance='%s' name='wci'/>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "  </connection>\n",
		  p.count,
		  m_config->m_implName,
		  m_appAssembly->m_implName, p.name);
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
    for (PortsIter pi = m_appAssembly->m_ports.begin(); pi != m_appAssembly->m_ports.end(); pi++)
      if ((*pi)->isData) {
	if (uNocs.empty() || uNocs.size() > 1) {
	  if (!attribute)
	    err = OU::esprintf("No single interconnect in platform configuration for assembly port %s",
			       (*pi)->name);
	  return;
	}
	for (PortsIter ii = m_config->m_ports.begin(); ii != m_config->m_ports.end(); ii++) {
	  Port &i = **ii;
	  if (i.master && i.type == NOCPort) {
	    ContConnect c;
	    c.external = *pi;
	    c.interconnect = &i;
	    if ((err = emitUNocConnection(assy, uNocs, index, baseIndex, c)))
	      return;
	    break;
	  }
	}
      }
  } else {
    for (DevInstancesIter di = m_devInstances.begin(); di != m_devInstances.end(); di++) {
      // Instance the device and connect its wci
      OU::formatAdd(assy, "  <instance name='%s' worker='%s' index='%zu'/>\n",
		    (*di).device.name().c_str(),
		    (*di).device.deviceType().name().c_str(), index);
      OU::formatAdd(assy,
		    "  <connection>\n"
		    "    <port instance='%s' name='wci' index='%zu'/>\n"
		    "    <port instance='%s' name='%s'/>\n"
		    "  </connection>\n",
		    m_config->m_implName, index - baseIndex,
		    (*di).device.name().c_str(),
		    (*di).device.deviceType().m_ports[0]->name);

      index++;
    }
    for (ContConnectsIter ci = connections.begin(); ci != connections.end(); ci++)
      if ((err = emitConnection(assy, uNocs, index, baseIndex, *ci)))
	return;
  }
  // Terminate the uNocs
  for (UNocsIter ii = uNocs.begin(); ii != uNocs.end(); ii++) {
    std::string prevInstance, prevPort;
    if (ii->second == 0) {
      prevInstance = m_config->m_implName;
      prevPort = ii->first;
    } else {
      OU::format(prevInstance, "%s_unoc%u", ii->first, ii->second - 1);
      prevPort = "down";
    }
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <port instance='%s' name='%s_slave'/>\n"
		  "  </connection>\n",
		  prevInstance.c_str(), prevPort.c_str(),
		  m_config->m_implName, ii->first);
  }
#if 0
  // Add all the requested device instances that are not already in the config
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++) {
    Device *dev = *di;
    for (DevicesIter cdi = m_config->devices().begin(); cdi != m_config->devices().end(); cdi++)
      if (*cdi == dev) {
	dev = NULL;
	break;
      }
    if (dev) {
      Worker &dt = *dev->m_deviceType;
      OU::formatAdd(assy, "  <instance worker='%s' name='%s' index='%zu'/>\n",
		    dt.m_implName, dev->m_name.c_str(), index);
      if (dt.m_noControl)
	OU::formatAdd(assy, ">\n");
      else {
	OU::formatAdd(assy, "index='%zu'/>\n", index);
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s' name='wci' index='%zu'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      m_config->m_implName, index++, dev->m_name.c_str(), dt.m_ports[0]->name);
      }
      // Add a time client instance as needed by device instances
      for (PortsIter pi = dt.m_ports.begin(); pi != dt.m_ports.end(); pi++)
	if ((*pi)->type == WTIPort)
	  OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
			dev->m_name.c_str());
    }
  }
#endif
  // Instance time clients for the assembly
  for (PortsIter pi = m_appAssembly->m_ports.begin(); pi != m_appAssembly->m_ports.end(); pi++)
    if ((*pi)->type == WTIPort)
      emitTimeClient(assy, m_config->m_implName, m_appAssembly->m_implName, (*pi)->name);
  OU::formatAdd(assy,
		"  <instance worker='metadata'/>\n"
		"    <connection>\n"
		"     <port instance='metadata' name='metadata'/>\n"
		"     <port instance='%s' name='metadata'/>\n"
		"    </connection>\n",
		m_config->m_implName);
  OU::formatAdd(assy, "</HdlContainerAssembly>\n");
  // The assembly will automatically inherit all the signals, prefixed by instance.
  if (!attribute)
    ocpiInfo("=======Begin generated container assembly=======\n"
	     "%s"
	     "=======End generated container assembly=======\n",
	     assy.c_str());
  // Now we hack the (inherited) worker to have the xml for the assembly we just generated.
  char *copy = strdup(assy.c_str());
  ezxml_t x = ezxml_parse_str(copy, strlen(copy));
  if (x && ezxml_error(x)[0]) {
      err = OU::esprintf("XML Parsing error on generated container assembly: %s", ezxml_error(x));
      return;
  }
  m_xml = x;
  err = parseHdl(NULL);
  m_assembly->m_isContainer = true;
}

HdlContainer::
~HdlContainer() {
  delete m_config;
}

const char *Worker::
emitContainerImplHDL(FILE *f) {
  const char *comment = hdlComment(m_language);
  fprintf(f,
	  "%s This file contains the implementation declarations for a container configuration %s\n"
	  "%s Interface definition signal names are defined with pattern rule: \"%s\"\n\n",
	  comment, m_implName, comment, m_pattern);
  fprintf(f,
	  "Library IEEE; use IEEE.std_logic_1164.all;\n"
	  "Library ocpi; use ocpi.all, ocpi.types.all;\n"
          "use work.%s_defs.all;\n",
	  m_implName);
  emitVhdlLibraries(f);
  fprintf(f,
	  "\nentity %s_rv is\n", m_implName);
  emitParameters(f, m_language);
  emitSignals(f, VHDL, false, true, true);
  fprintf(f, "end entity %s_rv;\n", m_implName);
  emitVhdlSignalWrapper(f, "ftop");
  return NULL;
}

const char *HdlContainer::
emitAttribute(const char *attr) {
  if (!strcasecmp(attr, "language"))
    printf(m_language == VHDL ? "VHDL" : "Verilog");
  else if (!strcasecmp(attr, "platform"))
    puts(m_config->platform().m_implName);
  else if (!strcasecmp(attr, "configuration"))
    puts(m_config->m_implName);
  else
    return OU::esprintf("Unknown container attribute: %s", attr);
  return NULL;
}

// Emit the artifact XML for an HDLcontainer
const char *HdlContainer::
emitArtHDL(const char *outDir, const char *wksFile) {
  const char *err;
  OU::Uuid uuid;
  OU::generateUuid(uuid);
  if ((err = emitUuidHDL(outDir, uuid)))
    return err;
  FILE *f;
  if ((err = openOutput(m_implName, outDir, "", "-art", ".xml", NULL, f)))
    return err;
  fprintf(f, "<!--\n");
  printgen(f, "", m_file.c_str());
  fprintf(f,
	  " This file contains the artifact descriptor XML for the container\n"
	  " named \"%s\". It must be attached (appended) to the bitstream\n",
	  m_implName);
  fprintf(f, "  -->\n");
  OU::UuidString uuid_string;
  OU::uuid2string(uuid, uuid_string);
  fprintf(f, "<artifact platform=\"%s\" device=\"%s\" uuid=\"%s\">\n",
	  m_config->platform().m_name.c_str(), device, uuid_string);
  // Define all workers: they come from three places:
  // 1. The configuration (which includes the platform worker)
  // 2. The assembly (application workers)
  // 3. The container
  m_config->emitWorkers(f);
  m_appAssembly->emitWorkers(f);
  emitWorkers(f);
  m_config->emitInstances(f, "p");
  m_appAssembly->emitInstances(f, "a");
  emitInstances(f, "c");
  m_config->emitInternalConnections(f, "p");
  m_appAssembly->emitInternalConnections(f, "a");
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
    if (cc.m_attachments.front()->m_instPort.m_port->isData) {
      InstancePort *ap = NULL, *pp = NULL, *ip = NULL; // instance ports for the app, for pf, for internal
      for (AttachmentsIter ai = cc.m_attachments.begin(); ai != cc.m_attachments.end(); ai++) {
	Attachment &a = **ai;
	(!strcasecmp(a.m_instPort.m_port->worker->m_implName, m_appAssembly->m_implName) ? ap :
	 !strcasecmp(a.m_instPort.m_port->worker->m_implName, m_config->m_implName) ? pp : ip)
	  = &a.m_instPort;
      }
      assert(ap || pp || ip);
      if (!ap && !pp)
	continue; // internal connection already dealt with
      // find the corresponding instport inside each side.
      InstancePort
	*aap = ap ? m_appAssembly->m_assembly->findInstancePort(ap->m_port->name) : NULL,
        *ppp = pp ? m_config->m_assembly->findInstancePort(pp->m_port->name) : NULL,
	*producer = (aap && aap->m_port->u.wdi.isProducer ? aap :
		     ppp && ppp->m_port->u.wdi.isProducer ? ppp : ip),
	*consumer = (aap && !aap->m_port->u.wdi.isProducer ? aap :
		     ppp && !ppp->m_port->u.wdi.isProducer ? ppp : ip);
      // Application is producing to an external consumer
      fprintf(f, "<connection from=\"%s/%s\" out=\"%s\" to=\"%s/%s\" in=\"%s\"/>\n",
	      producer == ip ? "c" : producer == aap ? "a" : "p",
	      producer->m_instance->name, producer->m_port->name,
	      consumer == ip ? "c" : consumer == aap ? "a" : "p",
	      consumer->m_instance->name, consumer->m_port->name);
    }
  }
  fprintf(f, "</artifact>\n");
  if (fclose(f))
    return "Could not close output file. No space?";
  if (wksFile)
    return emitWorkersHDL(outDir, wksFile);
  return 0;
}

HdlAssembly *HdlAssembly::
create(ezxml_t xml, const char *xfile, size_t &index, const char *&err) {
  HdlAssembly *ha = new HdlAssembly(xml, xfile, index, err);
  if (err) {
    delete ha;
    ha = NULL;
  }
  return ha;
}

HdlAssembly::
HdlAssembly(ezxml_t xml, const char *xfile, size_t &index, const char *&err)
  : Worker(xml, xfile, NULL, err) {
  if (!(err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS, (void*)0)) &&
      !(err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, ASSY_ELEMS, (void*)0)))
    err = parseHdl(NULL);
  if (err)
    return;
  assert(m_assembly);
  Instance *i = m_assembly->m_instances;
  for (unsigned n = 0; n < m_assembly->m_nInstances; n++, i++)
    if (!i->worker->m_assembly && !i->worker->m_noControl)
      i->index = index++;
}
HdlAssembly::
~HdlAssembly() {
}

SlotType::
SlotType(const char *file, const char *parent, const char *&err) {
  ezxml_t xml;
  const char *xfile;
  err = NULL;
  if ((err = parseFile(file, parent, NULL, &xml, &xfile)) ||
      (err = Signal::parseSignals(xml, m_signals)))
    return;
  OE::getOptionalString(xml, m_name, "name");
  if (m_name.empty())
    m_name = xfile;
  else if (m_name != xfile)
    err = OU::esprintf("File name (%s) does not match name attribute in XML (%s)",
		       xfile, m_name.c_str());
}
SlotType::
~SlotType() {
  Signal::deleteSignals(m_signals);
}

// Slot types are interned (only created when not already there)
// They are not inlined or included, but simply referenced by attributes.
SlotType *SlotType::
create(const char *name, const char *parent, SlotTypes &types, const char *&err) {
  SlotType *st = find(name, types);
  if (!st) {
    st = new SlotType(name, parent, err);
    if (err) {
      delete st;
      st = NULL;
    } else
      types[name] = st;
  }
  return st;
}

SlotType *SlotType::
find(const char *name, SlotTypes &types) {
  SlotTypesIter sti = types.find(name);
  return sti == types.end() ? NULL : sti->second;
}

// A slot may have a default mapping to the external platform's signals,
// ie. <slot-name>_signal.
Slot::
Slot(ezxml_t xml, const char */*parent*/, const char *name, SlotType &type, const char *&err)
  : m_name(name), m_type(type)
{
  err = NULL;
  // process non-default signals: slot=pfsig, platform=dddd
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_next(xs)) {
    std::string slot, platform;
    if ((err = OE::getRequiredString(xs, slot, "slot")) ||
	(err = OE::getRequiredString(xs, platform, "platform")))
      break;
    Signal *s = Signal::find(m_type.m_signals, slot.c_str());
    if (!s)
      err = OU::esprintf("Slot signal '%s' does not exist for slot type '%s'",
			 slot.c_str(), m_type.m_name.c_str());
    else if (m_signals.find(s) != m_signals.end())
      err = OU::esprintf("Duplicate slot signal: %s", slot.c_str());
    else
      m_signals[s] = platform;
  }
  if (err)
    err = OU::esprintf("Error for slot '%s': %s", m_name.c_str(), err);
}

Slot::
~Slot() {
}

// Slots are not interned, and we want the type to be a reference.
// Hence we check the type first.
Slot *Slot::
create(ezxml_t xml, const char *parent, SlotTypes &types, Slots &slots, const char *&err) {
  std::string type, name;
  if ((err = OE::getRequiredString(xml, type, "type")) ||
      (err = OE::getRequiredString(xml, name, "name")))
    return NULL;
  SlotType *t = SlotType::create(type.c_str(), OE::ezxml_tag(xml), types, err);
  if (!t)
    return NULL;
  Slot *s = find(name.c_str(), slots);
  if (s) {
    err = OU::esprintf("Duplicate slot name (%s) in '%s' element", name.c_str(), parent);
    return NULL;
  }
  s = new Slot(xml, parent, name.c_str(), *t, err);
  if (err) {
    delete s;
    return NULL;
  }
  return slots[s->m_name.c_str()] = s;
}

Slot *Slot::
find(const char *name, Slots &slots) {
  SlotsIter si = slots.find(name);
  return si == slots.end() ? NULL : si->second;
}

// A slot may have a default mapping to the external platform's signals,
// ie. <slot-name>_signal.
Card::
Card(ezxml_t xml, const char *name, SlotType &type, const char *&err)
  : m_name(name), m_type(type)
{
  // process non-default signals: slot=pfsig, platform=dddd
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_next(xs)) {
    std::string slot, card;
    if ((err = OE::getRequiredString(xs, slot, "slot")) ||
	(err = OE::getRequiredString(xs, card, "card")))
      break;
    Signal *s = Signal::find(m_type.m_signals, slot.c_str());
    if (!s)
      err = OU::esprintf("Slot signal '%s' does not exist for slot type '%s'",
			 slot.c_str(), m_type.m_name.c_str());
    else if (m_signals.find(s) != m_signals.end())
      err = OU::esprintf("Duplicate slot signal: %s", slot.c_str());
    else
      m_signals[s] = card;
  }
  // These devices are declaring that they are part of the card.
  for (ezxml_t xs = ezxml_cchild(xml, "Device"); xs; xs = ezxml_next(xs)) {
    const char *worker = ezxml_cattr(xs, "worker");
    bool single = true;
    for (ezxml_t x = ezxml_cchild(xml, "Device"); x; x = ezxml_next(x)) {
      const char *w = ezxml_cattr(x, "worker");
      if (x != xs && worker && w && !strcasecmp(worker, w))
	single = false;
    }
    Device *dev = Device::create(xs, xml->name, m_deviceTypes, single, err);
    if (!dev)
      return;
    m_devices.push_back(dev);
  }
  if (err)
    err = OU::esprintf("Error for slot '%s': %s", m_name.c_str(), err);
}

Card::
~Card() {
}

// Cards are interned, and we want the type to be a reference.
// Hence we check the type first.
Card *Card::
create(const char *file, const char *parent, SlotTypes &types, Cards &cards, const char *&err) {
  ezxml_t xml;
  const char *xfile;
  if ((err = parseFile(file, parent, NULL, &xml, &xfile)))
    return NULL;
  std::string name;
  OE::getOptionalString(xml, name, "name");
  if (name.empty())
    name = xfile;
  else if (name != xfile)
    err = OU::esprintf("File name (%s) does not match name attribute in XML (%s)",
		       xfile, name.c_str());
  Card *c = find(name.c_str(), cards);
  if (c)
    return c;
  std::string type;
  if ((err = OE::getRequiredString(xml, type, "type")))
    return NULL;
  SlotTypesIter ti = types.find(type.c_str());
  if (ti == types.end()) {
    err = OU::esprintf("Card '%s' refers to slot type '%s' that is not on this platform",
		       name.c_str(), type.c_str());
    return NULL;
  }
  c = new Card(xml, name.c_str(), *ti->second, err);
  if (err) {
    delete c;
    return NULL;
  }
  return cards[c->m_name.c_str()] = c;
}

Card *Card::
find(const char *name, Cards &cards) {
  CardsIter si = cards.find(name);
  return si == cards.end() ? NULL : si->second;
}

