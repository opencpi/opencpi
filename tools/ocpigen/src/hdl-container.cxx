#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "hdl-container.h"
#include "hdl-assembly.h"

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
  HdlContainer *p = new HdlContainer(xml, xfile, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}


HdlContainer::
HdlContainer(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, NULL, NULL, err), m_appAssembly(NULL), m_config(NULL) {
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
  if (err || (err = parseFile(myConfig.c_str(), xfile, "HdlConfig", &xml, &configFile)) ||
      !(m_config = HdlConfig::create(xml, configFile, err)))
    return;
  if ((err = parseFile(myAssy.c_str(), xfile, "HdlAssembly", &xml, &assyFile)) ||
      !(m_appAssembly = HdlAssembly::create(xml, assyFile, err)) ||
      // This is inferred by a connection, but might have devide attributes someday
      (err = HdlPlatform::parseDevInstances(m_xml, m_config->platform(),
					    &m_config->devInstances(), m_devInstances, m_cards, true)))
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
  size_t nWCIs = 0; // count control ports as we go, to scale the ocscp at the end
  std::string assy;
  OU::format(assy, "<HdlContainerAssembly name='%s' language='vhdl'>\n", m_name.c_str());
  // The platform configuration instance
  OU::formatAdd(assy, "  <instance name='pfconfig' worker='%s'/>\n", configFile);
  // Connect the platform configuration to the control plane
  if (!m_config->m_noControl) {
    Port &p = *m_config->m_ports[0];
    OU::formatAdd(assy,
		  "  <connection count='%zu'>\n"
		  "    <port instance='ocscp' name='wci'/>\n"
		  "    <port instance='pfconfig' name='%s'/>\n"
		  "  </connection>\n",
		  p.count, p.name);
    nWCIs += p.count;
  }
  // Instance the assembly and connect its wci
  OU::formatAdd(assy, "  <instance worker='%s'/>\n", assyFile);
  // Connect the assembly to the control plane
  if (!m_appAssembly->m_noControl) {
    Port &p = *m_appAssembly->m_ports[0];
    OU::formatAdd(assy,
		  "  <connection count='%zu'>\n"
		  "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "  </connection>\n",
		  p.count, nWCIs,
		  m_appAssembly->m_implName, p.name);
    nWCIs += p.count;
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
	    if ((err = emitUNocConnection(assy, uNocs, nWCIs, c)))
	      return;
	    break;
	  }
	}
      }
  } else {
    for (DevInstancesIter di = m_devInstances.begin(); di != m_devInstances.end(); di++) {
      // Instance the device and connect its wci
      OU::formatAdd(assy, "  <instance name='%s' worker='%s'/>\n",
		    (*di).device.name().c_str(),
		    (*di).device.deviceType().name().c_str());
      OU::formatAdd(assy,
		    "  <connection>\n"
		    "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		    "    <port instance='%s' name='%s'/>\n"
		    "  </connection>\n",
		    nWCIs, (*di).device.name().c_str(),
		    (*di).device.deviceType().m_ports[0]->name);

      nWCIs++;
    }
    for (ContConnectsIter ci = connections.begin(); ci != connections.end(); ci++)
      if ((err = emitConnection(assy, uNocs, nWCIs, *ci)))
	return;
  }
  // Instance the scalable control plane
  OU::formatAdd(assy,
		"  <instance worker='ocscp'>\n"
		"    <property name='nworkers' value='%zu'/>\n"
		"  </instance>\n", nWCIs);
  // Connect it to the pf config's cpmaster
  for (PortsIter ii = m_config->m_ports.begin(); ii != m_config->m_ports.end(); ii++) {
    Port &i = **ii;
    if (i.master && i.type == CPPort) {
      OU::formatAdd(assy,
		    "  <connection>\n"
		    "    <port instance='pfconfig' name='%s'/>\n"
		    "    <port instance='ocscp' name='cp'/>\n"
		    "  </connection>\n",
		    i.name);
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
		      "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      index++, dev->m_name.c_str(), dt.m_ports[0]->name);
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
      emitTimeClient(assy, m_appAssembly->m_implName, (*pi)->name);
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
  if ((err = OE::ezxml_parse_str(copy, strlen(copy), x)))
    err = OU::esprintf("XML Parsing error on generated container assembly: %s", err);
  else {
    m_xml = x;
    err = parseHdl();
    m_assembly->m_isContainer = true;
  }
}

HdlContainer::
~HdlContainer() {
  delete m_config;
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
    c.devInstance = HdlPlatform::findDeviceInstance(attr, card, slot, m_config->devInstances());
    if (c.devInstance)
      c.devInConfig = true;
    else if (!(c.devInstance =
	       HdlPlatform::findDeviceInstance(attr, card, slot, m_devInstances)) &&
	     (err =
	      HdlPlatform::addDevInstance(attr, OE::ezxml_tag(cx), card, slot, false,
					  m_config->platform(), m_cards, m_devInstances,
					  c.devInstance)))
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

// Make a connection to an interconnect
const char *HdlContainer::
emitUNocConnection(std::string &assy, UNocs &uNocs, size_t &index, const ContConnect &c) {
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
		"  <instance name='%s_ocdp%u' worker='ocdp' interconnect='%s' configure='%u'>\n"
		"    <property name='includePull' value='%u'/>\n"
		"    <property name='includePush' value='%u'/>\n"
		"  </instance>\n"
		"  <connection>\n"
		"    <port instance='%s_ocdp%u' name='ctl'/>\n"
		"    <port instance='ocscp' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		c.interconnect->name, unoc, c.interconnect->name, unoc,
		1, // port->u.wdi.isProducer ? 0 : 1,
		1, // port->u.wdi.isProducer ? 1 : 0,
		c.interconnect->name, unoc,
		index);
  index++;
  // instantiate sma, and connect its wci
  OU::formatAdd(assy,
		"  <instance name='%s_sma%u' worker='sma' adapter='%s' configure='%u'/>\n"
		"  <connection>\n"
		"    <port instance='%s_sma%u' name='ctl'/>\n"
		"    <port instance='ocscp' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		c.interconnect->name, unoc, c.interconnect->name, 
		port->u.wdi.isProducer ? 2 : 1,
		c.interconnect->name, unoc,
		index);
  index++;
  // Connect the new unoc node to the unoc
  std::string prevInstance, prevPort;
  if (unoc == 0) {
    prevInstance = "pfconfig";
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
  emitTimeClient(assy, prevInstance.c_str(), "wti");
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
  size_t index = 0;
  m_config->emitWorkers(f);
  m_appAssembly->emitWorkers(f);
  emitWorkers(f);
  m_config->emitInstances(f, "p", index);
  m_appAssembly->emitInstances(f, "a", index);
  emitInstances(f, "c", index);
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
