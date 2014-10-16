#include "cdkutils.h"
#include "hdl-slot.h"

namespace OE=OCPI::Util::EzXml;
namespace OU=OCPI::Util;

SlotTypes SlotType::s_slotTypes;

SlotType::
SlotType(const char *file, const std::string &parent, const char *&err) {
  ezxml_t xml;
  std::string xfile;
  err = NULL;
  if ((err = parseFile(file, parent, NULL, &xml, xfile)) ||
      (err = Signal::parseSignals(xml, parent, m_signals, m_sigmap)))
    return;
  OE::getOptionalString(xml, m_name, "name");
  char *cp = strdup(xfile.c_str());
  char *slash = strrchr(cp, '/');
  if (slash)
    slash++;
  else
    slash = cp;
  char *dot = strchr(slash, '.');
  if (dot)
    *dot = '\0';
  if (m_name.empty())
    m_name = slash;
  else if (m_name != slash)
    err = OU::esprintf("File name (%s) does not match name attribute in XML (%s)",
		       xfile.c_str(), m_name.c_str());
  free(cp);
}

SlotType::
~SlotType() {
  Signal::deleteSignals(m_signals);
}

// Slot types are interned (only created when not already there)
// They are not inlined or included, but simply referenced by attributes.
SlotType *SlotType::
get(const char *name, const char *parent, const char *&err) {
  SlotType *st = find(name);
  if (!st) {
    st = new SlotType(name, parent, err);
    if (err) {
      delete st;
      st = NULL;
    } else
      s_slotTypes[name] = st;
  }
  return st;
}

SlotType *SlotType::
find(const char *name) {
  SlotTypesIter sti = s_slotTypes.find(name);
  return sti == s_slotTypes.end() ? NULL : sti->second;
}

#if 0
// static
SlotType * SlotType::
find(const std::string &type, const char *&err) {
  SlotTypesIter ti = s_slotTypes.find(type.c_str());
  if (ti == s_slotTypes.end()) {
    err = OU::esprintf("Card '%s' refers to slot type '%s' that is not on this platform",
		       name.c_str(), type.c_str());
    return NULL;
  }
  return *ti->second;
}
#endif

// A slot may have a default mapping to the external platform's signals,
// ie. <slot-name>_signal.
Slot::
Slot(ezxml_t xml, const char */*parent*/, const std::string &name, const SlotType &type,
     unsigned ordinal, const char *&err)
  : m_name(name), m_type(type), m_ordinal(ordinal)
{
  err = NULL;
  // process non-default signals: slot=pfsig, platform=dddd
  for (ezxml_t xs = ezxml_cchild(xml, "Signal"); xs; xs = ezxml_next(xs)) {
    std::string slot, platform;
    if ((err = OE::getRequiredString(xs, slot, "slot")) ||
	(err = OE::getRequiredString(xs, platform, "platform")))
      break;
    const Signal *s = m_type.m_sigmap.findSignal(slot);
    if (!s)
      err = OU::esprintf("Slot signal '%s' does not exist for slot type '%s'",
			 slot.c_str(), m_type.m_name.c_str());
    else if (m_signals.find(s) != m_signals.end())
      err = OU::esprintf("Duplicate slot signal: %s", slot.c_str());
    else
      m_signals[s] = platform;
  }
  if (err)
    err = OU::esprintf("Error for slot '%s': %s", m_name.c_str(), err);
}

Slot::
~Slot() {
}

// Slots are not interned, and we want the type to be a reference.
// Hence we check the type first.
Slot *Slot::
create(ezxml_t xml, const char *parent, Slots &slots, unsigned typeOrdinal,
       unsigned typeTotal, unsigned ordinal, const char *&err) {
  std::string type;
  SlotType *t;
  if ((err = OE::getRequiredString(xml, type, "type")) ||
      !(t = SlotType::get(type.c_str(), OE::ezxml_tag(xml), err)))
    return NULL;
  std::string name;
  if (!OE::getOptionalString(xml, name, "name")) {
    name = type;
    if (typeTotal > 1)
      OU::format(name, "%s%u", type.c_str(), typeOrdinal);
  }
  if (find(name.c_str(), slots, err)) {
    err = OU::esprintf("Duplicate slot name (%s) in '%s' element", name.c_str(), parent);
    return NULL;
  }
  Slot *s = new Slot(xml, parent, name, *t, ordinal, err);
  if (err) {
    delete s;
    return NULL;
  }
  return slots[s->m_name.c_str()] = s;
}

Slot *Slot::
find(const char *name, const Slots &slots, const char *&err) {
  SlotsIter si = slots.find(name);
  if (si == slots.end()) {
    err = OU::esprintf("There is no slot named (%s) in the platform", name);
    return NULL;
  }
  return si->second;
}
