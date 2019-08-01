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

#include <errno.h>
#include "wip.h"
#include "hdl.h"
#include "hdl-device.h"
#include "hdl-slot.h"
#include "assembly.h"

//DeviceTypes DeviceType::s_types;

HdlDevice *HdlDevice::
create(ezxml_t xml, const char *xfile, const char *parentFile, Worker *parent,
       OU::Assembly::Properties *instancePVs, const char *&err) {
  HdlDevice *hd = new HdlDevice(xml, xfile, parentFile, parent, Worker::Device, instancePVs, err);
  if (err ||
      (err = OE::checkTag(xml, "HdlDevice", "Expected 'HdlDevice' as tag in '%s'", xfile)) ||
      (err = OE::checkAttrs(xml, PARSED_ATTRS, IMPL_ATTRS, HDL_TOP_ATTRS, HDL_IMPL_ATTRS,
			    "interconnect", "control", (void*)0)) ||
      (err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, (void*)0)) ||
      (err = hd->setParamConfig(instancePVs, 0))) {
    delete hd;
    hd = NULL;
  }
  return hd;
}
HdlDevice::
HdlDevice(ezxml_t xml, const char *file, const char *parentFile, Worker *parent,
	  Worker::WType type, OU::Assembly::Properties *instancePVs, const char *&err)
  : Worker(xml, file, parentFile ? parentFile : "", type, parent, instancePVs, err) {
  m_isDevice = true;
  if (err ||
      (err = OE::getBoolean(xml, "interconnect", &m_interconnect)) ||
      (err = OE::getBoolean(xml, "control", &m_canControl)) ||
      (err = parseHdl()))
    return;
  // Parse submodule support for users - note that this information is only used
  // for platform configurations and containers, but we do a bit of error checking here
  // We need to compute the ordinal for each device type we support
  std::map<std::string, unsigned> countPerSupportedWorkerType;
  for (ezxml_t spx = ezxml_cchild(m_xml, "supports"); spx; spx = ezxml_cnext(spx)) {
    std::string worker;
    if ((err = OE::checkAttrs(spx, "worker", NULL)) ||
	(err = OE::checkElements(spx, "connect", NULL)) ||
	(err = OE::getRequiredString(spx, worker, "worker")))
      return;
    // Record how many each worker type we support and remember the ordinal
    auto pair = countPerSupportedWorkerType.insert(std::make_pair(worker, 0));
    if (!pair.second)
      pair.first->second++;
    DeviceType *dt = get(worker.c_str(), spx, file, parent, err);
    if (err) {
      err = OU::esprintf("for supported worker %s: %s", worker.c_str(), err);
      return;
    }
    m_supports.push_back(Support(*dt));
    m_supports.back().m_ordinal = pair.first->second;
    if ((err = m_supports.back().parse(spx, *this)))
      return;
    for (ezxml_t cx = ezxml_cchild(spx, "connect"); cx; cx = ezxml_cnext(cx)) {
      std::string l_port, to;
      size_t index;
      bool idxFound = false;
      if ((err = OE::checkAttrs(cx, "port", "to", "index", NULL)) ||
	  (err = OE::checkElements(cx, NULL)) ||
	  (err = OE::getRequiredString(cx, l_port, "port")) ||
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
get(const char *a_name, ezxml_t dtxml, const char *parentFile, Worker *parent, const char *&err) {
  // New device type, which must be a file.
  ezxml_t xml;
  std::string xfile;
  std::string name;
  const char *dot = strrchr(a_name, '.');
  if (dot) {
    if (strcmp(dot + 1, "hdl")) {
      err = OU::esprintf("Worker model for %s cannot be anything other than hdl here.", a_name);
      return NULL;
    }
    name.assign(a_name, OCPI_SIZE_T_DIFF(dot, a_name));
  } else
    name = a_name;
  // First look for the file assuming we are building a worker and the sought worker is in the
  // same library as we are.
  std::string file("../" + name + ".hdl/" + name);
  if (!(err = parseFile(file.c_str(), parentFile, NULL, &xml, xfile)) ||
      !(err = parseFile(name.c_str(), parentFile, NULL, &xml, xfile))) {
    OU::Assembly::Properties instancePVs;
    // Here we parse the configuration settings for this device on this board.
    // These settings are similar to instance property values in an assembly, but are
    // applied wherever the device is instanced.
    instancePVs.resize(OE::countChildren(dtxml, "Property"));
    if (instancePVs.size()) {
      OU::Assembly::Property *pv = &instancePVs[0];
      for (ezxml_t px = ezxml_cchild(dtxml, "Property"); px; px = ezxml_cnext(px), pv++) {
	std::string value;
	if ((err = OE::checkAttrs(px, "name", "value", "valuefile", NULL)) ||
	    (err = OE::getRequiredString(px, pv->m_name, "name", "property")) ||
	    (err = pv->setValue(px)))
	  return NULL;
      }
    }
    return HdlDevice::create(xml, xfile.c_str(), parentFile, parent, &instancePVs, err);
  }
  return NULL;
}
const char *HdlDevice::
resolveExpressions(OCPI::Util::IdentResolver &ir) {
  Worker::resolveExpressions(ir);
  const char *err;
  for (SignalsIter si = m_signals.begin(); si != m_signals.end(); si++) {
    Signal &s = **si;
    if (s.m_directionExpr.length() &&
	(err = s.parseDirection(s.m_directionExpr.c_str(), NULL, ir)))
      return OU::esprintf("parsing the signal direction for \"%s\": %s", s.cname(), err);
    if (s.m_widthExpr.length() &&
  	(err = s.parseWidth(s.m_widthExpr.c_str(), NULL, ir)))
      return OU::esprintf("parsing the signal width for \"%s\": %s", s.cname(), err);
  }
  return NULL;
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
    base.resize(OCPI_SIZE_T_DIFF(paren, sname));
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
Device(Board &b, DeviceType &dt, const std::string &a_wname, ezxml_t xml, bool single,
       unsigned ordinal, SlotType *stype, const char *&err)
  : m_board(b), m_deviceType(dt), m_ordinal(ordinal) {
  std::string wname(a_wname);
  const char *cp = strchr(wname.c_str(), '.');
  if (cp)
    wname.resize(OCPI_SIZE_T_DIFF(cp, wname.c_str()));
  if (!single)
    wname += "%u";
  OE::getNameWithDefault(xml, m_name, wname.c_str(), ordinal);
  if (!err)
    err = parse(xml, b, stype);
}

Device *Device::
create(Board &b, ezxml_t xml, const char *parentFile, Worker *parent, bool single,
       unsigned ordinal, SlotType *stype, const char *&err) {
  std::string wname;
  DeviceType *dt;
  if ((err = OE::getRequiredString(xml, wname, "worker")) ||
      !(dt = DeviceType::get(wname.c_str(), xml, parentFile, parent, err)))
    return NULL;
  Device *d = new Device(b, *dt, wname, xml, single, ordinal, stype, err);
  if (err) {
    delete d;
    d = NULL;
  }
  return d;
}

// A separate method because it is also called when a platform is instanced in a pf config
const char *Device::
parseSignalMappings(ezxml_t xml, Board &b, SlotType *stype) {
  const char *err;
  // Now we parse the mapping between device-type signals and board signals
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_cnext(xs)) {
    std::string name, base;
    if (!OE::getOptionalString(xs, name, "name") && m_deviceType.m_type == Worker::Platform)
      continue;
    size_t index;
    bool hasIndex;
    if ((err = OE::getRequiredString(xs, name, "name")) ||
	(err = decodeSignal(name, base, index, hasIndex)))
      return err;
    std::string suffixed;
    // suffixed will be non-empty if we matched a suffixed signal, from diff or inout
    Signal *devSig = m_deviceType.m_sigmap.findSignal(base, &suffixed);
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
      else if (!plat) {
	if (m_deviceType.m_type == Worker::Platform)
	  continue;
	return OU::esprintf("For signal \"%s\", the platform attribute must be specified",
			    name.c_str());
      } else if (*plat) {
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
	  return OU::esprintf("Board signal \"%s\" is input to card/platform, "
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
    std::string devSigIndexed = suffixed.empty() ? devSig->cname() : suffixed.c_str();
    if (hasIndex) // devSig->m_width)
      OU::formatAdd(devSigIndexed, "(%zu)", index);
    m_strings.push_front(devSigIndexed);
    ocpiDebug("Mapping device signal %s to board signal %s (indexed %d)",
	      devSigIndexed.c_str(), boardSig ? boardSig->cname() : "<null>", hasIndex);
    m_dev2bd.push_back(devSig, index, boardSig ? boardSig->cname() : "", hasIndex);
  }
  // Now check for the unsupported condition where a signal is partially mapped.
  for (auto si = m_deviceType.m_signals.begin(); si != m_deviceType.m_signals.end(); si++) {
    Signal &s = **si;
    if (!s.m_width)
      continue;
    bool anyMapped = false, anyNotMapped = false;
    size_t firstMapped, firstNotMapped;
    for (size_t n = 0; n < s.m_width; ++n) {
      bool isSingle;
      if (m_dev2bd.findSignal(s, n, isSingle)) {
	if (!anyMapped) {
	  anyMapped = true;
	  firstMapped = n;
	}
      } else if (!anyNotMapped) {
	anyNotMapped = true;
	firstNotMapped = n;
      }
    }
    if (anyMapped && anyNotMapped)
      return OU::esprintf("for vector signal \"%s\" width %zu, of %s device \"%s\" "
			  "worker \"%s\":  index %zu is mapped to a %s signal, "
			  "but index %zu is not:  all or none of the indices must be mapped",
			  s.cname(), s.m_width, stype ? "card" : "platform", cname(),
			  m_deviceType.cname(), firstMapped, stype ? "slot" : "platform", firstNotMapped);
  }
  return NULL;
}
const char *Device::
parse(ezxml_t xml, Board &b, SlotType *stype) {
  const char *err;
  // This might happen in floating devices in containers.
  if (b.findDevice(m_name.c_str()))
    return OU::esprintf("Duplicate device name \"%s\" for platform/card", m_name.c_str());
  // Do the stuff in Worker::create - can we share more?
  if (m_deviceType.m_type != Worker::Platform &&
      ((err = m_deviceType.setParamConfig(&m_deviceType.m_instancePVs, 0, &m_deviceType.m_parentFile)) ||
       (err = m_deviceType.resolveExpressions(m_deviceType)) ||
       (err = m_deviceType.finalizeProperties()) ||
       (err = m_deviceType.finalizeHDL())))
    return err;
  // Parse the "supports map" that says exacty which platform/card/board device instances
  // that this supporting device supports.
  std::map<const char *, unsigned, OU::ConstCharComp> countPerSupportedWorkerType;
  for (ezxml_t xs = ezxml_cchild(xml, "supports"); xs; xs = ezxml_cnext(xs)) {
    if (!ezxml_cattr(xs, "device") && m_deviceType.m_type == Worker::Platform)
      continue; // If this is in a platform, <supports> is overloaded so this is the other usage
    std::string worker, device;
    if ((err = OE::getRequiredString(xs, worker, "worker")) ||
	(err = OE::getRequiredString(xs, device, "device")))
      return err;
    auto pair = countPerSupportedWorkerType.insert(std::make_pair(worker.c_str(), 0));
    // Find the support element of the device-type
    // Record how many each worker type we support and remember the ordinal
    Support *sup = NULL;
    for (auto si = m_deviceType.m_supports.begin(); si != m_deviceType.m_supports.end(); ++si)
      if (!strcasecmp((*si).m_type.m_implName, worker.c_str()) &&
	  pair.first->second == (*si).m_ordinal) {
	sup = &*si;
	break;
      }
    if (!sup)
      return OU::esprintf("<supports> element for device \"%s\" on board \"%s\" does not match "
			  "any <supports> element on the \"%s\" worker",
			  m_name.c_str(), b.cname(), m_deviceType.m_implName);
    // We found the matching one
    m_supportsMap[std::make_pair(worker.c_str(), pair.first->second)] = device;
    pair.first->second++;
  }
#if 1
  return parseSignalMappings(xml, b, stype);
#else
  // Now we parse the mapping between device-type signals and board signals
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_cnext(xs)) {
    std::string name, base;
    if (!OE::getOptionalString(xs, name, "name") && m_deviceType.m_type == Worker::Platform)
      continue;
    size_t index;
    bool hasIndex;
    if ((err = OE::getRequiredString(xs, name, "name")) ||
	(err = decodeSignal(name, base, index, hasIndex)))
      return err;
    std::string suffixed;
    // suffixed will be non-empty if we matched a suffixed signal, from diff or inout
    Signal *devSig = m_deviceType.m_sigmap.findSignal(base, &suffixed);
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
      else if (!plat) {
	if (m_deviceType.m_type == Worker::Platform)
	  continue;
	return OU::esprintf("For signal \"%s\", the platform attribute must be specified",
			    name.c_str());
      } else if (*plat) {
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
	  return OU::esprintf("Board signal \"%s\" is input to card/platform, "
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
    std::string devSigIndexed = suffixed.empty() ? devSig->cname() : suffixed.c_str();
    if (hasIndex) // devSig->m_width)
      OU::formatAdd(devSigIndexed, "(%zu)", index);
    m_strings.push_front(devSigIndexed);
    ocpiDebug("Mapping device signal %s to board signal %s (indexed %d)",
	      devSigIndexed.c_str(), boardSig ? boardSig->cname() : "<null>", hasIndex);
    m_dev2bd.push_back(devSig, index, boardSig ? boardSig->cname() : "", hasIndex);
  }
  // Now check for the unsupported condition where a signal is partially mapped.
  for (auto si = m_deviceType.m_signals.begin(); si != m_deviceType.m_signals.end(); si++) {
    Signal &s = **si;
    if (!s.m_width)
      continue;
    bool anyMapped = false, anyNotMapped = false;
    size_t firstMapped, firstNotMapped;
    for (size_t n = 0; n < s.m_width; ++n) {
      bool isSingle;
      if (m_dev2bd.findSignal(s, n, isSingle)) {
	if (!anyMapped) {
	  anyMapped = true;
	  firstMapped = n;
	}
      } else if (!anyNotMapped) {
	anyNotMapped = true;
	firstNotMapped = n;
      }
    }
    if (anyMapped && anyNotMapped)
      return OU::esprintf("for vector signal \"%s\" width %zu, of %s device \"%s\" "
			  "worker \"%s\":  index %zu is mapped to a %s signal, "
			  "but index %zu is not:  all or none of the indices must be mapped",
			  s.cname(), s.m_width, stype ? "card" : "platform", cname(),
			  m_deviceType.cname(), firstMapped, stype ? "slot" : "platform", firstNotMapped);
  }
  return NULL;
#endif
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
  for (ezxml_t xs = ezxml_cchild(xml, "Device"); xs; xs = ezxml_cnext(xs)) {
    const char *worker = ezxml_cattr(xs, "worker");
    bool single = true;
    bool seenMe = false;
    unsigned n = 0; // ordinal of devices of same type in this platform/card
    for (ezxml_t x = ezxml_cchild(xml, "Device"); x; x = ezxml_cnext(x)) {
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
  if (m_sup_port->m_master == m_port->m_master)
    return OU::esprintf("Supported worker port \"%s\" has same role (master) as port \"%s\"",
			to.c_str(), port.c_str());
  if (m_sup_port->m_arrayCount) {
    if (!m_indexed)
      return OU::esprintf("Supported worker port \"%s\" has count, index must be specified",
			  to.c_str());
    if (m_index >= m_sup_port->count())
      return OU::esprintf("Supported worker port \"%s\" has count %zu, index (%zu) too high",
			  to.c_str(), m_sup_port->count(), m_index);
  }
  // FIXME: check signal compatibility...
  return NULL;
}

Support::
Support(const DeviceType &dt)
  : m_type(dt), m_ordinal(0) {
}

const char *Support::
parse(ezxml_t spx, Worker &w) {
  const char *err;
  std::string worker;
  if ((err = OE::checkAttrs(spx, "worker", (void *)0)) ||
      (err = OE::checkElements(spx, "connect", (void*)0)) ||
      (err = OE::getRequiredString(spx, worker, "worker")))
    return err;
  for (ezxml_t cx = ezxml_cchild(spx, "connect"); cx; cx = ezxml_cnext(cx)) {
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
  for (ezxml_t sx = ezxml_cchild(x, "signal"); sx; sx = ezxml_cnext(sx)) {
    std::string l_name, base, external;
    size_t index;
    bool hasIndex;
    if ((err = OE::getRequiredString(sx, l_name, "name")) ||
	(err = OE::getRequiredString(sx, external, "external")) ||
	(err = decodeSignal(l_name, base, index, hasIndex)))
      return err;
    Signal *s = m_sigmap.findSignal(base);
    if (!s)
      return OU::esprintf("Worker \"%s\" of instance \"%s\" has no signal \"%s\"",
			  m_implName, i.cname(), l_name.c_str());
    assert(!hasIndex || s->m_width);
    if (external.length()) {
      bool single;
      if (i.m_extmap.findSignal(*s, index, single))
	return OU::esprintf("Duplicate signal \"%s\" for worker \"%s\" instance \"%s\"",
			    l_name.c_str(), m_implName, i.cname());
      size_t dummy;
      if (i.m_extmap.findSignal(external, dummy) && s->m_direction == Signal::OUT)
	return OU::esprintf("Multiple outputs drive external \"%s\" for worker \"%s\" "
			    "instance \"%s\"", external.c_str(), m_implName, i.cname());
      Signal *ps = parent.m_sigmap.findSignal(external.c_str());
      if (!ps)
	return OU::esprintf("External signal \"%s\" specified for signal \"%s\" of "
			    "instance \"%s\" of worker \"%s\" is not an external signal of the "
			    "assembly", external.c_str(), l_name.c_str(), i.cname(), m_implName);
      // If the board signal is bidirectional (can be anything), it should inherit
      // the direction of the device's signal
      if (ps->m_direction == Signal::BIDIRECTIONAL)
	ps->m_direction = s->m_direction;
      ps->m_pin = s->m_pin;
    }
    ocpiDebug("Instance '%s' signal '%s' index '%zu' mapped to '%s'",
	      i.cname(), s->cname(), index, external.c_str());
    i.m_extmap.push_back(s, index, external, hasIndex);
  }
  return NULL;
}
