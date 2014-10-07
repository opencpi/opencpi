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
  std::string name;
  DevInstance(const Device &d, const Card *c, const Slot *s, bool control = false)
  : device(d), card(c), slot(s), control(control) {
  }
};

typedef std::list<DevInstance> DevInstances;
typedef DevInstances::const_iterator DevInstancesIter;

#define HDL_CONFIG_ATTRS "platform"
#define HDL_CONFIG_ELEMS "cpmaster", "nocmaster", "device"

typedef std::vector<const Card*> Plugged;

class HdlHasDevInstances {
  HdlPlatform  &m_platform;
  Plugged      &m_plugged;
protected:
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

  const DevInstance *
  findDevInstance(const Device &dev, const Card *card, const Slot *slot,
		  DevInstances *baseInstances, bool *inBase);
  const char *
  addDevInstance(const Device &dev, const Card *, const Slot *slot, bool control,
		 const DevInstance *&devInstance);
};

class HdlContainer;
class HdlConfig : public Worker, public HdlHasDevInstances {
  friend class HdlContainer;
  HdlPlatform &m_platform;
  Plugged      m_plugged;      // what card is in each slot in this configuration
public:  
  static HdlConfig *
  create(ezxml_t xml, const char *xfile, const char *&err);
  HdlConfig(HdlPlatform &pf, ezxml_t xml, const char *xfile, const char *&err);
  virtual ~HdlConfig();

  HdlPlatform &platform() { return m_platform; }
  const char
    *addControlConnection(std::string &assy),
    *emitConfig(FILE *f);
};

#endif