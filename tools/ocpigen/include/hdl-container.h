#ifndef HDL_CONTAINER_H
#define HDL_CONTAINER_H
#include <map>
#include <list>
#include <set>
#include "hdl-config.h"

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
// The pf config has device instances, and cards in slots
// The container object has MORE devices instances and cards in slots
// You specify devices and external port mappings
// basically connecting external ports to device ports
// <connection external="foo" device="dev" [port="bar"]/>
// <device foo>
#define HDL_CONTAINER_ATTRS "platform", "config", "configuration", "assembly", "default"
#define HDL_CONTAINER_ELEMS "connection", "device"
class HdlAssembly;
class HdlContainer : public Worker, public HdlHasDevInstances {
  HdlAssembly &m_appAssembly;
  HdlConfig   &m_config;
  // This maps top level signals to the instance signal it is mapped from.
  typedef std::map<Signal*,std::string> ConnectedSignals;
  ConnectedSignals m_connectedSignals;
  typedef ConnectedSignals::iterator ConnectedSignalsIter;
  const char *
  parseConnection(ezxml_t cx, ContConnect &c);
  const char *
  emitUNocConnection(std::string &assy, UNocs &uNocs, size_t &index, const ContConnect &c);
  const char *
  emitSDPConnection(std::string &assy, unsigned &unoc, size_t &index, const ContConnect &c);
  const char *
  emitConnection(std::string &assy, UNocs &uNocs, size_t &index, const ContConnect &c);
public:  
  static HdlContainer *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlContainer(HdlConfig &config, HdlAssembly &appAssembly, ezxml_t xml, const char *xfile,
	       const char *&err);
  virtual ~HdlContainer();
  const char
    *emitAttribute(const char *attr),
    //    *emitArtXML(const char *wksFile),
    *emitContainer(FILE *f),
    *emitUuid(const OU::Uuid &uuid);

  void 
    emitDeviceSignalMapping(FILE *f, std::string &last, Signal &s),
    emitDeviceSignal(FILE *f, Language lang, std::string &last, Signal &s),
    recordSignalConnection(Signal &s, const char *from),
    emitTieoffSignals(FILE *f),
    emitXmlWorkers(FILE *f),
    emitXmlInstances(FILE *f),
    emitXmlConnections(FILE *f),
    mapDevSignals(std::string &assy, const DevInstance &devInstance, bool inContainer);
};

#endif
