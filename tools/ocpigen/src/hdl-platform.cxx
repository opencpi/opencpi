#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <map>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "assembly.h"
#include "hdl-platform.h"

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
  : Worker(xml, file, parent, NULL, err), m_count(0) {
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
  : Worker(xml, xfile, NULL, NULL, err), m_control(false) {
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
create(ezxml_t xml, const char *xfile, const char *&err) {
  err = NULL;
  HdlConfig *p = new HdlConfig(xml, xfile, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
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
  // Connect the control master port of the platform or device/interconnect worker to
  // the control plane worker
  OU::formatAdd(assy,
#if 0
		"  <connection>\n"
		"    <port Instance='occp' name='cp'/>\n"
		"    <port Instance='%s' name='%s'/>\n"
		"  </connection>\n",
#else
		"  <external instance='%s' port='%s'/>\n",
#endif
		cpInstanceName, cpPortName);
  return NULL;
}

const DevInstance *HdlPlatform::
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

const char *HdlPlatform::
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
const char *HdlPlatform::
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
HdlConfig(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, NULL, NULL, err), m_platform(NULL) {
  if (err ||
      (err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONFIG_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, HDL_CONFIG_ELEMS, (void*)0)))
    return;
  hdlAssy = true;
  // Set the fixed elements that would normally be parsed
  //  m_noControl = true;
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
      (err = HdlPlatform::parseDevInstances(xml, *m_platform, NULL, m_devInstances, m_cards,
					    false)))
    return;
  std::string assy;
  OU::format(assy, "<HdlPlatformAssembly name='%s'>\n", m_name.c_str());
  // Add the platform instance
  OU::formatAdd(assy, "  <instance worker='%s'/>\n", // index='%zu'/>\n",
		m_platform->m_name.c_str()); //, index++);
  // Add the control plane instance
  //   OU::formatAdd(assy, "  <instance worker='occp'/>\n"); not anymore - it is in the container now
  // Add all the device instances
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    Device &d = (*dii).device;
#if 0
    OU::formatAdd(assy, "  <instance worker='%s' name='%s' index='%zu'/>\n",
		  d.m_deviceType->m_implName, d.m_name.c_str(), index++);
#else
    OU::formatAdd(assy, "  <instance worker='%s' name='%s'/>\n",
		  d.m_deviceType->m_implName, d.m_name.c_str());
#endif
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
#if 0
  // 2. WCI connections to platform and device workers
  size_t nWCI = 0;
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='%s'/>\n"
		"    <external port='wci' index='%zu'/>\n"
		"  </connection>\n",
		m_platform->m_name.c_str(), m_platform->m_ports[0]->name, nWCI++);
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    Device &d = (*dii).device;
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <external port='wci' index='%zu'/>\n"
		  "  </connection>\n",
		  d.name().c_str(), d.deviceType().m_ports[0]->name,
		  nWCI++);
  }
#endif
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
		//		"  <external port='wci' count='%zu' role='slave'/>\n"
		"  <external instance='%s' port='time'/>\n"
		"  <external instance='%s' port='metadata'/>\n",
		//		nWCI,
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



SlotType::
SlotType(const char *file, const char *parent, const char *&err) {
  ezxml_t xml;
  const char *xfile;
  err = NULL;
  if ((err = parseFile(file, parent, NULL, &xml, &xfile)) ||
      (err = Signal::parseSignals(xml, m_signals)))
    return;
  OE::getOptionalString(xml, m_name, "name");
  // FIXME: change parseFile to make xfile a string reference...UGH
  char *cp = strdup(xfile);
  char *slash = strrchr(cp, '/');
  if (slash)
    slash++;
  else
    slash = cp;
  char *dot = strchr(slash, '.');
  if (dot)
    *dot = '\0';
  if (m_name.empty())
    m_name = slash;
  else if (m_name != slash)
    err = OU::esprintf("File name (%s) does not match name attribute in XML (%s)",
		       xfile, m_name.c_str());
  free(cp);
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

