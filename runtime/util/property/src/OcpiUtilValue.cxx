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

#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include "OcpiOsAssert.h"
#include "OcpiExprEvaluator.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilAutoMutex.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilDataTypes.h"

// Prefix for interpreting string and char types as expressions
#define EXPR_PREFIX "\\:"
#define EXPR_PREFIX_LEN (sizeof(EXPR_PREFIX)-1)
// Prefix for interpreting structure values as positional rather than sparse/named
#define POSITIONAL_PREFIX ":"
#define POSITIONAL_PREFIX_LEN (sizeof(POSITIONAL_PREFIX)-1)

namespace OA = OCPI::API;
namespace OE = OCPI::Util::EzXml;
namespace OCPI {
  namespace Util {

    namespace {
      pthread_once_t s_resolverOnce = PTHREAD_ONCE_INIT;
      pthread_key_t s_resolverKey;
      void makeResolverKey() {
	pthread_key_create(&s_resolverKey, NULL);
      }
      // thread-private data to communicate arbitrarily deep in a value parsing stack
      struct Resolver {
	const IdentResolver *resolver;
	bool *isVariable;
	Resolver(const IdentResolver *r, bool *iv) : resolver(r), isVariable(iv) {
	  if (isVariable)
	    *isVariable = false;
	  pthread_once(&s_resolverOnce, makeResolverKey);
	  pthread_setspecific(s_resolverKey, (void*)this);
	}
      };
      Resolver *getResolver() {
	return (Resolver *)pthread_getspecific(s_resolverKey);
      }
      inline long myRandom() { return random() * (random() & 1 ? -1 : 1); }

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
	  case '"':
	    // We have a double-quoted string to skip.
	    // !!! There is a very similar loop in parseString
	    while (unparsed < stop && *unparsed && *unparsed != '"') {
	      char dummy;
	      if (parseOneChar(unparsed, stop, dummy))
		return "bad double quoted string value";
	    }
	    if (unparsed >= stop || !*unparsed)
	      return "unterminated double quoted string";
	    unparsed++;
	    break;
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

    // This is inefficient, but reliable.  FIXME?
    Value &Value::
    operator=(const Value &v) {
      clear();
      init();
      if ((m_vt = v.m_vt)) {
	assert(!v.m_parent);
	std::string s;
	v.unparse(s);
	parse(s.c_str());
      }
      return *this;
    }

    Value::
    Value(const Value &v)
      : m_vt(s_vt), m_parent(s_parent) {
      init();
      *this = v;
    }
    void Value::setType(const ValueType &vt) {
      clear();
      init();
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
      m_pULongLong = NULL; // in case pointers are longer than 64 bits...
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
      if (endptr == cp || errno || (end && endptr > end))
	return "bad Double value";
      return NULL;
    }
    const char *Value::
    parseFloat(const char*cp, const char *end, float &vp) {
      char *endptr;
      errno = 0;
      vp = strtof(cp, &endptr);
      if (endptr == cp || errno || (end && endptr > end))
	return "bad Float value";
      return NULL;
    }
    const char *Value::
    parseShort(const char*cp, const char *end, int16_t &vp) {
      int64_t n;
      if (OE::getNum64(cp, end, n, 16) || n > INT16_MAX || n < INT16_MIN)
	return "bad Short value";
      vp = (int16_t)n;
      return NULL;
    }
    const char *Value::
    parseLong(const char*cp, const char *end, int32_t &vp) {
      int64_t n;
      if (OE::getNum64(cp, end, n, 32) || n > INT32_MAX || n < INT32_MIN)
	return "bad Long value";
      vp = (int32_t)n;
      return NULL;
    }
    const char *Value::
    parseUChar(const char *cp, const char *end, uint8_t &vp) {
      while (isspace(*cp))
	cp++;
      if (*cp == '\'') {
	cp++;
	while (end && end > cp+1 && isspace(end[-1]))
	  end--;
	if (end < cp+1 || end[-1] != '\'')
	  return "bad unsigned char value in single quotes";
	char c;
	const char *err = parseChar(cp, end, c);
	if (err)
	  return err;
	vp = (uint8_t)c;
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
      if (OE::getNum64(cp, end, n, 64) || n > INT64_MAX || n < INT64_MIN)
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
      // !!! There is a very similar loop in doElement
      for (unsigned len = 0; cp < end; len++)
	if (quoted && *cp == '"') {
	  if (cp + 1 != end)
	    return "double quoted string with invalid characters after the closing quote";
	  break;
	} else if (m_vt->m_stringLength && len >= m_vt->m_stringLength)
	  return "string too long";
	else if (parseOneChar(cp, end, nextStringChar()))
	  return "bad String value";
      setNextStringChar(0);
      vp = (const char *)start;
      return NULL;
    }
    // Parse the elements of the structure.  This is like the data-type-specific methods.
    const char *Value::
    parseStruct(const char *unparsed, const char *stop, StructValue &sv) {
      sv = m_structNext;
      m_structNext += m_vt->m_nMembers;
      size_t member = 0;
      bool positional =
	(size_t)(stop - unparsed) > POSITIONAL_PREFIX_LEN &&
	!strncmp(unparsed, POSITIONAL_PREFIX, POSITIONAL_PREFIX_LEN);
      if (positional)
	unparsed += POSITIONAL_PREFIX_LEN;
      while (unparsed < stop) {
	const char *start, *end;
	doElement(unparsed, stop, start, end);
	if (start == end)
	  return "empty member value in struct value";
	size_t n, len;
	if (positional) {
	  n = member++;
	  if (n >= m_vt->m_nMembers)
	    return "too many members in struct value";
	} else {
	  for (n = 0; n < m_vt->m_nMembers; n++) {
	    const char *mName = m_vt->m_members[n].m_name.c_str();
	    len = strlen(mName);
	    if (!strncmp(mName, start, len) && isspace(start[len]))
	      break;
	  }
	  if (n >= m_vt->m_nMembers) {
	    std::string err("struct member name \"");
	    const char *endOfName = start;
	    while (!isspace(*endOfName) && endOfName != end)
	      endOfName++;
	    err.append(start,  endOfName - start);
	    err += "\" did not match any of the expected member names (";
	    for (size_t ii = 0; ii < m_vt->m_nMembers; ii++)
	      formatAdd(err, "%s\"%s\"", ii ? ", " : "", m_vt->m_members[ii].cname());
	    err += ")";
	    return esprintf("%s%s", err.c_str(), *start == '{' ?
			    ", note that opening curly braces are only used for structs when "
			    "they occur within an array or sequence or struct" : "");
	  }
	  if (sv[n])
	    return "duplicate member name in struct value";
	  start += len;
	}
	Value *v = sv[n] = new Value(m_vt->m_members[n], this);
	const char *err;
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
      if (m_vt && (m_vt->m_isSequence || m_vt->m_arrayRank) &&
	  (m_nTotal || m_vt->m_baseType == OA::OCPI_String)) {
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
	// For string sequences we always need a "null ptr after the last one"
	if (m_nElements == 0 && m_vt->m_baseType != OA::OCPI_String)
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
	  if (m_vt->m_baseType == OA::OCPI_String) {   \
            /* NULL terminate sequences of strings */  \
	    m_pString = new OA::CharP[m_nTotal+1];     \
            memset(m_pString, 0, m_length+sizeof(run));\
          } else {                                     \
	    m_p##pretty = new run[m_nTotal];	       \
            /* FIXME: type-specific default value? */  \
            memset(m_p##pretty, 0, m_length);          \
          }                                            \
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
#if 0
	// If not allocated its because the caller will do the right thing with strings
      } else if (m_vt->m_baseType == OA::OCPI_String && !m_stringSpace) {
	assert(!add);
	m_stringSpaceLength = m_nTotal * (testMaxStringLength + 1);
	m_stringNext = m_stringSpace = new char[m_stringSpaceLength];
#endif
      }
      return NULL;
    }

    void Value::
    reserveStringSpace(size_t len, bool add) {
      char *old = m_stringSpace;
      size_t oldLength = m_stringSpaceLength;
      // the space required will never be larger than the input...
      m_stringSpaceLength = len + 1 + (add ? m_stringSpaceLength : 0);
      m_stringNext = m_stringSpace = new char[m_stringSpaceLength];
      if (add) {
	assert(m_vt->m_isSequence || m_vt->m_arrayRank);
	// Do the realloc of the string space, and adjust
	m_stringNext += oldLength;
	memcpy(m_stringSpace, old, oldLength);
	// Relocate string pointers
	for (unsigned n = 0; n < m_nElements; n++)
	  if (m_pString[n])
	    m_pString[n] = m_stringSpace + (m_pString[n] - old);
      }
      delete [] old;
    }

    // Overloaded with the base case (parsing a whole value),
    // and adding an element to a sequence value
    const char *Value::
    parse(const char *unparsed, const char *stop, bool add, const IdentResolver *resolver, 
	  bool *isVariable) {
      Resolver r(resolver, isVariable);
      const char *err = NULL;
      if (!stop)
	stop = unparsed + strlen(unparsed);
      if (!add)
	clear();
      if (m_vt->m_baseType == OA::OCPI_String)
	reserveStringSpace(stop - unparsed, add);
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
	// Null terminate 
	if (m_vt->m_baseType == OA::OCPI_String && !m_vt->m_arrayRank) {
	  assert(m_pString);
	  m_pString[m_nElements] = NULL;
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
    const char *Value::
    parseExpressionValue(const char *start, const char *end, size_t nSeq, size_t nArray) {
      Resolver *r = getResolver();
      struct Intercept : public IdentResolver {
	Value &value;
	Resolver *resolver;
	Intercept(Value &v, Resolver *a_r) : value(v), resolver(a_r) {}
	const char *getValue(const char *sym, ExprValue &val) const {
	  if (value.m_vt->m_baseType == OA::OCPI_Enum)
	    for (size_t n = 0; n < value.m_vt->m_nEnums; n++)
	      if (!strcasecmp(value.m_vt->m_enums[n], sym)) {
		val.setNumber(n);
		return NULL;
	      }
	  return resolver && resolver->resolver ? resolver->resolver->getValue(sym, val) :
	    "no symbols available for identifier in expression";
	}
      } mine(*this, r);
      const char *err;
      ExprValue ev;
      if (!(err = evalExpression(start, ev, &mine, end)) &&
	  !(err = ev.getTypedValue(*this, nSeq * m_vt->m_nItems + nArray)) &&
	  r->isVariable)
	*r->isVariable = ev.isVariable();
      return err;
    }

    // A single value - not sequence or array
    const char *Value::
    parseValue(const char *start, const char *end, size_t nSeq, size_t nArray) {
      // here we decide whether to use the expression parser or just the value parser.
      // For all numeric types and boolean, we can always use the expression parser.
      switch (m_vt->m_baseType) {
      case OA::OCPI_Struct:
      case OA::OCPI_Type:
	break;
	// Strings are ambiguous as to whether they are a value or an expression.
	// The need for expressions in string and char values is rare, so we need an indication
	// at the start of the value that says we have a string.
      case OA::OCPI_String:
      case OA::OCPI_Char:
	if ((size_t)(end - start) <= EXPR_PREFIX_LEN ||
	    strncmp(start, EXPR_PREFIX, EXPR_PREFIX_LEN))
	  break;
	start += EXPR_PREFIX_LEN;
      default:;
	return parseExpressionValue(start, end, nSeq, nArray);
      }	  
      // Parse a value, not an expresion
      const char *err;
      bool items = m_vt->m_isSequence || m_vt->m_arrayRank;
      size_t index = nSeq * m_vt->m_nItems + nArray;
      switch (m_vt->m_baseType) {
#define OCPI_DATA_TYPE(s,c,u,b,run,pretty,storage)			            \
	case OA::OCPI_##pretty:						            \
	  err = parse##pretty(start, end, items ? m_p##pretty[index] : m_##pretty); \
	  break;
	OCPI_PROPERTY_DATA_TYPES
	OCPI_DATA_TYPE(sca,corba,letter,bits,StructValue,Struct,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
        OCPI_DATA_TYPE(sca,corba,letter,bits,EnumValue,Enum,store)
#undef OCPI_DATA_TYPE
      default:
	err = "Unexpected illegal type in parsing value";
      }
      return err ?
	esprintf("in value \"%.*s\" (length of prop value is %zu chars): %s", (int)(end-start), start, end-start, err) : NULL;
    }

Unparser::
~Unparser()
{}

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

bool Unparser::
elementUnparse(const Value &v, std::string &s, unsigned nSeq, bool hex, char comma,
	       bool wrap, const Unparser &up) const {
  if (wrap) s += '{';
  bool r = v.unparseElement(s, nSeq, hex, comma, up);
  if (wrap) s += '}';
  return r;
}

bool Value::
unparseElement(std::string &s, unsigned nSeq, bool hex, char comma, const Unparser &up) const {
  return m_vt->m_arrayRank ?
    up.dimensionUnparse(*this, s, nSeq, 0, 0, m_vt->m_nItems, hex, comma, up) :
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

bool Value::
unparse(std::string &s, const Unparser *up, bool append, bool hex, char comma) const {
  if (!up)
    up = this;
  if (!append)
    s.clear();
  if (m_vt->m_isSequence) {
    if (!m_nElements)
      return true;
    // Now we have allocated the appropriate sequence array, so we can parse elements
    for (unsigned n = 0; n < m_nElements; n++) {
      if (n)
	s += comma;
      up->elementUnparse(*this, s, n, hex, comma, needsCommaElement(), *up);
    }
    return false; // a sequence with non-zero elements is never considered "empty".
  } else
    return up->elementUnparse(*this, s, 0, hex, comma, false, *up);
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
                              (m_vt->m_isSequence || m_vt->m_arrayRank) &&  \
                              m_p##pretty ?                                 \
			      m_p##pretty[nSeq * m_vt->m_nItems + nArray] : \
			      m_##pretty, hex);				    \
      break;
    OCPI_PROPERTY_DATA_TYPES
    OCPI_DATA_TYPE(sca,corba,letter,bits,TypeValue,Type,store)
#undef OCPI_DATA_TYPE
  case OA::OCPI_Enum:
      return up.unparseEnum(s, 
			    (m_vt->m_isSequence || m_vt->m_arrayRank) && m_pEnum ?
			    m_pEnum[nSeq * m_vt->m_nItems + nArray] :
			    m_Enum, m_vt->m_enums, m_vt->m_nEnums, hex);
  case OA::OCPI_Struct:
    return up.unparseStruct(s,
			    (m_vt->m_isSequence || m_vt->m_arrayRank) && m_pStruct ?
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
  ocpiCheck(asprintf(&cp, "%g", val) > 0);
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
  uint16_t u;
  if (val < 0) {
    u = (uint16_t)(~val + 1);
    s += '-';
  } else
    u = (uint16_t)val;
  doFormat(s, hex ? "0x%" PRIx16 : "%" PRIu16, u);
  return val == 0;
}
bool Unparser::
unparseLong(std::string &s, int32_t val, bool hex) const {
  uint32_t u;
  if (val < 0) {
    u = (uint32_t)~val + 1;
    s += '-';
  } else
    u = (uint32_t)val;
  doFormat(s, hex ? "0x%" PRIx32 : "%" PRIu32, u);
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
  uint64_t u;
  if (val < 0) {
    u = (uint64_t)~val + 1;
    s += '-';
  } else
    u = (uint64_t)val;
  doFormat(s, hex ? "0x%" PRIx64 : "%" PRIu64, u);
  return val == 0;
}
bool Unparser::
unparseULongLong(std::string &s, uint64_t val, bool hex) const {
  doFormat(s, hex ? "0x%" PRIx64 : "%" PRIu64, val);
  return val == 0;
}
size_t Value::
maxStringLength() const {
  assert(m_vt->m_baseType == OA::OCPI_String);
  size_t maxLen = 0, len;
  if (m_vt->m_isSequence || m_vt->m_arrayRank)
    for (size_t n = 0; n < m_nTotal; n++) {
      if ((len = m_pString[n] ? strlen(m_pString[n]) : 0) > maxLen)
	maxLen = len;
    }
  else if ((len = m_String ? strlen(m_String) : 0) > maxLen)
    maxLen = len;
  return maxLen;
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
#if 0
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
#endif
bool Unparser::
unparseStruct(std::string &s, StructValue val, Member *members, size_t nMembers, bool hex,
	      char comma) const {
  bool seenOne = false, empty = true;
  
  for (unsigned n = 0; val && n < nMembers; n++) {
    Value *v = *val++;
    if (v) {
      if (seenOne)
	s += comma;
      s += members[n].m_name;
      s += ' ';
      bool r;
      if (v->needsComma()) {
	s += '{';
	r = v->unparse(s, NULL, true, hex);
	s += '}';
      } else
	r = v->unparse(s, NULL, true, hex);
      if (!r)
	empty = false;
      seenOne = true;
    }
  }
  return empty;
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
unparseEnum(std::string &s, EnumValue val, const char **enums, size_t nEnums, bool) const {
  if (val < nEnums)
    s += enums[val];
  else
    formatAdd(s, "<invalid enum 0x%x>", val);
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
  if (m_vt->m_baseType == OA::OCPI_String)
    reserveStringSpace(m_nTotal * (testMaxStringLength + 1), false);
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
    setNextStringChar((char)(random() % (0177 - 036) + 036));
  setNextStringChar(0);
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
const char *Value::
getValue(ExprValue &val) const {
  return val.setFromTypedValue(*this);
#if 0
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
#endif
}
}
}

