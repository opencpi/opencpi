#ifndef HDL_PLATFORM_H
#define HDL_PLATFORM_H
#include <assert.h>
#include <string>
#include "hdl-device.h"
#include "hdl-slot.h"

#define HDL_PLATFORM_ATTRS "dummy", "control"
#define HDL_PLATFORM_ELEMS \
  "cpmaster", "nocmaster", "device", "metadata", "slot", "timebase"


class HdlConfig;
class HdlPlatform : public Worker, public Board {
  friend class HdlConfig;
  Slots       m_slots;   // slots of the platform
  bool        m_control; // should the platform be used for control?
			 // FIXME should inherit device type and be a device
public:  
  static HdlPlatform *create(ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  HdlPlatform(ezxml_t xml, const char *xfile, Worker *parent, const char *&err);
  virtual ~HdlPlatform();

  const char *cname() const { return m_name.c_str(); }
  const Slots &slots() const { return m_slots; }
  void setControl(bool c) { m_control = c; }
  Slot *findSlot(const char *name, const char *&err) const;
};

#endif
