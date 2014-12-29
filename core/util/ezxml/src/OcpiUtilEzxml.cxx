
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

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <iostream>
#include <errno.h>
#include <strings.h>

#include "ezxml.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilVfs.h"
#include "OcpiUtilEzxml.h"
#include "OcpiUtilMisc.h"

namespace OCPI {
  namespace Util {
    namespace EzXml {

      namespace OU = OCPI::Util;
      Doc::
      Doc ()
	throw ()
	: m_doc (0), m_rootNode (0)
      {
      }
      // reduce warnings about pointer members
      Doc::
      Doc (const Doc &)
	throw ()
	: m_doc (0), m_rootNode (0)
      {
      }

      Doc::
      Doc (const std::string & data)
	throw (std::string)
	: m_doc (0), m_rootNode (0)
      {
	parse (data);
	ocpiAssert (m_rootNode);
      }

      Doc::
      Doc (std::istream * in)
	throw (std::string)
	: m_doc (0), m_rootNode (0)
      {
	parse (in);
	ocpiAssert (m_rootNode);
      }

      Doc::
      Doc (OCPI::Util::Vfs::Vfs & fs, const std::string & fileName)
	throw (std::string)
	: m_doc (0), m_rootNode (0)
      {
	parse (fs, fileName);
	ocpiAssert (m_rootNode);
      }

      Doc::
      ~Doc ()
	throw ()
      {
	if (m_rootNode) {
	  ezxml_free (m_rootNode);
	}

	delete [] m_doc;
      }

      ezxml_t Doc::
      parse (const std::string & data)
	throw (std::string)
      {
	ocpiAssert (!m_rootNode);

	/*
	 * ezxml_parse_str() insists on modifiable data.
	 */

	std::string::size_type len = data.length ();
	m_doc = new char [len];
	memcpy (m_doc, data.data(), len);

	m_rootNode = ::ezxml_parse_str (m_doc, len);

	if (!m_rootNode) {
	  delete [] m_doc;
	  m_doc = 0;
	  throw std::string ("error parsing document");
	}

	if (*ezxml_error (m_rootNode)) {
	  std::string errMsg = ezxml_error (m_rootNode);
	  ezxml_free (m_rootNode);
	  m_rootNode = 0;
	  delete [] m_doc;
	  m_doc = 0;
	  throw errMsg;
	}

	return m_rootNode;
      }

      ezxml_t Doc::
      parse (char *data)
	throw (std::string)
      {
	ocpiAssert (!m_rootNode);

	m_doc = data;
	m_rootNode = ::ezxml_parse_str (m_doc, strlen(m_doc));

	if (!m_rootNode) {
	  delete [] m_doc;
	  m_doc = 0;
	  throw std::string ("error parsing document");
	}

	if (*ezxml_error (m_rootNode)) {
	  std::string errMsg = ezxml_error (m_rootNode);
	  ezxml_free (m_rootNode);
	  m_rootNode = 0;
	  delete [] m_doc;
	  m_doc = 0;
	  throw errMsg;
	}

	return m_rootNode;
      }

      ezxml_t Doc::
      parse (std::istream * in)
	throw (std::string)
      {
	unsigned int blockSize = 16384;
	size_t length = 0;
	unsigned int curAlloc = blockSize;
	m_doc = new char [blockSize];
	char * ptr = m_doc;

	ocpiAssert (!m_rootNode);

	while (!in->good()) {
	  if (length + blockSize > curAlloc) {
	    char * newBuffer = new char [curAlloc + blockSize];
	    memcpy (newBuffer, m_doc, length);
	    delete [] m_doc;
	    m_doc = newBuffer;
	    ptr = m_doc + length;
	    curAlloc += blockSize;
	  }

	  std::streamsize count = in->read (ptr, blockSize).gcount ();
	  length += count;
	  ptr += count;
	}

	if (in->fail()) {
	  delete [] m_doc;
	  m_doc = 0;
	  throw std::string ("error reading file");
	}

	m_rootNode = ::ezxml_parse_str (m_doc, length);

	if (!m_rootNode) {
	  delete [] m_doc;
	  m_doc = 0;
	  throw std::string ("error parsing document");
	}

	if (*ezxml_error (m_rootNode)) {
	  std::string errMsg = ezxml_error (m_rootNode);
	  ezxml_free (m_rootNode);
	  m_rootNode = 0;
	  delete [] m_doc;
	  m_doc = 0;
	  throw errMsg;
	}

	return m_rootNode;
      }

      ezxml_t Doc::
      parse (OCPI::Util::Vfs::Vfs & fs, const std::string & fileName)
	throw (std::string)
      {
	ocpiAssert (!m_rootNode);

	/*
	 * Opening the file may throw std::string.  Fine with us.
	 */

	std::istream * in = fs.openReadonly (fileName);

	/*
	 * Determine the file size so that we can pre-allocate the buffer instead
	 * of having to keep reallocating it.
	 */

	size_t fileSize;

	try {
	  fileSize = OCPI_UTRUNCATE(size_t, fs.size (fileName));
	}
	catch (...) {
	  /*
	   * Can't figure out the file size.  Try to just read it.
	   */

	  try {
	    parse (in);
	  }
	  catch (...) {
	    try {
	      fs.close (in);
	    }
	    catch (...) {
	    }

	    throw;
	  }

	  try {
	    fs.close (in);
	  }
	  catch (...) {
	    ezxml_free (m_rootNode);
	    m_rootNode = 0;
	    throw;
	  }

	  /*
	   * We successfully read the file and are done.
	   */

	  return m_rootNode;
	}

	m_doc = new char [fileSize];
	std::streamsize count = in->read (m_doc, fileSize).gcount ();

	try {
	  fs.close (in);
	}
	catch (...) {
	  delete [] m_doc;
	  m_doc = 0;
	  throw;
	}

	/*
	 * Should we check whether count != fileSize?
	 */

	m_rootNode = ::ezxml_parse_str (m_doc, count);

	if (!m_rootNode) {
	  delete [] m_doc;
	  m_doc = 0;
	  throw std::string ("error parsing document");
	}

	if (*ezxml_error (m_rootNode)) {
	  std::string errMsg = ezxml_error (m_rootNode);
	  ezxml_free (m_rootNode);
	  m_rootNode = 0;
	  delete [] m_doc;
	  m_doc = 0;
	  throw errMsg;
	}

	return m_rootNode;
      }


      ezxml_t Doc::
      getRootNode ()
	throw ()
      {
	ocpiAssert (m_rootNode);
	return m_rootNode;
      }

      const char *
      checkElements(ezxml_t x, ...) {
	va_list ap;
	if (!x || !x->child)
	  return 0;
	for (ezxml_t c = x->child; c; c = c->sibling) {
	  va_start(ap, x);
	  char *p;
	  while ((p = va_arg(ap, char*)))
	    if (!strcasecmp(p, c->name))
	      break;
	  va_end(ap);
	  if (!p)
	    return esprintf("Invalid element \"%s\", in a \"%s\" element", c->name, x->name);
	}
	return 0;
      }

      const char *
      checkAttrs(ezxml_t x, ...) {
	va_list ap;
	if (!x->attr)
	  return 0;
	for (char **a = x->attr; *a; a += 2) {
	  va_start(ap, x);
	  char *p;
	  while ((p = va_arg(ap, char*)))
	    if (!strcasecmp(p, *a))
	      break;
	  va_end(ap);
	  if (!p)
	    return esprintf("Invalid attribute name: \"%s\", in a %s element", *a, x->name);
	}
	return 0;
      }

      const char *
      checkAttrsVV(ezxml_t x, ...) {
	if (!x->attr)
	  return 0;
	for (char **a = x->attr; *a; a += 2) {
	  va_list ap;
	  va_start(ap, x);
	  for (const char **vap = va_arg(ap, const char **); vap; vap = va_arg(ap, const char **))
	    while (*vap)
	      if (!strcasecmp(*a, *vap++)) {
		va_end(ap);
		goto found;
	      }
	  va_end(ap);
	  return esprintf("Invalid attribute name: \"%s\", in a %s element", *a, x->name);
	found:;
	}
	return 0;
      }

      const char *
      checkAttrsV(ezxml_t x, const char **attrs) {
	if (!x->attr)
	  return 0;
	for (char **a = x->attr; *a; a += 2) {
	  const char **va;
	  for (va = attrs; *va; va++)
	    if (!strcasecmp(*a, *va))
	      break;
	  if (!*va)
	    return esprintf("Invalid attribute name: \"%s\", in a %s element", *a, x->name);
	}
	return 0;
      }

      bool
      getUNum(const char *s, size_t *valp) {
	char *endptr;
	errno = 0;
	unsigned long val = strtoul(s, &endptr, 0);
	if (errno == 0 && endptr != s) {
	  if (*endptr == 'K' || *endptr == 'k') {
	    endptr++;
	    val *= 1024;
	  } else if (*endptr == 'M' || *endptr == 'm') {
	    endptr++;
	    val *= 1024*1024;
	  } else if (*endptr == 'G' || *endptr == 'g') {
	    endptr++;
	    val *= 1024ul*1024ul*1024ul;
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
	  if (valp)
	    *valp = (size_t)val;
	  return false;
	}
	return true;
      }

      const char *
      getNumber(ezxml_t x, const char *attr, size_t *np, bool *found,
		size_t defaultValue, bool setDefault, bool required) {
	const char *a = ezxml_cattr(x, attr);
	if (!a) {
	  if (required)
	    return esprintf("Missing number attribute '%s' in element '%s'",
			     attr, ezxml_tag(x));
	  if (found)
	    *found = false;
	  if (setDefault && np)
	    *np = defaultValue;
	  return 0;
	}
	if (getUNum(a, np))
	  return esprintf("Bad numeric value: '%s' for attribute '%s' in element '%s'",
			  a, attr, x->name);
	if (found)
	  *found = true;
	return 0;
      }

      // FIXME: restrict to 8 bit values
      const char *
      getNumber8(ezxml_t x, const char *attr, uint8_t *np, bool *found,
		 uint32_t defaultValue, bool setDefault) {
	size_t n;
	const char *err = getNumber(x, attr, &n, found, defaultValue,
				    setDefault);
	if (!err)
	  *np = (uint8_t)n;
	return err;
      }

      // Return true on error. "end" can be NULL
      bool
      getUNum64(const char *s, const char *end, uint64_t &val) {
	char *endptr;
	errno = 0;
	val = strtoull(s, &endptr, 0);
	// We do not handle "end" pointing to a valid digit
	do {
	  if (errno != 0 || (end && endptr > end))
	    break;
	  while ((!end || endptr < end) && isspace(*endptr))
	    endptr++;
	  if (!end || endptr < end) {
	    if (*endptr == 'K' || *endptr == 'k') {
	      endptr++;
	      val *= 1024;
	    } else if (*endptr == 'M' || *endptr == 'm') {
	      endptr++;
	      val *= 1024*1024;
	    } else if (*endptr == 'G' || *endptr == 'g') {
	      endptr++;
	      val *= 1024ull*1024ull*1024ull;
	    } else if (*endptr == '-') {
	      endptr++;
	      while ((!end || endptr < end) && isspace(*endptr))
		endptr++;
	      if ((end && endptr >= end) || *endptr != '1')
		break;
	      val--;
	    }
	    while ((!end || endptr < end) && isspace(*endptr))
	      endptr++;
	    if ((end && endptr < end) || (!end && *endptr))
	      break;
	  }
	  return false;
	} while(0);
	return true;
      }

    // return true on error
    bool
    getNum64(const char *s, const char *end, int64_t &val) {
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
	if (getUNum64(s, end, uval))
	  break;
	if (minus) {
	  if (uval > ((uint64_t)1) << 63)
	    break;
	  val = -(int64_t)uval;
	} else if (uval > (uint64_t)INT64_MIN)
	  break;
	else
	  val = (int64_t)uval;
	return false;
      } while (0);
      return true;
    }
      const char *
      getNumber64(ezxml_t x, const char *attr, uint64_t *np, bool *found,
		  uint64_t defaultValue, bool setDefault, bool required) {
	const char *a = ezxml_cattr(x, attr);
	if (!a) {
	  if (required)
	    return esprintf("Missing number attribute '%s' in element '%s'",
			     attr, ezxml_tag(x));
	  if (found)
	    *found = false;
	  if (setDefault)
	    *np = defaultValue;
	  return 0;
	}
	if (getUNum64(a, NULL, *np))
	  return esprintf("Bad numeric value: \"%s\" for attribute %s in element %s",
			  a, attr, x->name);
	if (found)
	  *found = true;
	return 0;
      }

      bool
      parseBool(const char *a, const char *end, bool *b)
      {
	size_t n = end ? end - a : strlen(a);
	if (n == 4 && !strncasecmp(a, "true", 4) || n == 1 && !strncmp(a, "1", 1))
	  *b = true;
	else if (n == 5 && !strncasecmp(a, "false", 5)  || n == 1 && !strncmp(a, "0", 1))
	  *b =  false;
	else
	  return true;
	return false;
      }

      const char *
      getBoolean(ezxml_t x, const char *name, bool *b, bool trueOnly, bool setDefault,
		 bool *found) {
	const char *a = ezxml_cattr(x, name);
	if (a) {
	  bool val;
	  if (parseBool(a, NULL, &val))
	    return esprintf("parsing value \"%s\" as type Bool", a);
	  if (trueOnly && !val)
	    return "can only set the value to true in this context";
	  *b = val;
	  if (found)
	    *found = true;
	} else if (!trueOnly && setDefault)
	  *b = false;
	return 0;
      }

      bool
      hasAttrEq(ezxml_t x, const char *attrName, const char *val) {
	const char *attr = ezxml_cattr(x, attrName);
	return attr && !strcasecmp(attr, val);
      }

      ezxml_t
      findChildWithAttr(ezxml_t x, const char *cName, const char *aName,
			const char *value) {
	for (ezxml_t c = ezxml_cchild(x, cName); c; c = ezxml_next(c))
	  if (hasAttrEq(c, aName, value))
	    return c;
	return 0;
      }
      unsigned countChildren(ezxml_t x, const char*cName) {
	unsigned n = 0;
	for (ezxml_t c = ezxml_cchild(x, cName); c; c = ezxml_next(c))
	  n++;
	return n;
      }
      void getNameWithDefault(ezxml_t x, std::string &s, const char *fmt, unsigned &ord) {
	const char *name = ezxml_cattr(x, "name");
	if (name)
	  s = name;
	else
	  formatString(s, fmt, ord++);
      }
      const char *getRequiredString(ezxml_t x, std::string &s, const char *attr,
				    const char *element) {
	if (!element)
	  element = ezxml_tag(x);
	const char *cp = ezxml_cattr(x, attr);
	if (!cp)
	  return esprintf("Missing \"%s\" attribute for \"%s\" element", attr, element);
	s = cp;
	return NULL;
      }
      // Note that this sets the output string to empty if it is not found
      bool
      getOptionalString(ezxml_t x, std::string &s, const char *attr) {
	const char *cp = ezxml_cattr(x, attr);
	s = cp ? cp : "";
	return cp != NULL;
      }
      bool
      inList(const char *item, const char *list) {
	if (list) {
	  char
	    *mylist = strdup(list),
	    *base,
	    *last = 0,
	    *tok;
	  for (base = mylist; (tok = strtok_r(base, ", \t", &last)); base = NULL)
	    if (!strcasecmp(tok, item))
	      break;
	  free(mylist);
	  if (tok)
	    return true;
	}
        return false;
      }
      ezxml_t
      ezxml_firstChild(ezxml_t xml) {
	return xml ? xml->child : 0;
      }
      ezxml_t
      ezxml_nextChild(ezxml_t xml) {
	return xml->ordered;
      }
      const char *
      ezxml_cchildren(ezxml_t xml, const char* (*func)(ezxml_t child, void *arg), void *arg,
		      const char *tag) {
	const char *err;
	for (xml = ezxml_cchild(xml, tag); xml; xml = ezxml_next(xml))
	  if ((err = (*func)(xml, arg)))
	    return err;
	return 0;
      }
      const char *
      ezxml_children(ezxml_t xml, const char* (*func)(ezxml_t child, void *arg), void *arg) {
	const char *err;
	for (xml = xml ? xml->child : NULL; xml; xml = xml->ordered)
	  if ((err = (*func)(xml, arg)))
	    return err;
	return 0;
      }
      const char *
      ezxml_attrs(ezxml_t xml, const char* (*func)(const char *name, const char *value, void *arg), void *arg) {
	const char *err;
	for (char **ap = xml ? xml->attr : 0; ap && *ap; ap += 2)
	  if ((err = (*func)(ap[0], ap[1], arg)))
	    return err;
	return 0;
      }
      unsigned countAttributes(ezxml_t xml) {
	unsigned n = 0;
	for (char **ap = xml ? xml->attr : 0; ap && *ap; ap += 2)
	  n++;
	return n;
      }
      const char *
      getEnum(ezxml_t x, const char *attr, const char **enums, const char *type, size_t &n,
	      size_t def, bool required) {
	const char *a = ezxml_cattr(x, attr);
	if (!a) {
	  if (required)
	    return OU::esprintf("Missing %s attribute", type);
	  n = def;
	  return NULL;
	}
	for (const char **ap = enums; *ap; ap++)
	  if (**ap && !strcasecmp(a, *ap)) { // allow ignoring of empty enum strings
	    n = ap - enums;
	    return NULL;
	  }
	return esprintf("Unknown %s in %s attribute: \"%s\".", type, attr, a);
      }
      const char *
      ezxml_tag(ezxml_t xml) {
	const char *name = ezxml_name(xml);
	return name ? name : "";
      }
      const char *
      checkTag(ezxml_t xml, const char *tag, const char *fmt, ...) {
	va_list ap;
	if (!strcasecmp(ezxml_tag(xml), tag))
	  return NULL;
	va_start(ap, fmt);
	return evsprintf(fmt, ap);
      }
      const char *
      ezxml_parse_file(const char *file, ezxml_t &xml) {
	if (!(xml = ::ezxml_parse_file(file)))
	  return OU::esprintf("Could not parse xml file: '%s'", file);
	else if (ezxml_error(xml)[0])
	  return OU::esprintf("Error parsing xml file '%s': %s", file, ezxml_error(xml));
	return NULL;
      }
      const char *
      ezxml_parse_str(char *string, size_t len, ezxml_t &xml) {
	if (!(xml = ::ezxml_parse_str(string, len)))
	  return "Could not parse xml string";
	else if (ezxml_error(xml)[0])
	  return OU::esprintf("Error parsing xml string': %s", ezxml_error(xml));
	return NULL;
      }
    }
  }
}
