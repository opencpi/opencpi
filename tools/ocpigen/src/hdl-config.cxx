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
  : device(d), card(c), slot(s), m_control(control), m_parent(parent), m_worker(NULL) {
  m_connected.resize(d.m_deviceType.m_ports.size(), 0);
  if (slot) {
    m_name = slot->cname();
    m_name += "_";
    m_name += d.cname();
  } else
    m_name = d.cname();
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
	       bool control, const DevInstance *parent, DevInstances */*baseInstances*/,
	       ezxml_t xml, const DevInstance *&devInstance) {
  const char *err;
  m_devInstances.push_back(DevInstance(dev, card, slot, control, parent));
  assert((card && slot) || (!card && !slot));
  assert(!slot || !m_plugged[slot->m_ordinal] || card == m_plugged[slot->m_ordinal]);
  if (slot && !m_plugged[slot->m_ordinal])
    m_plugged[slot->m_ordinal] = card;
  DevInstance &di = m_devInstances.back();
#if 0
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
	if (!sdi && (err = addDevInstance(**bi, card, slot, control/* why? */, &di, NULL,
					  NULL, sdi)))
	  return err;
      }
#endif
  devInstance = &di;
  if ((err = dev.m_deviceType.parseDeviceProperties(xml, di.m_instancePVs)))
    return err;
  // Now that we have the instancePVs, we can create a worker that is parameterized for this
  // device instance;
  di.m_worker = Worker::create(dev.m_deviceType.m_file.c_str(), m_parent.m_file.c_str(), NULL, NULL, &m_parent,
			       &di.m_instancePVs, 0, err);
  return err;
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
		       plug->cname(), card->cname(), slot->cname());
    } else if (plug)
      card = plug;
    else
      OU::esprintf("No card specified for slot \"%s\"", slot->cname());
  } else if (card) {
    // Find a slot...
    switch (m_platform.slots().size()) {
    case 0:
      return OU::esprintf("Card \"%s\" specified when platform has no slots",
			  card->cname());
    case 1:
      slot = m_platform.slots().begin()->second;
      if (slot->type() != card->type())
	return OU::esprintf("Card \"%s\" has slot type \"%s\", which is not in the platform",
			    card->cname(), slot->type()->cname());
      break;
    default:
      for (SlotsIter si = m_platform.slots().begin(); si != m_platform.slots().end(); si++)
	if ((*si).second->type() == card->type()) {
	  if (slot)
	    return OU::esprintf("Multiple slots are possible for card \"%s\"",
				card->cname());
	  else
	    slot = (*si).second;
        }
    }
  }
  const Device *dev;
  if (card) {
    if (!(dev = card->findDevice(device)))
      return OU::esprintf("There is no device named \"%s\" on \"%s\" cards",
			  device, card->cname());
  } else if (!(dev = m_platform.findDevice(device)))
    return OU::esprintf("There is no device named \"%s\" on this platform",
			device);

  const DevInstance *di;
  assert((card && slot) || (!card && !slot));
  if (control && !dev->m_deviceType.m_canControl)
    return OU::esprintf("Device '%s' cannot have control since its type (%s) cannot",
			dev->cname(), dev->deviceType().cname());
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
		     di->device.cname(), di->card->cname(), di->slot->cname());
    else
      return OU::esprintf("Platform device '%s' is already in the platform configuration",
			  di->device.cname());
  }
  if ((err = addDevInstance(*dev, card, slot, control, NULL, baseInstances, x, di)))
    return err;
  if (result)
    *result = di;
  return NULL;
}

// Parse the properties (parameters) that should be applied to this instance.
// This is used both for specifying a device's properties on the board (platform or card),
// as well as additional properties for a device instance.  The properties from the base
// device are copied to the instance so they are all in one place, but an instance cannot
// override the parameters of the device as defined for the board.
const char *DeviceType::
parseDeviceProperties(ezxml_t x, OU::Assembly::Properties &iPVs) {
  const char *err;
  // First pass for error checking
  for (ezxml_t px = ezxml_cchild(x, "Property"); px; px = ezxml_cnext(px)) {
    std::string l_name;
    if ((err = OE::checkAttrs(px, "name", "value", "valuefile", NULL)) ||
	(err = OE::getRequiredString(px, l_name, "name", "property")))
      return err;
    OU::Property *p = findProperty(l_name.c_str());
    if (!p)
      return OU::esprintf("\"%s\" is not a property of device \"%s\"",
			  l_name.c_str(), m_implName);
    if (!p->m_isParameter)
      return OU::esprintf("\"%s\" is not a parameter property of device \"%s\"",
			  l_name.c_str(), m_implName);
  }
  assert(!iPVs.size());
  iPVs.resize(m_ctl.nParameters);
  unsigned nPVs = 0;
  for (PropertiesIter pi = m_ctl.properties.begin(); pi != m_ctl.properties.end(); pi++)
    if ((*pi)->m_isParameter) {
      OU::Property &p = **pi;
      for (ezxml_t px = ezxml_cchild(x, "Property"); px; px = ezxml_cnext(px))
	if (!strcasecmp(p.m_name.c_str(), ezxml_cattr(px, "name"))) {
	  if ((err = iPVs[nPVs].parse(px)))
	    return err;
	  for (unsigned i = 0; i < m_instancePVs.size(); i++)
	    if (!strcasecmp(p.m_name.c_str(), m_instancePVs[i].m_name.c_str()) &&
		iPVs[nPVs].m_value != m_instancePVs[i].m_value)
	      return OU::esprintf("device instance of device type \"%s\" cannot "
				  "override the parameter \"%s\" that is already specified "
				  "for the platform or card",
				  m_implName, p.m_name.c_str());
	  // This parameter not specified for the board so we can use it
	  nPVs++;
	  goto nextParam;
	}
      // For this parameter, nothing is specified in the instance.  If it is specified for
      // for the device, copy it to the instance.
      for (unsigned i = 0; i < m_instancePVs.size(); i++)
	if (!strcasecmp(p.m_name.c_str(), m_instancePVs[i].m_name.c_str())) {
	  iPVs[nPVs++] = m_instancePVs[i];
	  break;
	}
    nextParam:;
    }
  iPVs.resize(nPVs);
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
  for (ezxml_t xd = ezxml_cchild(xml, "Device"); !err && xd; xd = ezxml_cnext(xd)) {
    std::string name;
    bool control = false;
    bool floating = false;
    // FIXME: valid attributes/elements depend on "floating"
    if ((err = OE::checkAttrs(xd, "name", "control", "slot", "card", "floating", "worker",
			      (void*)0)) ||
	(err = OE::checkElements(xd, "property", "signal", (void*)0)) ||
	(err = OE::getBoolean(xd, "control", &control)) ||
	(err = OE::getBoolean(xd, "floating", &floating)))
      return err;
    if (floating) {
      if (!baseInstances)
	return "floating devices (not part of platform) not allowed in platform configurations";
      // FIXME:  Apologies for this gross unconsting, but its the least of various evils
      // Fixing would involve allowing containers to own devices...
      HdlPlatform &pf = *(HdlPlatform *)&m_platform;
      if (!(err = OE::checkAttrs(xd, "floating", "worker", (void*)0)) &&
	  !(err = OE::checkElements(xd, "property", "signal", (void*)0)))
	err = pf.addFloatingDevice(xd, parentFile, parent, name);
    } else if (!(err = OE::checkAttrs(xd, "name", "control", "slot", "card", "floating",
				      (void*)0)) &&
	       !(err = OE::checkElements(xd, "property", (void*)0)))
      err = OE::getRequiredString(xd, name, "name", "device");
    if (!err)
      err = parseDevInstance(name.c_str(), xd, parentFile, parent, control, baseInstances, NULL,
			     NULL);
  }
  if (err)
    return err;
  // After the first pass, which creates explicit device instances that might have parameters,
  // we auto-instantiate devices that support the ones we have
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const DevInstance &di = *dii;
    // See which (sub)devices on the same board support this added device, 
    // and make sure they are present.
    const Board &bd =
      di.card ? static_cast<const Board&>(*di.card) : static_cast<const Board&>(m_platform);
    for (DevicesIter bi = bd.m_devices.begin(); bi != bd.m_devices.end(); bi++)
      for (SupportsIter si = (*bi)->m_deviceType.m_supports.begin();
	   si != (*bi)->m_deviceType.m_supports.end(); si++)
	// FIXME: use the package name here...
	//      if (&(*si).m_type == &dev.m_deviceType && // the sdev supports this TYPE of device
	if (!strcasecmp((*si).m_type.m_implName, di.device.m_deviceType.m_implName) &&
	    (*bi)->m_ordinal == di.device.m_ordinal) { // the ordinals match. FIXME allow mapping
	  const DevInstance *sdi = findDevInstance(**bi, di.card, di.slot, baseInstances, NULL);
	  if (!sdi && (err = addDevInstance(**bi, di.card, di.slot, false, &di, NULL, NULL, sdi)))
	    return err;
      }
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
	for (auto si = s.m_deviceType.m_supports.begin();
	     si != s.m_deviceType.m_supports.end(); ++si) {
	  if (strcasecmp((*si).m_type.m_implName, d.m_deviceType.m_implName))
	    continue;
	  // The device type of this supports element matches the device type of the
	  // devinstance (*dii).  Now we see if it is for this PARTICULAR devinstance.
	  // If there are no mapping entries, then we use the implicit ordinal of the
	  // supports element among supports elements for that device type
	  if (s.m_supportsMap.empty()) {
	    if (si->m_ordinal != d.m_ordinal)
	      continue;
	  } else {
	    auto it =
	      s.m_supportsMap.find(std::make_pair(d.m_deviceType.m_implName, si->m_ordinal));
	    if (it == s.m_supportsMap.end() || it->second != d.m_name)
	      continue;
	  }
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
	Port &supPort = *(*sci).m_sup_port;
	// A port connection between the supporting device and this device instance (*dii)
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "    <port instance='%s' name='%s%s%s'",
		      (*dii).cname(), (*sci).m_port->pname(),
		      inConfig ? "pfconfig" : sdi->cname(),
		      inConfig ? sdi->cname() : "", inConfig ? "_" : "",
		      supPort.pname());
	// If this <connect> element expresses an index, make sure to account for
	size_t supOrdinal = supPort.m_ordinal;
	if ((*sci).m_indexed) { // <supports><connect> connection has an index
	  size_t
	    supIndex = (*sci).m_index,
	    unconnected = 0,
	    index = supIndex;
	  if (inConfig) { // 
	    // If we are in a container and the subdevice is in the config,
	    // we may need to index relative to what is NOT connected in the config,
	    // and thus externalized.
	    // We ASSUME that the indices in the config are contiguous and start at 0
	    // FIXME:  remove this constraint
	    for (size_t i = 0; i < supPort.count(); i++)
	      if (sdi->m_connected[supOrdinal] & (1u << i)) {
		assert(i != supIndex);
		if (i < supIndex)
		  index--;
	      } else
		unconnected++; // count how many were unconnected in the config
	    assert(unconnected > 0 && index < unconnected);
	  } else // the subdevice is where the device is, so record the connection
	    sdi->m_connected[supOrdinal] |= 1u << supIndex;
	  OU::formatAdd(assy, " index='%zu'", index);
	} else if (!inConfig) {
	  // supporting connection is not indexed, and is local,which means it is connected whole
	  uint64_t mask = ~(UINT64_MAX << supPort.count());
	  assert(!(sdi->m_connected[supOrdinal] & mask));
	  sdi->m_connected[supOrdinal] |= mask;
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
  if (myPlatform.empty()) {
    if (knownPlatform)
      myPlatform = knownPlatform;
    else if (::g_platform)
      myPlatform = ::g_platform;
    else {
	err = "No platform specified in HdlConfig nor on command line";
	return NULL;
    }
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
    HdlHasDevInstances(pf, m_plugged, *this),
    m_platform(pf), m_sdpWidth(1) {
  if (err ||
      (err = OE::checkAttrs(xml, IMPL_ATTRS, HDL_TOP_ATTRS,
			    HDL_CONFIG_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, HDL_CONFIG_ELEMS, (void*)0)) ||
      (err = OE::getNumber(xml, "sdp_width", &m_sdpWidth, NULL, 0, false)))
    return;
#if 0
  if (m_sdpWidth & (32-1)) {
    err = "SDP Width must be a multiple of 32";
    return;
  }
#endif
  pf.setParent(this);
  // Determine whether this platform worker has a control plane master port
  bool control = false;
  for (PortsIter ii = pf.m_ports.begin(); ii != pf.m_ports.end(); ii++) {
    Port &i = **ii;
    if (i.m_master && i.m_type == CPPort) {
      control = true;
      break;
    }
  }
  // Add the platform worker as a device instance
  const DevInstance *pfdi;
  const HdlPlatform &cpf = pf;
  if ((err = addDevInstance(cpf, NULL, NULL, control, NULL, NULL, xml, pfdi)) ||
      (err = pf.parseSignalMappings(xml, pf, NULL)))
    return;
  //hdlAssy = true;
  m_plugged.resize(pf.m_slots.size());
  if (!OE::findChildWithAttr(xml, "device", "name", "time_server")) {
    ezxml_t tx = ezxml_add_child(xml, "device", 0);
    ezxml_set_attr(tx, "name", "time_server");
  }
  if ((err = parseDevInstances(xml, xfile, this, NULL)))
    return;
  std::string assy;
  OU::format(assy, "<HdlPlatformAssembly name='%s'>\n", m_name.c_str());
  // Add all the device instances
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const DevInstance &di = *dii;
    const DeviceType &dt = di.device.m_deviceType;
    OU::formatAdd(assy, "  <instance worker='%s' name='%s'%s>\n",
		  dt.cname(), di.cname(), di.m_instancePVs.size() ? "" : "/");
    if (di.m_instancePVs.size()) {
      const OU::Assembly::Property *ap = &di.m_instancePVs[0];
      for (size_t n = di.m_instancePVs.size(); n; n--, ap++)
	OU::formatAdd(assy, "    <property name='%s' value='%s'/>\n",
		      ap->m_name.c_str(), ap->m_value.c_str());
      OU::formatAdd(assy, "  </instance>\n");
    }
    // Add a time client instance as needed by device instances
    for (PortsIter pi = dt.ports().begin();
	 pi != dt.ports().end(); pi++)
      if ((*pi)->m_type == WTIPort)
	OU::formatAdd(assy, "  <instance worker='time_client' name='%s_time_client'/>\n",
		      (*dii).cname());
  }
  // Internal connections:
  // 1. Control plane master to OCCP
  // 2. WCI connections to platform and device workers
  // 3. To and from time clients
  // 4. Between devices and required subdevices

  // So: 1. Externalize the internal control port to the container
  if ((err = addControlConnection(assy)))
    return;
  // 2. Connect the time service to the platform worker
  OU::formatAdd(assy,
		"  <connection>\n"
		"    <port instance='%s' name='timebase'/>\n"
		"    <port instance='time_server' name='timebase'/>\n"
		"  </connection>\n",
		m_platform.cname());
  // 3. To and from time clients
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const ::Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().ports().begin();
	 pi != d.deviceType().ports().end(); pi++)
      if ((*pi)->m_type == WTIPort) {
	// connection from platform worker's time service to the client
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='time_server' name='time'/>\n"
		      "    <port instance='%s_time_client' name='time'/>\n"
		      "  </connection>\n",
		      (*dii).cname());
	// connection from the time client to the device worker
	OU::formatAdd(assy,
		      "  <connection>\n"
		      "    <port instance='%s_time_client' name='wti'/>\n"
		      "    <port instance='%s' name='%s'/>\n"
		      "  </connection>\n",
		      (*dii).cname(), (*dii).cname(), (*pi)->pname());
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
		m_platform.cname());
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const ::Device &d = (*dii).device;
    for (PortsIter pi = d.deviceType().ports().begin(); pi != d.deviceType().ports().end(); pi++) {
      Port &p = **pi;
      if (p.isData() || p.m_type == NOCPort || p.m_type == SDPPort ||
	  (!p.m_master && (p.m_type == PropPort || p.m_type == DevSigPort))) {
	size_t unconnected = 0, first = 0;
	for (size_t i = 0; i < p.count(); i++)
	  if (!((*dii).m_connected[p.m_ordinal] & (1u << i))) {
	    if (!unconnected++)
	      first = i;
	  }
	// FIXME: (hard) this will not work if the connectivity is not simply contiguous.
	// (at one end or the other).
	if (unconnected) {
	  OU::formatAdd(assy,
			"  <external name='%s%s%s' instance='%s' port='%s'",
			p.m_type != NOCPort && p.m_type != SDPPort ? (*dii).cname() : "",
			p.m_type != NOCPort && p.m_type != SDPPort ? "_" : "", p.pname(),
			(*dii).cname(), p.pname());
	  if (p.isArray())
	    OU::formatAdd(assy, " index='%zu' count='%zu'", first, unconnected);
	  assy += "/>\n";
	}
      }
    }
  }

  OU::formatAdd(assy, "</HdlPlatformAssembly>\n");
  // The assembly will automatically inherit all the signals, prefixed by instance.
  //  if (!attribute)
    ocpiInfo("=======Begin generated platform configuration assembly=======\n"
	     "%s"
	     "=======End generated platform configuration assembly=======\n",
	     assy.c_str());
  // Now we update the (inherited) worker to have the xml for the assembly we just generated.
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
  // Externalize all the device signals, and cross-reference device instances to assy instances
  unsigned n = 0;
  for (Instance *i = &m_assembly->m_instances[0]; n < m_assembly->m_instances.size(); i++, n++) {
    Worker &w = *i->m_worker;
    for (SignalsIter si = w.m_signals.begin(); si != w.m_signals.end(); si++) {
      if ((**si).m_direction == Signal::UNUSED)
	continue;
      Signal *s = new Signal(**si);
      if (w.m_type != Worker::Platform)
	OU::format(s->m_name, "%s_%s", i->cname(), (**si).m_name.c_str());
      m_signals.push_back(s);
      m_sigmap[s->cname()] = s;
      ocpiDebug("Externalizing device signal '%s' for device '%s'", s->cname(), w.m_implName);
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
#if 0 // platform is not just a device
  for (PortsIter pi = m_platform.m_ports.begin(); pi != m_platform.m_ports.end(); pi++)
    if ((*pi)->type == CPPort) {
      cpInstanceName = m_platform.cname();
      cpPortName = (*pi)->pname();
      if (m_platform.m_control)
	nCpPorts++;
      break;
    }
#endif
  for (DevInstancesIter dii = m_devInstances.begin(); dii != m_devInstances.end(); dii++) {
    const ::Device &d = (*dii).device;
    for (PortsIter pi = d.m_deviceType.ports().begin(); pi != d.m_deviceType.ports().end(); pi++)
      if ((*pi)->m_type == CPPort) {
	if (cpInstanceName)
	  multiple = true;
	else {
	  cpInstanceName = d.m_name.c_str();
	  cpPortName = (*pi)->pname();
	}
	if ((*dii).m_control)
	  nCpPorts++;
      }  
  }
  if (multiple)
    if (nCpPorts  > 1)
      return "Multiple control ports are not yet supported";
    else
      return "No control-capable port designated among the multiple possibilities";
  else if (!cpInstanceName)
    return NULL; // "No feasible control port was found";
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
  return NULL;
}
