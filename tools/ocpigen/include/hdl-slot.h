#ifndef HDL_SLOT_H
#define HDL_SLOT_H
#include <string>
#include <map>
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "ocpigen.h"

// A slot type is really just a set of signals
// The direction is all from the perspective of the motherboard (a.k.a. carrier).
struct SlotType;
typedef std::map<const char *, SlotType *, OCPI::Util::ConstCharCaseEqual> SlotTypes;
typedef SlotTypes::iterator SlotTypesIter;
struct SlotType {
  std::string      m_name;
  Signals          m_signals;
  static SlotTypes s_slotTypes;
  SlotType(const char *file, const std::string &, const char *&err);
  virtual ~SlotType();

  const char *name() const { return m_name.c_str(); }
  static SlotType *
  get(const char *name, const char *parent, const char *&err);
  static SlotType
    *find(const char *name),
    *find(const std::string &name, const char *&err);
};

// A slot has a type, a name and a set of platform-specific names for its
// generic (standardized) signals.
// It is a physical part of a platform.
struct Slot;
typedef std::map<const char *, Slot *> Slots;
typedef Slots::const_iterator SlotsIter;
struct Slot {
  std::string                     m_name;
  const SlotType                 &m_type;
  unsigned                        m_ordinal;
  std::map<Signal *, std::string> m_signals;
  typedef std::map<Signal *, std::string>::const_iterator SignalsIter;
  Slot(ezxml_t xml, const char *parent, const std::string &name, const SlotType &type,
       unsigned ordinal, const char *&err);
  virtual ~Slot();
  static Slot
    // Make a new one.  It is not expected to exist
    *create(ezxml_t xml, const char *parent, Slots &slots, unsigned typeOrdinal,
	    unsigned typeTotal, unsigned ordinal, const char *&err),
    // Find an existing one.  It is expected to exist
    *find(const char *name, const Slots &slots, const char *&err);
  const char *name() const { return m_name.c_str(); }
  const SlotType *type() const { return &m_type; }
};


#endif
