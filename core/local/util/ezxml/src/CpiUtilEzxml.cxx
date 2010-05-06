#include <iostream>
#include <CpiOsAssert.h>
#include <CpiUtilVfs.h>
#include <CpiUtilEzxml.h>
#include <ezxml.h>

CPI::Util::EzXml::Doc::
Doc ()
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
  char * m_doc = new char [len];
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
