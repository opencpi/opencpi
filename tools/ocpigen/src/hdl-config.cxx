#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "assembly.h"
#include "hdl-config.h"
#include "hdl.h"

namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;

const DevInstance *HdlHasDevInstances::
findDevInstance(const Device &dev, const Card *card, const Slot *slot,
		DevInstances *baseInstances, bool *inBase) {
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
	       bool control, const DevInstance *&devInstance) {
  const char *err;
  m_devInstances.push_back(DevInstance(dev, card, slot, control));
  assert(card && slot || !card && !slot);
  assert(!slot || !m_plugged[slot->m_ordinal] || card == m_plugged[slot->m_ordinal]);
  if (slot && !m_plugged[slot->m_ordinal])
    m_plugged[slot->m_ordinal] = card;
  devInstance = &m_devInstances.back();
  for (RequiredsIter ri = dev.m_deviceType->m_requireds.begin();
       ri != dev.m_deviceType->m_requireds.end(); ri++) {
    const Required &r = *ri;
    const Device &rdev = dev.m_board.findRequired(r.m_type, dev.m_ordinal);
    // Note that we only look in the current devinstance list.
    // We can't share subdevices across the config-container boundary
    const DevInstance *rdi = findDevInstance(rdev, card, slot, NULL, NULL);
    if (!rdi &&
	(err = addDevInstance(rdev, card, slot, control, rdi)))
      return err;
  }      
  return NULL;
}

const char *HdlHasDevInstances::
parseDevInstance(const char *device, ezxml_t x, const char *parent, bool control,
		 DevInstances *baseInstances, const DevInstance **result, bool *inBase) {
  const char *err;
  std::string s;
  const Slot *slot = NULL;
  if (OE::getOptionalString(x, s, "slot") &&
      !(slot = m_platform.findSlot(s.c_str(), err)))
    return err;
  const Card *card = NULL;
  if (OE::getOptionalString(x, s, "card") &&
      !(card = Card::get(s.c_str(), parent, err)))
    return err;
  // Card and slots have been checked individually)
  if (slot) {
    const Card *plug = m_plugged[slot->m_ordinal];
    if (card) {
      if (card != plug)
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
  if (control && !dev->m_deviceType->m_canControl)
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
      return OU::esprintf("Platform Device '%s' is already in the platform configuration",
			  di->device.name());
  }
  if ((err = addDevInstance(*dev, card, slot, control, di)))
    return err;
  if (result)
    *result = di;
  return NULL;
}

// Parse references to devices for instantiation
// Used for platform configurations and containers
// for EXPLICIT instantiation
const char *HdlHasDevInstances::
parseDevInstances(ezxml_t xml, const char *parent, DevInstances *baseInstances) {
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
    if (control && baseInstances)
      return "It is invalid to specify a 'control' attribute to a device in a container";
    if (!strcasecmp(m_platform.name(), name.c_str())) {
      m_platform.setControl(control); // FIXME make the platform a device...
      continue;
    }
    if ((err = parseDevInstance(name.c_str(), xd, parent, control, baseInstances, NULL, NULL)))
      return err;
  }
  return NULL;
}

HdlConfig *HdlConfig::
create(ezxml_t xml, const char *xfile, const char *&err) {
  err = NULL;
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
	return NULL;
      }
    } else
      myPlatform = ::platform;
  }
  std::string pfile;
  ezxml_t pxml;
  HdlPlatform *pf;
  if ((err = parseFile(myPlatform.c_str(), xfile, "HdlPlatform", &pxml, pfile)) ||
      !(pf = HdlPlatform::create(pxml, pfile.c_str(), err)))
    return NULL;
  HdlConfig *p = new HdlConfig(*pf, xml, xfile, err);
  if (err) {
    delete p;
    p = NULL;
  }
  return p;
}

HdlConfig::
HdlConfig(HdlPlatform &pf, ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, "", NULL, err), HdlHasDevInstances(pf, m_plugged), m_platform(pf) {
  if (err ||
      (err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONFIG_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, HDL_CONFIG_ELEMS, (void*)0)))
    return;
  hdlAssy = true;
  m_plugged.resize(pf.m_slots.size());
  if ((err = parseDevInstances(xml, xfile, NULL)))
    return;
  std::string assy;
  OU::format(assy, "<HdlPlatformAssembly name='%s'>\n", m_name.c_str());
  // Add the platform instance
  OU::formatAdd(assy, "  <instance worker='%s'/>\n", // index='%zu'/>\n",
		m_platform.m_name.c_str()); //, index++);
  // Add the control plane instance
  //   OU::formatAdd(assy, "  <instance worker='occp'/>\n"); not anymore - it is in the container now
  // Add all the device instances
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const Device &d = (*dii).device;
    OU::formatAdd(assy, "  <instance worker='%s' name='%s'/>\n",
		  d.m_deviceType->name(), d.name());
    // Add a time client instance as needed by device instances
    for (PortsIter pi = d.m_deviceType->ports().begin();
	 pi != d.m_deviceType->ports().end(); pi++)
      if ((*pi)->type == WTIPort)
	OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
		      d.name());
  }
  // Internal connections:
  // 1. Control plane master to OCCP
  // 2. WCI connections to platform and device workers
  // 3. To and from time clients
  // 4. Between devices and required subdevices

  // So: 1. Add the internal connection from the platform and/or device workers to occp
  if ((err = addControlConnection(assy)))
    return;
  // 3. To and from time clients
  unsigned tIndex = 0;
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().ports().begin();
	 pi != d.deviceType().ports().end(); pi++)
      if ((*pi)->type == WTIPort) {
	// connection from platform worker's time service to the client
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s' name='time'/>\n"
		      "    <port instance='time_client%u' name='time'/>\n"
		      "  </connection>\n",
		      m_platform.m_name.c_str(), tIndex);
	// connection from the time client to the device worker
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='time_client%u' name='wti'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      tIndex++, d.name(), (*pi)->name());
      }
  }
  // 4. To and from subdevices
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const Device &d = (*dii).device;
    // add the port connections to subdevices
    // Now add the connections between devInstance and subdevices.
    for (RequiredsIter ri = d.m_deviceType->m_requireds.begin();
	 ri != d.m_deviceType->m_requireds.end(); ri++) {
      const Device &rdev = d.m_board.findRequired((*ri).m_type, d.m_ordinal);
      for (ReqConnectionsIter rci = (*ri).m_connections.begin();
	   rci != (*ri).m_connections.end(); rci++) {
	// A port connection
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "    <port instance='%s' name='%s'",
		      d.name(), (*rci).m_port->name(),
		      rdev.name(), (*rci).m_rq_port->name());
	if ((*rci).m_indexed)
	  OU::formatAdd(assy, " index='%zu'", (*rci).m_index);
	OU::formatAdd(assy,
		      "/>\n"
		      "  </connection>\n");
      }
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
		m_platform.m_name.c_str(),
		m_platform.m_name.c_str());
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().ports().begin(); pi != d.deviceType().ports().end(); pi++) {
      Port &p = **pi;
      if (p.isData() || p.type == NOCPort)
	OU::formatAdd(assy,
		      "  <external name='%s_%s' instance='%s' port='%s'/>\n",
		      d.name(), p.name(),
		      d.name(), p.name());
    }		
  }
  for (PortsIter pi = m_platform.m_ports.begin(); pi != m_platform.m_ports.end(); pi++) {
      Port &p = **pi;
      if (p.type == NOCPort) {
	// Port names of noc ports are interconnect names on the platform
	OU::formatAdd(assy,
		      "  <external name='%s' instance='%s' port='%s'/>\n",
		      p.name(), m_platform.m_name.c_str(), p.name());
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
      cpPortName = (*pi)->name();
      if (m_platform.m_control)
	nCpPorts++;
      break;
    }
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const Device &d = (*dii).device;
    for (PortsIter pi = d.m_deviceType->ports().begin();
	 pi != d.m_deviceType->ports().end(); pi++)
      if ((*pi)->type == CPPort) {
	if (cpInstanceName)
	  multiple = true;
	else {
	  cpInstanceName = d.m_name.c_str();
	  cpPortName = (*pi)->name();
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
          "use work.%s_defs.all;\n",
	  m_implName);
  emitVhdlLibraries(f);
  fprintf(f,
	  "\nentity %s_rv is\n", m_implName);
  emitParameters(f, m_language);
  emitSignals(f, VHDL, true, true, false);
  fprintf(f, "end entity %s_rv;\n", m_implName);
  return NULL;
}
