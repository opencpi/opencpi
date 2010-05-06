#include <CpiUtilHttpFs.h>
#include <CpiUtilVfs.h>
#include <CpiUtilHttpClient.h>
#include <CpiUtilMisc.h>
#include <CpiUtilAutoMutex.h>
#include <CpiOsFileSystem.h>
#include <CpiOsMutex.h>
#include <iostream>
#include <string>
#include <ctime>
#include <cassert>

CPI::Util::Http::HttpFsBase::
HttpFsBase (const std::string & scheme,
            const std::string & root)
  throw (std::string)
{
  if (root.length() && root[root.length()-1] == '/') {
    m_root = root.substr (0, root.length() - 1);
  }
  else {
    m_root = root;
  }

  if (m_root.length() && m_root[0] != '/') {
    throw std::string ("invalid root");
  }

  m_cwd = "/";
  m_scheme = scheme;
  m_baseURI = scheme;
  m_baseURI += ":/";

  if (m_root.length()) {
    std::string::size_type firstSlash = m_root.find ('/', 1);
    std::string firstPathComponent;

    if (firstSlash == std::string::npos) {
      firstPathComponent = m_root.substr (1);
    }
    else {
      firstPathComponent = m_root.substr (1, firstSlash-1);
    }

    m_baseURI += '/';
    m_baseURI += CPI::Util::Uri::encode (firstPathComponent, ":");

    if (firstSlash != std::string::npos) {
      std::string remainingPathComponents = m_root.substr (firstSlash);
      m_baseURI += CPI::Util::Uri::encode (remainingPathComponents, "/");
    }
  }

  m_baseURI += "/";
}

CPI::Util::Http::HttpFsBase::
~HttpFsBase ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * URI mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Http::HttpFsBase::
baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
CPI::Util::Http::HttpFsBase::
nameToURI (const std::string & fileName) const
  throw (std::string)
{
  return nativeFilename (fileName);
}

std::string
CPI::Util::Http::HttpFsBase::
URIToName (const std::string & uri) const
  throw (std::string)
{
  if (uri.length() < m_baseURI.length() ||
      uri.compare (0, m_baseURI.length(), m_baseURI) != 0) {
    std::string reason = "URI \"";
    reason += uri;
    reason += "\" not understood by this file system, expecting \"";
    reason += m_baseURI;
    reason += "\" prefix";
    throw reason;
  }

  std::string remainingName = uri.substr (m_baseURI.length() - 1);
  return CPI::Util::Uri::decode (remainingName);
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Http::HttpFsBase::absoluteNameLocked (const std::string & name) const
  throw (std::string)
{
  return CPI::Util::Vfs::joinNames (m_cwd, name);
}

/*
 * ----------------------------------------------------------------------
 * Directory Management
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Http::HttpFsBase::
cwd () const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_lock);
  return m_cwd;
}

void
CPI::Util::Http::HttpFsBase::
cd (const std::string & fileName)
  throw (std::string)
{
  testFilenameForValidity (fileName);
  CPI::Util::AutoMutex lock (m_lock);
  m_cwd = absoluteNameLocked (fileName);
}

void
CPI::Util::Http::HttpFsBase::
mkdir (const std::string &)
  throw (std::string)
{
  /*
   * cannot create directories via HTTP
   */

  throw std::string ("mkdir not supported over HTTP");
}

void
CPI::Util::Http::HttpFsBase::
rmdir (const std::string &)
  throw (std::string)
{
  /*
   * cannot remove directories via HTTP
   */

  throw std::string ("rmdir not supported over HTTP");
}


/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Iterator *
CPI::Util::Http::HttpFsBase::
list (const std::string &, const std::string &)
  throw (std::string)
{
  throw std::string ("cannot list files via HTTP");
  return 0; // silence some stupid compilers
}

void
CPI::Util::Http::HttpFsBase::
closeIterator (CPI::Util::Vfs::Iterator *)
  throw (std::string)
{
  throw std::string ("should not be here");
}

/*
 * ----------------------------------------------------------------------
 * File Information
 * ----------------------------------------------------------------------
 */

CPI::Util::Http::ClientStream *
CPI::Util::Http::HttpFsBase::
hgpr (const std::string & fileName,
      bool head, bool get, bool post, bool remove)
  throw (std::string)
{
  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);
  CPI::Util::Uri uri (nn);

  CPI::Util::Http::ClientStream * conn = makeConnection ();

  /*
   * To do: protect against circular redirections
   */

 again:
  try {
    if (head) {
      conn->head (uri);
    }
    else if (get) {
      conn->get (uri);
    }
    else if (post) {
      conn->post (uri);
    }
    else if (remove) {
      conn->remove (uri);
    }
    else {
      assert (0);
    }
  }
  catch (CPI::Util::Http::Redirection & redir) {
    if (redir.newLocation == uri.get()) {
      std::string reason = "redirection to identity at \"";
      reason += uri.get();
      reason += "\"";
      throw reason;
    }

    std::string duri = CPI::Util::Uri::decode (redir.newLocation);

    if (duri.length() < m_baseURI.length() ||
        duri.compare (0, m_baseURI.length(), m_baseURI) != 0) {
      throw;
    }

    uri = redir.newLocation;
    goto again;
  }
  catch (CPI::Util::Http::ClientError & error) {
    std::string reason = "file not found: \"";
    reason += error.reasonPhrase;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (CPI::Util::Http::ServerError & error) {
    std::string reason = "server error: \"";
    reason += error.reasonPhrase;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (...) {
    delete conn;
    throw;
  }

  return conn;
}

bool
CPI::Util::Http::HttpFsBase::
exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  CPI::Util::Http::ClientStream * conn;

  try {
    conn = hgpr (fileName, true, false, false, false);
  }
  catch (const std::string &) {
    return false;
  }

  int code = conn->statusCode ();
  delete conn;

  if (code >= 200 && code < 300) {
    if (isDir) {
      isDir = 0;
    }
    return true;
  }

  return false;
}

unsigned long long
CPI::Util::Http::HttpFsBase::
size (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::Http::ClientStream * conn = hgpr (fileName, true, false, false, false);
  unsigned long long res;

  try {
    res = conn->contentLength ();
  }
  catch (...) {
    delete conn;
    throw;
  }

  delete conn;
  return res;
}

std::time_t
CPI::Util::Http::HttpFsBase::
lastModified (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::Http::ClientStream * conn = hgpr (fileName, true, false, false, false);
  std::time_t res;

  try {
    res = conn->lastModified ();
  }
  catch (...) {
    delete conn;
    throw;
  }

  delete conn;
  return res;
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
CPI::Util::Http::HttpFsBase::
open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("file modification not supported");
  return 0;
}

std::istream *
CPI::Util::Http::HttpFsBase::
openReadonly (const std::string & fileName, std::ios_base::openmode)
  throw (std::string)
{ 
  return hgpr (fileName, false, true, false, false);
}

std::ostream *
CPI::Util::Http::HttpFsBase::openWriteonly (const std::string & fileName, std::ios_base::openmode)
  throw (std::string)
{
  return hgpr (fileName, false, false, true, false);
}

void
CPI::Util::Http::HttpFsBase::close (std::ios * str)
  throw (std::string)
{
  CPI::Util::Http::ClientStream * conn =
    dynamic_cast<CPI::Util::Http::ClientStream *> (str);

  if (!conn) {
    throw std::string ("unrecognized stream");
  }

  try {
    conn->close ();
  }
  catch (CPI::Util::Http::Redirection & redir) {
    std::string reason = "oops: unexpected redirection to \"";
    reason += redir.newLocation;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (CPI::Util::Http::ClientError & error) {
    std::string reason = "client-side error: \"";
    reason += error.reasonPhrase;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (CPI::Util::Http::ServerError & error) {
    std::string reason = "server error: \"";
    reason += error.reasonPhrase;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (...) {
    delete conn;
    throw;
  }

  delete conn;
}

/*
 * ----------------------------------------------------------------------
 * File system operations
 * ----------------------------------------------------------------------
 */

void
CPI::Util::Http::HttpFsBase::remove (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::Http::ClientStream * conn = hgpr (fileName, false, false, false, true);
  delete conn;
}

/*
 * ----------------------------------------------------------------------
 * Test whether a file name is valid. Throw an exception if not.
 * ----------------------------------------------------------------------
 */

void
CPI::Util::Http::HttpFsBase::testFilenameForValidity (const std::string & name) const
  throw (std::string)
{
  if (name.length() == 1 && name[0] == '/') {
    /*
     * An exception for the name of the root directory
     */
    return;
  }

  std::string::size_type pos = (name[0] == '/') ? 1 : 0;

  do {
    std::string::size_type nextSlash = name.find ('/', pos);
    std::string pathComponent;

    if (nextSlash == std::string::npos) {
      pathComponent = name.substr (pos);
      pos = std::string::npos;
    }
    else {
      pathComponent = name.substr (pos, nextSlash - pos);
      pos = nextSlash + 1;
    }

    /*
     * See if the path component is okay
     */

    if (pathComponent == "." || pathComponent == "..") {
      std::string reason = "invalid file name: \"";
      reason += pathComponent;
      reason += "\": invalid path component";
      throw reason;
    }

    if (pathComponent.find_first_of ("<>\\|\"") != std::string::npos) {
      std::string reason = "invalid file name: \"";
      reason += pathComponent;
      reason += "\": invalid character in path component";
      throw reason;
    }
  }
  while (pos != std::string::npos);
}

std::string
CPI::Util::Http::HttpFsBase::nativeFilename (const std::string & fileName) const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_lock);
  std::string an = absoluteNameLocked (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += CPI::Util::Uri::encode (an, "/");
  return uri;
}
