#include <errno.h>
#include "wip.h"
#include "hdl.h"
#include "hdl-device.h"
#include "hdl-slot.h"
#include "assembly.h"

//DeviceTypes DeviceType::s_types;

Worker *HdlDevice::
create(ezxml_t xml, const char *xfile, Worker *parent,
       OU::Assembly::Properties *instancePVs, const char *&err) {
  HdlDevice *hd = new HdlDevice(xml, xfile, "", parent, instancePVs, err);
  if (err ||
      (err = hd->setParamConfig(instancePVs, 0))) {
    delete hd;
    hd = NULL;
  }
  return hd;
}
HdlDevice::
HdlDevice(ezxml_t xml, const char *file, const char *parentFile, Worker *parent,
	  OU::Assembly::Properties *instancePVs, const char *&err)
  : Worker(xml, file, parentFile, Worker::Device, parent, instancePVs, err) {
  m_isDevice = true;
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
  // Parse submodule support for users - note that this information is only used
  // for platform configurations and containers, but we do a bit of error checking here
  for (ezxml_t spx = ezxml_cchild(m_xml, "supports"); spx; spx = ezxml_next(spx)) {
    std::string worker;
    if ((err = OE::checkAttrs(spx, "worker", NULL)) ||
	(err = OE::checkElements(spx, "connect", NULL)) ||
	(err = OE::getRequiredString(spx, worker, "worker")))
      return;
    //    std::string supFile;
    //    OU::format(supFile, "../%s.hdl/%s.xml", worker.c_str(),  worker.c_str());
    // Note that a supporting subdevice must be in a library built AFTER the
    // device it is supporting since it relies on the export of the xml of the
    // device it supports.
    //    OU::format(supFile, "%s.xml", worker.c_str());
    // New device type, which must be a file.
    DeviceType *dt = get(worker.c_str(), file, parent, err);
    if (err) {
      err = OU::esprintf("for supported worker %s: %s", worker.c_str(), err);
      return;
    }
    m_supports.push_back(Support(*dt));
    if ((err = m_supports.back().parse(spx, *this)))
      return;
    for (ezxml_t cx = ezxml_cchild(spx, "connect"); cx; cx = ezxml_next(cx)) {
      std::string port, to;
      size_t index;
      bool idxFound = false;
      if ((err = OE::checkAttrs(cx, "port", "to", "index", NULL)) ||
	  (err = OE::checkElements(cx, NULL)) ||
	  (err = OE::getRequiredString(cx, port, "port")) ||
	  (err = OE::getRequiredString(cx, to, "to")) ||
	  (err = OE::getNumber(cx, "index", &index, &idxFound)))
	return;
    }
  }
}

// This static method was intended to intern the device types, but now it doesn't,
// since each device on a board maybe parameterized/configured for that board.
// So now this method simply creates a new device-type-worker each time it is called.
// The name argument can be a file name.
HdlDevice *HdlDevice::
get(const char *name, const char *parentFile, Worker *parent, const char *&err) {
  // New device type, which must be a file.
  ezxml_t xml;
  std::string xfile;
  DeviceType *dt = NULL;
  if (!(err = parseFile(name, parentFile, NULL, &xml, xfile))) {
    dt = new DeviceType(xml, xfile.c_str(), parentFile, parent, NULL, err);
    if (err) {
      delete dt;
      dt = NULL;
    }
  }
  return dt;
}
const char *DeviceType::
cname() const {
  return m_implName;
}

static const char*
decodeSignal(std::string &name, std::string &base, size_t &index, bool &hasIndex) {
  const char *sname = name.c_str();
  const char *paren = strchr(sname, '(');
  base = name;
  if (paren) {
    char *end;
    errno = 0;
    index = strtoul(paren + 1, &end, 0);
    if (errno != 0 || end == paren + 1 || *end != ')')
      return OU::esprintf("Bad numeric format in signal index: %s", sname);
    base.resize(paren - sname);
    hasIndex = true;
  } else {
    hasIndex = false;
    index = 0;
  }
  return NULL;
}


// A device is not in its own file.
// It is an instance of a device type on a board
Device::
Device(Board &b, DeviceType &dt, ezxml_t xml, bool single, unsigned ordinal,
       SlotType *stype, const char *&err)
  : m_board(b), m_deviceType(dt), m_ordinal(ordinal) {
  std::string wname;
  if ((err = OE::getRequiredString(xml, wname, "worker")))
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
  err = parse(xml, b, stype);
}

Device *Device::
create(Board &b, ezxml_t xml, const char *parentFile, Worker *parent, bool single,
       unsigned ordinal, SlotType *stype, const char *&err) {
  std::string wname;
  DeviceType *dt;
  if ((err = OE::getRequiredString(xml, wname, "worker")) ||
      !(dt = DeviceType::get(wname.c_str(), parentFile, parent, err)))
    return NULL;
  Device *d = new Device(b, *dt, xml, single, ordinal, stype, err);
  if (err) {
    delete d;
    d = NULL;
  }
  return d;
}

const char *Device::
parse(ezxml_t xml, Board &b, SlotType *stype) {
  const char *err;
  // This might happen in floating devices in containers.
  if (b.findDevice(m_name.c_str()))
    return OU::esprintf("Duplicate device name \"%s\" for platform/card", m_name.c_str());
  // Here we parse the configuration settings for this device on this platform.
  // These settings are similar to instance property values in an assembly, but are
  // applied wherever the device is instanced.
  assert(!m_deviceType.m_instancePVs);
  m_deviceType.m_instancePVs = new OU::Assembly::Properties(OE::countChildren(xml, "Property"));
  OU::Assembly::Property *pv = &(*m_deviceType.m_instancePVs)[0];
  for (ezxml_t px = ezxml_cchild(xml, "Property"); px; px = ezxml_next(px), pv++) {
    std::string value;
    if ((err = OE::checkAttrs(px, "name", "value", "valuefile", NULL)) ||
	(err = OE::getRequiredString(px, pv->m_name, "name", "property")))
      return err;
    OU::Property *p = m_deviceType.findProperty(pv->m_name.c_str());
    if (!p)
      return OU::esprintf("There is no \"%s\" property for device type \"%s\"",
			  pv->m_name.c_str(), m_deviceType.m_implName);
    if ((err = pv->setValue(px)))
      return err;
  }
  // Now we parse the mapping between device-type signals and board signals
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_next(xs)) {
    std::string name, base;
    size_t index;
    bool hasIndex;
    if ((err = OE::getRequiredString(xs, name, "name")) ||
	(err = decodeSignal(name, base, index, hasIndex)))
      return err;
    Signal *devSig = m_deviceType.m_sigmap.findSignal(base);
    if (!devSig)
      return OU::esprintf("Signal \"%s\" is not defined for device type \"%s\"",
			  base.c_str(), m_deviceType.cname());
    if (hasIndex) {
      if (devSig->m_width == 0)
	return OU::esprintf("Device signal \"%s\" cannot be indexed", base.c_str());
      else if (index >= devSig->m_width)
	return OU::esprintf("Device signal \"%s\" has index higher than signal width (%zu)",
			    name.c_str(), devSig->m_width);
    }
    const char *plat = ezxml_cattr(xs, "platform");
    const char *card = ezxml_cattr(xs, "card"); // the slot 
    if (!card)
      card = ezxml_cattr(xs, "slot");
    const char *board;
    Signal *boardSig = NULL; // the signal we are mapping a device signal TO
    if (stype) {
      board = card;
      if (plat)
        return OU::esprintf("The platform attribute cannot be specified for signals on cards");
      else if (!card)
        return OU::esprintf("For signal \"%s\" for a device on a card, "
			   "the slot attribute must be specified", name.c_str());
      else if (*card && !(boardSig = b.m_extmap.findSignal(card)))
	// The slot signal is not a signal for the slot type of this card
	return OU::esprintf("For signal \"%s\", the card signal \"%s\" is not defined "
			    "for slot type \"%s\"", name.c_str(), card, stype->cname());
    } else {
      board = plat;
      if (card)
	return OU::esprintf("For signal \"%s\" for a platform device, "
			    "the slot attribute cannot be specified", name.c_str());
      else if (!plat)
	return OU::esprintf("For signal \"%s\", the platform attribute must be specified",
			    name.c_str());
      else if (*plat) {
	// FIXME: check for using the same signal in the same device
	// FIXME: specify mutex to allow same signal to be reused between mutex devices
	if (!(boardSig = b.m_extmap.findSignal(board))) {
	  ocpiDebug("Adding platform signal: %s", board);
	  boardSig = new Signal();
	  boardSig->m_name = board;
	  boardSig->m_direction = devSig->m_direction;
	  b.m_extmap[board] = boardSig;
	  b.m_extsignals.push_back(boardSig);
	} else
	  ocpiDebug("Mapping to existing platform signal: %s", board);
      }
    }
    // Check compatibility between device and slot/platform signal
    if (boardSig) {
      switch (boardSig->m_direction) {
      case Signal::IN: // input to board
	if (devSig->m_direction != Signal::IN)
	  return OU::esprintf("Board signal \"%s\" is input to card/paltform, "
			      "but \"%s\" is not input to device", boardSig->cname(),
			      devSig->cname());
	break;
      case Signal::OUT: // output from board
	if (devSig->m_direction != Signal::OUT)
	  return OU::esprintf("Slot signal \"%s\" is output from card/platform, "
			      "but \"%s\" is not output from device", boardSig->cname(),
			      devSig->cname());
	break;
      case Signal::INOUT: // FIXME: is this really allowed for slots?
	if (devSig->m_direction != Signal::INOUT)
	  return OU::esprintf("Slot signal \"%s\" is inout to/from card/platform, "
			      "but \"%s\" is not inout at device", boardSig->cname(),
			      devSig->cname());
	break;
      case Signal::BIDIRECTIONAL:
#if 0
	if (devSig->m_direction == Signal::INOUT)
	  return OU::esprintf("Slot signal \"%s\" is bidirectional to or from board, "
			      "but \"%s\" is inout at device", boardSig->cname(),
			      devSig->cname());
#endif
	break;
      case Signal::OUTIN:
	return OU::esprintf("Emulators can not be used with slots");
      default:
	;
      }
      // Here board and boardSig are set
      Signal *other = b.m_bd2dev.findSignal(board);
      if (other) {
	// Multiple device signals to the same board signal.  The other signal was ok by itself.
	switch (other->m_direction) {
	case Signal::IN:
	  if (devSig->m_direction != Signal::IN)
	    return OU::esprintf("Multiple incompatible device signals assigned to \"%s\" signal",
				board);
	  break;
	case Signal::OUT:
	  return OU::esprintf("Multiple device output signals driving \"%s\" signal ", board);
	default:
	  ;
	}
      } else {
	// First time we have seen this board signal
	// Map to find device signal and index from slot/board signal
	b.m_bd2dev.insert(board, devSig, index);
      }
    }
    // boardSig might be NULL here.
    std::string devSigIndexed = devSig->cname();
    if (hasIndex) // devSig->m_width)
      OU::formatAdd(devSigIndexed, "(%zu)", index);
    m_strings.push_front(devSigIndexed);
    ocpiDebug("Mapping device signal %s to board signal %s (indexed %d)",
	      devSigIndexed.c_str(), boardSig ? boardSig->cname() : "<null>", hasIndex);
    m_dev2bd.push_back(devSig, index, boardSig ? boardSig->cname() : "", hasIndex);
  }
  return NULL;
}

Board::
Board(SigMap &/*sigmap*/, Signals &/*signals*/)
#if 0
  : m_extmap(sigmap), m_extsignals(signals) {
#else
  {
#endif
}

const char *Board::
addFloatingDevice(ezxml_t xs, const char *parentFile, Worker *parent, std::string &name) {
  const char *err = NULL;
  Device *dev = Device::create(*this, xs, parentFile, parent, true, 0, NULL, err);
  if (dev) {
    m_devices.push_back(dev);
    name = dev->cname();
  }
  return err;
}

// Add all the devices for a board - static
const char *Board::
parseDevices(ezxml_t xml, SlotType *stype, const char *parentFile, Worker *parent) {
  // These devices are declaring that they are part of the board.
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
    Device *dev = Device::create(*this, xs, parentFile, parent, single, n, stype, err);
    if (dev)
      m_devices.push_back(dev);
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
#if 0
const Device &Device::
findSupport(const DeviceType &dt, unsigned ordinal, const Devices &devices) {
  for (DevicesIter di = devices.begin(); di != devices.end(); di++) {
    Device &dev = **di;
    // FIXME: intern the workers
    if (!strcasecmp(dev.m_deviceType.m_implName, dt.m_implName) && dev.m_ordinal == ordinal)
      return dev;
  }
  assert("Support (sub)device not found"==0);
  return *(Device*)NULL;
}
#endif
const Device *Board::
findDevice(const char *name) const {
  for (DevicesIter di = m_devices.begin(); di != m_devices.end(); di++) {
    Device &dev = **di;
    if (!strcasecmp(name, dev.cname()))
      return &dev;
  }
  return NULL;
}

SupportConnection::
SupportConnection()
  : m_port(NULL), m_sup_port(NULL), m_index(0), m_indexed(false) {
}

const char *SupportConnection::
parse(ezxml_t cx, Worker &w, Support &r) {
  const char *err;
  std::string port, to;
  if ((err = OE::checkAttrs(cx, "port", "signal", "to", "index", (void *)0)) ||
      (err = OE::checkElements(cx, NULL)) ||
      (err = OE::getRequiredString(cx, port, "port")) ||
      (err = OE::getRequiredString(cx, to, "to")) ||
      (err = OE::getNumber(cx, "index", &m_index, &m_indexed, 0, false)) ||
      (err = r.m_type.getPort(port.c_str(), m_port)) ||
      (err = w.getPort(to.c_str(), m_sup_port)))
    return err;
  if (m_sup_port->m_type != m_port->m_type)
    return OU::esprintf("Supported worker port \"%s\" is not the same type", to.c_str());
  if (m_sup_port->master == m_port->master)
    return OU::esprintf("Supported worker port \"%s\" has same role (master) as port \"%s\"",
			to.c_str(), port.c_str());
  if (m_sup_port->m_count > 1) {
    if (!m_indexed)
      return OU::esprintf("Supported worker port \"%s\" has count > 1, index must be specified",
			  to.c_str());
    if (m_index >= m_sup_port->m_count)
      return OU::esprintf("Supported worker port \"%s\" has count %zu, index (%zu) too high",
			  to.c_str(), m_sup_port->m_count, m_index);
  }      
  // FIXME: check signal compatibility...
  return NULL;
}

Support::
Support(const DeviceType &dt)
  : m_type(dt) {
}

const char *Support::
parse(ezxml_t spx, Worker &w) {
  const char *err;
  std::string worker;
  if ((err = OE::checkAttrs(spx, "worker", (void *)0)) ||
      (err = OE::checkElements(spx, "connect", (void*)0)) ||
      (err = OE::getRequiredString(spx, worker, "worker")))
    return err;
  for (ezxml_t cx = ezxml_cchild(spx, "connect"); cx; cx = ezxml_next(cx)) {
    m_connections.push_back(SupportConnection());
    if ((err = m_connections.back().parse(cx, w, *this)))
      return err;
  }
  return NULL;
}

// This does instance parsing for instances of HdlDevice workers.
// There is no HdlInstance class, so it is done here for now.
const char *Worker::
parseInstance(Worker &parent, Instance &i, ezxml_t x) {
  const char *err;
  for (ezxml_t sx = ezxml_cchild(x, "signal"); sx; sx = ezxml_next(sx)) {
    std::string name, base, external;
    size_t index;
    bool hasIndex;
    if ((err = OE::getRequiredString(sx, name, "name")) ||
	(err = OE::getRequiredString(sx, external, "external")) ||
	(err = decodeSignal(name, base, index, hasIndex)))
      return err;
    Signal *s = m_sigmap.findSignal(base);
    if (!s)
      return OU::esprintf("Worker \"%s\" of instance \"%s\" has no signal \"%s\"",
			  m_implName, i.name, name.c_str());
    assert(!hasIndex || s->m_width);
    if (external.length()) {
      bool single;
      if (i.m_extmap.findSignal(*s, index, single))
	return OU::esprintf("Duplicate signal \"%s\" for worker \"%s\" instance \"%s\"",
			    name.c_str(), m_implName, i.name);
      size_t dummy;
      if (i.m_extmap.findSignal(external, dummy) && s->m_direction == Signal::OUT)
	return OU::esprintf("Multiple outputs drive external \"%s\" for worker \"%s\" "
			    "instance \"%s\"", external.c_str(), m_implName, i.name);
      Signal *ps = parent.m_sigmap.findSignal(external.c_str());
      if (!ps)
	return OU::esprintf("External signal \"%s\" specified for signal \"%s\" of "
			    "instance \"%s\" of worker \"%s\" is not an external signal of the "
			    "assembly", external.c_str(), name.c_str(), i.name, m_implName);
      // If the board signal is bidirectional (can be anything), it should inherit
      // the direction of the device's signal
      if (ps->m_direction == Signal::BIDIRECTIONAL)
	ps->m_direction = s->m_direction;
    }
    i.m_extmap.push_back(s, index, external, hasIndex);
  }
  return NULL;
}
