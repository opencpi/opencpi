#define __STDC_LIMIT_MACROS
#include <stdint.h>
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
create(ezxml_t xml, const char *xfile, const char *&err) {
  err = NULL;
  HdlConfig *p = new HdlConfig(xml, xfile, err);
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
HdlConfig(ezxml_t xml, const char *xfile, const char *&err)
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
  unsigned index = 0;
  OU::format(assy, "<HdlPlatformAssembly name='%s'>\n", m_name.c_str());
  // Add the platform instance
  OU::formatAdd(assy, "  <instance worker='%s' index='%u'/>\n", m_platform->m_name.c_str(), index++);
  // Add the control plane instance
  OU::formatAdd(assy, "  <instance worker='occp'/>\n");
  // Add all the device instances
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++) {
    OU::formatAdd(assy, "  <instance worker='%s' name='%s' index='%u'/>\n",
		  (*di)->m_deviceType->m_implName, (*di)->m_name.c_str(), index++);
    // Add a time client instance as needed by device instances
    for (PortsIter pi = (*di)->m_deviceType->m_ports.begin();
	 pi != (*di)->m_deviceType->m_ports.end(); pi++)
      if ((*pi)->type == WTIPort)
	OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
		      (*di)->m_name.c_str());
  }
  // Internal connections:
  // 1. Control plane master to OCPC
  // 2. WCI connections to platform and device workers
  // 3. To and from time clients

  // So: 1. Add the internal connection from the platform and/or device workers to occp
  if ((err = addControlConnection(assy)))
    return;
  // 2. WCI connections to platform and device workers
  unsigned nWCI = 0;
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='%s'/>\n"
		"    <port instance='occp' name='wci' index='%u'/>\n"
		"  </connection>\n",
		m_platform->m_name.c_str(), m_platform->m_ports[0]->name, nWCI++);
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++)
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <port instance='occp' name='wci' index='%u'/>\n"
		  "  </connection>\n",
		  (*di)->m_name.c_str(), (*di)->m_deviceType->m_ports[0]->name,
		  nWCI++);
  // 3. To and from time clients
  index = 0;
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
		      m_platform->m_name.c_str(), index);
	// connection from the time client to the device worker
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='time_client%u' name='wti'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      index++, (*di)->m_name.c_str(), (*pi)->name);
      }
  // End of internal connections.
  // Start of external connections (not signals)
  //  1. WCI master
  //  2. Time service
  //  3. Metadata
  //  4. Any data ports from device worker
  //  5. Any unocs from device workers
  OU::formatAdd(assy,
		"  <external instance='occp' port='wci' index='%u' count='%u'/>\n"
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

HdlContainer::
HdlContainer(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, NULL, err), m_appAssembly(NULL), m_config(NULL) {
  if (err ||
      (err = OE::checkAttrs(m_xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONTAINER_ATTRS, (void*)0)) ||
      (err = OE::checkElements(m_xml, HDL_CONTAINER_ELEMS, (void*)0)))
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
  if (err || (err = parseFile(myConfig.c_str(), xfile, "HdlConfig", &xml, &configFile)) ||
      !(m_config = HdlConfig::create(xml, configFile, err)) ||
      (err = parseFile(myAssy.c_str(), xfile, "HdlAssembly", &xml, &assyFile)) ||
      !(m_appAssembly = HdlAssembly::create(xml, assyFile, err)) ||
      (err = parsePlatformDevices(xml, m_config->platform(), m_devices, true)))
    return;
  std::string assy;
  OU::format(assy, "<HdlContainerAssembly name='%s' language='vhdl'>\n", m_name.c_str());
  // The platform configuration instance, which will have a WCI master
  OU::formatAdd(assy, "  <instance worker='%s'/>\n", configFile);
  size_t index = 0;
  // Instance the assembly
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
    index += p.count;
  }
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
		    dt.m_implName, dev->m_name.c_str(), index++);
      if (!dt.m_noControl)
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s' name='wci' index='%zu'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      m_config->m_implName, index++, dev->m_name.c_str(), dt.m_ports[0]->name);
      // Add a time client instance as needed by device instances
      for (PortsIter pi = dt.m_ports.begin(); pi != dt.m_ports.end(); pi++)
	if ((*pi)->type == WTIPort)
	  OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
			dev->m_name.c_str());
    }
  }
  // Instance time clients for the assembly
  for (PortsIter pi = m_appAssembly->m_ports.begin(); pi != m_appAssembly->m_ports.end(); pi++)
    if ((*pi)->type == WTIPort)
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
		    m_appAssembly->m_implName, (*pi)->name,
		    m_appAssembly->m_implName, (*pi)->name,
		    m_appAssembly->m_implName, (*pi)->name,
		    m_config->m_implName,
		    m_appAssembly->m_implName, (*pi)->name);
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
#if 0
  ezxml_t dep;
  const char *file;
  if ((err = parseFile(container, 0, "HdlContainer", &dep, &file)))
    return err;
  Worker *dw = create(file, m_file.c_str(), NULL, err);
  if (!dw || (err = dw->parseHdlAssy()))
    return err;
  OU::UuidString uuid_string;
  OU::uuid2string(uuid, uuid_string);
  fprintf(f, "<artifact platform=\"%s\" device=\"%s\" uuid=\"%s\">\n",
	  platform, device, uuid_string);
  // Define all workers
  for (WorkersIter wi = m_assembly->m_workers.begin();
       wi != m_assembly->m_workers.end(); wi++)
    emitWorker(f, *wi);
  for (WorkersIter wi = dw->m_assembly->m_workers.begin();
       wi != dw->m_assembly->m_workers.end(); wi++)
    emitWorker(f, *wi);
  Instance *i, *di;
  unsigned nn, n;
  // For each app instance, we need to retrieve the index within the container
  // Then emit that app instance's info
  for (i = m_assembly->m_instances, n = 0; n < m_assembly->m_nInstances; n++, i++) {
    for (di = dw->m_assembly->m_instances, nn = 0; nn < dw->m_assembly->m_nInstances; nn++, di++) {
      if (di->name && !strcmp(di->name, i->name)) {
	i->index = di->index;
	break;
      }
    }
    if (nn >= dw->m_assembly->m_nInstances && !i->worker->m_noControl)
      return OU::esprintf("No instance in container assembly for assembly instance \"%s\"",
		      i->name);
    emitInstance(i, f);
  }
  // Now emit the container's instances
  for (di = dw->m_assembly->m_instances, nn = 0; nn < dw->m_assembly->m_nInstances; nn++, di++)
    if (di->worker)
      emitInstance(di, f);
  // Emit the connections between the container and the application
  // and within the container (adapters).
  for (ConnectionsIter cci = dw->m_assembly->m_connections.begin();
       cci != dw->m_assembly->m_connections.end(); cci++) {
    Connection *cc = NULL, *ac = NULL;
    for (ConnectionsIter aci = m_assembly->m_connections.begin();
	 aci != m_assembly->m_connections.end(); aci++)
      if ((*aci)->m_external && (*cci)->m_external &&
	  !strcmp((*aci)->m_name.c_str(), (*cci)->m_name.c_str())) {
	ac = *aci;
	cc = *cci;
	if (ac->m_external->m_instPort.m_port->u.wdi.isProducer) {
	  if (cc->m_external->m_instPort.m_port->u.wdi.isProducer)
	    return OU::esprintf("container connection \"%s\" has same direction (is producer) as "
				"application connection",
				cc->m_name.c_str());
	} else if (!ac->m_external->m_instPort.m_port->u.wdi.isBidirectional &&
		   !cc->m_external->m_instPort.m_port->u.wdi.isProducer &&
		   !cc->m_external->m_instPort.m_port->u.wdi.isBidirectional)
	  return OU::esprintf("container connection \"%s\" has same direction (is consumer) as "
			      "application connection",
			      cc->m_name.c_str());
	break;
      }
    if (!cc)
      continue;
    InstancePort *cip = NULL, *otherp = NULL;
    for (AttachmentsIter ai = cc->m_attachments.begin(); ai != cc->m_attachments.end(); ai++)
      if (&(*ai)->m_instPort != &cc->m_external->m_instPort) {
	cip = &(*ai)->m_instPort;
	break;
      }
    if (!cip)
      return OU::esprintf("container connection \"%s\" connects to no ports in the container",
			  cc->m_name.c_str());
    if (ac) {
      for (AttachmentsIter ai = ac->m_attachments.begin(); ai != ac->m_attachments.end(); ai++)
	if (&(*ai)->m_instPort != &ac->m_external->m_instPort) {
	  otherp = &(*ai)->m_instPort;
	  break;
	}
    } else
      // Internal connection
      for (AttachmentsIter ai = cc->m_attachments.begin(); ai != cc->m_attachments.end(); ai++)
	if (&(*ai)->m_instPort != cip) {
	  otherp = &(*ai)->m_instPort;
	  break;
	}
    assert(otherp != NULL);
    if (otherp->m_port->u.wdi.isProducer)
      // Application is producing to an external consumer
      fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
	      otherp->m_instance->name, otherp->m_port->name,
	      cip->m_instance->name, cip->m_port->name);
    else
      // Application is consuming from an external producer
      fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
	      cip->m_instance->name, cip->m_port->name,
	      otherp->m_instance->name, otherp->m_port->name);
  }
  // Emit the connections inside the application
  for (ConnectionsIter ci = m_assembly->m_connections.begin(); ci != m_assembly->m_connections.end(); ci++)
    if (!(*ci)->m_external) {
      InstancePort *from = 0, *to = 0;
      for (AttachmentsIter ai = (*ci)->m_attachments.begin(); ai != (*ci)->m_attachments.end(); ai++)
	if ((*ai)->m_instPort.m_port->u.wdi.isProducer)
	  from = &(*ai)->m_instPort;
        else
	  to = &(*ai)->m_instPort;
      fprintf(f, "<connection from=\"%s\" out=\"%s\" to=\"%s\" in=\"%s\"/>\n",
	      from->m_instance->name, from->m_port->name,
	      to->m_instance->name, to->m_port->name);
    }
  fprintf(f, "</artifact>\n");
#endif
  if (fclose(f))
    return "Could close output file. No space?";
#if 0
  if (wksFile)
    return dw->emitWorkersHDL(outDir, wksFile);
#endif
  return 0;
}

HdlAssembly *HdlAssembly::
create(ezxml_t xml, const char *xfile, const char *&err) {
  HdlAssembly *ha = new HdlAssembly(xml, xfile, err);
  if (err) {
    delete ha;
    ha = NULL;
  }
  return ha;
}

HdlAssembly::
HdlAssembly(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, NULL, err) {
  if (!(err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS, (void*)0)) &&
      !(err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, ASSY_ELEMS, (void*)0)))
  err = parseHdl(NULL);
}
HdlAssembly::
~HdlAssembly() {
}

