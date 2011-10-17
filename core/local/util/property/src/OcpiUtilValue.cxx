
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

#define __STDC_LIMIT_MACROS // wierd standards goof up
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilDataTypes.h"

namespace OCPI {

  namespace Util {
    namespace OA = OCPI::API;
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
	if (*endptr++ == '-') {
	  while (isspace(*endptr))
	    endptr++;
	  if (*endptr++ == '1') {
	    while (isspace(*endptr))
	      endptr++;
	    if (!*endptr)
	      val--;
	  }
	}
	*valp = val;
	return false;
      }
      return true;
    }
    // return true on error
    static bool
    getNum64(const char *s, int64_t *valp) {
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

#if 0

#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)		\
    bool Value::parse##pretty(const char *, const char *, unsigned length, run *);
    OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE

#endif

    bool Value::
    parseBool(const char *a, const char *end, bool &b)
    {
      (void)end;
      return OCPI::Util::EzXml::parseBool(a, &b);
    }
    bool Value::
    parseChar(const char*cp, const char *end, char &vp) {
      (void)end;
      while (isspace(*cp))
	cp++;
      if (*cp == '\'') {
	if (*++cp == '\\')
	  cp++;
	vp = *cp;
      } else {
	int64_t n;
	if (getNum64(cp, &n) || n > INT8_MAX || n < INT8_MIN)
	  return true;
	vp = n;
      }
      return false;
    }
    bool Value::
    parseDouble(const char*cp, const char *end, double &vp) {
      (void)end;
      char *endptr;
      errno = 0;
      vp = strtod(cp, &endptr);
      if (endptr == cp || errno)
	return true;
      return false;
    }
    bool Value::
    parseFloat(const char*cp, const char *end, float &vp) {
      (void)end;
      char *endptr;
      vp = strtof(cp, &endptr);
      if (endptr == cp || errno)
	return true;
      return false;
    }
    bool Value::
    parseShort(const char*cp, const char *end, int16_t &vp) {
      (void)end;
      int64_t n;
      if (getNum64(cp, &n) || n > INT16_MAX || n < INT16_MIN)
	return true;
      vp = n;
      return false;
    }
    bool Value::
    parseLong(const char*cp, const char *end, int32_t &vp) {
      (void)end;
      int64_t n;
      if (getNum64(cp, &n) || n > INT32_MAX || n < INT32_MIN)
	return true;
      vp = n;
      return false;
    }
    bool Value::
    parseUChar(const char*cp, const char *end, uint8_t &vp) {
      (void)end;
      while (isspace(*cp))
	cp++;
      if (*cp == '\'') {
	if (*++cp == '\\')
	  cp++;
	vp = *cp;
      } else {
	uint64_t n;
	if (getUNum64(cp, &n) || n > UINT8_MAX)
	  return true;
	vp = n;
      }
      return false;
    }
    bool Value::
    parseULong(const char*cp, const char *end, uint32_t &vp) {
      (void)end;
      uint64_t n;
      if (getUNum64(cp, &n) || n > UINT32_MAX)
	return true;
      vp = n;
      return false;
    }
    bool Value::
    parseUShort(const char*cp, const char *end, uint16_t &vp) {
      (void)end;
      uint64_t n;
      if (getUNum64(cp, &n) || n > UINT16_MAX)
	return true;
      vp = n;
      return false;
    }
    bool Value::
    parseLongLong(const char*cp, const char *end, int64_t &vp) {
      (void)end;
      int64_t n;
      if (getNum64(cp, &n) || n > INT64_MAX || n < INT64_MIN)
	return true;
      vp = n;
      return false;
    }
    bool Value::
    parseULongLong(const char*cp, const char *end, uint64_t &vp) {
      (void)end;
      uint64_t n;
      if (getUNum64(cp, &n))
	return true;
      vp = n;
      return false;
    }
    bool Value::
    parseString(const char*cp, const char *end, OA::CharP &vp) {
      // Check length if string is bounded
      if (m_vt.m_stringLength && (end - cp) > m_vt.m_stringLength)
	return true;
      vp = m_stringNext;
      while (cp < end)
	*m_stringNext++ = *cp++;
      *m_stringNext++ = 0;
      return false;
    }
    Value::Value(const ValueType &vt) : m_vt(vt), m_nElements(0), m_stringSpace(NULL) { clear(); }

    // We expect to parse into the value more that once.
    void Value::
    clear() {
      if (m_nElements)
	switch(m_vt.m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
	  case OA::OCPI_##pretty:		   \
	    delete [] m_p##pretty;		   \
	    break;
	  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	case OA::OCPI_Struct: case OA::OCPI_Type: case OA::OCPI_Enum:
	case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
	}
      m_nElements = 0;
      if (m_stringSpace) {
	delete [] m_stringSpace;
	m_stringSpace = NULL;
      }
    }
    Value::~Value() {
      clear();
    }

#if 0
    // No unbounded allowed
    void Value::allocate(const ValueType &vt) {
      clear();
      if (vt.m_isSequence || vt.m_arrayRank) {
	m_vector = true;
	m_length = 1;
	if (vt.m_arrayRank)
	  for (unsigned n = 0; n < vt.m_arrayRank; n++)
	    m_length *= vt.m_arrayDimensions[n];
	if (vt.m_isSequence && vt.m_sequenceLength)
	  m_length *= vt.m_sequenceLength;
	switch (vt.m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)	\
	  case OA::OCPI_##pretty:			\
	    m_p##pretty = new run[m_length];		\
	    break;
	  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	case OA::OCPI_none: case OA::OCPI_Struct: case OA::OCPI_scalar_type_limit:;
	}
      } else
	m_length = 1;
      m_baseType = vt.m_baseType;
      if (vt.m_baseType == OA::OCPI_String) {
	if (m_vector)
	  m_stringSpace = new char[(vt.m_stringLength + 1) * m_length];
	else
	  m_String = m_stringSpace = new char[vt.m_stringLength + 1];
      }
    }
#endif
    // Find an element, which might be empty, trimming whitespace
    // Return true if there is more
    static const char *
    doElement(const char *&unparsed, const char *stop, const char *&start, const char *&end) {
      // Skip initial white space
      while (unparsed != stop && isspace(*unparsed))
	unparsed++;
      // Find end
      start = unparsed;
      if (*unparsed == '{') {
	unsigned n = 1;
	do 
	  switch(*++unparsed) {
	  case '\\':
	    unparsed++; break;
	  case '{':
	    n++; break;
	  case '}':
	    n--; break;
	  case 0:
	    return "unbalanced curly braces in value";
	  default:;
	  }
	while (n);
	end = ++unparsed;
	while (unparsed != stop && *unparsed && *unparsed != ',')
	  if (!isspace(*unparsed++))
	    return "illegal characters between '}' and ','";
      } else {
	while (unparsed != stop && *unparsed && *unparsed != ',') {
	  if (*unparsed == '\\' && unparsed[1])
	    unparsed++;
	  unparsed++;
	}
	end = unparsed++;
	while (end > start && isspace(end[-1]))
	  end--;
      }
      return NULL;
    }

    const char *
    Value::parse(const char *unparsed, const char *stop) {
      clear();
      const char *err = 0;
      if (!stop)
	stop = unparsed + strlen(unparsed);
      if (m_vt.m_baseType == OA::OCPI_String)
	// the space required will never be larger than the input...
	m_stringNext = m_stringSpace = new char[stop - unparsed + 1];
      if (m_vt.m_isSequence) {
	const char *start, *end, *tmp = unparsed;
	if (!stop)
	  stop = unparsed + strlen(unparsed);

	for (m_nElements = 0; tmp < stop; m_nElements++)
	  if ((err = doElement(tmp, stop, start, end)))
	    return err;
	if (m_nElements == 1 && start == end)
	  m_nElements = 0;
	if (m_vt.m_sequenceLength && m_nElements > m_vt.m_sequenceLength)
	  return "Too many elements in bounded sequence";
	// Allocate sequence array
	if (m_nElements) {
	  switch (m_vt.m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)    \
	    case OA::OCPI_##pretty:		      \
	      if (m_vt.m_arrayRank)		      \
		m_pp##pretty = new run*[m_nElements]; \
	      else				      \
		m_p##pretty = new run[m_nElements];   \
	      break;
	    OCPI_PROPERTY_DATA_TYPES
	    OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
            OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
            OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
	  case OA::OCPI_none: case OA::OCPI_scalar_type_limit:;
	  }
	}
	// Now we have allocated the appropriate sequence array, so we can parse elements
	for (unsigned n = 0; n < m_nElements; n++) {
	  doElement(unparsed, stop, start, end);
	  if (*start == '{') {
	    assert(end[-1] == '}');
	    start++;
	    end--;
	  }
	  if ((err = parseElement(start, end, n)))
	    return err;
	}
	return NULL;
      }
      return parseElement(unparsed, unparsed + strlen(unparsed), 0);
    }
    const char *Value::
    parseDimension(const char *unparsed, const char *stop,
		   unsigned nseq, unsigned dim, unsigned offset) {
      unsigned nextDim = dim + 1;
      const char *err;
      for (unsigned n = 0; unparsed != stop && n < m_vt.m_arrayDimensions[dim]; n++) {
	const char *start, *end;
	doElement(unparsed, stop, start, end);
	if (nextDim < m_vt.m_arrayRank) {
	  if ((err = parseDimension(start, end, nseq, dim + 1, offset)))
	    return err;
	  offset += m_vt.m_arrayDimensions[nextDim];
	} else if ((err = parseValue(start, end, nseq, offset++)))
	  return err;
      }
      return NULL;
    }
    // Parse a value that is a sequence element or a single standalone value.
    const char *Value::
    parseElement(const char *start, const char *end, unsigned nSeq) {
      return m_vt.m_arrayRank ?
	parseDimension(start, end, nSeq, 0, 0) :
	parseValue(start, end, nSeq, 0);
    }
    // A single value
    const char *Value::
    parseValue(const char *unparsed, const char *stop, unsigned nSeq, unsigned nArray) {
      const char *start, *end;
      if (doElement(unparsed, stop, start, end))
	return "Multiple values when a single value is expected (unescaped comma?)";
      switch (m_vt.m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			             \
	case OA::OCPI_##pretty:						             \
	  if (parse##pretty                 					     \
	      (start, end,                       			             \
	       (m_vt.m_isSequence ?					             \
		(m_vt.m_arrayRank ? m_pp##pretty[nSeq][nArray] : m_p##pretty[nSeq]) :\
		(m_vt.m_arrayRank ? m_p##pretty[nArray] : m_##pretty))))	     \
	    return "Bad "#pretty" value";				             \
	  break;
	OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
      default:
	return "Unexpected illegal type in parsing value";
      }
      return NULL;
    }
    // Parse the elements of the structure.  This is like the data-type-specific methods.
    bool Value::
    parseStruct(const char *unparsed, const char *stop, StructValue &sv) {
      while (unparsed < stop) {
	const char *start, *end;
	doElement(unparsed, stop, start, end);
	if (start == end)
	  return "empty member value in struct value";
	unsigned n;
	for (n = 0; n < m_vt.m_nMembers; n++) {
	  const char *mName = m_vt.m_members[n].m_name.c_str();
	  unsigned len = strlen(mName);
	  if (!strncmp(mName, start, len) && isspace(start[len]))
	    break;
	}
	if (n >= m_vt.m_nMembers)
	  return true; // "unknown member name in struct value";
	if (sv[n])
	  return true; // "duplicate member name in struct value";
	sv[n] = new Value(m_vt.m_members[n]);
	const char *err;
	if ((err = sv[n]->parse(start, end)))
	  return true;
      }
      return false;
    }
#if 0
      doElement
	}

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
    }
#endif
    void Value::unparse(std::string &s) const {
      char *cp = 0;
      switch (m_vt.m_baseType) {
      case OA::OCPI_Float:
	asprintf(&cp, "%g", (double)m_Float);
	break;
      case OA::OCPI_Double:
	asprintf(&cp, "%g", m_Double);
	break;
#if 0
      case OA::OCPI_LongDouble:
	asprintf(&cp, "%Lg", m_LongDouble);
	break;
#endif
      case OA::OCPI_Short:
	asprintf(&cp, "%ld", (long)m_Short);
	break;
      case OA::OCPI_Long:
	asprintf(&cp, "%ld", (long)m_Long);
	break;
      case OA::OCPI_LongLong:
	asprintf(&cp, "%lld", (long long)m_LongLong);
	break;
      case OA::OCPI_UShort:
	asprintf(&cp, "%lu", (unsigned long)m_UShort);
	break;
      case OA::OCPI_ULong:
	asprintf(&cp, "%lu", (unsigned long)m_ULong);
	break;
      case OA::OCPI_ULongLong:
	asprintf(&cp, "%llu", (unsigned long long)m_ULongLong);
	break;
      case OA::OCPI_Char:
	cp = new char[10];
	if (isprint(m_Char)) {
	  switch (m_Char) {
	  case '\\':
	  case '"':
	  case '?':
	  case '\'':
	    *cp++ = '\\';
	    break;
	  default:
	    ;
	  }
	  *cp++ = m_Char;
	} else {
	  *cp++ = '\\';
	  switch (m_Char) {
	  case '\a':
	    *cp++ = 'a';
	    break;
	  case 'b':
	    *cp++ = 'b';
	    break;
	  case 'f':
	    *cp++ = 'f';
	    break;
	  case 'n':
	    *cp++ = 'n';
	    break;
	  case 'r':
	    *cp++ = 'r';
	    break;
	  case 't':
	    *cp++ = 't';
	    break;
	  case 'v':
	    *cp++ = 'v';
	    break;
	  default:
	    *cp++ = '0' + ((m_Char >> 6) & 7);
	    *cp++ = '0' + ((m_Char >> 3) & 7);
	    *cp++ = '0' + (m_Char & 7);
	  }
	case OA::OCPI_Struct: 
	case OA::OCPI_Type: 
	case OA::OCPI_Enum: 
	case OA::OCPI_none:
	case OA::OCPI_scalar_type_limit:;
	}
	*cp = 0;
	break;
#if 0
      case OA::OCPI_WChar:
	cp = (char *)malloc(MB_LEN_MAX + 1);
	wctomb(cp, m_wchar); // FIXME: could worry about escapes in this string?
	break;
#endif
      case OA::OCPI_Bool:
	asprintf(&cp, m_Bool ? "true" : "false");
	break;
      case OA::OCPI_UChar:
	asprintf(&cp, "0x%x", m_UChar);
	break;
#if 0
      case OA::OCPI_Objref:
#endif
      case OA::OCPI_String:
	s = m_String;
	break;
#if 0
      case Type::WSTRING:
      case Type::FIXED:
      case Type::ENUM:
      case Type::UNION:
	// These are not used for properties.
	break;
      case Type::STRUCT:
	assert(0); // unimplemented yet.
      case Type::ANY:
      case Type::VOID:
      case Type::NONE:
      default:
	// These are not used for properties.
	;
#endif
      }
      if (cp) {
	s = cp;
	delete []cp;
      }
    }
  }
}

