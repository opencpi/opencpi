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

struct UNocChannel {
  unsigned m_currentNode;
  bool m_control; // control attribute of deferred previous node
  std::string m_client, m_port; // remember the previous client for deferred connection
  UNocChannel() : m_currentNode(0) {}
};
// The bookkeepping class for a unoc
struct UNoc {
  const char *m_name;
  WIPType m_type;
  size_t  m_width;
  unsigned m_currentChannel;
  std::vector<UNocChannel> m_channels;
  UNoc(const char *name, WIPType type, size_t width, size_t size)
    : m_name(name), m_type(type), m_width(width), m_currentChannel(0), m_channels(size) {}
  void addClient(std::string &assy, bool control, const char *client, const char *port);
  void terminate(std::string &assy);
};
// A UNoc is actually an array of interconnect "channels" all of the same type.
// The value of the map is a pair of:
//    the current channel
//    the vector of number of nodes on a given channel
typedef std::map<const char *, UNoc, OU::ConstCharComp> UNocs;
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
  //  const char *
  //  emitSDPConnection(std::string &assy, UNoc &unoc, size_t &index, const ContConnect &c);
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
