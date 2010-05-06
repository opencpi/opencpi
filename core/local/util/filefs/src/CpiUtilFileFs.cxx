#include <CpiUtilFileFs.h>
#include <CpiUtilVfs.h>
#include <CpiUtilVfsIterator.h>
#include <CpiUtilUri.h>
#include <CpiUtilAutoMutex.h>
#include <CpiOsAssert.h>
#include <CpiOsFileSystem.h>
#include <CpiOsFileIterator.h>
#include <CpiOsMutex.h>
#include <CpiOsMisc.h>
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

  class FileFsIterator : public CPI::Util::Vfs::Iterator {
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
    CPI::OS::FileIterator m_osIterator;
  };

}

FileFsIterator::FileFsIterator (const std::string & root,
                                const std::string & dir,
                                const std::string & pattern)
  throw (std::string)
  : m_root (root),
    m_osIterator (dir, pattern)
{
  cpiAssert (m_root.length() > 0 && m_root[0] == '/');
  cpiAssert (dir.length() >= m_root.length() &&
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
    cpiAssert (fullName.length() >= m_root.length() &&
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

CPI::Util::FileFs::FileFs::FileFs (const std::string & root)
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

  if (!CPI::OS::FileSystem::exists (m_root, &isDir)) {
    throw std::string ("root directory does not exist");
  }
  else if (!isDir) {
    throw std::string ("root is not a directory");
  }

  /*
   * Compose base URI.
   */
  
  m_baseURI = "file://";
  m_baseURI += CPI::OS::getHostname ();
  m_baseURI += CPI::Util::Uri::encode (m_root, ":/");

  if (m_root != "/") {
    m_baseURI += "/";
  }
}

CPI::Util::FileFs::FileFs::~FileFs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name Path Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::FileFs::FileFs::
nameToPath (const std::string & fileName) const
  throw (std::string)
{
  return nativeFilename (fileName);
}

std::string
CPI::Util::FileFs::FileFs::
pathToName (const std::string & pathName) const
  throw (std::string)
{
  std::string absPath = CPI::OS::FileSystem::absoluteName (pathName);

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
CPI::Util::FileFs::FileFs::
toNativeName (const std::string & fileName) const
  throw (std::string)
{
  std::string fullPath = nativeFilename (fileName);
  return CPI::OS::FileSystem::toNativeName (fullPath);
}

std::string
CPI::Util::FileFs::FileFs::
fromNativeName (const std::string & nativeName) const
  throw (std::string)
{
  std::string path = CPI::OS::FileSystem::fromNativeName (nativeName);
  std::string absPath = CPI::OS::FileSystem::absoluteName (path);
  return pathToName (absPath);
}

/*
 * ----------------------------------------------------------------------
 * File Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::FileFs::FileFs::baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
CPI::Util::FileFs::FileFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_lock);
  testFilenameForValidity (fileName);
  std::string an = absoluteNameLocked (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += CPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
CPI::Util::FileFs::FileFs::URIToName (const std::string & struri) const
  throw (std::string)
{
  CPI::Util::Uri uri (struri);

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
        CPI::Util::Uri::isalpha (authority[0]) &&
        authority[1] == ':') {
      drivePrefix = authority;
      drivePrefix[0] = std::tolower (drivePrefix[0]);
    }
    else {
      /*
       * The Authority must be a local host name.
       */

      if (!CPI::OS::isLocalhost (authority)) {
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

  fullPath += CPI::Util::Uri::decode (uri.getPath ());
  return pathToName (fullPath);
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::FileFs::FileFs::absoluteNameLocked (const std::string & name) const
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
CPI::Util::FileFs::FileFs::cwd () const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_lock);
  return m_cwd;
}

void
CPI::Util::FileFs::FileFs::cd (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  bool isDir, exists;

  exists = CPI::OS::FileSystem::exists (absName, &isDir);

  if (!exists) {
    throw std::string ("name does not exist");
  }
  else if (!isDir) {
    throw std::string ("not a directory");
  }

  CPI::Util::AutoMutex lock (m_lock);
  m_cwd = CPI::Util::Vfs::joinNames (m_cwd, name);
}

void
CPI::Util::FileFs::FileFs::mkdir (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  CPI::OS::FileSystem::mkdir (absName);
}

void
CPI::Util::FileFs::FileFs::rmdir (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  CPI::OS::FileSystem::rmdir (absName);
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Iterator *
CPI::Util::FileFs::FileFs::list (const std::string & dir,
                                 const std::string & pattern)
  throw (std::string)
{
  testFilenameForValidity (dir);
  testFilenameForValidity (pattern);
  std::string absName = nativeFilename (dir);
  return new FileFsIterator (m_root, absName, pattern);
}

void
CPI::Util::FileFs::FileFs::closeIterator (CPI::Util::Vfs::Iterator * it)
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
CPI::Util::FileFs::FileFs::exists (const std::string & name, bool * isDir)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  return CPI::OS::FileSystem::exists (absName, isDir);
}

unsigned long long
CPI::Util::FileFs::FileFs::size (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  return CPI::OS::FileSystem::size (absName);
}

std::time_t
CPI::Util::FileFs::FileFs::lastModified (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  return CPI::OS::FileSystem::lastModified (absName);
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
CPI::Util::FileFs::FileFs::open (const std::string & name,
                                 std::ios_base::openmode mode)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  std::string nativeName = CPI::OS::FileSystem::toNativeName (absName);

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
CPI::Util::FileFs::FileFs::openReadonly (const std::string & name,
                                         std::ios_base::openmode mode)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  std::string nativeName = CPI::OS::FileSystem::toNativeName (absName);

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
CPI::Util::FileFs::FileFs::openWriteonly (const std::string & name,
                                          std::ios_base::openmode mode)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  std::string nativeName = CPI::OS::FileSystem::toNativeName (absName);

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
CPI::Util::FileFs::FileFs::close (std::ios * str)
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
CPI::Util::FileFs::FileFs::move (const std::string & oldName,
                                 Vfs * destFs,
                                 const std::string & newName)
  throw (std::string)
{
  /*
   * See if the target filesystem is a FileFs. If yes, we can
   * use CPI::OS::FileSystem::rename.
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
  CPI::OS::FileSystem::rename (oldAbsName, newAbsName);
}

void
CPI::Util::FileFs::FileFs::rename (const std::string & oldName,
                                   const std::string & newName)
  throw (std::string)
{
  testFilenameForValidity (oldName);
  testFilenameForValidity (newName);
  std::string oldAbsName = nativeFilename (oldName);
  std::string newAbsName = nativeFilename (newName);
  CPI::OS::FileSystem::rename (oldAbsName, newAbsName);
}

void
CPI::Util::FileFs::FileFs::remove (const std::string & name)
  throw (std::string)
{
  testFilenameForValidity (name);
  std::string absName = nativeFilename (name);
  CPI::OS::FileSystem::remove (absName);
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
CPI::Util::FileFs::FileFs::testFilenameForValidity (const std::string & name)
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
 * Compute the "native" file name to use with CPI::OS::FileSystem
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::FileFs::FileFs::nativeFilename (const std::string & name) const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_lock);

  std::string absName = CPI::Util::Vfs::joinNames (m_cwd, name);

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
