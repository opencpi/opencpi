#ifndef HDL_PLATFORM_H
#define HDL_PLATFORM_H
#include <assert.h>
#include <map>
#include "wip.h"
// These are for all implementaitons whether assembly or written
#define HDL_TOP_ATTRS "Pattern", "PortPattern", "DataWidth", "Language", "library"
// These are for implementaitons that you write (e.g. not generated assemblies)
#define HDL_IMPL_ATTRS GENERIC_IMPL_CONTROL_ATTRS, "RawProperties", "FirstRawProperty", "outer"
#define HDL_IMPL_ELEMS "timeinterface", "memoryinterface", "streaminterface", "messageinterface", "signal", "cpmaster", "time_service", "control", "metadata"

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

// A device type is the common information about a set of devices that can use
// the same device worker implementation.
// This structure is owned per platform for the sake of the ordinals
struct DeviceType;
typedef std::list<DeviceType *>     DeviceTypes;
typedef DeviceTypes::const_iterator DeviceTypesIter;
struct DeviceType : public Worker {
  bool         m_interconnect;  // Can this type of device be used for an interconnect?
  bool         m_canControl;    // Can this interconnect worker provide control?
  unsigned     m_count;         // How many of this type of device have we seen?
  DeviceType(ezxml_t xml, const char *file, const char *parent, const char *&err);
  static DeviceType *
  create(const char *name, const char *parent, DeviceTypes &types, const char *&err);
  virtual ~DeviceType() {}
  std::string name() { return m_implName; }
};

struct Device;
typedef std::list<Device *>     Devices;
typedef Devices::const_iterator DevicesIter;
struct Device {
  DeviceType *m_deviceType;     // not a reference due to construction issues
  std::string m_name;           // a platform-scoped device name - usually type<ordinal>
  // Constructor for defining new devices
  Device(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single,
	 const char *&err);
  static Device *
  create(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single,
	 const char *&err);
  static const char*
  parseDevices(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, Devices &devices);
  DeviceType &deviceType() { ocpiAssert(m_deviceType); return *m_deviceType; }
  std::string &name() { return m_name; }
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
  create(ezxml_t xml, const char *xfile, size_t &index, const char *&err);
  HdlConfig(ezxml_t xml, const char *xfile, size_t &index, const char *&err);
  virtual ~HdlConfig();
  HdlPlatform &platform() { assert(m_platform); return *m_platform; }
  DevInstances &devInstances() { return m_devInstances; }
  const char
    *addControlConnection(std::string &assy),
    *emitConfig(FILE *f);
};

// Container connections (user oriented high level)
struct ContConnect {
  Port *external;
  const DevInstance *devInstance;
  bool devInConfig;
  Port *port;
  Port *interconnect;
  ContConnect()
    : external(NULL), devInstance(NULL), devInConfig(false), port(NULL), interconnect(port) {
  }
};
typedef std::list<ContConnect> ContConnects;
typedef ContConnects::const_iterator ContConnectsIter;

typedef std::map<const char *, unsigned, OU::ConstCharComp> UNocs;
typedef UNocs::iterator UNocsIter;

// A container builds on a platform configuration and an assembly
// it adds devices and adapters
// You specify devices and external port mappings
// basically connecting external ports to device ports
// <connection external="foo" device="dev" [port="bar"]/>
// <device foo>
#define HDL_CONTAINER_ATTRS "platform", "config", "configuration", "assembly", "default"
#define HDL_CONTAINER_ELEMS "connection"
class HdlAssembly;
class HdlContainer : public Worker {
  HdlAssembly *m_appAssembly;
  HdlConfig   *m_config;       // FIXME should be reference
  DeviceTypes  m_deviceTypes;  // Device types introduced by cards in slots
  DevInstances m_devInstances; // basic physical devices, either on platform or cards
  Cards        m_cards;        // cards in this configuration
  const char *
  parseConnection(ezxml_t cx, ContConnect &c);
  const char *
  emitUNocConnection(std::string &assy, UNocs &uNocs, size_t &index, size_t baseIndex, const ContConnect &c);
  const char *
  emitConnection(std::string &assy, UNocs &uNocs, size_t &index, size_t baseIndex, const ContConnect &c);
public:  
  static HdlContainer *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlContainer(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~HdlContainer();
  const char
    *emitAttribute(const char *attr),
    *emitArtHDL(const char *outDir, const char *wksFile),
    *emitContainer(FILE *f);
};

#define HDL_ASSEMBLY_ATTRS "platform", "config", "configuration"
#define HDL_ASSEMBLY_ELEMS "connection"
class HdlAssembly : public Worker {
public:  
  static HdlAssembly *
  create(ezxml_t xml, const char *xfile, size_t &index, const char *&err);
  HdlAssembly(ezxml_t xml, const char *xfile, size_t &index, const char *&err);
  virtual ~HdlAssembly();
};



// Global to let worker know whether an assembly is being built or just a worker
extern bool hdlAssy;
#endif
