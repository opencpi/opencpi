
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

#include <OcpiUtilStaticMemFs.h>
#include <OcpiUtilStaticMemFile.h>
#include <OcpiUtilAutoRDLock.h>
#include <OcpiUtilAutoWRLock.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilVfsIterator.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiOsRWLock.h>
#include <iostream>
#include <map>
#include <set>

/*
 * ----------------------------------------------------------------------
 * Iterator object for directory listings
 * ----------------------------------------------------------------------
 */

#if 0
namespace {

  class StaticMemFsIterator : public OCPI::Util::Vfs::Iterator {
  public:
    StaticMemFsIterator (const std::string & dir,
                         const std::string & pattern,
                         const OCPI::Util::MemFs::StaticMemFs::FileList & contents)
      throw ();
    ~StaticMemFsIterator ()
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
    bool findFirstMatching ()
      throw (std::string);

  protected:
    bool m_match;
    std::string m_dir;
    std::string m_absPatDir;
    std::string m_relPat;
    std::set<std::string> m_seenDirectories;
    const OCPI::Util::MemFs::StaticMemFs::FileList & m_contents;
    OCPI::Util::MemFs::StaticMemFs::FileList::const_iterator m_iterator;
  };

}

StaticMemFsIterator::StaticMemFsIterator (const std::string & dir,
                                          const std::string & pattern,
                                          const OCPI::Util::MemFs::StaticMemFs::FileList & contents)
  throw ()
  : m_dir (dir),
    m_contents (contents)
{
  std::string absPat = OCPI::Util::Vfs::joinNames (dir, pattern);
  m_absPatDir = OCPI::Util::Vfs::directoryName (absPat);
  m_relPat = OCPI::Util::Vfs::relativeName (absPat);
  m_iterator = m_contents.begin ();
  m_match = false;
}

StaticMemFsIterator::~StaticMemFsIterator ()
  throw ()
{
}

bool
StaticMemFsIterator::end ()
  throw (std::string)
{
  if (m_match) {
    return false;
  }
  return !(m_match = findFirstMatching ());
}

bool
StaticMemFsIterator::next ()
  throw (std::string)
{
  if (m_iterator == m_contents.end()) {
    return false;
  }
  m_iterator++;
  return (m_match = findFirstMatching ());
}

std::string
StaticMemFsIterator::relativeName ()
  throw (std::string)
{
  /*
   * Truncate m_dir from the absolute name
   */

  ocpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;

  std::string::size_type dirLen = m_dir.length();
  std::string::size_type absDirLen = m_absPatDir.length ();

  ocpiAssert (absFileName.length() > absDirLen);
  ocpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  ocpiAssert (absDirLen == 1 || absFileName[absDirLen] == '/');

  std::string::size_type firstPos = (dirLen>1) ? dirLen+1 : 1;
  std::string::size_type firstCharInTailPos = (absDirLen>1) ? absDirLen+1 : 1;
  std::string::size_type nextSlash =
    absFileName.find ('/', firstCharInTailPos);

  if (nextSlash == std::string::npos) {
    return absFileName.substr (firstPos);
  }

  return absFileName.substr (firstPos, nextSlash - firstPos);
}

std::string
StaticMemFsIterator::absoluteName ()
  throw (std::string)
{
  ocpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;
  std::string::size_type absDirLen = m_absPatDir.length ();

  ocpiAssert (absFileName.length() > absDirLen);
  ocpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  ocpiAssert (absDirLen == 1 || absFileName[absDirLen] == '/');

  std::string::size_type firstCharInTailPos = (absDirLen>1) ? absDirLen+1 : 1;
  std::string::size_type nextSlash =
    absFileName.find ('/', firstCharInTailPos);

  if (nextSlash != std::string::npos) {
    return absFileName.substr (0, nextSlash);
  }

  return absFileName;
}

bool
StaticMemFsIterator::isDirectory ()
  throw (std::string)
{
  ocpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;

  std::string::size_type absDirLen = m_absPatDir.length();

  ocpiAssert (absFileName.length() > absDirLen);
  ocpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  ocpiAssert (absDirLen == 1 || absFileName[absDirLen] == '/');

  std::string::size_type nextSlash =
    absFileName.find ('/', (absDirLen>1) ? absDirLen+1 : 1);
  return ((nextSlash == std::string::npos) ? false : true);
}

unsigned long long
StaticMemFsIterator::size ()
  throw (std::string)
{
  ocpiAssert (m_iterator != m_contents.end());
  return ((*m_iterator).second.file->size());
}

std::time_t
StaticMemFsIterator::lastModified ()
  throw (std::string)
{
  ocpiAssert (m_iterator != m_contents.end());
  return ((*m_iterator).second.file->lastModified());
}

bool
StaticMemFsIterator::findFirstMatching ()
  throw (std::string)
{
  /*
   * Look for an element in the contents, whose prefix maches m_absPatDir,
   * and whose next path component matches m_pattern.
   */

  std::string::size_type pdl = m_absPatDir.length ();
  std::string::size_type firstFnPos;

  if (pdl == 1) {
    firstFnPos = 1;
  }
  else {
    firstFnPos = pdl + 1;
  }

  while (m_iterator != m_contents.end()) {
    const std::string & absFileName = (*m_iterator).first;

    if (absFileName.length() >= firstFnPos &&
        (pdl == 1 || absFileName[pdl] == '/') &&
        absFileName.compare (0, pdl, m_absPatDir) == 0) {
      std::string::size_type nextSlash =
        absFileName.find ('/', firstFnPos);
      std::string nextPathComponent;
      bool isDirectory;

      if (nextSlash == std::string::npos) {
        nextPathComponent = absFileName.substr (firstFnPos);
        isDirectory = false;
      }
      else {
        nextPathComponent =
          absFileName.substr (firstFnPos, nextSlash-firstFnPos);
        isDirectory = true;
      }

      if (OCPI::Util::Misc::glob (nextPathComponent, m_relPat)) {
        if (isDirectory) {
          if (m_seenDirectories.find (nextPathComponent) == m_seenDirectories.end()) {
            m_seenDirectories.insert (nextPathComponent);
            break;
          }
          else {
            // already seen this directory, do not break but continue
          }
        }
        else {
          // regular file
          break;
        }
      }
    }

    m_iterator++;
  }

  return ((m_iterator != m_contents.end()) ? true : false);
}
#endif
/*
 * ----------------------------------------------------------------------
 * StaticMemFs
 * ----------------------------------------------------------------------
 */

OCPI::Util::MemFs::StaticMemFs::StaticMemFs ()
  throw ()
{
  m_cwd = "/";
  m_baseURI = "static:///";
}

OCPI::Util::MemFs::StaticMemFs::~StaticMemFs ()
  throw ()
{
  for (FileList::iterator it = m_contents.begin();
       it != m_contents.end(); it++) {
    if ((*it).second.adopted) {
      delete (*it).second.file;
    }
  }
}

void
OCPI::Util::MemFs::StaticMemFs::mount (const std::string & fileName,
                                      StaticMemFile * file,
                                      bool adopt)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);

  std::string absName = absoluteNameLocked (fileName);
  FileList::iterator it = m_contents.find (absName);

  if (it != m_contents.end()) {
    throw std::string ("already mounted");
  }
  else {
    INode & node = m_contents[absName];
    node.adopted = adopt;
    node.file = file;
  }
}

/*
 * ----------------------------------------------------------------------
 * Fle Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::MemFs::StaticMemFs::baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
OCPI::Util::MemFs::StaticMemFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  testFilenameForValidity (fileName);
  std::string an = absoluteName (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += OCPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
OCPI::Util::MemFs::StaticMemFs::URIToName (const std::string & uri) const
  throw (std::string)
{
  if (uri.length() < m_baseURI.length() ||
      uri.compare (0, m_baseURI.length(), m_baseURI) != 0) {
    throw std::string ("URI not understood by this file system");
  }

  std::string eap = uri.substr (m_baseURI.length() - 1);
  return OCPI::Util::Uri::decode (eap);
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::MemFs::StaticMemFs::absoluteNameLocked (const std::string & name) const
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
OCPI::Util::MemFs::StaticMemFs::cwd () const
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);
  return m_cwd;
}

void
OCPI::Util::MemFs::StaticMemFs::cd (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = absoluteNameLocked (fileName);

  /*
   * Regular file?
   */

  if (m_contents.find (nn) != m_contents.end ()) {
    std::string reason = "cannot change cwd to \"";
    reason += fileName;
    reason += "\": file exists as a plain file";
    throw reason;
  }

  m_cwd = nn;
}

void
OCPI::Util::MemFs::StaticMemFs::mkdir (const std::string &)
  throw (std::string)
{
  throw std::string ("mkdir not supported on StaticMemFs");
}

void
OCPI::Util::MemFs::StaticMemFs::rmdir (const std::string &)
  throw (std::string)
{
  throw std::string ("rmdir not supported on StaticMemFs");
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */
#if 0

OCPI::Util::Vfs::Iterator *
OCPI::Util::MemFs::StaticMemFs::list (const std::string & dir,
				      const std::string & pattern, 
				      bool /* recursive */)
  throw (std::string)
{
  m_lock.rdLock ();
  testFilenameForValidity (pattern);
  std::string absDir = absoluteNameLocked (dir);
  return new StaticMemFsIterator (absDir, pattern, m_contents);
}

void
OCPI::Util::MemFs::StaticMemFs::closeIterator (OCPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  StaticMemFsIterator * smfi = dynamic_cast<StaticMemFsIterator *> (it);

  if (!smfi) {
    throw std::string ("invalid iterator");
  }

  delete smfi;
  m_lock.rdUnlock ();
}
#endif

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::MemFs::StaticMemFs::exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = absoluteNameLocked (fileName);

  /*
   * See if there is a file by this name
   */

  if (m_contents.find (nn) != m_contents.end ()) {
    if (isDir) {
      *isDir = false;
    }

    return true;
  }

  /*
   * Browse the file list, and see if there is a file with this prefix
   */

  FileList::iterator it;
  std::string::size_type nnlen = nn.length();

  for (it = m_contents.begin(); it != m_contents.end(); it++) {
    if ((*it).first.length () > nnlen &&
        (*it).first.compare (0, nnlen, nn) == 0 &&
        (*it).first[nnlen] == '/') {
      if (isDir) {
        *isDir = true;
      }

      return true;
    }
  }

  return false;
}

unsigned long long
OCPI::Util::MemFs::StaticMemFs::size (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = absoluteNameLocked (fileName);

  FileList::iterator it = m_contents.find (nn);

  if (it == m_contents.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.file->size ();
}

time_t
OCPI::Util::MemFs::StaticMemFs::lastModified (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = absoluteNameLocked (fileName);

  FileList::iterator it = m_contents.find (nn);

  if (it == m_contents.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.file->lastModified ();
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
OCPI::Util::MemFs::StaticMemFs::open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported");
  return 0;
}

std::istream *
OCPI::Util::MemFs::StaticMemFs::openReadonly (const std::string & fileName, std::ios_base::openmode)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = absoluteNameLocked (fileName);

  FileList::iterator it = m_contents.find (nn);

  if (it == m_contents.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.file->openReadonly ();
}

std::ostream *
OCPI::Util::MemFs::StaticMemFs::openWriteonly (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported");
  return 0;
}

void
OCPI::Util::MemFs::StaticMemFs::close (std::ios * str)
  throw (std::string)
{
  delete str;
}

void
OCPI::Util::MemFs::StaticMemFs::remove (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported");
}

/*
 * ----------------------------------------------------------------------
 * Test whether a file name is valid. Throw an exception if not.
 * ----------------------------------------------------------------------
 */

void
OCPI::Util::MemFs::StaticMemFs::
testFilenameForValidity (const std::string & name)
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
