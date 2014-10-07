#include "wip.h"
#include "hdl.h"
#include "hdl-device.h"

DeviceTypes DeviceType::s_types;

Worker *HdlDevice::
create(ezxml_t xml, const char *xfile, const char *&err) {
  HdlDevice *hd = new HdlDevice(xml, xfile, "", err);
  if (err) {
    delete hd;
    hd = NULL;
  }
  return hd;
}
HdlDevice::
HdlDevice(ezxml_t xml, const char *file, const char *parent, const char *&err)
  : Worker(xml, file, parent, NULL, err) {
  if (err ||
      (err = OE::checkTag(xml, "HdlDevice", "Expected 'HdlDevice' as tag in '%s'", file)) ||
      (err = OE::checkAttrs(xml, PARSED_ATTRS, IMPL_ATTRS, HDL_TOP_ATTRS, HDL_IMPL_ATTRS,
			    "interconnect", "control", (void*)0)) ||
      (err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, (void*)0)) ||
      (err = OE::getBoolean(xml, "interconnect", &m_interconnect)) ||
      (err = OE::getBoolean(xml, "control", &m_canControl)))
    return;
  if ((err = parseHdl()))
    return;
  // Parse submodule requirements - note that this information is only used
  // for platform configurations and containers, but we do a bit of error checking here
  for (ezxml_t rqx = ezxml_cchild(m_xml, "requires"); rqx; rqx = ezxml_next(rqx)) {
    std::string worker;
    if ((err = OE::checkAttrs(rqx, "worker", NULL)) ||
	(err = OE::checkElements(rqx, "connect", NULL)) ||
	(err = OE::getRequiredString(rqx, worker, "worker")))
      return;
    std::string rqFile;
    OU::format(rqFile, "../%s.hdl/%s.xml", worker.c_str(),  worker.c_str());
    DeviceType *dt = DeviceType::get(rqFile.c_str(), m_file.c_str(), err);
    if (!dt) {
      err = OU::esprintf("for required worker %s: %s", worker.c_str(), err);
      return;
    }
    m_requireds.push_back(Required(*dt));
    m_requireds.back().parse(rqx, *this);
    for (ezxml_t cx = ezxml_cchild(rqx, "connect"); cx; cx = ezxml_next(cx)) {
      std::string port, to;
      size_t index;
      bool idxFound = false;
      if ((err = OE::checkAttrs(cx, "port", "to", "index", NULL)) ||
	  (err = OE::checkElements(cx, NULL)) ||
	  (err = OE::getRequiredString(cx, port, "port")) ||
	  (err = OE::getRequiredString(cx, to, "to")) ||
	  (err = OE::getNumber(cx, "index", &index, &idxFound)))
	return ;
    }
  }
}

HdlDevice *HdlDevice::
get(const char *name, const char *parent, const char *&err) {
  DeviceType *dt = NULL;
  for (DeviceTypesIter dti = s_types.begin(); !dt && dti != s_types.end(); dti++)
    if (!strcasecmp(name, (*dti)->name()))
      dt = *dti;
  if (!dt) {
    // New device type, which must be a file.
    ezxml_t xml;
    std::string xfile;
    if (!(err = parseFile(name, parent, NULL, &xml, xfile))) {
      dt = new DeviceType(xml, xfile.c_str(), parent, err);
      if (err) {
	delete dt;
	dt = NULL;
      } else
	s_types.push_back(dt);
    }
  }
  return dt;
}
const char *DeviceType::
name() const {
  return m_implName;
}

// A device is not in its own file.
// It is an instance of a device type either on a platform or a card.
Device::
Device(Board &b, ezxml_t xml, const char *parent, bool single, unsigned ordinal,
       const char *&err)
  : m_board(b), m_ordinal(ordinal) {
  err = NULL;
  std::string wname;
  if ((err = OE::getRequiredString(xml, wname, "worker")) ||
      !(m_deviceType = DeviceType::get(wname.c_str(), parent, err)))
    return;
  const char *cp = strchr(wname.c_str(), '.');
  if (cp)
    wname.resize(cp - wname.c_str());
  if (single)
    m_name = wname;
  else {
    wname += "%u";
    OE::getNameWithDefault(xml, m_name, wname.c_str(), ordinal);
  }
}

Device *Device::
create(Board &b, ezxml_t xml, const char *parent, bool single, unsigned ordinal, const char *&err) {
  Device *d = new Device(b, xml, parent, single, ordinal, err);
  if (err) {
    delete d;
    d = NULL;
  }
  return d;
}


// Add all the devices for a platform or a card - static
const char *Device::
addDevices(Board &b, ezxml_t xml, Devices &devices) {
  // These devices are declaring that they are part of the platform.
  for (ezxml_t xs = ezxml_cchild(xml, "Device"); xs; xs = ezxml_next(xs)) {
    const char *worker = ezxml_cattr(xs, "worker");
    bool single = true;
    bool seenMe = false;
    unsigned n = 0; // ordinal of devices of same type in this platform/card
    for (ezxml_t x = ezxml_cchild(xml, "Device"); x; x = ezxml_next(x)) {
      const char *w = ezxml_cattr(x, "worker");
      if (x == xs)
	seenMe = true;
      else if (worker && w && !strcasecmp(worker, w)) {
	single = false;
	if (!seenMe)
	  n++;
      }
    }
    const char *err;
    Device *dev = Device::create(b, xs, xml->name, single, n, err);
    if (dev)
      devices.push_back(dev);
    else
      return err;
  }
  return NULL;
}
// Static
const Device *Device::
find(const char *name, const Devices &devices) {
  for (DevicesIter di = devices.begin(); di != devices.end(); di++) {
    Device &dev = **di;
    if (!strcasecmp(name, dev.m_name.c_str()))
      return &dev;
  }
  return NULL;
}
const Device &Device::
findRequired(const DeviceType &dt, unsigned ordinal, const Devices &devices) {
  for (DevicesIter di = devices.begin(); di != devices.end(); di++) {
    Device &dev = **di;
    // FIXME: intern the workers
    if (!strcasecmp(dev.m_deviceType->m_implName, dt.m_implName) && dev.m_ordinal == ordinal)
      return dev;
  }
  assert("Required device not found"==0);
  return *(Device*)NULL;
}
const Device *Board::
findDevice(const char *name) const {
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++) {
    Device &dev = **di;
    if (!strcasecmp(name, dev.name()))
      return &dev;
  }
  return NULL;
}

ReqConnection::
ReqConnection()
  : m_port(NULL), m_rq_port(NULL), m_index(0), m_indexed(false) {
}
const char *ReqConnection::
parse(ezxml_t cx, Worker &w, Required &r) {
  const char *err;
  std::string port, to;
  if ((err = OE::checkAttrs(cx, "port", "signal", "to", "index", (void *)0)) ||
      (err = OE::checkElements(cx, NULL)) ||
      (err = OE::getRequiredString(cx, port, "port")) ||
      (err = OE::getRequiredString(cx, to, "to")) ||
      (err = OE::getNumber(cx, "index", &m_index, &m_indexed, 0, false)) ||
      (err = w.getPort(port.c_str(), m_port)) ||
      (err = r.m_type.getPort(to.c_str(), m_rq_port)))
    return err;
  if (m_rq_port->type != m_port->type)
    return OU::esprintf("Required worker port \"%s\" is not the same type", to.c_str());
  if (m_rq_port->master == m_port->master)
    return OU::esprintf("Required worker port \"%s\" has same role (master) as port \"%s\"",
			to.c_str(), port.c_str());
  if (m_rq_port->count > 1) {
    if (!m_indexed)
      return OU::esprintf("Required worker port \"%s\" has count > 1, index must be specified",
			  to.c_str());
    if (m_index >= m_rq_port->count)
      return OU::esprintf("Required worker port \"%s\" has count %zu, index (%zu) too high",
			  to.c_str(), m_rq_port->count, m_index);
  }      
  // FIXME: check signal compatibility...
  return NULL;
}

Required::
Required(const DeviceType &dt)
  : m_type(dt) {
}

const char *Required::
parse(ezxml_t rqx, Worker &w) {
  const char *err;
  std::string worker;
  if ((err = OE::checkAttrs(rqx, "worker", (void *)0)) ||
      (err = OE::checkElements(rqx, "connect", (void*)0)) ||
      (err = OE::getRequiredString(rqx, worker, "worker")))
    return err;
  for (ezxml_t cx = ezxml_cchild(rqx, "connect"); cx; cx = ezxml_next(cx)) {
    m_connections.push_back(ReqConnection());
    if ((err = m_connections.back().parse(cx, w, *this)))
      return err;
  }
  return NULL;
}
