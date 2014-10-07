#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <map>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "assembly.h"
#include "hdl.h"
#include "hdl-platform.h"

namespace OU = OCPI::Util;
namespace OE = OCPI::Util::EzXml;

HdlPlatform *HdlPlatform::
create(ezxml_t xml, const char *xfile, const char *&err) {
  err = NULL;
  HdlPlatform *p = new HdlPlatform(xml, xfile, err);
  if (err) {
    delete p;
    return NULL;
  }
  return p;
}

HdlPlatform::
HdlPlatform(ezxml_t xml, const char *xfile, const char *&err)
  : Worker(xml, xfile, "", NULL, err), m_control(false) {
  if (err ||
      (err = OE::checkAttrs(xml, IMPL_ATTRS, GENERIC_IMPL_CONTROL_ATTRS, HDL_TOP_ATTRS,
			    HDL_IMPL_ATTRS, HDL_PLATFORM_ATTRS, (void*)0)) ||
      (err = OE::checkElements(xml, IMPL_ELEMS, HDL_IMPL_ELEMS, HDL_PLATFORM_ELEMS, (void*)0)) ||
      (err = parseHdl("ocpi")) ||
      (err = OE::getBoolean(xml, "control", &m_control)))
    return;
  if ((err = Device::addDevices(*this, xml, m_devices)))
    return;
  unsigned n = 0;
  for (ezxml_t xs = ezxml_cchild(xml, "slot"); xs; xs = ezxml_next(xs), n++) {
    const char *type = ezxml_cattr(xs, "type");
    unsigned ordinal = 0, count = 0;
    for (ezxml_t x = ezxml_cchild(xml, "slot"); x; x = ezxml_next(x)) {
      const char *otype = ezxml_cattr(x, "type");
      if (!strcasecmp(type, otype)) {
	if (x == xs)
	  ordinal = count;
	count++;
      }
    }
    // First, figure out how many there are of the same type
    Slot *s = Slot::create(xs, OE::ezxml_tag(xml), m_slots, ordinal, count, n, err);
    if (!s)
      return;
    // Add the slot signals to this worker
    for (SignalsIter si = s->m_type.m_signals.begin(); si != s->m_type.m_signals.end(); si++) {
      Signal *sig = new Signal(**si);
      Slot::SignalsIter ssi = s->m_signals.find((*si));
      sig->m_name = s->m_name + "_" +
	(ssi == s->m_signals.end() ? (*si)->name() : ssi->second.c_str());
      m_signals.push_back(sig);
    }
  }
}

HdlPlatform::
~HdlPlatform() {
  while (m_devices.size()) {
    Device *d = m_devices.front();
    m_devices.pop_front();
    delete d;
  }
}

Slot *HdlPlatform::
findSlot(const char *name, const char *&err) const {
  return Slot::find(name, m_slots, err);
}
