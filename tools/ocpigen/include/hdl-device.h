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

#ifndef HDL_DEVICE_H
#define HDL_DEVICE_H
#include <assert.h>
#include <map>
#include "ocpigen.h"
#include "hdl.h"

// A device type is the common information about a set of devices that can use
// the same device worker implementation.
// This structure is owned per platform for the sake of the ordinals
class Worker;
struct Support;
struct SupportConnection {
  Port  *m_port;
  Port  *m_sup_port;
  size_t m_index;
  bool   m_indexed;
  SupportConnection();
  const char *parse(ezxml_t x, Worker &w, Support &rq);
};
typedef std::list<SupportConnection> SupportConnections;
typedef SupportConnections::const_iterator SupportConnectionsIter;
// This class represents support for a device worker and the connections to it
class HdlDevice;
struct Support {
  const HdlDevice &m_type;
  unsigned m_ordinal;  // ordinal within "supports" relationships for the same device type/worker
  SupportConnections m_connections;
  Support(const HdlDevice &);
  const char *parse(ezxml_t rx, Worker &w);
};
typedef std::list<Support> Supports;
typedef Supports::const_iterator SupportsIter;
class HdlDevice : public Worker {
public:
  //  static DeviceTypes s_types;
  bool               m_interconnect;  // Can this type of device be used for an interconnect?
  bool               m_canControl;    // Can this interconnect worker provide control?
  Supports           m_supports;      // what subdevices are supported?
  static HdlDevice *
    get(const char *name, ezxml_t xml, const char *parentFile, Worker *parent, const char *&err);
  static HdlDevice *create(ezxml_t xml, const char *file, const char *parentFile, Worker *parent,
			   OU::Assembly::Properties *instancePVs, const char *&err);
  HdlDevice(ezxml_t xml, const char *file, const char *parentFile, Worker *parent,
	    Worker::WType type, OU::Assembly::Properties *instancePVs, const char *&err);
  virtual ~HdlDevice() {}
  const char *cname() const;
  const char *parseDeviceProperties(ezxml_t x, OU::Assembly::Properties &iPVs);
  const char *resolveExpressions(OCPI::Util::IdentResolver &ir);
};
typedef HdlDevice DeviceType;
struct Board;
struct SlotType;
struct Device {
  Board &m_board;
  DeviceType &m_deviceType;
  std::string m_name;           // a platform-scoped device name - usually type<ordinal>
  unsigned m_ordinal;           // Ordinal of this device on this platform/card
  ExtMap   m_dev2bd;            // map from device type signals to board signals
  std::list<std::string> m_strings; // storage management since sigmaps don't hold strings
  // A map for supporting(sub) devices, to indicate the actual devices they are supporting
  // when it isn't obvious (there is only one such supported device or the ordinals match)
  // Maps the pair:
  //       - devtypename
  //       - ordinal for supports elements for that devtype in supporting devicetype
  // to a device name on the platform/card/board
  std::map<std::pair<std::string, unsigned>, std::string> m_supportsMap;
  // Constructor for defining new devices.
  // If on a card, the stype will be supplied
  Device(Board &b, DeviceType &dt, const std::string &wname, ezxml_t xml, bool single,
	 unsigned ordinal, SlotType *stype, const char *&err);
  static Device *
  create(Board &b, ezxml_t xml, const char *parentFile, Worker *parent, bool single,
	 unsigned ordinal, SlotType *stype, const char *&err);
  const char *parse(ezxml_t x, Board &b, SlotType *stype);
  const char *parseSignalMappings(ezxml_t x, Board &b, SlotType *stype);
  const DeviceType &deviceType() const { return m_deviceType; }
  const char *cname() const { return m_name.c_str(); }
  static const Device *
  find(const char *name, const Devices &devices);
#if 0
  static const Device &
  findSupport(const DeviceType &dt, unsigned ordinal, const Devices &devices);
#endif
};

// common behavior for platforms and cards

struct Board {
  Devices     m_devices;     // physical devices on this type of board
  SigMapIdx   m_bd2dev;      // map from board/slot signal name to device signal + index
  SigMap      m_extmap;      // map from board signal name to platform/slot signal
  Signals     m_extsignals;  // board/slot signals
  Board(SigMap &sigmap, Signals &signals);
  virtual ~Board() {}
  virtual const char *cname() const = 0;
  const Devices &devices() const { return m_devices; }
  const Device *findDevice(const char *name) const;
  Devices &devices() { return m_devices; }
#if 0
  const Device &findSupport(const DeviceType &dt, unsigned ordinal) const {
    return Device::findSupport(dt, ordinal, m_devices);
  }
#endif
  const Device *findDevice(const char *name) {
    return Device::find(name, m_devices);
  }
  const char
  *parseDevices(ezxml_t xml, SlotType *stype, const char *parentFile, Worker *parent),
  *addFloatingDevice(ezxml_t xml, const char *parentFile, Worker *parent, std::string &name);
};


#endif
