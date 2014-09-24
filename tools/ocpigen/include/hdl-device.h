#ifndef HDL_DEVICE_H
#define HDL_DEVICE_H
#include <map>
#include "wip.h"

// A device type is the common information about a set of devices that can use
// the same device worker implementation.
// This structure is owned per platform for the sake of the ordinals
struct DeviceType;
typedef std::list<DeviceType *>     DeviceTypes;
typedef DeviceTypes::const_iterator DeviceTypesIter;
struct DeviceType : public Worker {
  bool         m_interconnect;  // Can this type of device be used for an interconnect?
  bool         m_canControl;    // Can this interconnect worker provide control?
  DeviceType(ezxml_t xml, const char *file, const char *parent, const char *&err);
  static DeviceType *
  create(const char *name, const char *parent, DeviceTypes &types, const char *&err);
  virtual ~DeviceType() {}
  std::string name() { return m_implName; }
};

struct Device;
typedef std::list<Device *>     Devices;
typedef Devices::const_iterator DevicesIter;
class HdlPlatform;
struct Device {
  DeviceType *m_deviceType;     // not a reference due to construction issues
  std::string m_name;           // a platform-scoped device name - usually type<ordinal>
  unsigned m_ordinal;           // Ordinal of this device on this platform/card
  // Constructor for defining new devices
  Device(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single,
	 unsigned ordinal, const char *&err);
  static Device *
  create(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single,
	 unsigned ordinal, const char *&err);
  static const char*
  parseDevices(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, Devices &devices);
  DeviceType &deviceType() { ocpiAssert(m_deviceType); return *m_deviceType; }
  std::string &name() { return m_name; }
};

// A slot type is really just a set of signals
// The direction is all from the perspective of the motherboard (a.k.a. carrier).
struct SlotType;
typedef std::map<const char *, SlotType *, OU::ConstCharCaseEqual> SlotTypes;
typedef SlotTypes::iterator SlotTypesIter;
struct SlotType {
  std::string m_name;
  Signals m_signals;
  SlotType(const char *file, const char *parent, const char *&err);
  virtual ~SlotType();

  static SlotType *
  create(const char *name, const char *parent, SlotTypes &types, const char *&err);
  static SlotType *
  find(const char *name, SlotTypes &types);
};

// A card has a type, a name and a set of card-specific names for its
// generic (standardized) signals.
struct Card;
typedef std::map<const char *, Card *> Cards;
typedef Cards::iterator CardsIter;
struct Card {
  std::string m_name;
  SlotType &m_type;
  std::map<Signal *, std::string> m_signals;
  DeviceTypes m_deviceTypes;  // usually from files, but possibly immediate
  Devices     m_devices;      // basic physical devices
  Card(ezxml_t xml, const char *name, SlotType &type, const char *&err);
  virtual ~Card();
  static Card *
  create(const char *file, const char *parent, SlotTypes &types, Cards &cards, const char *&err);
  static Card *
  find(const char *name, Cards &cards);
  Devices &devices() { return m_devices; }
  std::string &name() { return m_name; }
};

struct Slot;
struct DevInstance {
  Device &device;
  Card *card;
  Slot *slot;
  bool control;
  std::string name;
  DevInstance(Device &d, Card *c, Slot *s, bool control = false)
  : device(d), card(c), slot(s), control(control) {
  }
};
typedef std::list<DevInstance> DevInstances;
typedef DevInstances::const_iterator DevInstancesIter;

#endif
