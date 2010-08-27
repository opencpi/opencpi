#include <iostream>
#include <errno.h>
#include <strings.h>
#include <ezxml.h>
#include <CpiOsAssert.h>
#include <CpiUtilVfs.h>
#include <CpiUtilEzxml.h>


CPI::Util::EzXml::Doc::
Doc ()
  throw ()
  : m_doc (0),
    m_rootNode (0)
{
}
// reduce warnings about pointer members
CPI::Util::EzXml::Doc::
Doc (const Doc &)
  throw ()
  : m_doc (0),
    m_rootNode (0)
{
}

CPI::Util::EzXml::Doc::
Doc (const std::string & data)
  throw (std::string)
  : m_doc (0),
    m_rootNode (0)
{
  parse (data);
  cpiAssert (m_rootNode);
}

CPI::Util::EzXml::Doc::
Doc (std::istream * in)
  throw (std::string)
  : m_doc (0),
    m_rootNode (0)
{
  parse (in);
  cpiAssert (m_rootNode);
}

CPI::Util::EzXml::Doc::
Doc (CPI::Util::Vfs::Vfs & fs, const std::string & fileName)
  throw (std::string)
  : m_doc (0),
    m_rootNode (0)
{
  parse (fs, fileName);
  cpiAssert (m_rootNode);
}

CPI::Util::EzXml::Doc::
~Doc ()
  throw ()
{
  if (m_rootNode) {
    ezxml_free (m_rootNode);
  }

  delete [] m_doc;
}

ezxml_t
CPI::Util::EzXml::Doc::
parse (const std::string & data)
  throw (std::string)
{
  cpiAssert (!m_rootNode);

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

ezxml_t
CPI::Util::EzXml::Doc::
parse (std::istream * in)
  throw (std::string)
{
  unsigned int blockSize = 16384;
  unsigned int length = 0;
  unsigned int curAlloc = blockSize;
  m_doc = new char [blockSize];
  char * ptr = m_doc;

  cpiAssert (!m_rootNode);

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

ezxml_t
CPI::Util::EzXml::Doc::
parse (CPI::Util::Vfs::Vfs & fs, const std::string & fileName)
  throw (std::string)
{
  cpiAssert (!m_rootNode);

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


ezxml_t
CPI::Util::EzXml::Doc::
getRootNode ()
  throw ()
{
  cpiAssert (m_rootNode);
  return m_rootNode;
}

const char *
CPI::Util::EzXml::
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
bool
CPI::Util::EzXml::
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
CPI::Util::EzXml::
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
  if (CPI::Util::EzXml::getUNum(a, np))
    return esprintf("Bad numeric value: \"%s\" for attribute %s in element %s",
		    a, attr, x->name);
  if (found)
    *found = true;
  return 0;
}
 bool
CPI::Util::EzXml::
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
CPI::Util::EzXml::
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
  if (CPI::Util::EzXml::getUNum64(a, np))
    return esprintf("Bad numeric value: \"%s\" for attribute %s in element %s",
		    a, attr, x->name);
  if (found)
    *found = true;
  return 0;
}

  bool
CPI::Util::EzXml::
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
CPI::Util::EzXml::
getBoolean(ezxml_t x, const char *name, bool *b) {
  const char *a = ezxml_cattr(x, name);
  if (a) {
    if (parseBool(a, 0, b))
      return esprintf("parsing value \"%s\" as type Bool", a);
  } else
    *b = false;
  return 0;
}
const char *esprintf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *buf;
  vasprintf(&buf, fmt, ap);
  va_end(ap);
  return buf;
}
