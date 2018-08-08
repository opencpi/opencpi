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

#include <limits>
#include "OcpiOsAssert.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilException.h"
#include "OcpiContainerApi.h"

namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OCPI {
  namespace API {
    PropertyAccess::~PropertyAccess(){}
    void Property::
    checkTypeAlways(const OU::Member *mp, BaseType ctype, size_t n, bool write) const {
      ocpiDebug("checkTypeAlways on %s, for %s which is %s", OU::baseTypeNames[ctype],
		m_info.cname(), OU::baseTypeNames[m_info.m_baseType]);
      const OU::Member &m = mp ? *mp : m_info;
      const char *err = NULL;
      if (write && !m_info.m_isWritable)
	err = "trying to write a non-writable property";
      else if (write && !m_worker.beforeStart() && m_info.m_isInitial)
	err = "trying to write a an initial property after worker is started";
      else if (!write && !m_info.m_isReadable)
	err = "trying to read a non-readable property";
      else if (m.m_baseType == OCPI_Struct)
	err = "struct type used as scalar type";
      else if (ctype != m.m_baseType)
	err = "incorrect type for this property";
      else if (n && m.m_isSequence) {
	if (n % m.m_nItems)
	  err = "number of items not a multiple of array size";
	else {
	  n /= m.m_nItems;
	  if (write && n > m.m_sequenceLength)
	    err = "sequence or array too long for this property";
	  else if (!write && n < m.m_sequenceLength)
	    err = "sequence or array not large enough for this property";
	}
      } else if (n && n != m.m_nItems)
	  err = "wrong number of values for non-sequence type";
      if (err)
	throwError(err);
    }
    // This is user-visible, initialized from information in the metadata
    // It is intended to be constructed on the user's stack - a cache of
    // just the items needed for fastest access
    Property::Property(Worker &w, const char *aname) :
      m_worker(w), m_readVaddr(NULL), m_writeVaddr(NULL),
      m_info(w.setupProperty(aname, m_writeVaddr, m_readVaddr)), m_ordinal(m_info.m_ordinal),
      m_readSync(m_info.m_readSync), m_writeSync(m_info.m_writeSync) {
    }
    // This is a sort of side-door from the application code
    // that already knows the property ordinal
    Property::Property(Worker &w, unsigned n) :
      m_worker(w), m_readVaddr(NULL), m_writeVaddr(NULL),
      m_info(w.setupProperty(n, m_writeVaddr, m_readVaddr)), m_ordinal(m_info.m_ordinal),
      m_readSync(m_info.m_readSync), m_writeSync(m_info.m_writeSync) {
    }
    // determine the member and offset for the actual access given the access list.
    const OU::Member &Property::
    descend(AccessList &list, size_t &offset) const {
      const OU::Member *m = &m_info;
      size_t nAccess = list.size();
      const Access *a = list.begin();
      OU::Member *mm;
      offset = 0;
      do {
	mm = NULL;
	if (m->m_isSequence) {
	  if (!nAccess || !a->m_number)
	    throwError("sequence property not indexed");
	  if (a->m_index > m->m_sequenceLength)
	    throwError("index greater than maximum sequence length");
	  offset += a->m_index * m->m_nBytes;
	  a++, nAccess--;
	}
	if (m->m_arrayRank) {
	  for (unsigned n = 0; n < m->m_arrayRank; ++n) {
	    if (!nAccess || !a->m_number)
	      throwError("insufficient indices for array value");
	    if (a->m_index >= m->m_arrayDimensions[n])
	      throwError("array index out of range");
	    offset += a->m_index * m->m_elementBytes;
	    a++, nAccess--;
	  }
	}
	if (m->m_nMembers) {
	  if (!nAccess || a->m_number)
	    throwError("structure member not specified");
	  for (unsigned n = 0; n < m->m_nMembers; n++)
	    if (!strcasecmp(a->m_member, m->m_members[n].m_name.c_str())) {
	      m = mm = &m->m_members[n];
	      break;
	    }
	  if (!mm)
	    throwError("member name not found in structure");
	  offset += mm->m_offset;
	  a++, nAccess--;
	} else if (m->m_type)
	  m = mm = m->m_type;
      } while (mm);
      if (a != list.end())
	throwError("extraneous access modifiers (indices or member names)");
      return *m;
    }
    BaseType Property::baseType() const {return m_info.m_baseType;}
    size_t Property::stringBufferLength() const {
      if (m_info.m_baseType != OCPI_String)
	throw "cannot use stringBufferLength() on properties that are not strings";
      return m_info.m_stringLength + 1;
}
    void Property::throwError(const char *err) const {
      throw OU::Error("Access error for property \"%s\":  %s", m_info.cname(), err);
    }

// yes, we really do want to compare floats with zero here
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#if __GNUC__ < 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ <= 7 ) )
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
    // This internal template method is called after all error checking is done and is not called
    // on strings, so the static_casts should not cause any unexpected results.
    template <typename val_t> void Property::
    setValueInternal(const OU::Member &m, size_t offset, const val_t val) const {
      switch (m.m_baseType) {

#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
	case OCPI_##pretty: {						\
	  ocpiDebug("setting internal: " #pretty " %u %u %u",		\
		    std::numeric_limits<run>::is_integer,		\
		    std::numeric_limits<val_t>::digits,			\
		    std::numeric_limits<run>::digits);			\
	  if (!std::numeric_limits<run>::is_integer &&			\
	      std::numeric_limits<val_t>::digits > std::numeric_limits<run>::digits) \
	    throwError("setting property will lose precision");	\
	  if (std::numeric_limits<val_t>::is_signed && !std::numeric_limits<run>::is_signed && \
	      val < 0)							\
	    throwError("setting unsigned property to negative value");	\
	  /* complicated only to avoid signed/unsigned integer comparison warnings */ \
	  if ((!std::numeric_limits<val_t>::is_signed || val >= 0) &&	\
	      ((std::numeric_limits<run>::is_signed ==			\
		std::numeric_limits<val_t>::is_signed ||		\
		!std::numeric_limits<run>::is_integer ||		\
		!std::numeric_limits<val_t>::is_integer) ?		\
	       val > std::numeric_limits<run>::max() :			\
	       /* here when integer sign mismatch */			\
	       static_cast<uint64_t>(val) >				\
	       static_cast<uint64_t>(std::numeric_limits<run>::max()))) \
	    throwError("setting value greater than maximum allowed");	\
	  const run mymin = std::numeric_limits<run>::is_integer ?	\
	    std::numeric_limits<run>::min() :				\
	    static_cast<run>(-std::numeric_limits<run>::max());		\
	  if (std::numeric_limits<val_t>::is_signed &&			\
	      ((std::numeric_limits<run>::is_signed ==			\
		std::numeric_limits<val_t>::is_signed ||		\
		!std::numeric_limits<run>::is_integer ||		\
		!std::numeric_limits<val_t>::is_integer) ?		\
	       val < mymin :						\
	       /* here when integer sign mismatch */			\
	       static_cast<int64_t>(val) < static_cast<int64_t>(mymin))) \
	    throwError("setting value less than minimum allowed");	\
	  set##pretty##Value(&m, offset, static_cast<run>(val));	\
	  break;							\
	}
	OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

      default:;
      }
    }
    template <typename val_t> val_t Property::
    getValueInternal(const OU::Member &m, size_t offset) const {
      switch (m.m_baseType) {
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
      case OCPI_##pretty: {						\
	ocpiDebug("getting internal: " #pretty);			\
	run val = get##pretty##Value(&m, offset);			\
	if (!std::numeric_limits<val_t>::is_integer &&			\
	    std::numeric_limits<run>::digits > std::numeric_limits<val_t>::digits) \
	  throwError("getting property will lose precision");		\
	if (std::numeric_limits<run>::is_signed && !std::numeric_limits<val_t>::is_signed && \
	    val < 0)							\
	  throwError("value is negative when unsigned value requested"); \
	/* complicated only to avoid signed/unsigned integer comparison warnings */ \
	if ((!std::numeric_limits<run>::is_signed || val >= 0) &&	\
	    ((std::numeric_limits<val_t>::is_signed ==			\
	      std::numeric_limits<run>::is_signed ||			\
	      !std::numeric_limits<val_t>::is_integer ||		\
	      !std::numeric_limits<run>::is_integer) ?			\
	     val > std::numeric_limits<val_t>::max() :			\
	     /* here when integer sign mismatch */			\
	     static_cast<uint64_t>(val) >				\
	     static_cast<uint64_t>(std::numeric_limits<val_t>::max()))) \
	  throwError("value greater than maximum allowed for requested type"); \
	const val_t mymin = std::numeric_limits<val_t>::is_integer ?	\
	  std::numeric_limits<val_t>::min() :				\
	  -std::numeric_limits<val_t>::max();				\
	if (std::numeric_limits<run>::is_signed &&			\
	    ((std::numeric_limits<val_t>::is_signed ==			\
	      std::numeric_limits<run>::is_signed ||			\
	      !std::numeric_limits<val_t>::is_integer ||		\
	      !std::numeric_limits<run>::is_integer) ?			\
	     val < mymin :						\
	     /* here when integer sign mismatch */			\
	     static_cast<int64_t>(val) < static_cast<int64_t>(mymin)))	\
	  throwError("value less than minimum allowed for requested type"); \
	return static_cast<val_t>(val);					\
      }
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE

      default:;
      }
      return 0; // not reached
    }
    // type specific scalar property setters
    // the argument type is NOT the base type of the property, but the API caller's type
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca,corba,letter,bits,run,pretty,store)	\
    template <> void Property::						\
    setValue<run>(const run val, AccessList &list) const {		\
      size_t offset;							\
      const OU::Member &m = descend(list, offset);			\
      ocpiDebug("Property::setValue on %s %s->%s\n", m_info.cname(),	\
		OU::baseTypeNames[OCPI_##pretty], OU::baseTypeNames[m_info.m_baseType]); \
      if (m.m_baseType != OCPI_String)				\
	throwError("setting non-string property with string value");	\
      set##pretty##Value(&m, offset, val);				\
    }
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store)		\
    template <> void Property::						\
    setValue<run>(run val, AccessList &list) const {			\
      size_t offset;							\
      const OU::Member &m = descend(list, offset);			\
      ocpiDebug("setValue on %s %s->%s\n", m_info.cname(),		\
		OU::baseTypeNames[OCPI_##pretty], OU::baseTypeNames[m_info.m_baseType]); \
      if (m.m_baseType == OCPI_##pretty)				\
	set##pretty##Value(&m, offset, val);				\
      else if (m_info.m_baseType == OCPI_String)			\
	throwError("setting string property with " #run " value");	\
      else								\
	setValueInternal<run>(m, offset, val);				\
    }									\
    template <> run Property::						\
    getValue<run>(AccessList &list) const {				\
      size_t offset;							\
      const OU::Member &m = descend(list, offset);			\
      ocpiDebug("getValue on %s %s->%s\n", m_info.cname(),		\
		OU::baseTypeNames[OCPI_##pretty], OU::baseTypeNames[m_info.m_baseType]);  \
      if (m.m_baseType == OCPI_String)					\
	throwError("getting a " #run " value from a string property");	\
      return m.m_baseType == OCPI_##pretty ?				\
	get##pretty##Value(&m, offset) : getValueInternal<run>(m, offset); \
    }
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
// re-allow the -Wfloat-equal warning
#pragma GCC diagnostic pop

    template <> void Property::
    setValue<std::string>(std::string val, AccessList &list) const {
      setValue<String>(val.c_str(), list);
    }
    void Property::setValue(const std::string &val, AccessList &list) const {
      setValue<String>(val.c_str(), list);
    }
    template <> std::string Property::
    getValue<std::string>(AccessList &list) const {
      size_t offset;
      const OU::Member &m = descend(list, offset);
      ocpiDebug("getValue on %s %s->%s\n", m_info.cname(),
		OU::baseTypeNames[OCPI_String], OU::baseTypeNames[m_info.m_baseType]);
      if (m.m_baseType != OCPI_String)
	throwError("getting a string value from a non-string property");
      std::vector<char> s(m.m_stringLength + 1);
      getStringValue(&m, offset, &s[0], s.size());
      return &s[0];
    }
#if 1
// easier in C++11 with std::enable_if etc.
#ifdef __APPLE__
    template <> long Property::
    getValue<long>(AccessList &list) const {
      size_t offset;
      const OU::Member &m = descend(list, offset);
      ocpiDebug("getValue on %s %s->%s\n", m_info.cname(),
		OU::baseTypeNames[OCPI_Long], OU::baseTypeNames[m_info.m_baseType]);
      if (m.m_baseType == OCPI_String)
	throwError("getting a " "Long" " value from a string property");
      return m.m_baseType == OCPI_Long ?
	getLongValue(&m, offset) : getValueInternal<Long>(m, offset);
    }
    template <> unsigned long Property::
    getValue<unsigned long>(AccessList &list) const {
      size_t offset;
      const OU::Member &m = descend(list, offset);
      ocpiDebug("getValue on %s %s->%s\n", m_info.cname(),
		OU::baseTypeNames[OCPI_ULong], OU::baseTypeNames[m_info.m_baseType]);
      if (m.m_baseType == OCPI_String)
	throwError("getting a " "ULong" " value from a string property");
      return m.m_baseType == OCPI_ULong ?
	getULongValue(&m, offset) : getValueInternal<ULong>(m, offset);
    }
#endif
#endif
  }
}
