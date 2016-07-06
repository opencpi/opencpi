
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

#include <OcpiUtilHttpFs.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilHttpClient.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiOsFileSystem.h>
#include <OcpiOsMutex.h>
#include <iostream>
#include <string>
#include <ctime>
#include <cassert>

OCPI::Util::Http::HttpFsBase::
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
    m_baseURI += OCPI::Util::Uri::encode (firstPathComponent, ":");

    if (firstSlash != std::string::npos) {
      std::string remainingPathComponents = m_root.substr (firstSlash);
      m_baseURI += OCPI::Util::Uri::encode (remainingPathComponents, "/");
    }
  }

  m_baseURI += "/";
}

OCPI::Util::Http::HttpFsBase::
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
OCPI::Util::Http::HttpFsBase::
baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
OCPI::Util::Http::HttpFsBase::
nameToURI (const std::string & fileName) const
  throw (std::string)
{
  return nativeFilename (fileName);
}

std::string
OCPI::Util::Http::HttpFsBase::
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
  return OCPI::Util::Uri::decode (remainingName);
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Http::HttpFsBase::absoluteNameLocked (const std::string & name) const
  throw (std::string)
{
  return OCPI::Util::Vfs::joinNames (m_cwd, name);
}

/*
 * ----------------------------------------------------------------------
 * Directory Management
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Http::HttpFsBase::
cwd () const
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_lock);
  return m_cwd;
}

void
OCPI::Util::Http::HttpFsBase::
cd (const std::string & fileName)
  throw (std::string)
{
  testFilenameForValidity (fileName);
  OCPI::Util::AutoMutex lock (m_lock);
  m_cwd = absoluteNameLocked (fileName);
}

void
OCPI::Util::Http::HttpFsBase::
mkdir (const std::string &)
  throw (std::string)
{
  /*
   * cannot create directories via HTTP
   */

  throw std::string ("mkdir not supported over HTTP");
}

void
OCPI::Util::Http::HttpFsBase::
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

#if 0
OCPI::Util::Vfs::Iterator *
OCPI::Util::Http::HttpFsBase::
list (const std::string &, const std::string &)
  throw (std::string)
{
  throw std::string ("cannot list files via HTTP");
  return 0; // silence some stupid compilers
}

void
OCPI::Util::Http::HttpFsBase::
closeIterator (OCPI::Util::Vfs::Iterator *)
  throw (std::string)
{
  throw std::string ("should not be here");
}
#endif

OCPI::Util::Vfs::Dir &OCPI::Util::Http::HttpFsBase::openDir(const std::string &) throw(std::string) {
  throw std::string ("cannot list files via HTTP");
  return *(OCPI::Util::Vfs::Dir*)0; // silence some stupid compilers
}

/*
 * ----------------------------------------------------------------------
 * File Information
 * ----------------------------------------------------------------------
 */

OCPI::Util::Http::ClientStream *
OCPI::Util::Http::HttpFsBase::
hgpr (const std::string & fileName,
      bool head, bool get, bool post, bool a_remove)
  throw (std::string)
{
  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);
  OCPI::Util::Uri uri (nn);

  OCPI::Util::Http::ClientStream * conn = makeConnection ();

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
    else if (a_remove) {
      conn->remove (uri);
    }
    else {
      assert (0);
    }
  }
  catch (OCPI::Util::Http::Redirection & redir) {
    if (redir.newLocation == uri.get()) {
      std::string reason = "redirection to identity at \"";
      reason += uri.get();
      reason += "\"";
      throw reason;
    }

    std::string duri = OCPI::Util::Uri::decode (redir.newLocation);

    if (duri.length() < m_baseURI.length() ||
        duri.compare (0, m_baseURI.length(), m_baseURI) != 0) {
      throw;
    }

    uri = redir.newLocation;
    goto again;
  }
  catch (OCPI::Util::Http::ClientError & error) {
    std::string reason = "file not found: \"";
    reason += error.reasonPhrase;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (OCPI::Util::Http::ServerError & error) {
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
OCPI::Util::Http::HttpFsBase::
exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  OCPI::Util::Http::ClientStream * conn;

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
OCPI::Util::Http::HttpFsBase::
size (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::Http::ClientStream * conn = hgpr (fileName, true, false, false, false);
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
OCPI::Util::Http::HttpFsBase::
lastModified (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::Http::ClientStream * conn = hgpr (fileName, true, false, false, false);
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
OCPI::Util::Http::HttpFsBase::
open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("file modification not supported");
  return 0;
}

std::istream *
OCPI::Util::Http::HttpFsBase::
openReadonly (const std::string & fileName, std::ios_base::openmode)
  throw (std::string)
{ 
  return hgpr (fileName, false, true, false, false);
}

std::ostream *
OCPI::Util::Http::HttpFsBase::openWriteonly (const std::string & fileName, std::ios_base::openmode)
  throw (std::string)
{
  return hgpr (fileName, false, false, true, false);
}

void
OCPI::Util::Http::HttpFsBase::close (std::ios * str)
  throw (std::string)
{
  OCPI::Util::Http::ClientStream * conn =
    dynamic_cast<OCPI::Util::Http::ClientStream *> (str);

  if (!conn) {
    throw std::string ("unrecognized stream");
  }

  try {
    conn->close ();
  }
  catch (OCPI::Util::Http::Redirection & redir) {
    std::string reason = "oops: unexpected redirection to \"";
    reason += redir.newLocation;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (OCPI::Util::Http::ClientError & error) {
    std::string reason = "client-side error: \"";
    reason += error.reasonPhrase;
    reason += "\"";
    delete conn;
    throw reason;
  }
  catch (OCPI::Util::Http::ServerError & error) {
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
OCPI::Util::Http::HttpFsBase::remove (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::Http::ClientStream * conn = hgpr (fileName, false, false, false, true);
  delete conn;
}

/*
 * ----------------------------------------------------------------------
 * Test whether a file name is valid. Throw an exception if not.
 * ----------------------------------------------------------------------
 */

void
OCPI::Util::Http::HttpFsBase::testFilenameForValidity (const std::string & name) const
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
OCPI::Util::Http::HttpFsBase::nativeFilename (const std::string & fileName) const
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_lock);
  std::string an = absoluteNameLocked (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += OCPI::Util::Uri::encode (an, "/");
  return uri;
}
