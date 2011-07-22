
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

#include <iostream>
#include <errno.h>
#include <strings.h>
#include <ezxml.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilEzxml.h>

namespace OCPI {
  namespace Util {
    namespace EzXml {

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

	m_rootNode = ezxml_parse_str (m_doc, len);

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
	m_rootNode = ezxml_parse_str (m_doc, strlen(m_doc));

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
	unsigned int length = 0;
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

	  unsigned int count = in->read (ptr, blockSize).gcount ();
	  length += count;
	  ptr += count;
	}

	if (in->fail()) {
	  delete [] m_doc;
	  m_doc = 0;
	  throw std::string ("error reading file");
	}

	m_rootNode = ezxml_parse_str (m_doc, length);

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

	unsigned long long fileSize;

	try {
	  fileSize = fs.size (fileName);
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
	unsigned int count = in->read (m_doc, fileSize).gcount ();

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

	m_rootNode = ezxml_parse_str (m_doc, count);

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
      getUNum(const char *s, uint32_t *valp) {
	char *endptr;
	errno = 0;
	uint32_t val =  strtoul(s, &endptr, 0);
	if (errno == 0) {
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
	  *valp = val;
	  return false;
	}
	return true;
      }

      const char *
      getNumber(ezxml_t x, const char *attr, uint32_t *np, bool *found,
		uint32_t defaultValue, bool setDefault) {
	const char *a = ezxml_cattr(x, attr);
	if (!a) {
	  if (found)
	    *found = false;
	  if (setDefault)
	    *np = defaultValue;
	  return 0;
	}
	if (getUNum(a, np))
	  return esprintf("Bad numeric value: \"%s\" for attribute %s in element %s",
			  a, attr, x->name);
	if (found)
	  *found = true;
	return 0;
      }

      // FIXME: restrict to 8 bit values
      const char *
      getNumber8(ezxml_t x, const char *attr, uint8_t *np, bool *found,
		 uint32_t defaultValue, bool setDefault) {
	uint32_t i32;
	const char *err = getNumber(x, attr, &i32, found, defaultValue,
				    setDefault);
	if (!err)
	  *np = (uint8_t)i32;
	return err;
      }

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

      const char *
      getNumber64(ezxml_t x, const char *attr, uint64_t *np, bool *found,
		  uint64_t defaultValue, bool setDefault) {
	const char *a = ezxml_cattr(x, attr);
	if (!a) {
	  if (found)
	    *found = false;
	  if (setDefault)
	    *np = defaultValue;
	  return 0;
	}
	if (getUNum64(a, np))
	  return esprintf("Bad numeric value: \"%s\" for attribute %s in element %s",
			  a, attr, x->name);
	if (found)
	  *found = true;
	return 0;
      }

      bool
      parseBool(const char *a, unsigned length, bool *b)
      {
	(void)length;
	if (!strcasecmp(a, "true") || !strcmp(a, "1"))
	  *b = true;
	else if (!strcasecmp(a, "false")  || !strcmp(a, "0"))
	  *b =  false;
	else
	  return true;
	return false;
      }

      const char *
      getBoolean(ezxml_t x, const char *name, bool *b) {
	const char *a = ezxml_cattr(x, name);
	if (a) {
	  if (parseBool(a, 0, b))
	    return esprintf("parsing value \"%s\" as type Bool", a);
	} else
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
      bool
      inList(const char *item, const char *list) {
	if (list) {
	  char
	    *mylist = strdup(list),
	    *base = mylist, 
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
    }
  }
}
const char *esprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *buf;
  vasprintf(&buf, fmt, ap);
  va_end(ap);
  return buf;
}
