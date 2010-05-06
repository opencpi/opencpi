#include <CpiUtilStaticMemFs.h>
#include <CpiUtilStaticMemFile.h>
#include <CpiUtilAutoRDLock.h>
#include <CpiUtilAutoWRLock.h>
#include <CpiUtilVfs.h>
#include <CpiUtilVfsIterator.h>
#include <CpiUtilUri.h>
#include <CpiUtilMisc.h>
#include <CpiOsAssert.h>
#include <CpiOsRWLock.h>
#include <iostream>
#include <map>
#include <set>

/*
 * ----------------------------------------------------------------------
 * Iterator object for directory listings
 * ----------------------------------------------------------------------
 */

namespace {

  class StaticMemFsIterator : public CPI::Util::Vfs::Iterator {
  public:
    StaticMemFsIterator (const std::string & dir,
                         const std::string & pattern,
                         const CPI::Util::MemFs::StaticMemFs::FileList & contents)
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
    const CPI::Util::MemFs::StaticMemFs::FileList & m_contents;
    CPI::Util::MemFs::StaticMemFs::FileList::const_iterator m_iterator;
  };

}

StaticMemFsIterator::StaticMemFsIterator (const std::string & dir,
                                          const std::string & pattern,
                                          const CPI::Util::MemFs::StaticMemFs::FileList & contents)
  throw ()
  : m_dir (dir),
    m_contents (contents)
{
  std::string absPat = CPI::Util::Vfs::joinNames (dir, pattern);
  m_absPatDir = CPI::Util::Vfs::directoryName (absPat);
  m_relPat = CPI::Util::Vfs::relativeName (absPat);
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

  cpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;

  std::string::size_type dirLen = m_dir.length();
  std::string::size_type absDirLen = m_absPatDir.length ();

  cpiAssert (absFileName.length() > absDirLen);
  cpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  cpiAssert (absDirLen == 1 || absFileName[absDirLen] == '/');

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
  cpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;
  std::string::size_type absDirLen = m_absPatDir.length ();

  cpiAssert (absFileName.length() > absDirLen);
  cpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  cpiAssert (absDirLen == 1 || absFileName[absDirLen] == '/');

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
  cpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;

  std::string::size_type absDirLen = m_absPatDir.length();

  cpiAssert (absFileName.length() > absDirLen);
  cpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  cpiAssert (absDirLen == 1 || absFileName[absDirLen] == '/');

  std::string::size_type nextSlash =
    absFileName.find ('/', (absDirLen>1) ? absDirLen+1 : 1);
  return ((nextSlash == std::string::npos) ? false : true);
}

unsigned long long
StaticMemFsIterator::size ()
  throw (std::string)
{
  cpiAssert (m_iterator != m_contents.end());
  return ((*m_iterator).second.file->size());
}

std::time_t
StaticMemFsIterator::lastModified ()
  throw (std::string)
{
  cpiAssert (m_iterator != m_contents.end());
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

      if (CPI::Util::Misc::glob (nextPathComponent, m_relPat)) {
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

/*
 * ----------------------------------------------------------------------
 * StaticMemFs
 * ----------------------------------------------------------------------
 */

CPI::Util::MemFs::StaticMemFs::StaticMemFs ()
  throw ()
{
  m_cwd = "/";
  m_baseURI = "static:///";
}

CPI::Util::MemFs::StaticMemFs::~StaticMemFs ()
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
CPI::Util::MemFs::StaticMemFs::mount (const std::string & fileName,
                                      StaticMemFile * file,
                                      bool adopt)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);

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
CPI::Util::MemFs::StaticMemFs::baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
CPI::Util::MemFs::StaticMemFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  testFilenameForValidity (fileName);
  std::string an = absoluteName (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += CPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
CPI::Util::MemFs::StaticMemFs::URIToName (const std::string & uri) const
  throw (std::string)
{
  if (uri.length() < m_baseURI.length() ||
      uri.compare (0, m_baseURI.length(), m_baseURI) != 0) {
    throw std::string ("URI not understood by this file system");
  }

  std::string eap = uri.substr (m_baseURI.length() - 1);
  return CPI::Util::Uri::decode (eap);
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::MemFs::StaticMemFs::absoluteNameLocked (const std::string & name) const
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
CPI::Util::MemFs::StaticMemFs::cwd () const
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  return m_cwd;
}

void
CPI::Util::MemFs::StaticMemFs::cd (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);

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
CPI::Util::MemFs::StaticMemFs::mkdir (const std::string &)
  throw (std::string)
{
  throw std::string ("mkdir not supported on StaticMemFs");
}

void
CPI::Util::MemFs::StaticMemFs::rmdir (const std::string &)
  throw (std::string)
{
  throw std::string ("rmdir not supported on StaticMemFs");
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Iterator *
CPI::Util::MemFs::StaticMemFs::list (const std::string & dir,
                                     const std::string & pattern)
  throw (std::string)
{
  m_lock.rdLock ();
  testFilenameForValidity (pattern);
  std::string absDir = absoluteNameLocked (dir);
  return new StaticMemFsIterator (absDir, pattern, m_contents);
}

void
CPI::Util::MemFs::StaticMemFs::closeIterator (CPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  StaticMemFsIterator * smfi = dynamic_cast<StaticMemFsIterator *> (it);

  if (!smfi) {
    throw std::string ("invalid iterator");
  }

  delete smfi;
  m_lock.rdUnlock ();
}

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::MemFs::StaticMemFs::exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

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
CPI::Util::MemFs::StaticMemFs::size (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = absoluteNameLocked (fileName);

  FileList::iterator it = m_contents.find (nn);

  if (it == m_contents.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.file->size ();
}

time_t
CPI::Util::MemFs::StaticMemFs::lastModified (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

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
CPI::Util::MemFs::StaticMemFs::open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported");
  return 0;
}

std::istream *
CPI::Util::MemFs::StaticMemFs::openReadonly (const std::string & fileName, std::ios_base::openmode)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = absoluteNameLocked (fileName);

  FileList::iterator it = m_contents.find (nn);

  if (it == m_contents.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.file->openReadonly ();
}

std::ostream *
CPI::Util::MemFs::StaticMemFs::openWriteonly (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported");
  return 0;
}

void
CPI::Util::MemFs::StaticMemFs::close (std::ios * str)
  throw (std::string)
{
  delete str;
}

void
CPI::Util::MemFs::StaticMemFs::remove (const std::string &)
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
CPI::Util::MemFs::StaticMemFs::
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
