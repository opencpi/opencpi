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
Device(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single, const char *&err)
  : m_control(false) {
  err = NULL;
  std::string wname;
  if ((err = OE::getRequiredString(xml, wname, "worker", "device")) ||
      !(m_deviceType = DeviceType::create(wname.c_str(), parent, deviceTypes, err)) ||
      (err = OE::getNumber(xml, "width", &m_width, NULL, 0, false)) ||
      (err = OE::getBoolean(xml, "control", &m_control)))
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


const char *Device::
parseDevices(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, Devices &devices) {
  const char *err = NULL;
  Device *dev;
  for (ezxml_t xs = ezxml_cchild(xml, "Device"); !err && xs; xs = ezxml_next(xs)) {
    const char *worker = ezxml_cattr(xs, "worker");
    bool single = true;
    for (ezxml_t x = ezxml_cchild(xml, "Device"); !err && x; x = ezxml_next(x)) {
      const char *w = ezxml_cattr(x, "worker");
      if (x != xs && worker && w && !strcasecmp(worker, w))
	single = false;
    }
    if ((dev = Device::create(xs, parent, deviceTypes, single, err)))
      devices.push_back(dev);
  }
  return err;
}

#if 0
hmmm - should a device type BE a device worker?
just like HdlPlatform is a platform worker,
HdlDevice is a device worker.
-- this implies that declaring the existence of a device type means at least a skeleton of a worker. Ok.
This means that the properties are part of the definition.  I LIKE THIS.
thus the signals would come for free...
#endif

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

  // Done with generic and worker stuff.
  if ((err = Device::parseDevices(m_xml, m_file.c_str(), m_deviceTypes, m_devices)))
    return;
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
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++)
    for (PortsIter pi = (*di)->m_deviceType->m_ports.begin();
	 pi != (*di)->m_deviceType->m_ports.end(); pi++)
      if ((*pi)->type == CPPort) {
	if (cpInstanceName)
	  multiple = true;
	else
	  cpInstanceName = (*di)->m_name.c_str();
	if ((*di)->m_control)
	  nCpPorts++;
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

// Used for configurations and containers
static const char *
parsePlatformDevices(ezxml_t xml, HdlPlatform &platform, Devices &devices, bool container) {
  // Now we have a platform to work from.  Here we parse the extra info needed to
  // generate this platform configuration.
  const char *err = NULL;
  Devices &pDevices(platform.devices());
  for (ezxml_t xd = ezxml_cchild(xml, "Device"); !err && xd; xd = ezxml_next(xd)) {
    std::string name;
    bool control = false;
    if ((err = OE::checkAttrs(xd, "name", "control", (void*)0)) ||
	(err = OE::getBoolean(xd, "control", &control)) ||
	(err = OE::getRequiredString(xd, name, "name", "device")))
      return err;
    if (control && container)
      return "It is invalid to specify a 'control' attribute to a device in a container";
    if (strcasecmp(platform.m_name.c_str(), name.c_str())) {
      Device *dev = NULL;
      for (DevicesIter di = pDevices.begin(); di != pDevices.end(); di++)
	if (!strcasecmp(name.c_str(), (*di)->m_name.c_str())) {
	  dev = *di;
	  break;
      }
      if (!dev)
	return OU::esprintf("No device named '%s' found in platform '%s'",
			    name.c_str(), platform.m_name.c_str());
      dev->m_control = control;
      devices.push_back(dev);
    } else
      platform.setControl(control);
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
      (err = parsePlatformDevices(xml, *m_platform, m_devices, false)))
    return;
  std::string assy;
  OU::format(assy, "<HdlPlatformAssembly name='%s'>\n", m_name.c_str());
  // Add the platform instance
  OU::formatAdd(assy, "  <instance worker='%s' index='%zu'/>\n", m_platform->m_name.c_str(), index++);
  // Add the control plane instance
  OU::formatAdd(assy, "  <instance worker='occp'/>\n");
  // Add all the device instances
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++) {
    OU::formatAdd(assy, "  <instance worker='%s' name='%s' index='%zu'/>\n",
		  (*di)->m_deviceType->m_implName, (*di)->m_name.c_str(), index++);
    // Add a time client instance as needed by device instances
    for (PortsIter pi = (*di)->m_deviceType->m_ports.begin();
	 pi != (*di)->m_deviceType->m_ports.end(); pi++)
      if ((*pi)->type == WTIPort)
	OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
		      (*di)->m_name.c_str());
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
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++)
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <port instance='occp' name='wci' index='%zu'/>\n"
		  "  </connection>\n",
		  (*di)->m_name.c_str(), (*di)->m_deviceType->m_ports[0]->name,
		  nWCI++);
  // 3. To and from time clients
  unsigned tIndex = 0;
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++)
    for (PortsIter pi = (*di)->m_deviceType->m_ports.begin();
	 pi != (*di)->m_deviceType->m_ports.end(); pi++)
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
		      tIndex++, (*di)->m_name.c_str(), (*pi)->name);
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
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++)
    for (PortsIter pi = (*di)->m_deviceType->m_ports.begin();
	 pi != (*di)->m_deviceType->m_ports.end(); pi++) {
      Port &p = **pi;
      if (p.isData || p.type == NOCPort)
	OU::formatAdd(assy,
		      "  <external name='%s_%s' instance='%s' port='%s'/>\n",
		      (*di)->m_name.c_str(), p.name,
		      (*di)->m_name.c_str(), p.name);
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

const char *HdlContainer::
parseConnection(ezxml_t cx, Port *&external, Device *&device,
		bool &devInConfig, Port *&port, Port *&interconnect) {
  const char *err;
  if ((err = OE::checkAttrs(cx, "external", "instance", "device", "interconnect", "port",
			    "otherdevice", "otherport", NULL)))
    return err;
  const char *attr;
  external = NULL;
  if ((attr = ezxml_cattr(cx, "external"))) {
    // Instance time clients for the assembly
    for (PortsIter pi = m_appAssembly->m_ports.begin(); pi != m_appAssembly->m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name, attr)) {
	external = *pi;
	break;
      }
    if (!external)
      return OU::esprintf("External assembly port '%s' not found in assembly", attr);
  }
  device = NULL;
  devInConfig = true;
  if ((attr = ezxml_cattr(cx, "device"))) {
    Devices &pdevs = m_config->platform().devices();
    for (DevicesIter pdi = pdevs.begin(); pdi != pdevs.end(); pdi++)
      if (!strcasecmp((*pdi)->m_name.c_str(), attr)) {
	device = *pdi;
	break;
      }
    if (!device)
      return OU::esprintf("Device '%s' not found for platform '%s'", attr,
			  m_config->platform().m_name.c_str());
    // Now we have a device, is it in the platform config?
    for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++) {
      if (*di == device) {
	devInConfig = false;
	break;
      }
    }
  }
  port = NULL;
  if ((attr = ezxml_cattr(cx, "port"))) {
    if (!device)
      return OU::esprintf("Port '%s' specified without specifying a device", attr);
    for (PortsIter pi = device->deviceType().m_ports.begin(); pi != m_config->m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name, attr)) {
	port = *pi;
	break;
      }
    if (!port)
      return OU::esprintf("Port '%s' not found for device '%s'", attr, device->m_name.c_str());
    if (!port->isData)
      return OU::esprintf("Port '%s' for device '%s' is not a data port", attr, device->m_name.c_str());
  } else if (device) {
    // Find the one data port
    for (PortsIter pi = device->deviceType().m_ports.begin(); pi != m_config->m_ports.end(); pi++)
      if ((*pi)->isData) {
	if (port)
	  return OU::esprintf("There are multiple data ports for device '%s'; you must specify one",
			      device->m_name.c_str());
	port = *pi;
      }
    if (!port)
      return OU::esprintf("There are no data ports for device '%s'", device->m_name.c_str());
  }
  interconnect = NULL;
  if ((attr = ezxml_cattr(cx, "interconnect"))) {
    // An interconnect can be on any device worker, but for now it is on the config.
    for (PortsIter pi = m_config->m_ports.begin(); pi != m_config->m_ports.end(); pi++)
      if (!strcasecmp((*pi)->name, attr)) {
	interconnect = *pi;
	break;
      }
    if (!interconnect ||
	interconnect->type != NOCPort || !interconnect->master)
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

 const char *HdlContainer::
emitConnection(std::string &assy, Port *external, Port *interconnect, unsigned &unoc,
	       size_t &index, size_t baseIndex) {
   if (external->type != WSIPort || interconnect->type != NOCPort || !interconnect->master)
     return OU::esprintf("unsupported container connection between external %s and interconnect %s",
			 external->name, interconnect->name);
  // Create the three instances:
  // 1. A unoc node to use the interconnect unoc
  // 2. A DP/DMA module to stream to/from another place on the interconnect
  // 3. An SMA to adapt the WMI on the DP to the WSI that is needed (for now).
  OU::formatAdd(assy,
		"  <instance name='%s_unoc%u' worker='unoc_node'>\n"
		"    <property name='control' value='false'/>\n"
		"    <property name='position' value='%u'/>\n"
		"  </instance>\n",
		interconnect->name, unoc, unoc);
  OU::formatAdd(assy,
		"  <instance name='%s_ocdp%u' worker='ocdp' index='%zu' interconnect='%s' configure='%u'/>\n"
		"  <connection>\n"
		"    <port instance='%s_ocdp%u' name='ctl'/>\n"
		"    <port instance='%s' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		interconnect->name, unoc, index, interconnect->name, unoc,
		interconnect->name, unoc,
		m_config->m_implName, index - baseIndex);
  index++;
  OU::formatAdd(assy,
		"  <instance name='%s_sma%u' worker='sma' index='%zu' adapter='%s' configure='%u'/>\n"
		"  <connection>\n"
		"    <port instance='%s_sma%u' name='ctl'/>\n"
		"    <port instance='%s' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		interconnect->name, unoc, index, interconnect->name, 
		external->u.wdi.isProducer ? 2 : 1,
		interconnect->name, unoc,
		m_config->m_implName, index - baseIndex);
  index++;
  // Connect the new unoc node to the unoc
  std::string prevInstance, prevPort;
  if (unoc == 0) {
    prevInstance = m_config->m_implName;
    prevPort = interconnect->name;
  } else {
    OU::format(prevInstance, "%s_unoc%u", interconnect->name, unoc - 1);
    prevPort = "down";
  }
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='%s'/>\n"
		"    <port instance='%s_unoc%u' name='up'/>\n"
		"  </connection>\n",
		prevInstance.c_str(), prevPort.c_str(), interconnect->name, unoc);
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s_unoc%u' name='client'/>\n"
		"    <port instance='%s_ocdp%u' name='client'/>\n"
		"  </connection>\n",
		interconnect->name, unoc, interconnect->name, unoc);
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s_ocdp%u' name='data'/>\n"
		"    <port instance='%s_sma%u' name='message'/>\n"
		"  </connection>\n",
		interconnect->name, unoc, interconnect->name, unoc);
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s_sma%u' name='%s'/>\n"
		"    <port instance='%s' name='%s'/>\n"
		"  </connection>\n",
		interconnect->name, unoc, external->u.wdi.isProducer ? "in" : "out",
		m_appAssembly->m_implName, external->name);
  OU::format(prevInstance, "%s_ocdp%u", interconnect->name, unoc);
  emitTimeClient(assy, m_config->m_implName, prevInstance.c_str(), "wti");
  unoc++;
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
      (err = parsePlatformDevices(m_xml, m_config->platform(), m_devices, true)))
    return;
  // Prepare to build (and terminate) the uNocs for interconnects
  // We remember the last instance for each uNoc
  typedef std::map<const char *, unsigned, OU::ConstCharComp> UNocs;
  typedef UNocs::iterator UNocsIter;
  UNocs uNocs;
  // Establish the NOC usage, if there is any.
  // An interconnect can be on any device worker, but for now it is on the config.
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
    // Instance time clients for the assembly
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
	    if ((err = emitConnection(assy, *pi, &i, uNocs[i.name], index, baseIndex)))
	      return;
	    break;
	  }
	}
      }
  } else
    // Parse connections here - perhaps we could share some code with the generic
    // assembly parser, but not yet...
    for (ezxml_t cx = ezxml_cchild(m_xml, "Connection"); cx; cx = ezxml_next(cx)) {
      Port *external;
      Device *device;
      bool devInConfig;
      Port *port;
      Port *interconnect;
      if ((err = parseConnection(cx, external, device, devInConfig, port, interconnect)))
	return;
      if (external && interconnect) {
	// Find uNoc
	unsigned &unoc = uNocs.at(interconnect->name);
	if ((err = emitConnection(assy, external, interconnect, unoc, index, baseIndex)))
	  return;
      } else {
	err = "unsupported container connection";
	return;
      }
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
      err = OU::esprintf("XML Parsing error on generated platform configuration: %s", ezxml_error(x));
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

