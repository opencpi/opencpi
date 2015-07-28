#ifndef HDL_CONFIG_H
#define HDL_CONFIG_H
#include <assert.h>
#include <vector>
#include <list>
#include "hdl-platform.h"
#include "hdl-card.h"


struct DevInstance {
  const Device &device;
  const Card *card;
  const Slot *slot;
  bool control;
  const DevInstance *parent;
  std::string m_name;
  std::vector<uint64_t> m_connected;
  DevInstance(const Device &d, const Card *c, const Slot *s, bool control,
	      const DevInstance *parent);
  const char *name() const { return m_name.c_str(); }
};

typedef std::list<DevInstance> DevInstances;
typedef DevInstances::iterator DevInstancesIter;

#define HDL_CONFIG_ATTRS "platform", "sdpWidth"
#define HDL_CONFIG_ELEMS "cpmaster", "nocmaster", "device", "property"

typedef std::vector<const Card*> Plugged;

class HdlHasDevInstances {
protected:
  HdlPlatform  &m_platform;
  Plugged      &m_plugged;
  DevInstances  m_devInstances; // instantiated in this config (or container)
  HdlHasDevInstances(HdlPlatform &platform, Plugged &plugged)
    : m_platform(platform), m_plugged(plugged) {}
  DevInstances &devInstances() { return m_devInstances; }
  const char *
  parseDevInstances(ezxml_t xml, const char *parent, DevInstances *baseInstances); 
  const char *
  parseDevInstance(const char *device, ezxml_t x, const char *parent, bool control,
		   DevInstances *baseInstances, // other list of instances to look at
		   const DevInstance **result,  // whether to report the result, and
		                                // if !NULL, its ok for it to already exist
		   bool *inBase);               // whether it was found in the base list

  DevInstance *
  findDevInstance(const Device &dev, const Card *card, const Slot *slot,
		  DevInstances *baseInstances, bool *inBase);
  const char *
  addDevInstance(const Device &dev, const Card *, const Slot *slot, bool control,
		 const DevInstance *parent, DevInstances *baseInstances,
		 const DevInstance *&devInstance);
  void emitSubdeviceConnections(std::string &assy, DevInstances *baseInstances);
};

class HdlContainer;
class HdlConfig : public Worker, public HdlHasDevInstances {
  friend class HdlContainer;
  HdlPlatform &m_platform;
  Plugged      m_plugged;      // what card is in each slot in this configuration
  size_t       m_sdpWidth;
public:  
  static HdlConfig *
  create(ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  HdlConfig(HdlPlatform &pf, ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  virtual ~HdlConfig();

  HdlPlatform &platform() { return m_platform; }
  size_t sdpWidth() { return m_sdpWidth; }
  const char
    *addControlConnection(std::string &assy),
    *emitConfig(FILE *f);
};

#endif
