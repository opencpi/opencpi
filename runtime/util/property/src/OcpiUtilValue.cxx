/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "OcpiOsAssert.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilDataTypes.h"

namespace OA = OCPI::API;
namespace OE = OCPI::Util::EzXml;
namespace OCPI {
  namespace Util {

    static inline long myRandom() { return random() * (random() & 1 ? -1 : 1); }
#if 0
    // Return true on error
    bool
    getUNum64(const char *s, uint64_t *valp) {
      char *endptr;
      errno = 0;
      uint64_t val =  strtoull(s, &endptr, 0);
      if (errno == 0) {
	if (*endptr == 'K' || *endptr == 'k') {
	  endptr++;
	  val *= 1024;
	} else if (*endptr == 'M' || *endptr == 'm') {
	  endptr++;
	  val *= 1024*1024;
	} else if (*endptr == 'G' || *endptr == 'g') {
	  endptr++;
	  val *= 1024ull*1024ull*1024ull;
	}
	while (isspace(*endptr))
	  endptr++;
	if (*endptr == '-') {
	  endptr++;
	  while (isspace(*endptr))
	    endptr++;
	  if (*endptr++ == '1') {
	    while (isspace(*endptr))
	      endptr++;
	    if (!*endptr)
	      val--;
	  }
	}
	if (*endptr)
	  return true;
	*valp = val;
	return false;
      }
      return true;
    }
#endif
#if 0
    // return true on error
    static bool
    getNum64(const char *s, const char *end, int64_t &valp) {
      while ((!end || s < end) && isspace(*s))
	s++;
      do {
	if (end && s >= end)
	  break;
	bool minus = false;
	if (*s == '-') {
	  minus = true;
	  s++;
	  while ((!end || s < end) && isspace(*s))
	    s++;
	  if (end && s >= end)
	    break;
	}
	uint64_t uval;
	if (OE::getUNum64(s, end, uval))
	  break;
	if (minus) {
	  if (uval > ((uint64_t)1) << 63)
	    break;
	} else if (uval > INT64_MIN)
	  break;
	val = (int64_t)uval;
	return false;
      } while (0);
      char *endptr;
      errno = 0;
      int64_t val =  strtoll(s, &endptr, 0);
      if (errno == 0) {
	if (*endptr == 'K' || *endptr == 'k') {
	  endptr++;
	  val *= 1024;
	} else if (*endptr == 'M' || *endptr == 'm') {
	  endptr++;
	  val *= 1024*1024;
	} else if (*endptr == 'G' || *endptr == 'g') {
	  endptr++;
	  val *= 1024ll*1024ll*1024ll;
	}
	*valp = val;
	return false;
      }
      return true;
    }
#endif

    // Find an element, which might be empty, trimming whitespace
    // Return true if there is more
    // "unparsed" is advanced past the found element.
    // "start" and "end" is the element found
    // The fundamental delimiter of elements is comma,
    // but braces must be counted since they can have commas within
    // The "comma" argument indicates whether the comma must be present.
    // If false, a top level close brace can also terminate the element.
    static const char *
    doElement(const char *&unparsed, const char *stop, const char *&start, const char *&end,
	      bool comma = true) {
      // Skip initial white space
      const char *tmp = unparsed;
      while (unparsed != stop && isspace(*unparsed))
	unparsed++;
      start = unparsed;
      end = stop;
      unsigned nBraces = 0;
      const char *last = start;
      while (unparsed < stop && *unparsed)
	switch (*unparsed++) {
	case ',':
	  if (nBraces == 0) {
	    if (!comma)
	      return "unexpected comma when parsing arrays";
	    end = unparsed - 1;
	    goto break2;
	  }
	  break;
	case '\\':
	  if (unparsed < stop)
	    unparsed++;
	  last = unparsed; // remember last white space to protect
	  break;
	case '{':
	  nBraces++; break;
	case '}':
	  if (nBraces == 0)
	    return esprintf("unbalanced braces - extra close brace for (%*s) (%zu)",
			    (int)(stop - tmp), tmp, stop - tmp);
	  if (--nBraces == 0 && !comma) {
	    end = unparsed;
	    goto break2;
	  }
	}
    break2:
      while (end > last && isspace(end[-1]))
	end--;
      return NULL;
    }

    Value::Value(const ValueType &vt, const Value *parent)
      : m_vt(&vt), m_parent(parent) {
      init();
    }

    // Used when it is in an array
    Value::Value()
      : m_vt(s_vt), m_parent(s_parent) {
      init();
    }

    void Value::setType(const ValueType &vt) {
      m_vt = &vt;
      m_parent = NULL;
    }

    void Value::init() {
      m_nElements = 0;
      m_nTotal = 0;
      m_stringSpace = NULL;
      m_stringNext = NULL;
      m_stringSpaceLength = 0;
      m_struct = NULL;
      m_structNext = NULL;
      m_types = NULL;
      m_typeNext = NULL;
      //      m_parent = NULL;
      m_next = 0;
      m_length = 0;
      m_ULongLong = 0;
      m_parsed = false;
    }

    const char *Value::
    parseBool(const char *a, const char *end, bool &b)
    {
      (void)end;
      return OCPI::Util::EzXml::parseBool(a, end, &b) ? "bad boolean value" : NULL;
    }

    // cp points after slash at the 'x'
    // leave cp on last char used if no error
    static bool
    hex(const char *&cp, const char *end, char &vp) {
      if (cp < --end) { // end is now last, not past last
	char c = *++cp;      // must have one more to be valid
	if (isxdigit(c)) {
	  unsigned n;
	  n = isdigit(c) ? c - '0' : (tolower(c) - 'a') + 10;
	  if (cp < end && isxdigit(cp[1])) {
	    n <<= 4;
	    c = *++cp;
	    n += isdigit(c) ? c - '0' : (tolower(c) - 'a') + 10;
	  }
	  vp = (char)(n);
	  return false;
	}
      }
      return true;
    }

    // cp points after slash at the 'd'
    // leave cp on last char used if no error
    static bool
    decimal(const char *&cp, const char *end, char &vp, bool isSigned) {
      if (cp < --end) { // end is now last, not past last
	char c = *++cp;      // must have one more to be valid
	bool minus = false;
	if (isSigned && c == '-') {
	  if (cp >= end)
	    return true;
	  minus = true;
	  c = *++cp;
	}
	if (isdigit(c)) {
	  int n = c - '0';
	  if (cp < end && isdigit(cp[1])) {
	    n *= 10;
	    n += *++cp - '0';
	    if (cp < end && isdigit(cp[1])) {
	      n *= 10;
	      n += *++cp - '0';
	      if (n > (minus ? 128 : (isSigned ? 127 : 255)))
		return true;
	    }	    
	  }
	  vp = (char)(minus ? -n : n);
	  return false;
	}
      }
      return true;
    }

    // cp points after slash to an octal digit - 1, 2 or 3 octal digits
    // leave cp on last char used if no error
    static bool
    octal(const char *&cp, const char *end, char &vp) {
      char c = *cp;
      unsigned n = c - '0';
      if (cp < --end) { // end is now the last, not past the last
	c = cp[1];
	if (isdigit(c) && c <= '7') {
	  cp++;
	  n <<= 3;
	  n += c - '0';
	  if (cp < end) {
	    c = cp[1];
	    if (isdigit(c) && c <= '7') {
	      cp++;
	      n <<= 3;
	      n += c - '0';
	      if (n > 0xff)
		return true;
	    }
	  }
	}
      }
      vp = (char)n;
      return false;
    }
    // We implement the defined escapes (in IDL, and C/C++), and we make sure
    // escaping commas and braces works too
    // We advance cp over the char
    bool
    parseOneChar(const char *&cp, const char *end, char &vp) {
      if (!*cp || cp == end)
	return true;
      if (*cp != '\\' || cp + 1 >= end) {
	vp = *cp++;
	return false;
      }
      switch (*++cp) {
      case 'n': vp = '\n'; break;
      case 't': vp = '\t'; break;
      case 'v': vp = '\v'; break;
      case 'b': vp = '\b'; break;
      case 'r': vp = '\r'; break;
      case 'f': vp = '\f'; break;
      case 'a': vp = '\a'; break;
      case 'd':
	if (decimal(cp, end, vp, true))
	  return true;
	break;
      case 'u':
	if (decimal(cp, end, vp, false))
	  return true;
	break;
      case 'x':
	if (hex(cp, end, vp))
	  return true;
	break;
      case '0': case '1': case '2': case '3': 
      case '4': case '5': case '6': case '7':
	if (octal(cp, end, vp))
	  return true;
	break;
      case '\\':
      case '?':
      case '\'':
      case '\"':
      case ',': case '{': case '}': case ' ':// these are ours, not from C/C++/IDL
	vp = *cp;
	break;
      default:
	return true;
      }
      cp++;
      return false;
    }
    const char *Value::
    parseChar(const char*cp, const char *end, char &vp) {
      const char *tmp = cp;
      return parseOneChar(tmp, end, vp) || tmp != end ? "bad Char value" : NULL;
    }
    const char *Value::
    parseDouble(const char*cp, const char *end, double &vp) {
      char *endptr;
      errno = 0;
      vp = strtod(cp, &endptr);
      if (endptr == cp || errno || end && endptr > end)
	return "bad Double value";
      return NULL;
    }
    const char *Value::
    parseFloat(const char*cp, const char *end, float &vp) {
      char *endptr;
      errno = 0;
      vp = strtof(cp, &endptr);
      if (endptr == cp || errno || end && endptr > end)
	return "bad Float value";
      return NULL;
    }
    const char *Value::
    parseShort(const char*cp, const char *end, int16_t &vp) {
      int64_t n;
      if (OE::getNum64(cp, end, n) || n > INT16_MAX || n < INT16_MIN)
	return "bad Short value";
      vp = (int16_t)n;
      return NULL;
    }
    const char *Value::
    parseLong(const char*cp, const char *end, int32_t &vp) {
      int64_t n;
      if (OE::getNum64(cp, end, n) || n > INT32_MAX || n < INT32_MIN)
	return "bad Long value";
      vp = (int32_t)n;
      return NULL;
    }
    const char *Value::
    parseUChar(const char*cp, const char *end, uint8_t &vp) {
      while (isspace(*cp))
	cp++;
      if (*cp == '\'') {
	if (*++cp == '\\')
	  cp++;
	vp = *cp;
      } else {
	uint64_t n;
	if (OE::getUNum64(cp, end, n) || n > UINT8_MAX)
	  return "bad UChar value";
	vp = (uint8_t)n;
      }
      return NULL;
    }
    const char *Value::
    parseULong(const char *cp, const char *end, uint32_t &vp) {
      uint64_t n;
      if (OE::getUNum64(cp, end, n) || n > UINT32_MAX)
	return "bad ULong value";
      vp = (uint32_t)n;
      return NULL;
    }
    const char *Value::
    parseUShort(const char*cp, const char *end, uint16_t &vp) {
      uint64_t n;
      if (OE::getUNum64(cp, end, n) || n > UINT16_MAX)
	return "bad UShort value";
      vp = (uint16_t)n;
      return NULL;
    }
    const char *Value::
    parseLongLong(const char*cp, const char *end, int64_t &vp) {
      int64_t n;
      if (OE::getNum64(cp, end, n) || n > INT64_MAX || n < INT64_MIN)
	return "bad LongLong value";
      vp = n;
      return NULL;
    }
    const char *Value::
    parseULongLong(const char*cp, const char *end, uint64_t &vp) {
      uint64_t n;
      if (OE::getUNum64(cp, end, n))
	return "bad ULongLong value";
      vp = n;
      return NULL;
    }
    // Strings can have double quotes
    // In particular, "" is the only way to define an empty string that
    // doesn't look like an empty sequence when you have a sequence of strings
    // with one empty string in it.
    const char *Value::
    parseString(const char*cp, const char *end, OA::CharP &vp) {
      // Check length if string is bounded
      
      char *start = m_stringNext;
      bool quoted = false;
      if (*cp == '"') {
	quoted = true;
	cp++;
      }
	
      for (unsigned len = 0; cp < end; len++)
	if (quoted && *cp == '"') {
	  if (cp + 1 != end)
	    return "double quoted string with invalid characters after the closing quote";
	  break;
	} else if (m_vt->m_stringLength && len >= m_vt->m_stringLength ||
		   parseOneChar(cp, end, *m_stringNext++))
	  return "bad String value";
      *m_stringNext++ = 0;
      vp = (const char *)start;
      return NULL;
    }
    // Parse the elements of the structure.  This is like the data-type-specific methods.
    const char *Value::
    parseStruct(const char *unparsed, const char *stop, StructValue &sv) {
      sv = m_structNext;
      m_structNext += m_vt->m_nMembers;
      while (unparsed < stop) {
	const char *start, *end;
	doElement(unparsed, stop, start, end);
	if (start == end)
	  return "empty member value in struct value";
	size_t n, len;
	for (n = 0; n < m_vt->m_nMembers; n++) {
	  const char *mName = m_vt->m_members[n].m_name.c_str();
	  len = strlen(mName);
	  if (!strncmp(mName, start, len) && isspace(start[len]))
	    break;
	}
	if (n >= m_vt->m_nMembers)
	  return "unknown member name in struct value";
	if (sv[n])
	  return "duplicate member name in struct value";
	Value *v = sv[n] = new Value(m_vt->m_members[n], this);
	const char *err;
	start += len;
	while (isspace(*start) && start < end)
	  start++;
	if (v->needsComma()) {
	  if (*start != '{' || end[-1] != '}')
	    return esprintf("struct member value needs be enclosed in {} (%*s) (%zu)",
			    (int)(end - start), start, end - start);
	  start++;
	  end--;
	}
	if ((err = v->parse(start, end)))
	  return err;
      }
      return NULL;
    }
    // Parse the elements of the structure.  This is like the data-type-specific methods.
    const char *Value::
    parseType(const char *unparsed, const char *stop, TypeValue &sv) {
      sv = m_typeNext++;
      if (sv->needsComma()) {
	if (*unparsed != '{' || stop[-1] != '}')
	  return "recursize type needs to be enclosed in {}";
	unparsed++;
	stop--;
      }
      //      if (!strncmp("{}", unparsed, 2))
      // srand(0);
      return sv->parse(unparsed, stop);
    }
    const char *Value::
    parseEnum(const char *unparsed, const char *stop, EnumValue &sv) {
      for (size_t n = 0; n < m_vt->m_nEnums; n++) {
	size_t len = strlen(m_vt->m_enums[n]);
	if (!strncasecmp(m_vt->m_enums[n], unparsed, len) &&
	    unparsed + len == stop) {
	  sv = (EnumValue)n;
	  return NULL;
	}
      }
      return "unknown enum value";
    }

    // We expect to parse into the value more that once.
    void Value::
    clear() {
      // Special storage for sparse structure values
      if (m_struct) {
	for (unsigned n = 0; n < m_nTotal * m_vt->m_nMembers; n++)
	  if (m_struct[n]) {
	    delete m_struct[n];
	    m_struct[n] = 0;
	  }
	delete []m_struct;
      }
      // Special storage for string values
      if (m_stringSpace) {
	delete [] m_stringSpace;
	m_stringSpace = NULL;
	m_stringSpaceLength = 0;
      }
      if (m_types) {
	delete [] m_types;
	m_types = NULL;
      }
      if (m_nTotal && (m_vt->m_isSequence || m_vt->m_arrayRank)) {
	switch(m_vt->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)	 \
	case OA::OCPI_##pretty:			         \
	  delete [] m_p##pretty;			 \
	  break;
	  OCPI_PROPERTY_DATA_TYPES
	  OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
          OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
          OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
	case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
	}
	m_nTotal = 0;
	m_nElements = 0;
      }
    }
    Value::~Value() {
      clear();
    }

    const ValueType *Value::s_vt;
    Value *Value::s_parent;
    OS::Mutex Value::s_mutex;

    // This method is used both for parsing and for generating test values.
    // m_nElements has been established
    const char *
    Value::allocate(bool add) {
      if (m_vt->m_isSequence) {
	if (m_nElements == 0)
	  return NULL;
	if (m_vt->m_sequenceLength && m_nElements > m_vt->m_sequenceLength)
	  return esprintf("Too many elements (%zu) in bounded sequence (%zu)",
			  m_nElements, m_vt->m_sequenceLength);
      }
      size_t oldLength = m_length;
      switch (m_vt->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)     \
      case OA::OCPI_##pretty:			       \
	m_length = m_nTotal * sizeof(run);	       \
	if (m_vt->m_isSequence || m_vt->m_arrayRank) { \
	  run *old = m_p##pretty;                      \
	  m_p##pretty = new run[m_nTotal];	       \
          /* FIXME: type-specific default value? */    \
          memset(m_p##pretty, 0, m_length);            \
	  if (add) {                                   \
	    memcpy(m_p##pretty, old, oldLength);       \
	    delete [] old;                             \
	  }                                            \
        }                                              \
	break;
	OCPI_PROPERTY_DATA_TYPES
	OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
	OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
	OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
#undef OCPI_DATA_TYPE
      case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
      }
      if (m_vt->m_baseType == OA::OCPI_Struct) {
	assert(!add);
	size_t nElements = m_nTotal * m_vt->m_nMembers;
	m_struct = m_structNext = new Value *[nElements];
	for (size_t n = 0; n < nElements; n++)
	  m_struct[n] = 0;
      } else if (m_vt->m_baseType == OA::OCPI_Type) {
	assert(!add);
	// This mutex/static ugliness is to supply an argument to the default constructor of Value
	AutoMutex am(s_mutex);
	Value::s_vt = m_vt->m_type;
	Value::s_parent = this;
	m_types = m_typeNext = new Value[m_nTotal];
      } else if (m_vt->m_baseType == OA::OCPI_String && !m_stringSpace) {
	assert(!add);
	m_stringSpaceLength = m_nTotal * (testMaxStringLength + 1);
	m_stringNext = m_stringSpace = new char[m_stringSpaceLength];
      }
      return NULL;
    }

    // Overloaded with the base case (parsing a whole value),
    // and adding an element to a sequence value
    const char *Value::
    parse(const char *unparsed, const char *stop, bool add) {
      const char *err = NULL;
      if (!stop)
	stop = unparsed + strlen(unparsed);
      if (!add)
	clear();
      if (m_vt->m_baseType == OA::OCPI_String) {
	char *old = m_stringSpace;
	size_t oldLength = m_stringSpaceLength;
	// the space required will never be larger than the input...
	m_stringSpaceLength += stop - unparsed + 1; // note += for 'add' case
	m_stringNext = m_stringSpace = new char[m_stringSpaceLength];
	if (add) {
	  assert(m_vt->m_isSequence);
	  // Do the realloc of the string space, and adjust
	  m_stringNext += oldLength;
	  memcpy(m_stringSpace, old, oldLength);
	  // Relocate string pointers
	  for (unsigned n = 0; n < m_nElements; n++)
	    m_pString[n] = m_stringSpace + (m_pString[n] - old);
	}
      }
      if (add)
	m_nElements++;
      else {
	// Count elements
	if (m_vt->m_isSequence) {
	  const char *start, *end, *tmp = unparsed;
	  // Figure out how many elements in the sequence
	  m_nElements = 0;
	  size_t len;
	  do {
	    if ((err = doElement(tmp, stop, start, end)))
	      return err;
	    len = end - start;
	    while (end < tmp && isspace(*end))
	      end++;
	    m_nElements++;
	  } while (end < stop);
	  if (m_nElements == 1 && len == 0)
	    //(len == 0 ||
	    //len == 2 && !strncmp("{}", start, 2)))
	    m_nElements = 0;
	}
      }
      m_nTotal = m_vt->m_nItems * (m_vt->m_isSequence ? m_nElements : 1);
      if ((err = allocate(add)))
	return err;
      if (m_vt->m_isSequence) {
	const char *start, *end;
	if (add) {
	  if ((err = parseElement(unparsed, stop, m_nElements-1)))
	    return err;
	} else
	  // Now we have allocated the appropriate sequence array, so we can parse elements
	  for (unsigned n = 0; n < m_nElements; n++) {
	    doElement(unparsed, stop, start, end);
	    if (needsCommaElement()) {
	      if (*start != '{' || end[-1] != '}')
		return "sequence elements not enclosed in braces";
	      start++;
	      end--;
	    }
	    if ((err = parseElement(start, end, n)))
	      return err;
	  }
      } else if ((err = parseElement(unparsed, stop, 0)))
	return err;
      m_parsed = true;
      return NULL;
    }
    const char *Value::
    parseDimension(const char *unparsed, const char *stop,
		   size_t nseq, size_t dim, size_t offset, size_t nItems) {
      size_t
	nextDim = dim + 1,
	dimension = m_vt->m_arrayDimensions[dim],
	skip = nItems/dimension;
      const char *err;
      for (unsigned n = 0; n < dimension; n++) {
	const char *start, *end;
	if ((err = doElement(unparsed, stop, start, end, true))) // nextDim != 1))) // = m_vt->m_arrayRank)))
	  return err;
	if (n && start == end)
	  break; // return "insufficient array elements";
	if (nextDim < m_vt->m_arrayRank) {
	  if (start == end || start[0] != '{' || end[-1] != '}')
	    return esprintf("array elements not enclosed in {} for (%*s) (%zu)",
			    (int)(end - start), start, end - start);
	  if ((err = parseDimension(start+1, end-1, nseq, nextDim, offset, skip)))
	    return err;
	  offset += skip;
	} else {
	  // strip braces if they were added
	  if (needsCommaDimension()) {
	    if (start[0] != '{' || end[-1] != '}')
	      return esprintf("struct or array element not enclosed in {} for (%*s) (%zu)",
			      (int)(end - start), start, end - start);
	    start++;
	    end--;
	  }
	  if ((err = parseValue(start, end, nseq, offset++)))
	    return err;
	}
      }
      if (unparsed < stop)
	return esprintf("excess data for array: \"%-*s\"", (int)(stop - unparsed), unparsed);
      return NULL;
    }
    // Parse a value that is a sequence element or a single standalone value.
    const char *Value::
    parseElement(const char *start, const char *end, size_t nSeq) {
      return m_vt->m_arrayRank ?
	parseDimension(start, end, nSeq, 0, 0, m_vt->m_nItems) :
	parseValue(start, end, nSeq, 0);
    }
    // A single value
    const char *Value::
    parseValue(const char *start, const char *end, size_t nSeq, size_t nArray) {
      const char *err;
      switch (m_vt->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			        \
	case OA::OCPI_##pretty:						        \
	  if ((err = parse##pretty(start, end,					\
				   m_vt->m_isSequence || m_vt->m_arrayRank ?      \
				   m_p##pretty[nSeq * m_vt->m_nItems + nArray] : \
				   m_##pretty)))			        \
	    return esprintf("in value \"%.*s\" (%zu): %s", (int)(end-start), start, end-start, err); \
	  break;
	OCPI_PROPERTY_DATA_TYPES
	OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      default:
	return "Unexpected illegal type in parsing value";
      }
      return NULL;
    }
#if 0
      doElement
      allocate(vt);
      unsigned n;
      for (n = 0; n < m_length && *unparsed; n++) {
	// Skip initial white space
	while (isspace(*unparsed))
	  unparsed++;
	if (!m_vector && !*unparsed)
	  return "Empty scalar value";
	// Find end
	const char *start = unparsed;
	while (*unparsed && *unparsed != ',') {
	  if (*unparsed == '\\' && unparsed[1])
	    unparsed++;
	  unparsed++;
	}
	char *unparsedSingle = new char[unparsed - start + 1];
	char *tmp = unparsedSingle;
	char *lastSpace = 0;
	for (tmp = unparsedSingle; *start && *start != ','; start++) {
	  if (*start == '\\' && start[1]) {
	    start++;
	    lastSpace = 0;
	  } else if (isspace(*start)) {
	    if (!lastSpace)
	      lastSpace = tmp;
	  } else
	    lastSpace = 0;
	  *tmp++ = *start;
	}
	if (lastSpace)
	  *lastSpace = 0;
	else
	  *tmp = 0;
	if (!m_vector && *unparsed)
	  err = "multiple values supplied for (single) scalar value";
	else switch (vt.m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			\
	    case OA::OCPI_##pretty:					\
	      if (parse##pretty(unparsedSingle, vt.m_stringLength, \
					    m_vector ? &m_p##pretty[n] : \
					    &m_##pretty))		\
		err = #pretty;						\
	      break;
	    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	  default:
	    err ="Unexpected illegal type in parsing value";
	  }
	if (err)
	  asprintf((char **)&err, "Bad value \"%s\" for value of type \"%s\"",
		   unparsedSingle, err);
	delete [] unparsedSingle;
	if (err)
	  break;
	if (*start)
	  start++;
      }
      if (!err && *unparsed)
	err = esprintf("Too many values (> %d) for value", vt.m_sequenceLength);
      if (err)
	clear();
      else
	m_length = n;
      return err;
#endif

bool Unparser::
dimensionUnparse(const Value &val, std::string &s, unsigned nseq, size_t dim, size_t offset,
		 size_t nItems, bool hex, char comma, const Unparser &up) const {
  size_t
    nextDim = dim + 1,
    dimension = val.m_vt->m_arrayDimensions[dim],
    skip = nItems/dimension;
  bool prevNull = false, allNull = true;
  size_t length = 0;
  std::string v;
  for (unsigned n = 0; n < val.m_vt->m_arrayDimensions[dim]; n++) {
    bool thisNull;
    if (nextDim < val.m_vt->m_arrayRank) { // if not outer dimension
      if (n != 0)
	v += comma;
      v += '{';
      thisNull = up.dimensionUnparse(val, v, nseq, nextDim, offset, skip, hex, comma, up);
      v += '}';
      offset += skip;
    } else { // outer dimension
      if (n != 0)
	v += comma;
#if 0
      if (needsCommaDimension()) {
	v += '{';
	thisNull = unparseValue(v, nseq, offset++, hex, comma, up);
	v += '}';
      } else
	thisNull = unparseValue(v, nseq, offset++, hex, comma, up);
#else
      thisNull = up.valueUnparse(val, v, nseq, offset++, hex, comma, val.needsCommaDimension(), up);
#endif
    }
    if (thisNull) {
      if (!prevNull)
	length = v.length();
      prevNull = true;
    } else {
      length = 0;
      allNull = prevNull = false;
    }
  }
  s.append(v.c_str(), length ? length : v.length());
  return allNull;
}

void Unparser::
elementUnparse(const Value &v, std::string &s, unsigned nSeq, bool hex, char comma,
	       bool wrap, const Unparser &up) const {
  if (wrap) s += '{';
  v.unparseElement(s, nSeq, hex, comma, up);
  if (wrap) s += '}';
}

void Value::
unparseElement(std::string &s, unsigned nSeq, bool hex, char comma, const Unparser &up) const {
  if (m_vt->m_arrayRank)
    up.dimensionUnparse(*this, s, nSeq, 0, 0, m_vt->m_nItems, hex, comma, up);
  else
    up.valueUnparse(*this, s, nSeq, 0, hex, comma, false, up);
}

// Format the result, possibly using a user-specified format string
void Unparser::
doFormat(std::string &s, const char *fmt, ...) const {
  va_list ap;
  va_start(ap, fmt);
  //  formatAddV(s, m_vt->m_format.empty() ? fmt : m_vt->m_format.c_str(), ap);
  formatAddV(s, fmt, ap);
  va_end(ap);
}

void Value::
unparse(std::string &s, const Unparser *up, bool append, bool hex, char comma) const {
  if (!up)
    up = this;
  if (!append)
    s.clear();
  if (m_vt->m_isSequence) {
    // Now we have allocated the appropriate sequence array, so we can parse elements
    for (unsigned n = 0; n < m_nElements; n++) {
      if (n)
	s += comma;
#if 0
      if (needsCommaElement()) {
	s += '{';
	unparseElement(s, n, hex, comma, *up);
	s += '}';
      } else
	unparseElement(s, n, hex, comma, *up);
#else
      up->elementUnparse(*this, s, n, hex, comma, needsCommaElement(), *up);
#endif
    }
  } else
    up->elementUnparse(*this, s, 0, hex, comma, false, *up);
}

bool Unparser::
valueUnparse(const Value &v, std::string &s, unsigned nSeq, size_t nArray, bool hex, char comma,
	     bool wrap, const Unparser &up) const {
  if (wrap) s += '{';
  bool r = v.unparseValue(s, nSeq, nArray, hex, comma, up);
  if (wrap) s += '}';
  return r;
}

bool Value::
unparseValue(std::string &s, unsigned nSeq, size_t nArray, bool hex, char comma,
	     const Unparser &up) const {
  switch (m_vt->m_baseType) {
#define OCPI_DATA_TYPE(sca,c,u,b,run,pretty,storage)     		    \
  case OA::OCPI_##pretty:		        			    \
    return up.unparse##pretty(s,					    \
			      m_vt->m_isSequence || m_vt->m_arrayRank ?	    \
			      m_p##pretty[nSeq * m_vt->m_nItems + nArray] : \
			      m_##pretty, hex);				    \
      break;
    OCPI_PROPERTY_DATA_TYPES
    OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
#undef OCPI_DATA_TYPE
  case OA::OCPI_Enum:
      return up.unparseEnum(s, 
			    m_vt->m_isSequence || m_vt->m_arrayRank ?
			    m_pEnum[nSeq * m_vt->m_nItems + nArray] :
			    m_Enum, m_vt->m_enums, hex);
  case OA::OCPI_Struct:
    return up.unparseStruct(s,
			    m_vt->m_isSequence || m_vt->m_arrayRank ?
			    m_pStruct[nSeq * m_vt->m_nItems + nArray] :
			    m_Struct, m_vt->m_members, m_vt->m_nMembers, hex, comma);
  case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
  }
  return false;
}
bool Unparser::
unparseBool(std::string &s, bool val, bool) const {
  s += val ? "true" : "false";
  return !val;
}
bool Unparser::
unparseChar(std::string &s, char argVal, bool hex) const {
  uint8_t val = (uint8_t)(argVal & 0xff);
  if (isprint(val)) {
    switch (val) {
    case '\\':
    case '"':
    case ' ':
    case ',':
    case '}':
    case '{':
      s += '\\';
      break;
    default:
      ;
    }
    s += val;
  } else {
    s += '\\';
    char c;
    switch (val) {
    case '\n': c = 'n'; break;
    case '\t': c = 't'; break;
    case '\v': c = 'v'; break;
    case '\b': c = 'b'; break;
    case '\r': c = 'r'; break;
    case '\f': c = 'f'; break;
    case '\a': c = 'a'; break;
    default:
      if (hex) {
	s+= 'x';
	s+= "0123456789abcdef"[(val >> 4) & 0xf];
	c = "0123456789abcdef"[val & 0xf];
      } else {
	s += (char)('0' + ((val >> 6) & 7));
	s += (char)('0' + ((val >> 3) & 7));
	c = (char)('0' + (val & 7));
      }
    }
    s += c;
  }
  return argVal == 0;
}
bool Unparser::
unparseDouble(std::string &s, double val, bool) const {
  char *cp;
  asprintf(&cp, "%g", val);
  for (char *p = cp; *p; p++)
    if (*p == 'e' || *p == 'E') {
      while (p > cp && p[-1] == '0') {
	strcpy(p - 1, p); // FIXME valgrind overlap
	p--;
      }
      break;
    }
  s += cp;
  free(cp);
  return *(uint64_t *)&val == 0;
}
bool Unparser::
unparseFloat(std::string &s, float val, bool hex) const {
  unparseDouble(s, (double)val, hex);
  return *(uint32_t*)&val == 0;
}
bool Unparser::
unparseShort(std::string &s, int16_t val, bool hex) const {
  doFormat(s, hex ? "0x%lx" : "%ld", (long)val);
  return val == 0;
}
bool Unparser::
unparseLong(std::string &s, int32_t val, bool hex) const {
  doFormat(s, hex ? "0x%lx" : "%ld", (long)val);
  return val == 0;
}
bool Unparser::
unparseUChar(std::string &s, uint8_t val, bool hex) const {
  doFormat(s, hex ? "0x%x" : "%u", val);
  return val == 0;
}
bool Unparser::
unparseULong(std::string &s, uint32_t val, bool hex) const {
  doFormat(s, hex ? "0x%lx" : "%lu", (unsigned long)val);
  return val == 0;
}
bool Unparser::
unparseUShort(std::string &s, uint16_t val, bool hex) const {
  doFormat(s, hex ? "0x%lx" : "%lu", (unsigned long)val);
  return val == 0;
}
bool Unparser::
unparseLongLong(std::string &s, int64_t val, bool hex) const {
  doFormat(s, hex ? "0x%llx" : "%lld", (long long)val);
  return val == 0;
}
bool Unparser::
unparseULongLong(std::string &s, uint64_t val, bool hex) const {
  doFormat(s, hex ? "0x%llx" : "%llu", (unsigned long long)val);
  return val == 0;
}
bool Unparser::
unparseString(std::string &s, const char *val, bool hex) const {
  if (!val || !*val) {
    s += "\"\"";
    return true;
  } else
    while (*val)
      unparseChar(s, *val++, hex);
  return false;
}
bool Value::needsComma() const {
  return m_vt->m_isSequence || m_vt->m_arrayRank != 0 || m_vt->m_baseType == OA::OCPI_Struct;
}
bool Value::needsCommaDimension() const {
  return /* m_vt->m_arrayRank == 1 || */ m_vt->m_baseType == OA::OCPI_Struct;
}
// This value is an element of a sequence.  Does it need to be wrapped in braces?
bool Value::needsCommaElement() const {
  return m_vt->m_arrayRank != 0 || m_vt->m_baseType == OA::OCPI_Struct;
}
bool Unparser::
unparseStruct(std::string &s, StructValue val, Member *members, size_t nMembers, bool hex,
	      char comma) const {
  bool seenOne = false;
  
  for (unsigned n = 0; val && n < nMembers; n++) {
    Value *v = *val++;
    if (v) {
      if (seenOne)
	s += comma;
      s += members[n].m_name;
      s += ' ';
      if (v->needsComma()) {
	s += '{';
	v->unparse(s, NULL, true, hex);
	s += '}';
      } else
	v->unparse(s, NULL, true, hex);
      seenOne = true;
    }
  }
  return !seenOne;
}
bool Unparser::
unparseType(std::string &s, TypeValue val, bool hex) const {
  if (val->needsComma()) {
    s += '{';
    val->unparse(s, NULL, true, hex);
    s += '}';
  } else
    val->unparse(s, NULL, true, hex);
  return false;
}
bool Unparser::
unparseEnum(std::string &s, EnumValue val, const char **enums, bool) const {
  s += enums[val];
  return val == 0;
}
void Value::
generateDimension(unsigned nseq, size_t dim, size_t offset, size_t nItems) {
  size_t
    nextDim = dim + 1,
    dimension = m_vt->m_arrayDimensions[dim],
    skip = nItems/dimension;
  for (unsigned n = 0; n < dimension; n++)
    if (nextDim < m_vt->m_arrayRank) {
      generateDimension(nseq, nextDim, offset, skip);
      offset += skip;
    } else
      generateValue(nseq, offset++);
}

// Parse a value that is a sequence element or a single standalone value.
void Value::
generateElement(unsigned nSeq) {
  if (m_vt->m_arrayRank)
    generateDimension(nSeq, 0, 0, m_vt->m_nItems);
  else
    generateValue(nSeq, 0);
}

void Value::generate() {
  clear();
  m_nTotal = m_vt->m_nItems;
  if (m_vt->m_isSequence) {
    m_nElements = random() % (m_vt->m_sequenceLength ? m_vt->m_sequenceLength : 5);
    m_nTotal *= m_nElements;
  }
  // string space?
  ocpiCheck(allocate() == 0);
  if (m_vt->m_isSequence)
    // Now we have allocated the appropriate sequence array, so we can parse elements
    for (unsigned n = 0; n < m_nElements; n++)
      generateElement(n);
  else
    generateElement(0);
}

// Generate a random value for this type
void Value::
generateValue(unsigned nSeq, size_t nArray) {
  switch (m_vt->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)  \
    case OA::OCPI_##pretty:			    \
      (m_vt->m_isSequence || m_vt->m_arrayRank ?      \
       m_p##pretty[nSeq * m_vt->m_nItems + nArray] : \
       m_##pretty) = generate##pretty();            \
      break;
    OCPI_PROPERTY_DATA_TYPES
    OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
    OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
    OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
  case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
  }
}
bool Value::
generateBool() {
  return (random() & 1) != 0;
}
char Value::
generateChar() {
  return (char)random();
}
double Value::
generateDouble() {
  return (double)myRandom();
}
float Value::
generateFloat() {
  return (float)myRandom();
}
int16_t Value::
generateShort() {
  return (int16_t)myRandom();
}
int32_t Value::
generateLong() {
  return (int32_t)myRandom();
}
uint8_t Value::
generateUChar() {
  return (uint8_t)random();
}
uint32_t Value::
generateULong() {
  return (uint32_t)myRandom();
}
uint16_t Value::
generateUShort() {
  return (uint16_t)myRandom();
}
int64_t Value::
generateLongLong() {
  return myRandom();
}
uint64_t Value::
generateULongLong() {
  return myRandom();
}
OA::CharP Value::
generateString() {
  char *cp = m_stringNext;
  for (size_t len = random() % (m_vt->m_stringLength ? m_vt->m_stringLength + 1 : testMaxStringLength);
       len; len--)
    *m_stringNext++ = (char)(random() % (0177 - 036) + 036);
  *m_stringNext++ = 0;
  return cp;
}
StructValue Value::
generateStruct() {
  StructValue sv = m_structNext;
  m_structNext += m_vt->m_nMembers;
  for (unsigned n = 0; n < m_vt->m_nMembers; n++)
    if (random() % 4) {
      sv[n] = new Value(m_vt->m_members[n], this);
      sv[n]->generate();
    }
  return sv;
}
TypeValue Value::
generateType() {
  TypeValue tv = m_typeNext++;
  tv->generate();
  return tv;
}
EnumValue Value::
generateEnum() {
  return (EnumValue)(random() % m_vt->m_nEnums);
}
const char *Value::getValue(ExprValue &val) {
  val.isNumber = true;
  switch (m_vt->m_baseType) {
  case OA::OCPI_Bool: val.number = m_Bool; break;
  case OA::OCPI_Char: val.isNumber = false; val.string.assign(1, m_Char); break;
  case OA::OCPI_Double: val.number = (int64_t)m_Double; break;
  case OA::OCPI_Float: val.number = (int64_t)m_Float; break;
  case OA::OCPI_Short: val.number = m_Short; break;
  case OA::OCPI_Long: val.number = m_Long; break;
  case OA::OCPI_UChar: val.number = m_UChar; break;
  case OA::OCPI_Enum:
  case OA::OCPI_ULong: val.number = m_ULong; break;
  case OA::OCPI_UShort: val.number = m_UShort; break;
  case OA::OCPI_LongLong: val.number = m_LongLong; break;
  case OA::OCPI_ULongLong: val.number = m_ULongLong; break;
  case OA::OCPI_String: val.isNumber = false; val.string = m_String; break;
  default:
    return "bad data type for property";
  }
  return NULL;
}
}
}

