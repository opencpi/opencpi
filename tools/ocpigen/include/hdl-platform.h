#ifndef HDL_PLATFORM_H
#define HDL_PLATFORM_H
#include <assert.h>
#include <map>
#include <list>
#include "hdl-device.h"
// These are for all implementaitons whether assembly or written
#define HDL_TOP_ATTRS "Pattern", "PortPattern", "DataWidth", "Language", "library"
// These are for implementaitons that you write (e.g. not generated assemblies)
#define HDL_IMPL_ATTRS GENERIC_IMPL_CONTROL_ATTRS, "RawProperties", "FirstRawProperty", "outer"
#define HDL_IMPL_ELEMS "timeinterface", "memoryinterface", "streaminterface", "messageinterface", "signal", "cpmaster", "time_service", "control", "metadata", "rawprop", "requires"


// A slot has a type, a name and a set of platform-specific names for its
// generic (standardized) signals.
// FIXME: consider eliding with Device class
struct Slot;
typedef std::map<const char *, Slot *> Slots;
typedef Slots::iterator SlotsIter;
struct Slot {
  std::string m_name;
  SlotType &m_type;
  std::map<Signal *, std::string> m_signals;
  Slot(ezxml_t xml, const char *parent, const char *name, SlotType &type, const char *&err);
  virtual ~Slot();
  static Slot *
  create(ezxml_t xml, const char *parent, SlotTypes &types, Slots &slots, const char *&err);
  static Slot *
  find(const char *name, Slots &slots);
  std::string &name() { return m_name; }
};



#define HDL_PLATFORM_ATTRS "dummy", "control"
#define HDL_PLATFORM_ELEMS "cpmaster", "nocmaster", "device", "timeservice", "metadata", "slot"
class HdlConfig;
class HdlPlatform : public Worker {
  friend class HdlConfig;
  DeviceTypes m_deviceTypes;  // device types of the platform
  Devices     m_devices;      // devices of the platform
  SlotTypes m_slotTypes;      // slot types of the platform
  Slots m_slots;              // slots of the platform
  bool m_control;             // should the platform be used for control?
			 // FIXME should inherit device type and be a device
public:  
  static HdlPlatform *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlPlatform(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~HdlPlatform();
  std::string &name() { return m_name; }
  Devices &devices() { return m_devices; }
  SlotTypes &slotTypes() { return m_slotTypes; }
  Slots &slots() { return m_slots; }
  void setControl(bool c) { m_control = c; }
  static const char *
  parseDevInstances(ezxml_t xml, HdlPlatform &platform, DevInstances *baseInstances,
		    DevInstances &devInstances, Cards &cards, bool container);
  static const DevInstance *
  findDevInstance(const char *device, std::string &card, std::string &slot,
		     DevInstances &devInstances);
  static const char *
  addDevInstance(const char *name, const char *parent, std::string &card, std::string &slot,
		 bool control, HdlPlatform &platform, Cards &cards,
		 DevInstances &devInstances, const DevInstance *&devInstance);
  static const char *
    addDevices(ezxml_t xml, Devices &devices, DeviceTypes &deviceTypes);
};

#define HDL_CONFIG_ATTRS "platform"
#define HDL_CONFIG_ELEMS "cpmaster", "nocmaster", "device"
class HdlConfig : public Worker {
  HdlPlatform *m_platform;     // The platform being configured (FIXME: should be a reference)
  DeviceTypes  m_deviceTypes;  // Device types introduced by cards in slots
  DevInstances m_devInstances; // basic physical devices, either on platform or cards
  Cards        m_cards;        // cards in this configuration
public:  
  static HdlConfig *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlConfig(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~HdlConfig();
  HdlPlatform &platform() { assert(m_platform); return *m_platform; }
  DevInstances &devInstances() { return m_devInstances; }
  const char
    *addControlConnection(std::string &assy),
    *emitConfig(FILE *f);
};

#endif
