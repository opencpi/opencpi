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

#include <stdint.h>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "HdlOCCP.h"
#include "assembly.h"
#include "hdl.h"
#include "hdl-container.h"
static void
emitTimeClient(std::string &assy, const char *instance, const char *portName, Port *port = NULL) {
  OU::formatAdd(assy,
		"  <instance worker='time_client%s' name='%s_%s_time_client'/>\n"
		"  <connection>\n"
		"    <port instance='%s_%s_time_client' name='wti'/>\n"
		"    <port instance='%s' name='%s'/>\n"
		"  </connection>\n"
		"  <connection>\n"
		"    <port instance='pfconfig' name='time'/>\n"
		"    <port instance='%s_%s_time_client' name='time'/>\n"
		"  </connection>\n",
		port && port->m_myClock && !port->m_clock->m_output ?  "_co" : "",
		instance, portName,
		instance, portName,
		instance, portName,
		instance, portName);
}

HdlContainer *HdlContainer::
create(ezxml_t xml, const char *xfile, const char *&err) {
  std::string myConfig, myPlatform, myAssy, myConstraints;
  OrderedStringSet platforms;
  // "only" is for backward compatibility
  if ((err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS, PLATFORM_ATTRS,
			    HDL_CONTAINER_ATTRS, "only", (void*)0)) ||
      (err = OE::checkElements(xml, HDL_CONTAINER_ELEMS, (void*)0)) ||
      (err = parsePlatform(xml, myConfig, myConstraints, platforms)))
    return NULL;
  if (platforms.empty()) {
    if (!g_platform)
      err = "No platform or platform configuration specified in HdlContainer";
  } else if (platforms.size() == 1) {
    if (g_platform) {
      if (platforms.front() != g_platform)
	err = OU::esprintf("platform specified as \"%s\" does not match container's \"%s\"",
			   g_platform, platforms.front().c_str());
    } else
      myPlatform = platforms.front();
  } else if (g_platform) {
    if (platforms.find(g_platform) == platforms.end())
      err = OU::esprintf("platform specified as \"%s\" is not one of container's platforms",
			 g_platform);
  } else
    err = "multiple platforms are possible for this container, but none was specified";
  if (err)
    return NULL;
  if (g_platform)
    myPlatform = g_platform;
  OE::getOptionalString(xml, myAssy, "assembly");
  if (myAssy.empty()) {
    if (assembly)
      myAssy = assembly;
    else {
      err = OU::esprintf("No assembly specified for container specified in %s", xfile);
      return NULL;
    }
  }
  std::string configFile, assyFile, configName;
  HdlConfig *config;
  HdlAssembly *appAssembly;
  ezxml_t x;
  OU::format(configName, "%s/hdl/%s", ::platformDir, myConfig.c_str());
  if ((err = parseFile(configName.c_str(), xfile, "HdlConfig", &x, configFile))) {
    configName = myPlatform + "/gen/" + myConfig;
    if (parseFile(configName.c_str(), xfile, "HdlConfig", &x, configFile))
      return NULL;
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

// When we add a client, we delay instantiating the split/join until it is actually needed.
// So this method actually is providing the split/join for the previous client if there is one
// If the current client is NULL, the current channel is terminated.
void UNoc::
addClient(std::string &assy, bool control, const char *client, const char *port) {
  UNocChannel &unc = m_channels[m_currentChannel];
  std::string prevInstance, prevPort, node;
  // The choices are:
  // 1. Is there a "previous client" (unc.m_currentNode != 0), remembered from a previous call?
  // 2. Are we adding a node (client != NULL) or just terminating (client == NULL).
  if (unc.m_currentNode) {
    // Deal with the previously added client now that we know everything we need to know,
    // namely whether it will be the last client or not (which SDP optimizes).
    unsigned pos = unc.m_currentNode - 1;
    if (pos == 0) { // the previous client is the first one so it attaches to the pfconfig
      prevInstance = "pfconfig";
      OU::format(prevPort, "name='%s' index='%u'", m_name, m_currentChannel);
    } else {
      OU::format(prevInstance, "%s_unoc%u_%u", m_name, m_currentChannel, pos - 1);
      prevPort = "name='down'";
    }
    if (client || m_type != SDPPort) {
      // If the channel we are using has a previous node or we are not SDP, then
      // instance its split/join node, and connect it upstream
      OU::format(node,  "%s_unoc%u_%u", m_name, m_currentChannel, pos);
      if (m_type == SDPPort)
	OU::formatAdd(assy,
		      "  <instance name='%s' worker='sdp_node'>\n"
		      "    <property name='sdp_width' value='%zu'/>\n"
		      "  </instance>\n",
		      node.c_str(), m_width);
      else {
	assert(unc.m_control || pos);
	OU::formatAdd(assy,
		      "  <instance name='%s' worker='unoc_node'>\n"
		      "    <property name='control' value='%s'/>\n"
		      "    <property name='position' value='%u'/>\n"
		      "  </instance>\n",
		      // We are assuming there is always an initial control node on unocs
		      node.c_str(), unc.m_control ? "true" : "false", 
		      unc.m_control ? 0 : pos - 1);
      }
      // Connect the inserted node to its upstream master and its client.
      OU::formatAdd(assy,
		    "  <connection>\n"
		    "    <port instance='%s' %s/>\n"
		    "    <port instance='%s' name='up'/>\n"
		    "  </connection>\n"
		    "  <connection>\n"
		    "    <port instance='%s' name='client'/>\n"
		    "    <port instance='%s' name='%s'/>\n"
		    "  </connection>\n",
		  prevInstance.c_str(), prevPort.c_str(), node.c_str(),
		  node.c_str(), unc.m_client.c_str(), unc.m_port.c_str());
    } else {
      // The previous client is the last client, and we are SDP.
      // Terminate the SDP channel by using the last client as the termination.
      // Connect the inserted node to its upstream master and its client.
      // FIXME:  set a parameter on the last client indicating its role as terminator
      OU::formatAdd(assy,
		    "  <connection>\n"
		    "    <port instance='%s' %s/>\n"
		    "    <port instance='%s' name='%s'/>\n"
		    "  </connection>\n",
		    prevInstance.c_str(), prevPort.c_str(), unc.m_client.c_str(),
		    unc.m_port.c_str());
    }
  }
  // if !client then terminate the channel
  if (client) {
    unc.m_control = control;
    unc.m_client = client;
    unc.m_port = port;
    unc.m_currentNode++;
    m_currentChannel = (m_currentChannel + 1) % (unsigned)m_channels.size();
  } else if (m_type != SDPPort || unc.m_currentNode == 0) {
    // Terminate a unoc channel or an empty SDP channel
    prevInstance = unc.m_currentNode == 0 ? "pfconfig" : node.c_str();
    if (unc.m_currentNode)
      prevPort = "name='down'";
    else
      OU::format(prevPort, "name='%s'", m_name);
    if (m_type == SDPPort && unc.m_currentNode == 0)
      OU::formatAdd(prevPort, " index='%u'", m_currentChannel);
    std::string term;
    OU::format(term, "unoc_term%u_%u",  m_currentChannel, unc.m_currentNode);
    OU::formatAdd(assy,
		  "  <instance worker='%s_term' name='%s'/>\n"
		  "  <connection>\n"
		  "    <port instance='%s' %s/>\n"
		  "    <port instance='%s' name='up'/>\n"
		  "  </connection>\n",
		  m_type == SDPPort ? "sdp" : "unoc", term.c_str(), prevInstance.c_str(),
		  prevPort.c_str(), term.c_str());
  }
}

void UNoc::
terminate(std::string &assy) {
  for (unsigned c = 0; c < m_channels.size(); c++) {
      m_currentChannel = c;
      addClient(assy, false, NULL, NULL);
  }
}



HdlContainer::
HdlContainer(HdlConfig &config, HdlAssembly &appAssembly, ezxml_t xml, const char *xfile,
	     const char *&err)
  : Worker(xml, xfile, "", Worker::Container, NULL, NULL, err),
    HdlHasDevInstances(config.m_platform, config.m_plugged, *this),
    m_appAssembly(appAssembly), m_config(config) {
  appAssembly.setParent(this);
  config.setParent(this);
  if (!g_platform)
    g_platform = m_config.platform().cname();
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
  Port
    *icp = NULL, // the interconnect port on the platform config, if one exists
    *cp = NULL;  // the CP master port on the platform config, if one exists
  for (PortsIter pi = m_config.m_ports.begin(); pi != m_config.m_ports.end(); pi++) {
    Port &p = **pi;
    if (p.m_master) {
      if (p.m_type == CPPort)
	cp = &p;
      else if (p.m_type == SDPPort || p.m_type == NOCPort) {
	icp = &p;
#if 0
	size_t len = p.m_name.length();

	Port *slave = NULL;
	for (PortsIter si = m_config.m_ports.begin(); si != m_config.m_ports.end(); si++) {
	  Port &sp = **si;
	  if (!sp.m_master && (sp.m_type == NOCPort || sp.m_type == SDPPort) &&
	      !strncasecmp(p.pname(), sp.pname(), len) && !strcasecmp(sp.pname() + len, "_slave")) {
	    assert(!slave);
	    slave = &sp;
	  }
	}
	ocpiAssert(p.m_type == SDPPort || slave);
#endif
	uNocs.insert(std::make_pair(p.pname(),
				    UNoc(p.pname(), p.m_type, m_config.sdpWidth(), p.m_count)));
      }
    }
  }
  // Preinstall device instances.  These may be devices in the platform OR may be
  // random devices that are just standalone workers.
#if 1
  if ((err = parseDevInstances(m_xml, xfile, this, &m_config.devInstances())))
    return;
#else
  for (ezxml_t dx = ezxml_cchild(m_xml, "device"); dx; dx = ezxml_cnext(dx)) {
    bool floating = false;
    if ((err = OE::getBoolean(dx, "floating", &floating)))
      return;
    std::string name;
    if (floating) {
      // This device is not part of the platform.
      // FIXME:  bad coding practice
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
#endif
  // Establish connections, WHICH MAY ALSO IMPLICITLY CREATE DEVICE INSTANCES
  ContConnects connections;
  for (ezxml_t cx = ezxml_cchild(m_xml, "connection"); cx; cx = ezxml_cnext(cx)) {
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
  OU::formatAdd(assy, "  </instance>\n");
  // Connect the platform configuration to the control plane
  if (!m_config.m_noControl) {
    Port &p = *m_config.m_ports[0];
    OU::formatAdd(assy,
		  "  <connection count='%zu'>\n"
		  "    <port instance='ocscp' name='wci'/>\n"
		  "    <port instance='pfconfig' name='%s'/>\n"
		  "  </connection>\n",
		  p.m_count, p.pname());
    nWCIs += p.m_count;
  }
  if (m_appAssembly.m_assembly && m_appAssembly.m_assembly->m_instances.size() != 0) {
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
		    p.m_count, nWCIs,
		    m_appAssembly.m_implName, p.pname());
      nWCIs += p.m_count;
    }
  }
  if (icp && !cp) {
    UNoc &unoc = uNocs.at(icp->pname());
    std::string client;
    const char *icPort;
    OU::format(client, "%s_unoc2cp", icp->pname());
    if (icp->m_type == SDPPort) {
      OU::formatAdd(assy,
		    "  <instance name='%s' worker='sdp2cp'>\n"
		    "    <property name='sdp_width' value='%zu'/>\n"
		    "  </instance>\n",
		    client.c_str(), m_config.sdpWidth());

      icPort = "sdp";
    } else {
      OU::formatAdd(assy, "  <instance name='%s' worker='unoc2cp'/>\n", client.c_str());
      icPort = "client";
    }
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='cp'/>\n"
		  "    <port instance='ocscp' name='cp'/>\n"
		  "  </connection>\n",
		  client.c_str());
    unoc.addClient(assy, true, client.c_str(), icPort);
  } else
    // Connect it to the pf config's cpmaster
    for (PortsIter ii = m_config.m_ports.begin(); ii != m_config.m_ports.end(); ii++) {
      Port &i = **ii;
      if (i.m_master && i.m_type == CPPort) {
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='pfconfig' name='%s'/>\n"
		      "    <port instance='ocscp' name='cp'/>\n"
		      "  </connection>\n",
		      i.pname());
	break;
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
			       (*pi)->pname());
	  return;
	}
	for (PortsIter ii = m_config.m_ports.begin(); ii != m_config.m_ports.end(); ii++) {
	  Port &i = **ii;
	  if (i.m_master && (i.m_type == NOCPort || i.m_type == SDPPort)) {
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
    for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
      const DevInstance &di = *dii;
      const DeviceType &dt = di.device.deviceType();
      // Decide whether to map the signals or not, based on whether we are an emulator
      // or are paired with an emulator
      const DevInstance *emulator = NULL;
      for (DevInstancesIter edi = m_devInstances.begin(); edi != m_devInstances.end(); edi++)
	if ((*edi).device.deviceType().m_emulate &&
	    !strcasecmp((*edi).device.deviceType().m_emulate->m_implName, dt.m_implName)) {
	  emulator = &*edi;
	  break;
	}
      // Instance the device and connect its wci
      // FIXME this is copied from hdl-config - consolidate
      OU::formatAdd(assy, "  <instance name='%s' worker='%s'%s>\n",
		    di.cname(), dt.cname(), emulator ? " emulated='1'" : "");
      if (di.m_instancePVs.size()) {
	const OU::Assembly::Property *ap = &di.m_instancePVs[0];
	for (size_t n = di.m_instancePVs.size(); n; n--, ap++)
	  OU::formatAdd(assy, "    <property name='%s' value='%s'/>\n",
			ap->m_name.c_str(), ap->m_value.c_str());
      }
      if (!emulator && !dt.m_emulate)
	mapDevSignals(assy, di, true);
      assy += "  </instance>\n";
      if (!dt.m_noControl) {
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      nWCIs, di.cname(),
		      dt.ports()[0]->pname());
	nWCIs++;
      }
      // Instance time clients for the device
      for (auto pi = dt.ports().begin(); pi != dt.ports().end(); ++pi) {
        Port &p = **pi;
        if (p.m_type == WTIPort)
          emitTimeClient(assy, di.cname(), p.pname(), &p);
      }
    }
    for (ContConnectsIter ci = connections.begin(); ci != connections.end(); ci++)
      if ((err = emitConnection(assy, uNocs, nWCIs, *ci)))
	return;
    emitSubdeviceConnections(assy, &m_config.m_devInstances);
  }
  // Instance the scalable control plane and adapter to SDP/uNoc if present.
  OU::formatAdd(assy,
		"  <instance worker='ocscp'>\n"
		"    <property name='nworkers' value='%zu'/>\n"
		"    <property name='ocpi_endian' value='%s'/>\n"
		"  </instance>\n", nWCIs,
		endians[m_endian]);
  // Terminate the uNocs
  for (UNocsIter ii = uNocs.begin(); ii != uNocs.end(); ii++)
    ii->second.terminate(assy);
  // Instance time clients for the assembly
  for (auto pi = m_appAssembly.m_ports.begin(); pi != m_appAssembly.m_ports.end(); ++pi) {
    Port &p = **pi;
    if (p.m_type == WTIPort)
      emitTimeClient(assy, m_appAssembly.m_implName, p.pname(), &p);
  }
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
  // The (inherited) worker is updated to have the xml for the assembly we just generated.
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
      if (ssi != sl.m_signals.end() && ssi->second.c_str()[0] == '/')
	sig.m_name = &ssi->second.c_str()[1];
      else
	sig.m_name = sl.m_prefix + (ssi == sl.m_signals.end() ? (*si)->cname() :
				    ssi->second.c_str());
      if (!m_sigmap.findSignal(sig.m_name)) {
	m_signals.push_back(&sig);
	m_sigmap[sig.cname()] = &sig;
	ocpiDebug("Creating container signal from slot signal: %s", sig.cname());
      }
    }
  }
  m_xml = x;
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
	  bool isSingle = false; // slow down to suppress warning
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
  // During the parsing of the container assembly we KNOW what the platform is,
  // but the platform config XML that might be parsed might think it is defaulting
  // from the platform where it lives, so we temporarily set the global to the
  // platform we know.
  const char *save = g_platform;
  g_platform = m_platform.cname();
  if ((err = parseHdl()))
    return;
  g_platform = save;
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
      if (!strcasecmp((*pi)->pname(), attr)) {
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
      if (!strcasecmp((*pi)->pname(), attr)) {
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
  //TODO: There should be much more code sharing between device and otherdevice 
  c.otherDevInConfig = false;
  if ((attr = ezxml_cattr(cx, "otherdevice")) &&
      (err = parseDevInstance(attr, cx, m_file.c_str(), this, false, &m_config.devInstances(),
			      &c.otherDevInstance, &c.otherDevInConfig)))
    return err;
  ocpiDebug("attr is: %s",attr);
  if ((attr = ezxml_cattr(cx, "otherport"))) {
    ocpiDebug("Found otherport");
    if (!c.otherDevInstance)
      return OU::esprintf("Port '%s' specified without specifying a device", attr);
    const ::Device &d = c.otherDevInstance->device;
    for (PortsIter pi = d.deviceType().ports().begin();
	 pi != d.deviceType().ports().end(); pi++)
      if (!strcasecmp((*pi)->pname(), attr)) {
	c.otherPort = *pi;
	break;
      }
    if (!c.otherPort)
      return OU::esprintf("Port '%s' not found for device '%s'", attr, d.cname());
    if (!c.otherPort->isData())
      return OU::esprintf("Port '%s' for device '%s' is not a data port", attr, d.cname());
  } else if (c.otherDevInstance) {
    ocpiDebug("Did not find otherport");
    const ::Device &d = c.otherDevInstance->device;
    // Find the one data port
    for (PortsIter pi = d.deviceType().ports().begin();
	 pi != d.deviceType().ports().end(); pi++)
      if ((*pi)->isData()) {
	if (c.otherPort)
	  return OU::esprintf("There are multiple data ports for device '%s'; you must specify one",
			      d.cname());
	c.otherPort = *pi;
      }
    if (!c.otherPort)
      return OU::esprintf("There are no data ports for device '%s'", d.cname());
  }
  if ((attr = ezxml_cattr(cx, "interconnect"))) {
    // An interconnect can be on any device worker, but for now it is on the config.
    for (PortsIter pi = m_config.m_ports.begin(); pi != m_config.m_ports.end(); pi++) {
      Port &p = **pi;
      if (!strcasecmp(p.pname(), attr) ||
	  (!strcmp(attr, "*") && p.m_master && (p.m_type == NOCPort || p.m_type == SDPPort))) {
	c.interconnect = &p;
	break;
      }
    }
    if (!c.interconnect ||
	(c.interconnect->m_type != NOCPort && c.interconnect->m_type != SDPPort) ||
	!c.interconnect->m_master)
      return OU::esprintf("Interconnect '%s' not found for platform '%s'", attr,
			   m_config.platform().cname());
  }
  return NULL;
}

// Make a connection to an interconnect
const char *HdlContainer::
emitUNocConnection(std::string &assy, UNocs &uNocs, size_t &index, const ContConnect &c) {
    // Find uNoc
  const char *iname = c.interconnect->pname();
  UNoc &unoc = uNocs.at(iname);
  UNocChannel &unc = unoc.m_channels[unoc.m_currentChannel];
  Port *port = c.external ? c.external : c.port;
  if (port->m_type != WSIPort ||
      (c.interconnect->m_type != NOCPort && c.interconnect->m_type != SDPPort) ||
      !c.interconnect->m_master)
    return OU::esprintf("unsupported container connection between "
			"port %s of %s%s and interconnect %s",
			port->pname(), iname,
			c.external ? "assembly" : "device",
			c.external ? "" : c.devInstance->device.cname());
  std::string dma, sma, ctl;
  const char *unocPort;
  if (c.interconnect->m_type == SDPPort) {
    unocPort = "sdp";
    OU::format(dma, "%s_sdp_%s%u_%u", iname, port->isDataProducer() ? "send" : "receive",
	       unoc.m_currentChannel, unc.m_currentNode);
    sma = dma;
    ctl = dma;
    // Create a sender or receiver DP/DMA module to stream to/from another place on the
    // interconnect.
    OU::formatAdd(assy,
		  "  <instance name='%s' worker='sdp_%s' interconnect='%s'>\n"
		  "    <property name='sdp_width' value='%zu'/>\n"
		  "  </instance>\n",
		  dma.c_str(), port->isDataProducer() ? "send" : "receive", iname,
		  m_config.sdpWidth());
    // Instance a pipeline node upstream and connect it to the dma module
    // FIXME make this conditional
    std::string pipeline;
    OU::format(pipeline, "%s_sdp_pipeline%u_%u",
	       iname, unoc.m_currentChannel, unc.m_currentNode);
    unocPort = "up";
    OU::formatAdd(assy,
		  "  <instance name='%s' worker='sdp_pipeline'>\n"
		  "    <property name='sdp_width' value='%zu'/>\n"
		  "  </instance>\n"
		  "  <connection>\n"
		  "    <port instance='%s' name='down'/>\n"
		  "    <port instance='%s' name='sdp'/>\n"
		  "  </connection>\n",
		  pipeline.c_str(), m_config.sdpWidth(), pipeline.c_str(), dma.c_str());
    dma = pipeline;
  } else {
    OU::format(dma, "%s_ocdp%u_%u", iname, unoc.m_currentChannel, unc.m_currentNode);
    OU::format(sma, "%s_sma%u_%u",  iname, unoc.m_currentChannel, unc.m_currentNode);
    unocPort = "client";
    // Create the three instances:
    // 1. A unoc node to use the interconnect's unoc
    // 2. A DP/DMA module to stream to/from another place on the interconnect
    // 3. An SMA to adapt the WMI on the DP to the WSI that is needed (for now).
    OU::formatAdd(assy,
		  "  <instance name='%s' worker='sma' adapter='%s' configure='%u'/>\n"
		  "  <instance name='%s' worker='ocdp' interconnect='%s' configure='%u'>\n"
		  "    <property name='includePull' value='%u'/>\n"
		  "    <property name='includePush' value='%u'/>\n"
		  "  </instance>\n",
		  sma.c_str(), iname, port->isDataProducer() ? 2 : 1,
                  dma.c_str(), iname, unc.m_currentNode,
		  1, // port->u.wdi.isProducer ? 0 : 1,
		  1); // port->u.wdi.isProducer ? 1 : 0

    // connect the SMA to the WCI and connect the DP to the SMA, increment WCI count
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='ctl'/>\n"
		  "    <port instance='ocscp' name='wci' index='%zu'/>\n"
		  "  </connection>\n"
		  "  <connection>\n"
		  "    <port instance='%s' %s='data'/>\n"
		  "    <port instance='%s' %s='message'/>\n"
		  "  </connection>\n",
		  sma.c_str(), index++,
		  dma.c_str(), port->isDataProducer() ? "to" : "from",
		  sma.c_str(), port->isDataProducer() ? "from" : "to");
    // Add time client to OCDP's WTI port
    emitTimeClient(assy, dma.c_str(), "wti");
    ctl = dma;
  }
  // Connect the dma to the wci, incrementing the WCI count
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='ctl'/>\n"
		"    <port instance='ocscp' name='wci' index='%zu'/>\n"
		"  </connection>\n",
		ctl.c_str(), index++);
  // Connect to the port
  std::string other;
  if (c.devInConfig)
    OU::format(other, "%s_%s", c.devInstance->cname(), port->pname());
  else
    other = port->pname();
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' %s='%s'/>\n"
		"    <port instance='%s' %s='%s'/>\n"
		"  </connection>\n",
		sma.c_str(),
		port->isDataProducer() ? "to" : "from",
		port->isDataProducer() ? "in" : "out",
		c.external ? m_appAssembly.m_implName :
		(c.devInConfig ? "pfconfig" : c.devInstance->cname()),
		port->isDataProducer() ? "from" : "to",
		other.c_str());
  unoc.addClient(assy, false, dma.c_str(), unocPort);
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
      OU::format(devport, "%s_%s", c.devInstance->cname(), c.port->pname());
    OU::formatAdd(assy,
		  "  <connection>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "    <port instance='%s' name='%s'/>\n"
		  "  </connection>\n",
		  m_appAssembly.m_implName, c.external->pname(),
		  c.devInConfig ? "pfconfig" : c.devInstance->cname(),
		  c.devInConfig ? devport.c_str() : c.port->pname());
  } else if (c.devInstance && c.otherDevInstance) {
    // We need to connect a port of a device instance to another device.
    std::string devport, otherDevport;
    if (c.devInConfig)
      OU::format(devport, "%s_%s", c.devInstance->cname(), c.port->pname());
    if (c.otherDevInConfig)
      OU::format(otherDevport, "%s_%s", c.otherDevInstance->cname(), c.otherPort->pname());
    OU::formatAdd(assy,
  		  "  <connection>\n"
  		  "    <port instance='%s' name='%s'/>\n"
  		  "    <port instance='%s' name='%s'/>\n"
  		  "  </connection>\n",
  		  c.devInConfig ? "pfconfig" : c.devInstance->cname(),
  		  c.devInConfig ? devport.c_str() : c.port->pname(),
  		  c.otherDevInConfig ? "pfconfig" : c.otherDevInstance->cname(),
  		  c.otherDevInConfig ? otherDevport.c_str() : c.otherPort->pname());
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
	(!strcasecmp(a.m_instPort.m_port->worker().m_implName, m_appAssembly.m_implName) ? ap :
	 !strcasecmp(a.m_instPort.m_port->worker().m_implName, m_config.m_implName) ? pp : ip)
	  = &a.m_instPort;
      }
      assert(ap || pp || ip);
      if (!ap && !pp)
	continue; // internal connection already dealt with
      // find the corresponding instport inside each side.
      InstancePort
	*aap = ap ? m_appAssembly.m_assembly->findInstancePort(ap->m_port->pname()) : NULL,
        *ppp = pp ? m_config.m_assembly->findInstancePort(pp->m_port->pname()) : NULL,
	*producer = (aap && aap->m_port->isDataProducer() ? aap :
		     ppp && ppp->m_port->isDataProducer() ? ppp : ip),
	*consumer = (aap && !aap->m_port->isDataProducer() ? aap :
		     ppp && !ppp->m_port->isDataProducer() ? ppp : ip);
      // Application is producing to an external consumer
      fprintf(f, "<connection from=\"%s/%s\" out=\"%s\" to=\"%s/%s\" in=\"%s\"/>\n",
	      producer == ip ? "c" : producer == aap ? "a" : "p",
	      producer->m_instance->cname(), producer->m_port->pname(),
	      consumer == ip ? "c" : consumer == aap ? "a" : "p",
	      consumer->m_instance->cname(), consumer->m_port->pname());
    }
  }
}

// inContainer means the device is instanced in the container as opposed to
// in the platform configuration.  If in a platform configuration the device signals
// are actually signals of the platform configuration worker.
void HdlContainer::
mapDevSignals(std::string &assy, const DevInstance &di, bool inContainer) {
  const Signals
    &devSigs = di.device.deviceType().m_signals,
    &instSigs = di.m_worker->m_signals;
  for (SignalsIter s = devSigs.begin(), i = instSigs.begin(); s != devSigs.end(); ++s, ++i) {
    assert((*s)->m_name == (*i)->m_name);
    if ((*i)->m_direction == Signal::UNUSED)
      continue;
    for (unsigned n = 0; (*s)->m_width ? n < (*s)->m_width : n == 0; n++) {
      // (Re)create the signal name of the pf_config signal
      const char *boardName;
      bool isSingle;
      if ((boardName = di.device.m_dev2bd.findSignal(**s, n, isSingle))) {
	std::string devSig = (*s)->cname();
	if ((*s)->m_width && isSingle)
	  OU::formatAdd(devSig, "(%u)", n);
	std::string dname, ename;
	if (di.slot && !inContainer)
	  OU::format(dname, "%s_%s_%s", di.slot->m_name.c_str(), di.device.cname(), devSig.c_str());
	else if (inContainer || di.device.m_deviceType.m_type == Worker::Platform)
	  dname = devSig.c_str();
	else
	  OU::format(dname, "%s_%s", di.device.cname(), devSig.c_str());
	if (*boardName && di.slot) {
	  Signal *slotSig = di.device.m_board.m_extmap.findSignal(boardName);
	  assert(slotSig);
	  Slot::SignalsIter ssi = di.slot->m_signals.find(slotSig);
	  // Only set ename if this slot's signal is available on the platform.
	  // I.e. that pin of the slot might not be connected to a signal available to the FPGA
	  // in which case the device worker's signal will be unconnected.
	  if (ssi != di.slot->m_signals.end() && ssi->second.c_str()[0] == '/')
	    ename = &ssi->second.c_str()[1];
	  else if (ssi == di.slot->m_signals.end() || ssi->second.c_str()[0])
	    OU::format(ename, "%s%s", di.slot->m_prefix.c_str(),
		       ssi == di.slot->m_signals.end()  ?
		       slotSig->cname() : ssi->second.c_str());
	} else
	  ename = boardName;
	//	if (ename.length())
	OU::formatAdd(assy, "    <signal name='%s' external='%s'/>\n",
		      dname.c_str(), ename.c_str());
      } else {
	Signal *ns = new Signal(**i);
	if (di.device.deviceType().m_type != Worker::Platform)
	  OU::format(ns->m_name, "%s_%s", di.device.cname(), ns->cname());
	m_signals.push_back(ns);
	m_sigmap[ns->m_name.c_str()] = ns;
	break;
      }
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
  memcpy(uuidRegs.uuid, uuid.uuid, sizeof(uuidRegs.uuid));
  uuidRegs.birthday = (uint32_t)time(0);
  strncpy(uuidRegs.platform, g_platform, sizeof(uuidRegs.platform));
  strncpy(uuidRegs.device, g_device, sizeof(uuidRegs.device));
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
emitDeviceSignalMapping(FILE *f, std::string &last, Signal &s, const char *prefix) {
  std::string name;
  if (s.m_direction == Signal::INOUT)
    fprintf(f, "%s      %s => %s", last.c_str(), s.m_name.c_str(), s.m_name.c_str());
  else
    Worker::emitDeviceSignalMapping(f, last, s, prefix);
}
void HdlContainer::
emitDeviceSignal(FILE *f, Language lang, std::string &last, Signal &s, const char *prefix) {
  if (s.m_direction == Signal::INOUT)
    // Inouts are different for containers since the tristate IOBUF is inserted.
    emitSignal(s.m_name.c_str(), f, lang, s.m_direction, last,
	       s.m_width ? (int)s.m_width : -1, 0, "", s.m_type);
  else
    Worker::emitDeviceSignal(f, lang, last, s, prefix);
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
      case Signal::UNUSED: // not really supported properly
	fprintf(f, "  -- Signal \"%s\" is unused and unconnected in this module\n",
		s.cname());
	break;
      default:
	assert("Unexpected signal type for assembly tieoff" == 0);
      }
    } else if (s.m_direction == Signal::INOUT && !s.m_pin) {
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

// This is to help the container build scripts extract the platform
// and platform configuration from a container XML file without fully parsing the container.
// I.e. the only error checks done are the simple lexical check on this XML file,
// and the correct top level element and platform attributes.  Any other errors in this file
// or files it references will be generated later, in a better place to report them.
// If onlyValidPlatforms is true, make sure each platform is valid.
// A static method.
const char *HdlContainer::
parsePlatform(ezxml_t xml, std::string &config, std::string &constraints,
	      OrderedStringSet &platforms, bool onlyValidPlatforms) {
  const char
    *err,
    *pf = ezxml_cattr(xml, "platform"),
    *cf = ezxml_cattr(xml, "config"),
    *csf = ezxml_cattr(xml, "constraints"),
    *only = ezxml_cattr(xml, "only"),
    *exclude = ezxml_cattr(xml, "exclude");
  if (!only)
    only = ezxml_cattr(xml, "onlyplatforms");
  if (!exclude)
    exclude = ezxml_cattr(xml, "excludeplatforms");
  if ((pf ? 1 : 0) + (only ? 1 : 0) + (exclude ? 1 : 0) > 1)
    return "only one attribute of \"platform\", \"only\" or \"exclude\" is allowed";
  if (cf && strchr(cf, '/'))
    return OU::esprintf("invalid platform configuration name: \"%s\"", cf);
  std::string p;
  if (pf) {
    const char *slash = strchr(pf, '/');
    if (slash) {
      if (cf)
	return "specifying both \"config\" attribute and config after slash in \"platform\" "
	  "attribute is invalid";
      p.assign(pf, OCPI_SIZE_T_DIFF(slash, pf));
      cf = slash + 1;
    } else
      p = pf;
    if (p.length() > 3 && !strcmp(p.c_str() + p.length() - 3, "_pf"))
      p.resize(p.length() - 3);
    only = p.c_str();
  }
  if (only) {
    if ((err = getPlatforms(only, platforms, HdlModel, onlyValidPlatforms)))
      return err;
  } else if (exclude) {
    OrderedStringSet excludedPlatforms, allPlatforms;
    const StringSet *hdlPlatforms;
    if ((err = getPlatforms(exclude, excludedPlatforms, HdlModel, onlyValidPlatforms)) ||
	(err = getAllPlatforms(hdlPlatforms, HdlModel)))
      return err;
    for (auto pi = hdlPlatforms->begin(); pi != hdlPlatforms->end(); ++pi)
      if (excludedPlatforms.find(*pi) == excludedPlatforms.end())
	platforms.push_back(*pi);
  }
  config = cf ? cf : "base";
  constraints = csf ? csf : "-";
  return NULL;
}
