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

struct ExtTuple {
  Signal *signal;
  size_t index;
  std::string ext;
  bool single; // mapping is for a single signal in a vector
ExtTuple(Signal *signal, size_t index, const std::string &ext, bool single)
: signal(signal), index(index), ext(ext), single(single) {
  }
};
typedef std::list<ExtTuple> ExtMap_;
class ExtMap : public ExtMap_ {
 public:
  Signal *findSignal(const std::string &s, size_t &n) {
    for (ExtMap_::const_iterator i = begin(); i != end(); i++)
      if (!strcasecmp((*i).ext.c_str(), s.c_str())) {
	n = (*i).index;
	return (*i).signal;
      }
    return NULL;
  }
  const char *findSignal(Signal &s, size_t n, bool &isSingle) const {
    for (ExtMap_::const_iterator i = begin(); i != end(); i++)
      if ((*i).signal == &s && (*i).index == n) {
	isSingle = (*i).single;
	return (*i).ext.c_str();
      }
    return NULL;
  }
  void push_back(Signal *s, size_t n, const std::string &e, bool single) {
    ExtMap_::push_back(ExtTuple(s, n, e, single));
  }
};

#define myComment() hdlComment(m_language)
static inline const char *hdlComment(Language lang) { return lang == VHDL ? "--" : "//"; }
extern const char *endians[];

// These are for all implementaitons whether assembly or written
#define HDL_TOP_ATTRS "Pattern", "PortPattern", "DataWidth", "Language", "library"
// These are for implementaitons that you write (e.g. not generated assemblies)
#define HDL_IMPL_ATTRS GENERIC_IMPL_CONTROL_ATTRS, "RawProperties", "FirstRawProperty", "outer", "endian","emulate"
#define HDL_IMPL_ELEMS "timeinterface", "memoryinterface", "streaminterface", "messageinterface", "signal", "cpmaster", "time_service", "control", "metadata", "devsignals", "rawprop", "supports", "clock", "timebase"
#define HDL_ASSEMBLY_ATTRS "platform", "config", "configuration"
#define HDL_ASSEMBLY_ELEMS "connection"
class HdlAssembly : public Worker {
public:  
  static HdlAssembly *
    create(ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  HdlAssembly(ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  virtual ~HdlAssembly();
};



// Global to let worker know whether an assembly is being built or just a worker
//extern bool hdlAssy;

#endif
