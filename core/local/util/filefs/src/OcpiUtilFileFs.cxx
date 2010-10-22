
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

#include <OcpiUtilFileFs.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilVfsIterator.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilAutoMutex.h>
#include <OcpiOsAssert.h>
#include <OcpiOsFileSystem.h>
#include <OcpiOsFileIterator.h>
#include <OcpiOsMutex.h>
#include <OcpiOsMisc.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cctype>

/*
 * ----------------------------------------------------------------------
 * Iterator object for directory listings
 * ----------------------------------------------------------------------
 */

namespace {

  class FileFsIterator : public OCPI::Util::Vfs::Iterator {
  public:
    FileFsIterator (const std::string & root,
                    const std::string & dir,
                    const std::string & pattern)
      throw (std::string);
    ~FileFsIterator ()
      throw ();

    bool end ()
      throw (std::string);

    bool next ()
      throw (std::string);

    std::string relativeName ()
      throw (std::string);
    std::string absoluteName ()
      throw (std::string);

    bool isDirectory ()
      throw (std::string);
    unsigned long long size ()
      throw (std::string);
    std::time_t lastModified ()
      throw (std::string);

  protected:
    std::string m_root;
    OCPI::OS::FileIterator m_osIterator;
  };

}

FileFsIterator::FileFsIterator (const std::string & root,
                                const std::string & dir,
                                const std::string & pattern)
  throw (std::string)
  : m_root (root),
    m_osIterator (dir, pattern)
{
  ocpiAssert (m_root.length() > 0 && m_root[0] == '/');
  ocpiAssert (dir.length() >= m_root.length() &&
             dir.substr (0, m_root.length()) == m_root);
}

FileFsIterator::~FileFsIterator ()
  throw ()
{
  m_osIterator.close ();
}

bool
FileFsIterator::end ()
  throw (std::string)
{
  return m_osIterator.end ();
}

bool
FileFsIterator::next ()
  throw (std::string)
{
  return m_osIterator.next ();
}

std::string
FileFsIterator::relativeName ()
  throw (std::string)
{
  return m_osIterator.relativeName ();
}

std::string
FileFsIterator::absoluteName ()
  throw (std::string)
{
  /*
   * Must truncate our "root" directory
   */

  std::string fullName = m_osIterator.absoluteName ();

  if (m_root.length() > 1) {
    ocpiAssert (fullName.length() >= m_root.length() &&
               fullName.substr (0, m_root.length()) == m_root);

    fullName = fullName.substr (m_root.length());
  }

  return fullName;
}

bool
FileFsIterator::isDirectory ()
  throw (std::string)
{
  return m_osIterator.isDirectory ();
}

unsigned long long
FileFsIterator::size ()
  throw (std::string)
{
  return m_osIterator.size ();
}

std::time_t
FileFsIterator::lastModified ()
  throw (std::string)
{
  return m_osIterator.lastModified ();
}

/*
 * ----------------------------------------------------------------------
 * Constructor and Destructor
 * ----------------------------------------------------------------------
 */

OCPI::Util::FileFs::FileFs::FileFs (const std::string & root)
  throw (std::string)
  : m_root (root),
    m_cwd ("/")
{
  /*
   * Our "root" must be an absolute directory name.
   */

  testFilenameForValidity (root);

  if (!root.length() || root[0] != '/') {
    throw std::string ("root must be absolute");
  }

  /*
   * Make sure that our root exists, and is a directory.
   */

  bool isDir;

  if (!OCPI::OS::FileSystem::exists (m_root, &isDir)) {
    throw std::string ("root directory does not exist");
  }
  else if (!isDir) {
    throw std::string ("root is not a directory");
  }

  /*
   * Compose base URI.
   */
  
  m_baseURI = "file://";
  m_baseURI += OCPI::OS::getHostname ();
  m_baseURI += OCPI::Util::Uri::encode (m_root, ":/");

  if (m_root != "/") {
    m_baseURI += "/";
  }
}

OCPI::Util::FileFs::FileFs::~FileFs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name Path Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::FileFs::FileFs::
nameToPath (const std::string & fileName) const
  throw (std::string)
{
  return nativeFilename (fileName);
}

std::string
OCPI::Util::FileFs::FileFs::
pathToName (const std::string & pathName) const
  throw (std::string)
{
  std::string absPath = OCPI::OS::FileSystem::absoluteName (pathName);

  if (m_root.length() == 1) {
    return absPath;
  }

  if (m_root.length() > 2 && absPath.length() > 2 &&
      m_root[2] == ':' && absPath[2] == ':' &&
      std::tolower (m_root[1]) == std::tolower (absPath[1])) {
    /*
     * Windows path names; the above compares the drive letter case
     * insensitively, the next statement compares the rest of the
     * path.
     */

    if (absPath.length() <= m_root.length() ||
        absPath.substr (3, m_root.length() - 3) != m_root.substr (3) ||
        absPath[m_root.length()] != '/') {
      throw std::string ("file name points outside this file system");
    }
  }
  else {
    if (absPath.length() <= m_root.length() ||
        absPath.substr (0, m_root.length()) != m_root ||
        absPath[m_root.length()] != '/') {
      throw std::string ("file name points outside this file system");
    }
  }

  return absPath.substr (m_root.length());
}

std::string
OCPI::Util::FileFs::FileFs::
toNativeName (const std::string & fileName) const
  throw (std::string)
{
  std::string fullPath = nativeFilename (fileName);
  return OCPI::OS::FileSystem::toNativeName (fullPath);
}

std::string
OCPI::Util::FileFs::FileFs::
fromNativeName (const std::string & nativeName) const
  throw (std::string)
{
  std::string path = OCPI::OS::FileSystem::fromNativeName (nativeName);
  std::string absPath = OCPI::OS::FileSystem::absoluteName (path);
  return pathToName (absPath);
}

/*
 * ----------------------------------------------------------------------
 * File Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::FileFs::FileFs::baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
OCPI::Util::FileFs::FileFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_lock);
  testFilenameForValidity (fileName);
  std::string an = absoluteNameLocked (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += OCPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
OCPI::Util::FileFs::FileFs::URIToName (const std::string & struri) const
  throw (std::string)
{
  OCPI::Util::Uri uri (struri);

  if (uri.getScheme() != "file") {
    std::string reason = "file system does not support \"";
    reason += uri.getScheme ();
    reason += "\" URIs";
    throw reason;
  }

  /*
   * Accept an empty authority as "localhost".
   */

  std::string drivePrefix;

  if (uri.getAuthority().length()) {
    std::string authority = uri.getAuthority ();
    /*
     * For compatibility with other Windows applications, accept a drive
     * name, i.e., a letter followed by a colon.
     */

    if (authority.length() == 2 &&
        OCPI::Util::Uri::isalpha (authority[0]) &&
        authority[1] == ':') {
      drivePrefix = authority;
      drivePrefix[0] = std::tolower (drivePrefix[0]);
    }
    else {
      /*
       * The Authority must be a local host name.
       */

      if (!OCPI::OS::isLocalhost (authority)) {
        std::string reason = "authority \"";
        reason += authority;
        reason += "\" does not look like a local host name";
        throw reason;
      }
    }
  }

  std::string fullPath;

  if (drivePrefix.length()) {
    fullPath = "/";
    fullPath += drivePrefix;
  }

  fullPath += OCPI::Util::Uri::decode (uri.getPath ());
  return pathToName (fullPath);
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::FileFs::FileFs::absoluteNameLocked (const std::string & name) const
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
OCPI::Util::FileFs::FileFs::cwd () const
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_lock);
  return m_cwd;
}

void
OCPI::Util::FileFs::FileFs::cd (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  bool isDir, exists;

  exists = OCPI::OS::FileSystem::exists (absName, &isDir);

  if (!exists) {
    throw std::string ("name does not exist");
  }
  else if (!isDir) {
    throw std::string ("not a directory");
  }

  OCPI::Util::AutoMutex lock (m_lock);
  m_cwd = OCPI::Util::Vfs::joinNames (m_cwd, name);
}

void
OCPI::Util::FileFs::FileFs::mkdir (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  OCPI::OS::FileSystem::mkdir (absName);
}

void
OCPI::Util::FileFs::FileFs::rmdir (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  OCPI::OS::FileSystem::rmdir (absName);
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

OCPI::Util::Vfs::Iterator *
OCPI::Util::FileFs::FileFs::list (const std::string & dir,
                                 const std::string & pattern)
  throw (std::string)
{
  testFilenameForValidity (dir);
  testFilenameForValidity (pattern);
  std::string absName = nativeFilename (dir);
  return new FileFsIterator (m_root, absName, pattern);
}

void
OCPI::Util::FileFs::FileFs::closeIterator (OCPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  FileFsIterator * ffi = dynamic_cast<FileFsIterator *> (it);

  if (!ffi) {
    throw std::string ("invalid iterator");
  }

  delete ffi;
}

/*
 * ----------------------------------------------------------------------
 * File Information
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::FileFs::FileFs::exists (const std::string & name, bool * isDir)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  return OCPI::OS::FileSystem::exists (absName, isDir);
}

unsigned long long
OCPI::Util::FileFs::FileFs::size (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  return OCPI::OS::FileSystem::size (absName);
}

std::time_t
OCPI::Util::FileFs::FileFs::lastModified (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  return OCPI::OS::FileSystem::lastModified (absName);
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
OCPI::Util::FileFs::FileFs::open (const std::string & name,
                                 std::ios_base::openmode mode)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  std::string nativeName = OCPI::OS::FileSystem::toNativeName (absName);

  mode |= std::ios_base::in | std::ios_base::out;

  std::fstream * fs = new std::fstream (nativeName.c_str(), mode);

  if (!fs->good()) {
    delete fs;
    std::string reason = "cannot open file \"";
    reason += name;
    reason += "\" for r/w";
    throw reason;
  }

  return fs;
}

std::istream *
OCPI::Util::FileFs::FileFs::openReadonly (const std::string & name,
                                         std::ios_base::openmode mode)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  std::string nativeName = OCPI::OS::FileSystem::toNativeName (absName);

  std::ifstream * is = new std::ifstream (nativeName.c_str(), mode);

  if (!is->good()) {
    delete is;
    std::string reason = "cannot open file \"";
    reason += name;
    reason += "\" for reading";
    throw reason;
  }

  return is;
}

std::ostream *
OCPI::Util::FileFs::FileFs::openWriteonly (const std::string & name,
                                          std::ios_base::openmode mode)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  std::string nativeName = OCPI::OS::FileSystem::toNativeName (absName);

  std::ofstream * os = new std::ofstream (nativeName.c_str(), mode);

  if (!os->good()) {
    delete os;
    std::string reason = "cannot open file \"";
    reason += name;
    reason += "\" for writing";
    throw reason;
  }

  return os;
}

void
OCPI::Util::FileFs::FileFs::close (std::ios * str)
  throw (std::string)
{
  if (dynamic_cast<std::fstream *> (str) == 0 &&
      dynamic_cast<std::ifstream *> (str) == 0 &&
      dynamic_cast<std::ofstream *> (str) == 0) {
    throw std::string ("invalid stream");
  }

  std::ofstream * fstr = dynamic_cast<std::ofstream *> (str);

  if (fstr) {
    fstr->close ();

    bool good = str->good();
    delete fstr;

    if (!good) {
      throw std::string ("error closing file");
    }
  }
  else {
    delete str;
  }
}

/*
 * ----------------------------------------------------------------------
 * File System Operations
 * ----------------------------------------------------------------------
 */

void
OCPI::Util::FileFs::FileFs::move (const std::string & oldName,
                                 Vfs * destFs,
                                 const std::string & newName)
  throw (std::string)
{
  /*
   * See if the target filesystem is a FileFs. If yes, we can
   * use OCPI::OS::FileSystem::rename.
   */

  FileFs * destFileFs = dynamic_cast<FileFs *> (destFs);

  if (!destFileFs) {
    Vfs::move (oldName, destFs, newName);
    return;
  }

  testFilenameForValidity (oldName);
  testFilenameForValidity (newName);
  std::string oldAbsName = nativeFilename (oldName);
  std::string newAbsName = destFileFs->nativeFilename (newName);
  OCPI::OS::FileSystem::rename (oldAbsName, newAbsName);
}

void
OCPI::Util::FileFs::FileFs::rename (const std::string & oldName,
                                   const std::string & newName)
  throw (std::string)
{
  testFilenameForValidity (oldName);
  testFilenameForValidity (newName);
  std::string oldAbsName = nativeFilename (oldName);
  std::string newAbsName = nativeFilename (newName);
  OCPI::OS::FileSystem::rename (oldAbsName, newAbsName);
}

void
OCPI::Util::FileFs::FileFs::remove (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  OCPI::OS::FileSystem::remove (absName);
}

/*
 * ----------------------------------------------------------------------
 * File Name Validity Test
 * ----------------------------------------------------------------------
 */

/*
 * The file name must not contain any "." or ".." path components.
 */

void
OCPI::Util::FileFs::FileFs::testFilenameForValidity (const std::string & name)
  throw (std::string)
{
  if (!name.length()) {
    throw std::string ("empty file name");
  }

  if (name.length() == 1 && name[0] == '/') {
    /*
     * An exception for the name of the root directory
     */
    return;
  }

  if (name[name.length()-1] == '/') {
    /*
     * Special complaint about a name that ends with a slash
     */
    throw std::string ("file name may not end with a slash");
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

    if (!pathComponent.length()) {
      throw std::string ("invalid file name: empty path component");
    }

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

/*
 * ----------------------------------------------------------------------
 * Compute the "native" file name to use with OCPI::OS::FileSystem
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::FileFs::FileFs::nativeFilename (const std::string & name) const
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_lock);

  std::string absName = OCPI::Util::Vfs::joinNames (m_cwd, name);

  if (m_root.length() == 1 && m_root[0] == '/') {
    return absName;
  }

  if (absName.length() == 1 && absName[0] == '/') {
    return m_root;
  }

  std::string fullName = m_root;
  fullName += absName;
  return fullName;
}
