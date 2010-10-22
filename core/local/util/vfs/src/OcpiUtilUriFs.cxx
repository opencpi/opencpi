
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

#include <OcpiUtilUriFs.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilUri.h>
#include <OcpiOsAssert.h>
#include <OcpiOsRWLock.h>
#include <OcpiUtilAutoRDLock.h>
#include <OcpiUtilAutoWRLock.h>
#include <iostream>
#include <string>
#include <ctime>
#include <map>
#include <vector>

/*
 * ----------------------------------------------------------------------
 *
 * An "URI" file system in which the file names are URI components.
 *
 * ----------------------------------------------------------------------
 */

OCPI::Util::Vfs::UriFs::UriFs ()
  throw ()
{
  m_cwd = "/";
}

OCPI::Util::Vfs::UriFs::~UriFs ()
  throw ()
{
  /*
   * There mustn't be any open files or iterators
   */

  ocpiAssert (m_openFiles.size() == 0);
  ocpiAssert (m_openIterators.size() == 0);

  /*
   * Delete all adopted file systems
   */

  for (MountPoints::iterator it = m_mountPoints.begin();
       it != m_mountPoints.end(); it++) {
    if ((*it).adopted) {
      delete (*it).fs;
    }
  }
}

/*
 * ----------------------------------------------------------------------
 * Mounting File Systems
 * ----------------------------------------------------------------------
 */

void
OCPI::Util::Vfs::UriFs::mount (OCPI::Util::Vfs::Vfs * fs, bool adopt)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  std::string baseURI = fs->baseURI ();

  MountPoint mp;
  mp.adopted = adopt;
  mp.baseURI = baseURI;
  mp.fs = fs;
  m_mountPoints.push_back (mp);
}

void
OCPI::Util::Vfs::UriFs::unmount (OCPI::Util::Vfs::Vfs * fs)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);

  std::string baseURI = fs->baseURI ();
  std::string mountPoint = URIToName (baseURI);

  MountPoints::iterator it;

  for (it = m_mountPoints.begin(); it != m_mountPoints.end(); it++) {
    if ((*it).fs == fs) {
      break;
    }
  }

  if (it == m_mountPoints.end()) {
    throw std::string ("no such mount point");
  }

  /*
   * Make sure that there are no open files or iterators
   */

  for (OpenIterators::iterator oiit = m_openIterators.begin();
       oiit != m_openIterators.end(); oiit++) {
    if ((*oiit).second == fs) {
      throw std::string ("file system in use");
    }
  }

  for (OpenFiles::iterator ofit = m_openFiles.begin();
       ofit != m_openFiles.end(); ofit++) {
    if ((*ofit).second == fs) {
      throw std::string ("file system in use");
    }
  }

  m_mountPoints.erase (it);
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Vfs::UriFs::absoluteNameLocked (const std::string & name) const
  throw (std::string)
{
  return OCPI::Util::Vfs::joinNames (m_cwd, name);
}

/*
 * ----------------------------------------------------------------------
 * File Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Vfs::UriFs::baseURI () const
  throw ()
{
  return std::string();
}

std::string
OCPI::Util::Vfs::UriFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  return absoluteNameToURI (absName);
}

std::string
OCPI::Util::Vfs::UriFs::absoluteNameToURI (const std::string & absName) const
  throw (std::string)
{
  if (absName.length() == 0 || absName[0] != '/') {
    throw std::string ("invalid file name");
  }

  if (absName.length() == 1) {
    return std::string ();
  }

  std::string::size_type pos = 1;
  std::string::size_type nextSlash = absName.find ('/', 1);
  std::string result;

  if (nextSlash == std::string::npos) {
    result = absName.substr (1);
    result += "://";
    return result;
  }

  result = absName.substr (1, nextSlash-1);
  result += ":/";

  pos = nextSlash + 1;

  do {
    std::string pathComponent;

    if ((nextSlash = absName.find ('/', pos)) == std::string::npos) {
      pathComponent = absName.substr (pos);
      pos = std::string::npos;
    }
    else {
      pathComponent = absName.substr (pos, nextSlash-pos);
      pos = nextSlash + 1;
    }

    result += '/';
    result += OCPI::Util::Uri::encode (pathComponent, ":");
  }
  while (pos != std::string::npos);

  return result;
}

std::string
OCPI::Util::Vfs::UriFs::URIToName (const std::string & uriName) const
  throw (std::string)
{
  if (uriName.length() == 0) {
    return "/";
  }

  OCPI::Util::Uri uri (uriName);
  std::string result = "/";
  result += uri.getScheme ();
  result += "/";
  result += OCPI::Util::Uri::decode (uri.getAuthority ());
  result += OCPI::Util::Uri::decode (uri.getPath ());

  /*
   * Make sure that we know of a file system that can handle this URI.
   * findFs throws an exception if we don't.
   */

  {
    OCPI::Util::AutoRDLock lock (m_lock);
    std::string localName;
    findFs (result, localName);
  }

  return result;
}

/*
 * ----------------------------------------------------------------------
 * Directory Management
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Vfs::UriFs::cwd () const
  throw (std::string)
{
  OCPI::Util::Vfs::UriFs * me = const_cast<OCPI::Util::Vfs::UriFs *> (this);
  OCPI::Util::AutoRDLock lock (me->m_lock);
  return m_cwd;
}

void
OCPI::Util::Vfs::UriFs::cd (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  std::string absName = absoluteNameLocked (fileName);
  m_cwd = absName;
}

void
OCPI::Util::Vfs::UriFs::mkdir (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  mp->mkdir (localName);
}

void
OCPI::Util::Vfs::UriFs::rmdir (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  mp->rmdir (localName);
}

/*
 * Directory Listing
 */

OCPI::Util::Vfs::Iterator *
OCPI::Util::Vfs::UriFs::list (const std::string & dir,
                             const std::string & pattern)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (dir, localName);
  OCPI::Util::Vfs::Iterator * it = mp->list (localName, pattern);
  m_openIterators[it] = mp;
  return it;
}

void
OCPI::Util::Vfs::UriFs::closeIterator (OCPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  OpenIterators::iterator openIt = m_openIterators.find (it);

  if (openIt == m_openIterators.end()) {
    throw std::string ("invalid iterator");
  }

  OCPI::Util::Vfs::Vfs * mp = (*openIt).second;
  m_openIterators.erase (openIt);
  mp->closeIterator (it);
}

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::Vfs::UriFs::exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  return mp->exists (localName, isDir);
}

unsigned long long
OCPI::Util::Vfs::UriFs::size (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  return mp->size (localName);
}

std::time_t
OCPI::Util::Vfs::UriFs::lastModified (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  return mp->lastModified (localName);
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
OCPI::Util::Vfs::UriFs::open (const std::string & fileName, std::ios_base::openmode mode)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  std::iostream * res = mp->open (localName, mode);
  m_openFiles[res] = mp;
  return res;
}

std::istream *
OCPI::Util::Vfs::UriFs::openReadonly (const std::string & fileName, std::ios_base::openmode mode)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  std::istream * res = mp->openReadonly (localName, mode);
  m_openFiles[res] = mp;
  return res;
}

std::ostream *
OCPI::Util::Vfs::UriFs::openWriteonly (const std::string & fileName, std::ios_base::openmode mode)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  std::ostream * res = mp->openWriteonly (localName, mode);
  m_openFiles[res] = mp;
  return res;
}

void
OCPI::Util::Vfs::UriFs::close (std::ios * str)
  throw (std::string)
{
  OCPI::Util::AutoWRLock lock (m_lock);
  OpenFiles::iterator ofit = m_openFiles.find (str);

  if (ofit == m_openFiles.end()) {
    throw std::string ("invalid stream");
  }

  OCPI::Util::Vfs::Vfs * mp = (*ofit).second;
  m_openFiles.erase (ofit);
  mp->close (str);
}

/*
 * ----------------------------------------------------------------------
 * File System Operations
 * ----------------------------------------------------------------------
 */

void
OCPI::Util::Vfs::UriFs::copy (const std::string & oldName,
                             OCPI::Util::Vfs::Vfs * destFs,
                             const std::string & newName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);

  OCPI::Util::Vfs::UriFs * otherFs =
    dynamic_cast<OCPI::Util::Vfs::UriFs *> (destFs);

  std::string oldLocalName;
  OCPI::Util::Vfs::Vfs * oldFs = findFs (oldName, oldLocalName);

  if (!otherFs) {
    oldFs->copy (oldLocalName, destFs, newName);
    return;
  }

  std::string newLocalName;
  OCPI::Util::Vfs::Vfs * newFs = otherFs->findFs (newName, newLocalName);
  oldFs->copy (oldLocalName, newFs, newLocalName);
}

void
OCPI::Util::Vfs::UriFs::move (const std::string & oldName,
                             OCPI::Util::Vfs::Vfs * destFs,
                             const std::string & newName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);

  OCPI::Util::Vfs::UriFs * otherFs =
    dynamic_cast<OCPI::Util::Vfs::UriFs *> (destFs);

  std::string oldLocalName;
  OCPI::Util::Vfs::Vfs * oldFs = findFs (oldName, oldLocalName);

  if (!otherFs) {
    oldFs->move (oldLocalName, destFs, newName);
    return;
  }

  std::string newLocalName;
  OCPI::Util::Vfs::Vfs * newFs = otherFs->findFs (newName, newLocalName);
  oldFs->move (oldLocalName, newFs, newLocalName);
}

void
OCPI::Util::Vfs::UriFs::remove (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  OCPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  mp->remove (localName);
}

/*
 * ----------------------------------------------------------------------
 * Find the file system that is responsible for some file.
 * ----------------------------------------------------------------------
 */

OCPI::Util::Vfs::Vfs *
OCPI::Util::Vfs::UriFs::findFs (const std::string & fileName,
                               std::string & localName) const
  throw (std::string)
{
  std::string absName = absoluteNameLocked (fileName);
  std::string fileUri = absoluteNameToURI (absName);

  /*
   * Look for a mount point that knows how to handle this URI.
   *
   * This is done to allow mounted file systems to handle multiple
   * "base" URIs. For example, FileFs allows any alias of the local
   * host name in the authority component.
   */

  bool good = false;

  for (MountPoints::const_iterator it = m_mountPoints.begin();
       it != m_mountPoints.end(); it++) {
    try {
      localName = (*it).fs->URIToName (fileUri);
      good = true;
    }
    catch (...) {
    }

    if (good) {
      return (*it).fs;
    }
  }

  throw std::string ("no file system to handle file");
  return 0;
}
