#include <CpiUtilZipFs.h>
#include <CpiUtilZipStream.h>
#include <CpiUtilVfs.h>
#include <CpiUtilVfsIterator.h>
#include <CpiUtilAutoRDLock.h>
#include <CpiUtilAutoWRLock.h>
#include <CpiUtilUri.h>
#include <CpiUtilMisc.h>
#include <CpiOsAssert.h>
#include <CpiOsRWLock.h>
#include <CpiOsSizeCheck.h>
#include <iostream>
#include <string>
#include <ctime>
#include <map>
#include <set>
#include "unzip.h"
#include "zip.h"

/*
 * ----------------------------------------------------------------------
 * Implementation of the ZLIB I/O API
 * ----------------------------------------------------------------------
 */

namespace {
  struct zfsopaque {
    std::istream * is;
    std::ostream * os;
  };
}

extern "C" {
  static void *
  ios_open_file_func (void * opaque, const char * filename, int mode)
  {
    CPI::Util::Vfs::Vfs * fs = static_cast<CPI::Util::Vfs::Vfs *> (opaque);

    if (!fs) {
      return 0;
    }

    if (!(mode & ZLIB_FILEFUNC_MODE_WRITE)) {
      std::istream * is;

      try {
	is = fs->openReadonly (filename, std::ios_base::binary);
      }
      catch (...) {
	return 0;
      }

      zfsopaque * zfs = new zfsopaque;
      zfs->is = is;
      zfs->os = 0;
      return zfs;
    }

    std::ios_base::openmode iosmode = std::ios_base::binary;
    std::iostream * ios;

    if ((mode & ZLIB_FILEFUNC_MODE_CREATE)) {
      iosmode |= std::ios_base::trunc;
    }

    try {
      ios = fs->open (filename, iosmode);
    }
    catch (...) {
      return 0;
    }

    zfsopaque * zfs = new zfsopaque;
    zfs->is = ios;
    zfs->os = ios;
    return zfs;
  }

  static uLong
  ios_read_file_func (void *, void * stream, void * buf, uLong size)
  {
    zfsopaque * zfs = static_cast<zfsopaque *> (stream);

    if (!zfs || !zfs->is) {
      return 0;
    }

    zfs->is->read (static_cast<char*> (buf), (std::streamsize) size);
    return (uLong) zfs->is->gcount();
  }

  static uLong
  ios_write_file_func (void *, void * stream, const void * buf, uLong size)
  {
    zfsopaque * zfs = static_cast<zfsopaque *> (stream);

    if (!zfs || !zfs->os) {
      return 0;
    }

    zfs->os->write (static_cast<const char*> (buf), (std::streamsize) size);
    return (zfs->os->good() ? size : 0);
  }
  
  static long
  ios_tell_file_func (void *, void * stream)
  {
    zfsopaque * zfs = static_cast<zfsopaque *> (stream);

    if (!zfs || !zfs->is) {
      return (long) -1;
    }

    return (long) zfs->is->tellg ();
  }

  static long
  ios_seek_file_func (void *, void * stream, uLong offset, int origin)
  {
    zfsopaque * zfs = static_cast<zfsopaque *> (stream);

    if (!zfs || !zfs->is) {
      return (long) -1;
    }

    if (!zfs->is->good()) {
      return (long) -1;
    }

    switch (origin) {
    case ZLIB_FILEFUNC_SEEK_SET:
      zfs->is->seekg ((std::ios::pos_type) offset);
      break;

    case ZLIB_FILEFUNC_SEEK_CUR:
      zfs->is->seekg ((std::ios::off_type) offset, std::ios_base::cur);
      break;

    case ZLIB_FILEFUNC_SEEK_END:
      zfs->is->seekg ((std::ios::off_type) offset, std::ios_base::end);
      break;

    default:
      return (long) -1;
    }

    if (!zfs->is->good()) {
      return (long) -1;
    }

    return 0;
  }

  static int
  ios_close_file_func (void * opaque, void * stream)
  {
    CPI::Util::Vfs::Vfs * fs = static_cast<CPI::Util::Vfs::Vfs *> (opaque);
    zfsopaque * zfs = static_cast<zfsopaque *> (stream);
    int res;

    if (!fs || !zfs || !zfs->is) {
      res = -1;
    }
    else {
      res = 0;

      try {
	fs->close (zfs->is);
      }
      catch (...) {
	res = 1;
      }
    }

    delete zfs;
    return res;
  }

  static int
  ios_testerror_file_func (void *, void * stream)
  {
    zfsopaque * zfs = static_cast<zfsopaque *> (stream);

    if (!zfs || !zfs->is) {
      return 1;
    }

    if (!zfs->is->good()) {
      return 1;
    }

    if (zfs->os && !zfs->os->good()) {
      return 1;
    }

    return 0;
  }
}

/*
 * ----------------------------------------------------------------------
 * A helper to update the contents of a ZIP file
 * ----------------------------------------------------------------------
 */

namespace {

  typedef std::set<std::string> ZipFileSet;
  typedef std::map<std::string, std::string> ZipFileMap;

  /*
   * Helper: Update a ZIP file, deleting some files and renaming
   * others. The ZLIB library does not support doing that in-place,
   * so the best we can do is to copy all files into a new ZIP
   * (using raw mode, so that we don't have to decompress and re-
   * compress), to delete the original file, and then to rename the
   * temporary ZIP to the orignal file name.
   */

  enum {
    ZIP_BUFFER_SIZE = 65536
  };

  void
  updateZipFile (CPI::Util::Vfs::Vfs * fs,
		 const std::string & zipFileName,
		 zlib_filefunc_def * zFileFuncs,
		 const ZipFileSet & filesToDelete,
		 const ZipFileMap & filesToRename)
    throw (std::string)
  {
    std::string tempName = zipFileName + ".tmp";

    try {
      fs->remove (tempName);
    }
    catch (...) {
    }

    zipFile tzf = zipOpen2 (tempName.c_str(),
			    APPEND_STATUS_CREATE,
			    0, zFileFuncs);

    if (!tzf) {
      std::string reason = "Can not open temp ZIP file \"";
      reason += tempName;
      reason += "\" for writing";
      throw reason;
    }

    unzFile uzf = unzOpen2 (zipFileName.c_str(), zFileFuncs);

    if (!uzf) {
      zipClose (tzf, 0);
      std::string reason = "Can not open ZIP file \"";
      reason += zipFileName;
      reason += "\" for reading";
      throw reason;
    }

    std::string destFileName;
    zip_fileinfo zFileInfo;
    unz_file_info uFileInfo;
    char fileName[1024];
    char buffer[ZIP_BUFFER_SIZE];
    int count;

    int status = unzGoToFirstFile (uzf);
    bool good = true;

    while (status == UNZ_OK) {
      if (unzGetCurrentFileInfo (uzf, &uFileInfo,
				 fileName, 1024,
				 0, 0, 0, 0) != UNZ_OK) {
	good = false;
	break;
      }

      /*
       * Is this file to be deleted? Then ignore.
       */

      destFileName = fileName;

      if (filesToDelete.find (destFileName) != filesToDelete.end()) {
	status = unzGoToNextFile (uzf);
	continue;
      }

      /*
       * Is this file to be renamed?
       */

      ZipFileMap::const_iterator it = filesToRename.find (destFileName);

      if (it != filesToRename.end()) {
	destFileName = (*it).second;
      }

      zFileInfo.tmz_date.tm_sec  = uFileInfo.tmu_date.tm_sec;
      zFileInfo.tmz_date.tm_min  = uFileInfo.tmu_date.tm_min;
      zFileInfo.tmz_date.tm_hour = uFileInfo.tmu_date.tm_hour;
      zFileInfo.tmz_date.tm_mday = uFileInfo.tmu_date.tm_mday;
      zFileInfo.tmz_date.tm_mon  = uFileInfo.tmu_date.tm_mon;
      zFileInfo.tmz_date.tm_year = uFileInfo.tmu_date.tm_year;
      zFileInfo.dosDate = uFileInfo.dosDate;
      zFileInfo.internal_fa = uFileInfo.internal_fa;
      zFileInfo.external_fa = uFileInfo.external_fa;

      int method, level;

      if (unzOpenCurrentFile2 (uzf, &method, &level, 1) != UNZ_OK) {
	good = false;
	break;
      }

      if (zipOpenNewFileInZip2 (tzf, destFileName.c_str(), &zFileInfo,
				0, 0, 0, 0, 0,
				method, level, 1) != ZIP_OK) {
	unzCloseCurrentFile (uzf);
	good = false;
	break;
      }

      count = unzReadCurrentFile (uzf, buffer, ZIP_BUFFER_SIZE);
      while (count > 0) {
	if (zipWriteInFileInZip (tzf, buffer, count) != ZIP_OK) {
	  count = -1;
	  break;
	}
	count = unzReadCurrentFile (uzf, buffer, ZIP_BUFFER_SIZE);
      }

      if (count < 0) {
	zipCloseFileInZipRaw (tzf,
			      uFileInfo.uncompressed_size,
			      uFileInfo.crc);
	unzCloseCurrentFile (uzf);
	good = false;
	break;
      }

      if (unzCloseCurrentFile (uzf) != UNZ_OK) {
	zipCloseFileInZipRaw (tzf,
			      uFileInfo.uncompressed_size,
			      uFileInfo.crc);
	unzCloseCurrentFile (uzf);
	good = false;
	break;
      }

      if (zipCloseFileInZipRaw (tzf,
				uFileInfo.uncompressed_size,
				uFileInfo.crc)) {
	good = false;
	break;
      }

      status = unzGoToNextFile (uzf);
    }

    if (status != UNZ_OK && status != UNZ_END_OF_LIST_OF_FILE) {
      good = false;
    }

    if (zipClose (tzf, 0) != UNZ_OK) {
      good = false;
    }

    if (unzClose (uzf) != UNZ_OK) {
      good = false;
    }

    if (!good) {
      throw std::string ("Can not delete file from ZIP: error copying file");
    }

    std::string oldName = zipFileName + "~";

    try {
      fs->remove (oldName);
    }
    catch (...) {
    }
    
    fs->rename (zipFileName, oldName);
    fs->rename (tempName, zipFileName);
    fs->remove (oldName);
  }

}

/*
 * ----------------------------------------------------------------------
 * Iterator object for directory listings
 * ----------------------------------------------------------------------
 */

namespace {

  class ZipFsIterator : public CPI::Util::Vfs::Iterator {
  public:
    ZipFsIterator (const std::string & dir,
		   const std::string & pattern,
		   const CPI::Util::ZipFs::ZipFs::FileInfos & contents)
      throw ();
    ~ZipFsIterator ()
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
    const CPI::Util::ZipFs::ZipFs::FileInfos & m_contents;
    CPI::Util::ZipFs::ZipFs::FileInfos::const_iterator m_iterator;
  };

}

ZipFsIterator::ZipFsIterator (const std::string & dir,
			      const std::string & pattern,
			      const CPI::Util::ZipFs::ZipFs::FileInfos & contents)
  throw ()
  : m_contents (contents)
{
  m_dir = dir.substr (1);
  std::string absPat = CPI::Util::Vfs::joinNames (dir, pattern);
  m_absPatDir = CPI::Util::Vfs::directoryName (absPat);
  m_absPatDir = m_absPatDir.substr (1);
  m_relPat = CPI::Util::Vfs::relativeName (absPat);
  m_iterator = m_contents.begin ();
  m_match = false;
}

ZipFsIterator::~ZipFsIterator ()
  throw ()
{
}

bool
ZipFsIterator::end ()
  throw (std::string)
{
  if (m_match) {
    return false;
  }
  return !(m_match = findFirstMatching ());
}

bool
ZipFsIterator::next ()
  throw (std::string)
{
  if (m_iterator == m_contents.end()) {
    return false;
  }
  m_iterator++;
  return (m_match = findFirstMatching ());
}

std::string
ZipFsIterator::relativeName ()
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
  cpiAssert (absDirLen == 0 || absFileName[absDirLen] == '/');

  std::string::size_type firstPos = dirLen ? dirLen+1 : 0;
  std::string::size_type firstCharInTailPos = absDirLen ? absDirLen+1 : 0;
  std::string::size_type nextSlash =
    absFileName.find ('/', firstCharInTailPos);

  if (nextSlash == std::string::npos) {
    return absFileName.substr (firstPos);
  }

  return absFileName.substr (firstPos, nextSlash - firstPos);
}

std::string
ZipFsIterator::absoluteName ()
  throw (std::string)
{
  cpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;
  std::string::size_type absDirLen = m_absPatDir.length ();

  cpiAssert (absFileName.length() > absDirLen);
  cpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  cpiAssert (absDirLen == 0 || absFileName[absDirLen] == '/');

  std::string::size_type firstCharInTailPos = absDirLen ? absDirLen+1 : 0;
  std::string::size_type nextSlash =
    absFileName.find ('/', firstCharInTailPos);

  if (nextSlash != std::string::npos) {
    return absFileName.substr (0, nextSlash);
  }

  return absFileName;
}

bool
ZipFsIterator::isDirectory ()
  throw (std::string)
{
  cpiAssert (m_iterator != m_contents.end());
  const std::string & absFileName = (*m_iterator).first;

  std::string::size_type absDirLen = m_absPatDir.length();

  cpiAssert (absFileName.length() > absDirLen);
  cpiAssert (absFileName.compare (0, absDirLen, m_absPatDir) == 0);
  cpiAssert (absDirLen == 0 || absFileName[absDirLen] == '/');

  std::string::size_type nextSlash =
    absFileName.find ('/', absDirLen ? (absDirLen + 1) : 0);
  return ((nextSlash == std::string::npos) ? false : true);
}

unsigned long long
ZipFsIterator::size ()
  throw (std::string)
{
  cpiAssert (m_iterator != m_contents.end());
  return ((*m_iterator).second.size);
}

std::time_t
ZipFsIterator::lastModified ()
  throw (std::string)
{
  cpiAssert (m_iterator != m_contents.end());
  return ((*m_iterator).second.lastModified);
}

bool
ZipFsIterator::findFirstMatching ()
  throw (std::string)
{
  /*
   * Look for an element in the contents, whose prefix maches m_absPatDir,
   * and whose next path component matches m_pattern.
   */

  std::string::size_type pdl = m_absPatDir.length ();
  std::string::size_type firstFnPos;

  if (pdl == 0) {
    firstFnPos = 0;
  }
  else {
    firstFnPos = pdl + 1;
  }

  while (m_iterator != m_contents.end()) {
    const std::string & absFileName = (*m_iterator).first;

    if (absFileName.length() >= firstFnPos &&
	(pdl == 0 || absFileName[pdl] == '/') &&
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
 * Implementation of the VFS interface
 * ----------------------------------------------------------------------
 */

inline
zlib_filefunc_def &
o2zfd (void ** ptr)
  throw ()
{
  return *reinterpret_cast<zlib_filefunc_def *> (ptr);
}

CPI::Util::ZipFs::ZipFs::ZipFs ()
  throw ()
  : m_fs (0),
    m_adopted (false),
    m_keepOpen (false),
    m_zipFile (0),
    m_unzFile (0)
{
  cpiAssert (sizeof (m_zFileFuncsOpaque) >= sizeof (zlib_filefunc_def));
  zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
  zFileFuncs.opaque = 0;
}

CPI::Util::ZipFs::ZipFs::ZipFs (CPI::Util::Vfs::Vfs * fs,
				const std::string & zipFileName,
				std::ios_base::openmode mode,
				bool adopt,
				bool keepOpen)
  throw (std::string)
  : m_fs (0),
    m_adopted (false),
    m_keepOpen (false),
    m_zipFile (0),
    m_unzFile (0)
{
  cpiAssert (sizeof (m_zFileFuncsOpaque) >= sizeof (zlib_filefunc_def));
  zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
  zFileFuncs.opaque = 0;
  openZip (fs, zipFileName, mode, adopt, keepOpen);
}

CPI::Util::ZipFs::ZipFs::~ZipFs ()
  throw ()
{
  if (m_fs) {
    try {
      closeZip ();
    }
    catch (...) {
    }
  }
}

void
CPI::Util::ZipFs::ZipFs::openZip (CPI::Util::Vfs::Vfs * fs,
				  const std::string & zipFileName,
				  std::ios_base::openmode mode,
				  bool adopt,
				  bool keepOpen)
  throw (std::string)
{
  if (m_fs) {
    throw std::string ("already open");
  }

  m_fs = fs;
  m_zipFileName = zipFileName;
  m_adopted = adopt;
  m_keepOpen = keepOpen;
  m_zipFile = 0;
  m_unzFile = 0;
  m_cwd = "/";
  m_mode = mode;

  zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
  zFileFuncs.zopen_file = ios_open_file_func;
  zFileFuncs.zread_file = ios_read_file_func;
  zFileFuncs.zwrite_file = ios_write_file_func;
  zFileFuncs.ztell_file = ios_tell_file_func;
  zFileFuncs.zseek_file = ios_seek_file_func;
  zFileFuncs.zclose_file = ios_close_file_func;
  zFileFuncs.zerror_file = ios_testerror_file_func;
  zFileFuncs.opaque = fs;

  /*
   * Compose our URI
   */

  std::string authority = fs->nameToURI (zipFileName);

  m_baseURI  = "zipfs://";
  m_baseURI += CPI::Util::Uri::encode (authority);
  m_baseURI += "/";

  /*
   * If the file is supposed to exist, try to read its TOC
   */

  if (!(mode & std::ios_base::ate)  &&
      !(mode & std::ios_base::trunc)) {
    updateContents ();
  }
}

void
CPI::Util::ZipFs::ZipFs::closeZip ()
  throw (std::string)
{
  if (!m_fs) {
    throw std::string ("not open");
  }

  zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
  zFileFuncs.opaque = 0;

  if (m_zipFile) {
    zipFile zf = static_cast<zipFile> (m_zipFile);
    zipClose (zf, 0);
    m_zipFile = 0;
  }

  if (m_unzFile) {
    unzFile zf = static_cast<unzFile> (m_unzFile);
    unzClose (zf);
    m_unzFile = 0;
  }

  if (m_adopted) {
    delete m_fs;
  }

  m_fs = 0;
  m_baseURI.clear ();
  m_contents.clear ();
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::ZipFs::ZipFs::absoluteNameLocked (const std::string & name) const
  throw (std::string)
{
  return CPI::Util::Vfs::joinNames (m_cwd, name);
}

/*
 * ----------------------------------------------------------------------
 * URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::ZipFs::ZipFs::baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
CPI::Util::ZipFs::ZipFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  testFilenameForValidity (fileName);
  std::string an = absoluteName (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += CPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
CPI::Util::ZipFs::ZipFs::URIToName (const std::string & uri) const
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
 * Directory Management
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::ZipFs::ZipFs::cwd () const
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);
  return m_cwd;
}

void
CPI::Util::ZipFs::ZipFs::cd (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  /*
   * Regular file?
   */

  if (m_contents.find (nn) != m_contents.end ()) {
    std::string reason = "Can not change cwd to \"";
    reason += fileName;
    reason += "\": file exists as a plain file";
    throw reason;
  }

  m_cwd = absoluteNameLocked (fileName);
}

void
CPI::Util::ZipFs::ZipFs::mkdir (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  if (!(m_mode & std::ios_base::out)) {
    throw std::string ("ZIP file not open for writing");
  }

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  /*
   * Regular file?
   */

  if (m_contents.find (nn) != m_contents.end ()) {
    std::string reason = "Can not make directory \"";
    reason += fileName;
    reason += "\": file exists as a plain file";
    throw reason;
  }
}

void
CPI::Util::ZipFs::ZipFs::rmdir (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  if (!(m_mode & std::ios_base::out)) {
    throw std::string ("ZIP file not open for writing");
  }

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  /*
   * Regular file?
   */

  if (m_contents.find (nn) != m_contents.end ()) {
    std::string reason = "Can not remove directory \"";
    reason += fileName;
    reason += "\": file exists as a plain file";
    throw reason;
  }

  /*
   * Make sure that this directory is empty, i.e. that there are no
   * files that have this directory name as a prefix.
   */

  std::string::size_type dnlen = nn.length();
  FileInfos::iterator it;

  for (it = m_contents.begin(); it != m_contents.end(); it++) {
    if ((*it).first.length () > dnlen &&
	(*it).first.compare (0, dnlen, nn) == 0 &&
	(*it).first[dnlen] == '/') {
      std::string reason = "Can not remove directory \"";
      reason += fileName;
      reason += "\": not empty";
      throw reason;
    }
  }
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Iterator *
CPI::Util::ZipFs::ZipFs::list (const std::string & dir,
			       const std::string & pattern)
  throw (std::string)
{
  m_lock.rdLock ();
  testFilenameForValidity (pattern);
  std::string absDir = absoluteNameLocked (dir);
  return new ZipFsIterator (absDir, pattern, m_contents);
}

void
CPI::Util::ZipFs::ZipFs::closeIterator (CPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  ZipFsIterator * zfi = dynamic_cast<ZipFsIterator *> (it);

  if (!zfi) {
    throw std::string ("invalid iterator");
  }

  delete zfi;
  m_lock.rdUnlock ();
}

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::ZipFs::ZipFs::exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);
  std::string::size_type nnlen = nn.length();

  /*
   * Special case for "/" (whose native name becomes the empty string).
   */

  if (!nnlen) {
    if (isDir) {
      *isDir = true;
    }

    return true;
  }

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

  FileInfos::iterator it;

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
CPI::Util::ZipFs::ZipFs::size (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  FileInfos::iterator it = m_contents.find (nn);

  if (it == m_contents.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.size;
}

std::time_t
CPI::Util::ZipFs::ZipFs::lastModified (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoRDLock lock (m_lock);

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  FileInfos::iterator it = m_contents.find (nn);

  if (it == m_contents.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.lastModified;
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
CPI::Util::ZipFs::ZipFs::open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  /*
   * Cannot modify a file within a ZIP archive. We have the option of
   * 1) extracting the file from the zip,
   * 2) returning an fstream
   * 3) delete the file from the zip
   * 4) copying the file back into the zip
   * 5) delete the temp file
   * but consider that too complicated.
   */

  throw std::string ("not supported");
  return 0;
}

std::istream *
CPI::Util::ZipFs::ZipFs::openReadonly (const std::string & fileName,
				       std::ios_base::openmode)
  throw (std::string)
{
  m_lock.rdLock ();

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  if (m_zipFile) {
    zipFile zf = static_cast<zipFile> (m_zipFile);
    m_zipFile = 0;
    if (zipClose (zf, 0) != ZIP_OK) {
      throw std::string ("error closing ZIP file from previous write");
    }
  }

  unzFile zf;

  if (m_unzFile) {
    zf = static_cast<unzFile> (m_unzFile);
  }
  else {
    zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
    zf = unzOpen2 (m_zipFileName.c_str(), &zFileFuncs);

    if (!zf) {
      m_lock.rdUnlock ();
      std::string reason = "Can not open ZIP file \"";
      reason += fileName;
      reason += "\"";
      throw reason;
    }

    if (m_keepOpen) {
      m_unzFile = static_cast<void *> (zf);
    }
  }

  if (unzLocateFile (zf, nn.c_str(), 1) != UNZ_OK) {
    if (!m_keepOpen) {
      unzClose (zf);
      m_unzFile = 0;
    }

    m_lock.rdUnlock ();
    std::string reason = "Can not find \"";
    reason += fileName;
    reason += "\" in ZIP file \"";
    reason += m_zipFileName;
    reason += "\"";
    throw reason;
  }

  if (unzOpenCurrentFile (zf) != UNZ_OK) {
    if (!m_keepOpen) {
      unzClose (zf);
      m_unzFile = 0;
    }

    m_lock.rdUnlock ();
    std::string reason = "Can not open \"";
    reason += fileName;
    reason += "\" in ZIP file \"";
    reason += m_zipFileName;
    reason += "\"";
    throw reason;
  }

  return new CPI::Util::ZipFs::zipistream (zf);
}

std::ostream *
CPI::Util::ZipFs::ZipFs::openWriteonly (const std::string & fileName,
					std::ios_base::openmode mode)
  throw (std::string)
{
  m_lock.wrLock ();

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  if (!(m_mode & std::ios_base::out)) {
    m_lock.wrUnlock ();
    throw std::string ("ZIP file is not open for writing");
  }

  if (!(mode & std::ios_base::trunc)) {
    m_lock.wrUnlock ();
    throw std::string ("implementation constraint: can not not truncate");
  }

  /*
   * If the file exists, we have to delete it
   */

  if (m_contents.find (nn) != m_contents.end ()) {
    if ((m_mode & std::ios_base::app)) {
      m_lock.wrUnlock ();
      throw std::string ("Can not overwrite files in append mode");
    }
    removeLocked (nn);
  }

  /*
   * If the ZIP file does not exist, then we create it
   */

  zipFile zf;
  int appflag;

  if ((m_mode & std::ios_base::ate)) {
    appflag = APPEND_STATUS_CREATEAFTER;
    m_mode &= ~std::ios_base::ate;
  }
  else if ((m_mode & std::ios_base::trunc)) {
    appflag = APPEND_STATUS_CREATE;
    m_mode &= ~std::ios_base::trunc;
  }
  else {
    appflag = APPEND_STATUS_ADDINZIP;
  }

  if (m_unzFile) {
    unzFile zf = static_cast<unzFile> (m_unzFile);
    m_unzFile = 0;
    if (unzClose (zf) != UNZ_OK) {
      throw std::string ("error closing ZIP file from previous read");
    }
  }

  if (m_zipFile) {
    zf = static_cast<zipFile> (m_zipFile);
  }
  else {
    zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
    zf = zipOpen2 (m_zipFileName.c_str(), appflag,
		   0, &zFileFuncs);

    if (!zf) {
      m_lock.wrUnlock ();
      std::string reason = "Can not open ZIP file \"";
      reason += m_zipFileName;
      reason += "\"";
      throw reason;
    }

    if (m_keepOpen) {
      m_zipFile = static_cast<void *> (zf);
    }
  }

  std::time_t theTime = time (0);
  struct tm * timeStr = localtime (&theTime);

  zip_fileinfo zfi;
  zfi.dosDate = 0;
  zfi.internal_fa = 0;
  zfi.external_fa = 0;
  zfi.tmz_date.tm_sec = timeStr->tm_sec;
  zfi.tmz_date.tm_min = timeStr->tm_min;
  zfi.tmz_date.tm_hour = timeStr->tm_hour;
  zfi.tmz_date.tm_mday = timeStr->tm_mday;
  zfi.tmz_date.tm_mon = timeStr->tm_mon;
  zfi.tmz_date.tm_year = timeStr->tm_year + 1900;

  if (zipOpenNewFileInZip (zf, nn.c_str(), &zfi,
			   0, 0, 0, 0, 0,
			   Z_DEFLATED,
			   Z_DEFAULT_COMPRESSION) != ZIP_OK) {
    if (!m_keepOpen) {
      zipClose (zf, 0);
      m_zipFile = 0;
    }

    m_lock.wrUnlock ();
    std::string reason = "Can not open \"";
    reason += fileName;
    reason += "\" in ZIP file \"";
    reason += m_zipFileName;
    reason += "\" for writing";
    throw reason;
  }

  FileInfo & stat = m_contents[nn];
  stat.size = 0;
  stat.lastModified = std::time (0);

  return new CPI::Util::ZipFs::zipostream (zf);
}

void
CPI::Util::ZipFs::ZipFs::close (std::ios * str)
  throw (std::string)
{
  if (dynamic_cast<CPI::Util::ZipFs::zipistream *> (str) == 0 &&
      dynamic_cast<CPI::Util::ZipFs::zipostream *> (str) == 0) {
    throw std::string ("invalid stream");
  }

  std::string reason;
  bool good = true;

  CPI::Util::ZipFs::zipistream * zis =
    dynamic_cast<CPI::Util::ZipFs::zipistream *> (str);

  if (zis) {
    int status = unzCloseCurrentFile (zis->m_zip);

    if (status == UNZ_CRCERROR) {
      reason = "CRC error";
      good = false;
    }
    else if (status != UNZ_OK) {
      reason = "error closing file";
      good = false;
    }

    if (!m_keepOpen) {
      if (unzClose (zis->m_zip) != UNZ_OK && good) {
	reason = "error closing ZIP file";
	good = false;
      }
    }

    delete zis;

    m_lock.rdUnlock ();

    if (!good) {
      throw reason;
    }

    return;
  }

  CPI::Util::ZipFs::zipostream * zos =
    dynamic_cast<CPI::Util::ZipFs::zipostream *> (str);

  if (zos) {
    if (zipCloseFileInZip (zos->m_zip) != ZIP_OK) {
      reason = "error closing file";
      good = false;
    }

    if (!m_keepOpen) {
      if (zipClose (zos->m_zip, 0) != ZIP_OK) {
	reason = "error closing ZIP file";
	good = false;
      }
    }

    delete zos;

    /*
     * After writing a file, the ZIP's contents may have changed.
     */

    m_lock.wrUnlock ();

    if (!good) {
      throw reason;
    }

    return;
  }

  /*
   * Shouldn't be here
   */

  throw std::string ("not a ZIP stream");
}

/*
 * ----------------------------------------------------------------------
 * File system operations
 * ----------------------------------------------------------------------
 *
 * To rename or to remove a file, we have to copy the ZIP file, see the
 * updateZipFile helper above. Repeating that over and over is inefficient,
 * for example if multiple files are deleted from a ZIP.
 *
 * The implementation could be changed so that file renames and deletions
 * are just filed away. Update of the ZIP file could then be delayed and
 * done in the destructor, or when writing to a not-yet-deleted file.
 * However, the first delete/rename should not be delayed, so that errors
 * due to a non-writeable file system aren't recognized too late.
 */

void
CPI::Util::ZipFs::ZipFs::rename (const std::string & oldName,
				 const std::string & newName)
  throw (std::string)
{
  /*
   * Renaming a file within this ZIP
   */

  CPI::Util::AutoWRLock lock (m_lock);

  if (!(m_mode & std::ios_base::out)) {
    throw std::string ("ZIP file not open for writing");
  }

  if ((m_mode & std::ios_base::app)) {
    throw std::string ("Can not rename files in ZIP append mode");
  }

  testFilenameForValidity (oldName);
  testFilenameForValidity (oldName);
  std::string onn = nativeFilename (oldName);
  std::string nnn = nativeFilename (newName);

  if (!exists (oldName)) {
    std::string reason = "Can not rename \"";
    reason += oldName;
    reason += "\": file not found";
    throw reason;
  }

  ZipFileSet toBeDeleted;
  ZipFileMap toBeRenamed;
  toBeRenamed[onn] = nnn;

  if (exists (newName)) {
    toBeDeleted.insert (nnn);
  }

  if (m_zipFile) {
    zipFile zf = static_cast<zipFile> (m_zipFile);
    m_zipFile = 0;
    if (zipClose (zf, 0) != ZIP_OK) {
      throw std::string ("error closing ZIP file from previous write");
    }
  }

  if (m_unzFile) {
    unzFile zf = static_cast<unzFile> (m_unzFile);
    m_unzFile = 0;
    if (unzClose (zf) != UNZ_OK) {
      throw std::string ("error closing ZIP file from previous read");
    }
  }

  zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
  updateZipFile (m_fs, m_zipFileName, &zFileFuncs,
		 toBeDeleted, toBeRenamed);

  m_contents[nnn] = m_contents[onn];
  m_contents.erase (onn);
}

void
CPI::Util::ZipFs::ZipFs::remove (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoWRLock lock (m_lock);
  removeLocked (fileName);
}

void
CPI::Util::ZipFs::ZipFs::removeLocked (const std::string & fileName)
  throw (std::string)
{
  if (!(m_mode & std::ios_base::out)) {
    throw std::string ("ZIP file not open for writing");
  }

  if ((m_mode & std::ios_base::app)) {
    throw std::string ("Can not rename files in ZIP append mode");
  }

  testFilenameForValidity (fileName);
  std::string nn = nativeFilename (fileName);

  if (!exists (fileName)) {
    std::string reason = "Can not delete \"";
    reason += fileName;
    reason += "\": file not found";
    throw reason;
  }

  ZipFileSet toBeDeleted;
  toBeDeleted.insert (nn);

  if (m_zipFile) {
    zipFile zf = static_cast<zipFile> (m_zipFile);
    m_zipFile = 0;
    if (zipClose (zf, 0) != ZIP_OK) {
      throw std::string ("error closing ZIP file from previous write");
    }
  }

  if (m_unzFile) {
    unzFile zf = static_cast<unzFile> (m_unzFile);
    m_unzFile = 0;
    if (unzClose (zf) != UNZ_OK) {
      throw std::string ("error closing ZIP file from previous read");
    }
  }

  zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
  updateZipFile (m_fs, m_zipFileName, &zFileFuncs,
		 toBeDeleted, ZipFileMap());

  m_contents.erase (nn);
}

/*
 * Test whether a file name is valid. A file name must be either absolute
 * (start with a slash) or be relative (doesn't contain any slash). For
 * absolute file names, we don't allow any "." or ".." components.
 *
 * Throw an exception if the file name is not valid.
 */

void
CPI::Util::ZipFs::ZipFs::
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

std::string
CPI::Util::ZipFs::ZipFs::nativeFilename (const std::string & name)
  throw ()
{
  cpiAssert (name.length());

  std::string res;

  if (name[0] != '/') {
    res = m_cwd;
    if (m_cwd.length() > 1) {
      res += "/";
    }
  }

  res += name;
  return res.substr (1);
}

/*
 * ----------------------------------------------------------------------
 * Update TOC
 * ----------------------------------------------------------------------
 */

void
CPI::Util::ZipFs::ZipFs::updateContents ()
  throw (std::string)
{
  if (m_zipFile) {
    zipFile zf = static_cast<zipFile> (m_zipFile);
    m_zipFile = 0;
    if (zipClose (zf, 0) != ZIP_OK) {
      throw std::string ("error closing ZIP file from previous write");
    }
  }

  unzFile zf;

  if (m_unzFile) {
    zf = static_cast<unzFile> (m_unzFile);
  }
  else {
    zlib_filefunc_def & zFileFuncs = o2zfd (m_zFileFuncsOpaque);
    zf = unzOpen2 (m_zipFileName.c_str(), &zFileFuncs);

    if (!zf) {
      std::string reason = "Can not open ZIP file \"";
      reason += m_zipFileName;
      reason += "\"";
      throw reason;
    }

    if (m_keepOpen) {
      m_unzFile = static_cast<void *> (zf);
    }
  }

  m_contents.clear ();

  unz_file_info fileInfo;
  char fileName[1024];
  struct tm timeStr = {0};

  int status = unzGoToFirstFile (zf);

  while (status == UNZ_OK) {
    if (unzGetCurrentFileInfo (zf, &fileInfo,
			       fileName, 1024,
			       0, 0, 0, 0) != UNZ_OK) {
      break;
    }

    timeStr.tm_mday = fileInfo.tmu_date.tm_mday;
    timeStr.tm_mon  = fileInfo.tmu_date.tm_mon;
    timeStr.tm_year = fileInfo.tmu_date.tm_year - 1900;
    timeStr.tm_hour = fileInfo.tmu_date.tm_hour;
    timeStr.tm_min  = fileInfo.tmu_date.tm_min;
    timeStr.tm_sec  = fileInfo.tmu_date.tm_sec;

    FileInfo & stat = m_contents[fileName];
    stat.size = fileInfo.uncompressed_size;
    stat.lastModified = mktime (&timeStr);

    status = unzGoToNextFile (zf);
  }

  if (!m_keepOpen) {
    unzClose (zf);
  }
}
