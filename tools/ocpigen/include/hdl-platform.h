#ifndef HDL_PLATFORM_H
#define HDL_PLATFORM_H
#include <assert.h>
#include "wip.h"
// These are for all implementaitons whether assembly or written
#define HDL_TOP_ATTRS "Pattern", "PortPattern", "DataWidth", "Language", "library"
// These are for implementaitons that you write (e.g. not generated assemblies)
#define HDL_IMPL_ATTRS GENERIC_IMPL_CONTROL_ATTRS, "RawProperties", "FirstRawProperty", "outer"
#define HDL_IMPL_ELEMS "timeinterface", "memoryinterface", "streaminterface", "messageinterface", "signal", "cpmaster", "time_service", "control", "metadata"

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
};

struct Device;
typedef std::list<Device *>     Devices;
typedef Devices::const_iterator DevicesIter;
struct Device {
  DeviceType *m_deviceType;     // not a reference due to construction issues
  std::string m_name;           // a platform-scoped device name - usually type<ordinal>
  size_t     m_width;
  bool        m_control;        // This device should control the platform
  Device(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single,
	 const char *&err);
  static Device *
  create(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, bool single,
	 const char *&err);
  static const char*
  parseDevices(ezxml_t xml, const char *parent, DeviceTypes &deviceTypes, Devices &devices);
  DeviceType &deviceType() { ocpiAssert(m_deviceType); return *m_deviceType; }
};

#define HDL_PLATFORM_ATTRS "dummy", "control"
#define HDL_PLATFORM_ELEMS "cpmaster", "nocmaster", "device", "timeservice", "metadata"
class HdlConfig;
class HdlPlatform : public Worker {
  DeviceTypes m_deviceTypes;  // usually from files, but possibly immediate
  friend class HdlConfig;
  bool m_control; // FIXME should inherit device type and be a device
  Devices     m_devices;      // basic physical devices
public:  
  static HdlPlatform *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlPlatform(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~HdlPlatform();
  Devices &devices() { return m_devices; }
  void setControl(bool c) { m_control = c; }
};

#define HDL_CONFIG_ATTRS "platform"
#define HDL_CONFIG_ELEMS "cpmaster", "nocmaster", "device"
class HdlConfig : public Worker {
  HdlPlatform *m_platform;
  //  DeviceTypes m_deviceTypes;  // usually from files, but possibly immediate
  
  Devices     m_devices; // basic physical devices
public:  
  static HdlConfig *
  create(ezxml_t xml, const char *xfile, size_t &index, const char *&err);
  HdlConfig(ezxml_t xml, const char *xfile, size_t &index, const char *&err);
  virtual ~HdlConfig();
  HdlPlatform &platform() { assert(m_platform); return *m_platform; }
  Devices &devices() { return m_devices; }
  const char
    *addControlConnection(std::string &assy),
    *emitConfig(FILE *f);
};

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
  HdlConfig *m_config;
  //  DeviceTypes m_deviceTypes;  // usually from files, but possibly immediate
  
  Devices     m_devices;      // basic physical devices
  const char *
  parseConnection(ezxml_t cx, Port *&external, Device *&device,
		  bool &devInConfig, Port *&port, Port *&interconnect);
  const char *
  emitConnection(std::string &assy, Port *external, Port *interconnect,
		 unsigned &unoc, size_t &index, size_t baseIndex);
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
