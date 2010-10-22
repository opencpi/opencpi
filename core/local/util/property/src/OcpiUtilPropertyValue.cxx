
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

 const char *
Scalar::Value::parse(const char *value, Scalar::Type type, unsigned length) {
  switch (type) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
    case Scalar::OCPI_##pretty:		       \
      if (parse##pretty(value, length, &v##pretty)) \
	return "Error parsing value";\
      break;
OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
  default:
    return "Unexpected illegal type in parsing value";
  }
  return 0;
}

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

    }
  }
}

