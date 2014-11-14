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
struct Required;
struct ReqConnection {
  Port  *m_port;
  Port  *m_rq_port;
  size_t m_index;
  bool   m_indexed;
  ReqConnection();
  const char *parse(ezxml_t x, Worker &w, Required &rq);
};
typedef std::list<ReqConnection> ReqConnections;
typedef ReqConnections::const_iterator ReqConnectionsIter;
// This class represents a required worker and the connections to it
class HdlDevice;
struct Required {
  const HdlDevice &m_type;
  ReqConnections m_connections;
  Required(const HdlDevice &);
  const char *parse(ezxml_t rx, Worker &w);
};
typedef std::list<Required> Requireds;
typedef Requireds::const_iterator RequiredsIter;
class HdlDevice : public Worker {
public:
  bool               m_interconnect;  // Can this type of device be used for an interconnect?
  bool               m_canControl;    // Can this interconnect worker provide control?
  Requireds          m_requireds;     // what subdevices are required?
  static Worker *create(ezxml_t xml, const char *file, Worker *parent, const char *&err);
  static DeviceTypes s_types;
  HdlDevice(ezxml_t xml, const char *file, const char *parentFile, Worker *parent,
	    const char *&err);
  static HdlDevice *
  get(const char *name, const char *parent, const char *&err);
  virtual ~HdlDevice() {}
  const char *name() const;
  const Ports &ports() const { return m_ports; }
};
typedef HdlDevice DeviceType;
struct Board;
struct SlotType;
struct Device {
  Board &m_board;
  DeviceType &m_deviceType;     // not a reference due to construction issues
  std::string m_name;           // a platform-scoped device name - usually type<ordinal>
  unsigned m_ordinal;           // Ordinal of this device on this platform/card
  SigMap   m_sigmap;            // map from device type signals (WITH INDICES) to board signals
  Signals  m_signals;           // map from device type signals (WITH INDICES) to board signals
  std::list<std::string> m_strings; // storage management since sigmaps don't hold strings
  // The map from the device's signal to the board's signal.
  //  std::map<Signal *, Signal *> m_dev2bd;
  // Constructor for defining new devices.
  // If on a card, the stype will be supplied
  Device(Board &b, DeviceType &dt, ezxml_t xml, bool single,
	 unsigned ordinal, SlotType *stype, const char *&err);
  static Device *
  create(Board &b, ezxml_t xml, const char *parent, bool single, unsigned ordinal,
	 SlotType *stype, const char *&err);
  const DeviceType &deviceType() const { return m_deviceType; }
  const char *name() const { return m_name.c_str(); }
  static const Device *
  find(const char *name, const Devices &devices);
  static const Device &
  findRequired(const DeviceType &dt, unsigned ordinal, const Devices &devices);
};

// common behavior for platforms and cards

struct Board {
  Devices     m_devices;   // physical devices on this type of board
  SigMapIdx   m_bd2dev;    // map from board/slot signal name to device signal + index
  SigMap      &m_extmap;   // map from board/slot signal name to board/slot signal
  Signals     &m_extsignals;  // board/slot signals
  Board(SigMap &sigmap, Signals &signals);
  const Devices &devices() const { return m_devices; }
  const Device *findDevice(const char *name) const;
  Devices &devices() { return m_devices; }
  const Device &findRequired(const DeviceType &dt, unsigned ordinal) const {
    return Device::findRequired(dt, ordinal, m_devices);
  }
  const Device *findDevice(const char *name) {
    return Device::find(name, m_devices);
  }
  const char *parseDevices(ezxml_t xml, SlotType *stype);
};


#endif
