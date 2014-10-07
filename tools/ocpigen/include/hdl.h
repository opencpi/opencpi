#ifndef HDL_H
#define HDL_H
#include <wip.h>

class HdlDevice;
typedef HdlDevice DeviceType;
typedef std::list<DeviceType *>     DeviceTypes;
typedef DeviceTypes::const_iterator DeviceTypesIter;
struct Device;
typedef std::list<Device *>     Devices;
typedef Devices::const_iterator DevicesIter;
#define myComment() hdlComment(m_language)
static inline const char *hdlComment(Language lang) { return lang == VHDL ? "--" : "//"; }

// These are for all implementaitons whether assembly or written
#define HDL_TOP_ATTRS "Pattern", "PortPattern", "DataWidth", "Language", "library"
// These are for implementaitons that you write (e.g. not generated assemblies)
#define HDL_IMPL_ATTRS GENERIC_IMPL_CONTROL_ATTRS, "RawProperties", "FirstRawProperty", "outer"
#define HDL_IMPL_ELEMS "timeinterface", "memoryinterface", "streaminterface", "messageinterface", "signal", "cpmaster", "time_service", "control", "metadata", "devsignals", "rawprop", "requires"
#define HDL_ASSEMBLY_ATTRS "platform", "config", "configuration"
#define HDL_ASSEMBLY_ELEMS "connection"
class HdlAssembly : public Worker {
public:  
  static HdlAssembly *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlAssembly(ezxml_t xml, const char *xfile, const char *&err);
  virtual ~HdlAssembly();
};



// Global to let worker know whether an assembly is being built or just a worker
extern bool hdlAssy;

#endif
