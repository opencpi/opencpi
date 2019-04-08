/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
// key type is a pointer since it all comes from parsed XML files, but std::string would be safer
typedef std::map<const char *, SlotType *, OCPI::Util::ConstCharCaseComp> SlotTypes;
// AV-1482/AV-1490 fix: typedef std::map<std::string, SlotType *, OCPI::Util::ConstStringCaseComp> SlotTypes;
typedef SlotTypes::iterator SlotTypesIter;
struct SlotType {
  std::string      m_name;
  Signals          m_signals;
  SigMap           m_sigmap;
  static SlotTypes s_slotTypes;
  SlotType(const char *file, const std::string &, const char *&err);
  virtual ~SlotType();

  const char *cname() const { return m_name.c_str(); }
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
typedef std::map<const char *, Slot *, OCPI::Util::ConstCharCaseComp> Slots;
typedef Slots::const_iterator SlotsIter;
struct Slot {
  std::string                     m_name;
  std::string                     m_prefix; // prefix for platform signals if not <name>_
  const SlotType                 &m_type;
  unsigned                        m_ordinal;
  // A map from the underlying slot type signal to an override signal
  std::map<const Signal *, std::string> m_signals;
  typedef std::map<const Signal *, std::string>::const_iterator SignalsIter;
  Slot(ezxml_t xml, const char *parent, const std::string &name, const SlotType &type,
       unsigned ordinal, const char *&err);
  virtual ~Slot();
  static Slot
    // Make a new one.  It is not expected to exist
    *create(ezxml_t xml, const char *parent, Slots &slots, unsigned typeOrdinal,
	    unsigned typeTotal, unsigned ordinal, const char *&err),
    // Find an existing one.  It is expected to exist
    *find(const char *name, const Slots &slots, const char *&err);
  const char *cname() const { return m_name.c_str(); }
  const SlotType *type() const { return &m_type; }
};


#endif
