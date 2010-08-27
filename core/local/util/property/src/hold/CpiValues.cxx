#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "CpiUtilPropertyType.h"

namespace CPI {

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

#define CPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
  bool parse##pretty(const char *, unsigned length, run *);
CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE

 const char *
Value::parse(const char *value, CM::Property::Type type, unsigned length) {
  switch (type) {
#define CPI_DATA_TYPE(s,c,u,b,run,pretty,storage) \
    case CM::Property::CPI_##pretty:		       \
      if (parse##pretty(value, length, &v##pretty)) \
	return "Error parsing value";\
      break;
CPI_PROPERTY_DATA_TYPES
#undef CPI_DATA_TYPE
  default:
    return "Unexpected illegal type in parsing value";
  }
  return 0;
}

  bool
parseBool(const char *a, unsigned length, bool *b)
{
  if (!strcasecmp(a, "true") || !strcmp(a, "1"))
    *b = true;
  else if (!strcasecmp(a, "false")  || !strcmp(a, "0"))
    *b =  false;
  else
    return true;
  return false;
}
 bool
parseChar(const char*cp, unsigned length, char *vp) {
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
  char *endptr;
  errno = 0;
  *vp = strtod(cp, &endptr);
  if (endptr == cp || errno)
    return true;
  return false;
}
 bool
parseFloat(const char*cp, unsigned length, float*vp) {
  return false;
}
 bool
parseShort(const char*cp, unsigned length, int16_t*vp) {
  int64_t n;
  if (getNum64(cp, &n) || n > INT16_MAX || n < INT16_MIN)
    return true;
  *vp = (int16_t)n;
  return false;
}
 bool
parseLong(const char*cp, unsigned length, int32_t*vp) {
  int64_t n;
  if (getNum64(cp, &n) || n > INT32_MAX || n < INT32_MIN)
    return true;
  *vp = (int32_t)n;
  return false;
}
 bool
parseUChar(const char*cp, unsigned length, uint8_t*vp) {
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
  uint64_t n;
  if (getUNum64(cp, &n) || n > UINT32_MAX)
    return true;
  *vp = (uint32_t)n;
  return false;
}
 bool
parseUShort(const char*cp, unsigned length, uint16_t*vp) {
  uint64_t n;
  if (getUNum64(cp, &n) || n > UINT16_MAX)
    return true;
  *vp = (uint16_t)n;
  return false;
}
 bool
parseLongLong(const char*cp, unsigned length, int64_t*vp) {
  int64_t n;
  if (getNum64(cp, &n) || n > INT64_MAX || n < INT64_MIN)
    return true;
  *vp = n;
  return false;
}
 bool
parseULongLong(const char*cp, unsigned length, uint64_t*vp) {
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

