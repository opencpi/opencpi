#include <CpiUtilUriFs.h>
#include <CpiUtilVfs.h>
#include <CpiUtilUri.h>
#include <CpiOsAssert.h>
#include <CpiOsRWLock.h>
#include <CpiUtilAutoRDLock.h>
#include <CpiUtilAutoWRLock.h>
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

CPI::Util::Vfs::UriFs::UriFs ()
  throw ()
{
  m_cwd = "/";
}

CPI::Util::Vfs::UriFs::~UriFs ()
  throw ()
{
  /*
   * There mustn't be any open files or iterators
   */

  cpiAssert (m_openFiles.size() == 0);
  cpiAssert (m_openIterators.size() == 0);

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
CPI::Util::Vfs::UriFs::mount (CPI::Util::Vfs::Vfs * fs, bool adopt)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  std::string baseURI = fs->baseURI ();

  MountPoint mp;
  mp.adopted = adopt;
  mp.baseURI = baseURI;
  mp.fs = fs;
  m_mountPoints.push_back (mp);
}

void
CPI::Util::Vfs::UriFs::unmount (CPI::Util::Vfs::Vfs * fs)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);

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
CPI::Util::Vfs::UriFs::absoluteNameLocked (const std::string & name) const
  throw (std::string)
{
  return CPI::Util::Vfs::joinNames (m_cwd, name);
}

/*
 * ----------------------------------------------------------------------
 * File Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Vfs::UriFs::baseURI () const
  throw ()
{
  return std::string();
}

std::string
CPI::Util::Vfs::UriFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  return absoluteNameToURI (absName);
}

std::string
CPI::Util::Vfs::UriFs::absoluteNameToURI (const std::string & absName) const
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
    result += CPI::Util::Uri::encode (pathComponent, ":");
  }
  while (pos != std::string::npos);

  return result;
}

std::string
CPI::Util::Vfs::UriFs::URIToName (const std::string & uriName) const
  throw (std::string)
{
  if (uriName.length() == 0) {
    return "/";
  }

  CPI::Util::Uri uri (uriName);
  std::string result = "/";
  result += uri.getScheme ();
  result += "/";
  result += CPI::Util::Uri::decode (uri.getAuthority ());
  result += CPI::Util::Uri::decode (uri.getPath ());

  /*
   * Make sure that we know of a file system that can handle this URI.
   * findFs throws an exception if we don't.
   */

  {
    CPI::Util::AutoRDLock lock (m_lock);
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
CPI::Util::Vfs::UriFs::cwd () const
  throw (std::string)
{
  CPI::Util::Vfs::UriFs * me = const_cast<CPI::Util::Vfs::UriFs *> (this);
  CPI::Util::AutoRDLock lock (me->m_lock);
  return m_cwd;
}

void
CPI::Util::Vfs::UriFs::cd (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  std::string absName = absoluteNameLocked (fileName);
  m_cwd = absName;
}

void
CPI::Util::Vfs::UriFs::mkdir (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  mp->mkdir (localName);
}

void
CPI::Util::Vfs::UriFs::rmdir (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  mp->rmdir (localName);
}

/*
 * Directory Listing
 */

CPI::Util::Vfs::Iterator *
CPI::Util::Vfs::UriFs::list (const std::string & dir,
                             const std::string & pattern)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (dir, localName);
  CPI::Util::Vfs::Iterator * it = mp->list (localName, pattern);
  m_openIterators[it] = mp;
  return it;
}

void
CPI::Util::Vfs::UriFs::closeIterator (CPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  OpenIterators::iterator openIt = m_openIterators.find (it);

  if (openIt == m_openIterators.end()) {
    throw std::string ("invalid iterator");
  }

  CPI::Util::Vfs::Vfs * mp = (*openIt).second;
  m_openIterators.erase (openIt);
  mp->closeIterator (it);
}

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::Vfs::UriFs::exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  return mp->exists (localName, isDir);
}

unsigned long long
CPI::Util::Vfs::UriFs::size (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  return mp->size (localName);
}

std::time_t
CPI::Util::Vfs::UriFs::lastModified (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  return mp->lastModified (localName);
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
CPI::Util::Vfs::UriFs::open (const std::string & fileName, std::ios_base::openmode mode)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  std::iostream * res = mp->open (localName, mode);
  m_openFiles[res] = mp;
  return res;
}

std::istream *
CPI::Util::Vfs::UriFs::openReadonly (const std::string & fileName, std::ios_base::openmode mode)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  std::istream * res = mp->openReadonly (localName, mode);
  m_openFiles[res] = mp;
  return res;
}

std::ostream *
CPI::Util::Vfs::UriFs::openWriteonly (const std::string & fileName, std::ios_base::openmode mode)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  std::ostream * res = mp->openWriteonly (localName, mode);
  m_openFiles[res] = mp;
  return res;
}

void
CPI::Util::Vfs::UriFs::close (std::ios * str)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  OpenFiles::iterator ofit = m_openFiles.find (str);

  if (ofit == m_openFiles.end()) {
    throw std::string ("invalid stream");
  }

  CPI::Util::Vfs::Vfs * mp = (*ofit).second;
  m_openFiles.erase (ofit);
  mp->close (str);
}

/*
 * ----------------------------------------------------------------------
 * File System Operations
 * ----------------------------------------------------------------------
 */

void
CPI::Util::Vfs::UriFs::copy (const std::string & oldName,
                             CPI::Util::Vfs::Vfs * destFs,
                             const std::string & newName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  CPI::Util::Vfs::UriFs * otherFs =
    dynamic_cast<CPI::Util::Vfs::UriFs *> (destFs);

  std::string oldLocalName;
  CPI::Util::Vfs::Vfs * oldFs = findFs (oldName, oldLocalName);

  if (!otherFs) {
    oldFs->copy (oldLocalName, destFs, newName);
    return;
  }

  std::string newLocalName;
  CPI::Util::Vfs::Vfs * newFs = otherFs->findFs (newName, newLocalName);
  oldFs->copy (oldLocalName, newFs, newLocalName);
}

void
CPI::Util::Vfs::UriFs::move (const std::string & oldName,
                             CPI::Util::Vfs::Vfs * destFs,
                             const std::string & newName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  CPI::Util::Vfs::UriFs * otherFs =
    dynamic_cast<CPI::Util::Vfs::UriFs *> (destFs);

  std::string oldLocalName;
  CPI::Util::Vfs::Vfs * oldFs = findFs (oldName, oldLocalName);

  if (!otherFs) {
    oldFs->move (oldLocalName, destFs, newName);
    return;
  }

  std::string newLocalName;
  CPI::Util::Vfs::Vfs * newFs = otherFs->findFs (newName, newLocalName);
  oldFs->move (oldLocalName, newFs, newLocalName);
}

void
CPI::Util::Vfs::UriFs::remove (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  std::string localName;
  CPI::Util::Vfs::Vfs * mp = findFs (fileName, localName);
  mp->remove (localName);
}

/*
 * ----------------------------------------------------------------------
 * Find the file system that is responsible for some file.
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Vfs *
CPI::Util::Vfs::UriFs::findFs (const std::string & fileName,
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
