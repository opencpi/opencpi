
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
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "OcpiUtilEzxml.h"
#include "OcpiUtilPropertyType.h"

namespace OCPI {

  namespace Util {

    namespace Prop {

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

#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
  bool parse##pretty(const char *, unsigned length, run *);
OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE

  bool
parseBool(const char *a, unsigned length, bool *b)
{
  return OCPI::Util::EzXml::parseBool(a, length, b);
}
 bool
parseChar(const char*cp, unsigned length, char *vp) {
  (void)length;
  while (isspace(*cp))
    cp++;
  if (*cp == '\'') {
    if (*++cp == '\\')
      cp++;
    *vp = *cp;
  } else {
    int64_t n;
    if (getNum64(cp, &n) || n > INT8_MAX || n < INT8_MIN)
      return true;
    *vp = (char)n;
  }
  return false;
}
 bool
parseDouble(const char*cp, unsigned length, double*vp) {
  (void)length;
  char *endptr;
  errno = 0;
  *vp = strtod(cp, &endptr);
  if (endptr == cp || errno)
    return true;
  return false;
}
 bool
parseFloat(const char*cp, unsigned length, float*vp) {
  (void)length;
  ( void ) cp;
  ( void ) vp;
  return false;
}
 bool
parseShort(const char*cp, unsigned length, int16_t*vp) {
  (void)length;
  int64_t n;
  if (getNum64(cp, &n) || n > INT16_MAX || n < INT16_MIN)
    return true;
  *vp = (int16_t)n;
  return false;
}
 bool
parseLong(const char*cp, unsigned length, int32_t*vp) {
  (void)length;
  int64_t n;
  if (getNum64(cp, &n) || n > INT32_MAX || n < INT32_MIN)
    return true;
  *vp = (int32_t)n;
  return false;
}
 bool
parseUChar(const char*cp, unsigned length, uint8_t*vp) {
  (void)length;
  while (isspace(*cp))
    cp++;
  if (*cp == '\'') {
    if (*++cp == '\\')
      cp++;
    *vp = *cp;
  } else {
    uint64_t n;
    if (getUNum64(cp, &n) || n > UINT8_MAX)
      return true;
    *vp = (uint8_t)n;
  }
  return false;
}
 bool
parseULong(const char*cp, unsigned length, uint32_t*vp) {
  (void)length;
  uint64_t n;
  if (getUNum64(cp, &n) || n > UINT32_MAX)
    return true;
  *vp = (uint32_t)n;
  return false;
}
 bool
parseUShort(const char*cp, unsigned length, uint16_t*vp) {
  (void)length;
  uint64_t n;
  if (getUNum64(cp, &n) || n > UINT16_MAX)
    return true;
  *vp = (uint16_t)n;
  return false;
}
 bool
parseLongLong(const char*cp, unsigned length, int64_t*vp) {
  (void)length;
  int64_t n;
  if (getNum64(cp, &n) || n > INT64_MAX || n < INT64_MIN)
    return true;
  *vp = n;
  return false;
}
 bool
parseULongLong(const char*cp, unsigned length, uint64_t*vp) {
  (void)length;
  uint64_t n;
  if (getUNum64(cp, &n))
    return true;
  *vp = n;
  return false;
}
 bool
parseString(const char*cp, unsigned length, char**vp) {
  if (strlen(cp) > length)
    return true;
  *vp = strdup(cp);
  return false;
}

    } // close Prop
  } // close Util

  namespace API {

    Value::Value() : m_vector(false), m_stringSpace(NULL) { clear(); }
    Value::Value(const ValueType &vt) : m_vector(false), m_stringSpace(NULL) {
      allocate(vt);
    }
    // Parse a value, which may be a sequence/array/string, but not a struct (yet)
    // We expect to parse into the value more that once.
    void Value::
    clear() {
      if (m_vector) {
	switch(m_type) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
	  case OCPI_##pretty:			   \
	    delete [] m_p##pretty;		   \
	    break;
	  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	case OCPI_none: case OCPI_scalar_type_limit:;
	}
	m_vector = false;
      }
      m_length = 0;
      if (m_stringSpace) {
	delete [] m_stringSpace;
	m_stringSpace = NULL;
      }
    }
    Value::~Value() {
      clear();
    }

    void Value::allocate(const ValueType &vt) {
      clear();
      if (vt.isSequence || vt.isArray) {
	switch (vt.scalar) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)	\
	case OCPI_##pretty:				\
	    m_p##pretty = new run[vt.length];		\
	    break;
	  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
	case OCPI_none: case OCPI_scalar_type_limit:;
	}
      } else
	m_length = 1;
      m_type = vt.scalar;
      if (vt.scalar == OCPI_String) {
	if (vt.isSequence || vt.isArray)
	  m_stringSpace = new char[(vt.stringLength + 1) * vt.length];
	else
	  m_String = m_stringSpace = new char[vt.stringLength + 1];
      }
    }
    const char *
    Value::parse(const ValueType &vt, const char *unparsed) {
      const char *err = 0;
      allocate(vt);
      unsigned n;
      for (n = 0; n < vt.length && *unparsed; n++) {
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
	else switch (vt.scalar) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			\
	    case OCPI_##pretty:					\
	      if (OCPI::Util::Prop::parse##pretty(unparsedSingle, vt.stringLength, \
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
	err = esprintf("Too many values (> %d) for value", vt.length);
      if (err)
	clear();
      else
	m_length = n;
      return err;
    }
    void Value::unparse(std::string &s) const {
      char *cp = 0;
      switch (m_type) {
      case OCPI_Float:
	asprintf(&cp, "%g", (double)m_Float);
	break;
      case OCPI_Double:
	asprintf(&cp, "%g", m_Double);
	break;
#if 0
      case OCPI_LongDouble:
	asprintf(&cp, "%Lg", m_LongDouble);
	break;
#endif
      case OCPI_Short:
	asprintf(&cp, "%ld", (long)m_Short);
	break;
      case OCPI_Long:
	asprintf(&cp, "%ld", (long)m_Long);
	break;
      case OCPI_LongLong:
	asprintf(&cp, "%lld", (long long)m_LongLong);
	break;
      case OCPI_UShort:
	asprintf(&cp, "%lu", (unsigned long)m_UShort);
	break;
      case OCPI_ULong:
	asprintf(&cp, "%lu", (unsigned long)m_ULong);
	break;
      case OCPI_ULongLong:
	asprintf(&cp, "%llu", (unsigned long long)m_ULongLong);
	break;
      case OCPI_Char:
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
	case OCPI_none: case OCPI_scalar_type_limit:;
	}
	*cp = 0;
	break;
#if 0
      case OCPI_WChar:
	cp = (char *)malloc(MB_LEN_MAX + 1);
	wctomb(cp, m_wchar); // FIXME: could worry about escapes in this string?
	break;
#endif
      case OCPI_Bool:
	asprintf(&cp, m_Bool ? "true" : "false");
	break;
      case OCPI_UChar:
	asprintf(&cp, "0x%x", m_UChar);
	break;
#if 0
      case OCPI_Objref:
#endif
      case OCPI_String:
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

