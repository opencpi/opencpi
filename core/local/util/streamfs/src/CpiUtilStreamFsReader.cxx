/*
 * Extract a set of files from a single data stream.
 *
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  GCC bug 40391 workaround.
 *
 *     05/27/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <ctime>
#include <cstring>
#include <cstdlib>
#include <CpiOsAssert.h>
#include <CpiOsMutex.h>
#include <CpiUtilUri.h>
#include <CpiUtilMisc.h>
#include <CpiUtilAutoMutex.h>
#include "CpiUtilVfs.h"
#include "CpiUtilVfsIterator.h"
#include "CpiUtilStreamFsReader.h"

/*
 * ----------------------------------------------------------------------
 * StreamFsReaderStream: std::istream implementation for StreamFs
 * ----------------------------------------------------------------------
 */

namespace CpiUtilStreamFsReader {

  class StreamFsReaderStream : public std::istream {
  protected:
    class StreamBuf : public std::streambuf {
    public:
      StreamBuf (std::istream * stream,
                 unsigned long long beg,
                 unsigned long long size);
      ~StreamBuf ();

    protected:
      int_type pbackfail (int_type = std::streambuf::traits_type::eof());
      pos_type seekoff (off_type off, std::ios_base::seekdir way,
                        std::ios_base::openmode which);
      pos_type seekpos (pos_type pos, std::ios_base::openmode which);
      std::streamsize xsgetn (char *, std::streamsize);
      int_type underflow ();
      int_type uflow ();

    protected:
      std::istream * m_stream;
      unsigned long long m_beg;
      unsigned long long m_pos;
      unsigned long long m_size;
    };

  public:
    StreamFsReaderStream (std::istream *,
                          unsigned long long,
                          unsigned long long);
    ~StreamFsReaderStream ();

  protected:
    StreamBuf m_buf;
  };

  StreamFsReaderStream::StreamBuf::
  StreamBuf (std::istream * str,
             unsigned long long beg,
             unsigned long long size)
    : m_stream (str),
      m_beg (beg),
      m_pos (0),
      m_size (size)
  {
  }

  StreamFsReaderStream::StreamBuf::
  ~StreamBuf ()
  {
  }

  std::streambuf::int_type
  StreamFsReaderStream::StreamBuf::
  pbackfail (int_type c)
  {
    if (!m_pos) {
      return traits_type::eof ();
    }

    if (traits_type::eq_int_type (c, traits_type::eof())) {
      m_stream->unget ();
    }
    else {
      m_stream->putback (traits_type::to_char_type (c));
    }

    if (!m_stream->good()) {
      return traits_type::eof ();
    }

    m_pos--;
    return traits_type::not_eof (c);
  }

  std::streambuf::pos_type
  StreamFsReaderStream::StreamBuf::
  seekoff (off_type off, std::ios_base::seekdir way,
           std::ios_base::openmode mode)
  {
    unsigned long long origin=0;

    switch (way) {
    case std::ios_base::beg:
      origin = 0;
      break;

    case std::ios_base::cur:
      origin = m_pos;
      break;

    case std::ios_base::end:
      origin = m_size;
      break;

    default:  // get rid of the compiler warnings
      break;
    }

    if (off < 0 && static_cast<unsigned long long> (-off) > origin) {
      return static_cast<pos_type> (-1);
    }

    unsigned long long newpos = origin + off;
    pos_type ptpos = CPI::Util::Misc::unsignedToStreamsize (newpos);

    if (ptpos == static_cast<pos_type> (-1)) {
      return static_cast<pos_type> (-1);
    }

    return seekpos (ptpos, mode);
  }

  std::streambuf::pos_type
  StreamFsReaderStream::StreamBuf::
  seekpos (pos_type pos, std::ios_base::openmode)
  {
    if (pos < 0 || static_cast<unsigned long long> (pos) > m_size) {
      return static_cast<pos_type> (-1);
    }

    if (static_cast<unsigned long long> (pos) == m_pos) {
      return CPI::Util::Misc::unsignedToStreamsize (pos);
    }

    unsigned long long spos = m_beg + pos;
    pos_type ptspos = CPI::Util::Misc::unsignedToStreamsize (spos);

    if (ptspos == static_cast<pos_type> (-1)) {
      return static_cast<pos_type> (-1);
    }

    if (m_stream->seekg (ptspos) < 0) {
      return static_cast<pos_type> (-1);
    }

    m_pos = pos;
    return CPI::Util::Misc::unsignedToStreamsize (pos);
  }

  std::streamsize
  StreamFsReaderStream::StreamBuf::
  xsgetn (char * buffer, std::streamsize count)
  {
    if (count < 0) {
      return 0;
    }

    if (m_pos >= m_size) {
      return 0;
    }

    unsigned long long remaining = m_size - m_pos;
    std::streamsize strem = CPI::Util::Misc::unsignedToStreamsize (remaining);
    std::streamsize amount = (count < strem) ? count : strem;

    m_stream->read (buffer, amount);

    if (m_stream->gcount() != amount) {
      m_stream->setstate (std::ios_base::failbit);
      return m_stream->gcount();
    }

    m_pos += amount;
    return amount;
  }

  std::streambuf::int_type
  StreamFsReaderStream::StreamBuf::
  underflow ()
  {
    if (m_pos >= m_size) {
      return traits_type::eof ();
    }

    return m_stream->peek ();
  }

  std::streambuf::int_type
  StreamFsReaderStream::StreamBuf::
  uflow ()
  {
    if (m_pos >= m_size) {
      return traits_type::eof ();
    }

    std::streambuf::int_type c = m_stream->get ();

    if (traits_type::eq_int_type (c, traits_type::eof())) {
      return traits_type::eof ();
    }

    m_pos++;
    return c;
  }

  StreamFsReaderStream::StreamFsReaderStream (std::istream * str,
                                              unsigned long long beg,
                                              unsigned long long size)
    : std::istream (0),
      m_buf (str, beg, size)
  {
    this->init (&m_buf);
  }

  StreamFsReaderStream::~StreamFsReaderStream ()
  {
  }

}

using namespace CpiUtilStreamFsReader;

/*
 * ----------------------------------------------------------------------
 * Iterator object for directory listings
 * ----------------------------------------------------------------------
 */

namespace {

  class StreamFsIterator : public CPI::Util::Vfs::Iterator {
  public:
    StreamFsIterator (const std::string & dir,
                      const std::string & pattern,
                      const CPI::Util::StreamFs::StreamFsReader::TOC & contents)
      throw ();

    ~StreamFsIterator ()
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
    const CPI::Util::StreamFs::StreamFsReader::TOC & m_contents;
    CPI::Util::StreamFs::StreamFsReader::TOC::const_iterator m_iterator;
  };

  StreamFsIterator::StreamFsIterator (const std::string & dir,
                                      const std::string & pattern,
                                      const CPI::Util::StreamFs::StreamFsReader::TOC & contents)
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

  StreamFsIterator::~StreamFsIterator ()
    throw ()
  {
  }

  bool
  StreamFsIterator::end ()
    throw (std::string)
  {
    if (m_match) {
      return false;
    }
    return !(m_match = findFirstMatching ());
  }

  bool
  StreamFsIterator::next ()
    throw (std::string)
  {
    if (m_iterator == m_contents.end()) {
      return false;
    }
    m_iterator++;
    return (m_match = findFirstMatching ());
  }

  std::string
  StreamFsIterator::relativeName ()
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
  StreamFsIterator::absoluteName ()
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
  StreamFsIterator::isDirectory ()
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
  StreamFsIterator::size ()
    throw (std::string)
  {
    cpiAssert (m_iterator != m_contents.end());
    return ((*m_iterator).second.size);
  }

  std::time_t
  StreamFsIterator::lastModified ()
    throw (std::string)
  {
    cpiAssert (m_iterator != m_contents.end());
    return ((*m_iterator).second.lastModified);
  }

  bool
  StreamFsIterator::findFirstMatching ()
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

}

/*
 * ----------------------------------------------------------------------
 * StreamFsReader
 * ----------------------------------------------------------------------
 */

CPI::Util::StreamFs::StreamFsReader::
StreamFsReader ()
  throw ()
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
}

CPI::Util::StreamFs::StreamFsReader::
StreamFsReader (std::istream * stream)
  throw (std::string)
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
  openFs (stream);
}

CPI::Util::StreamFs::StreamFsReader::
StreamFsReader (Vfs * fs, const std::string & name)
  throw (std::string)
  : m_fs (fs),
    m_name (name),
    m_stream (0),
    m_cwd ("/")
{
  openFs (fs, name);
}

CPI::Util::StreamFs::StreamFsReader::
~StreamFsReader ()
  throw ()
{
  if (m_stream) {
    try {
      closeFs ();
    }
    catch (...) {
    }
  }
}

void
CPI::Util::StreamFs::StreamFsReader::
openFs (std::istream * stream)
    throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  if (m_stream) {
    throw std::string ("already open");
  }

  m_stream = stream;
  m_fs = 0;
  m_cwd = "/";
  m_pos = static_cast<unsigned long long> (-1);
  m_openFiles = 0;
  m_openIterators = 0;

  m_baseURI = "streamfs://[somestream]/";

  try {
    readTOC ();
  }
  catch (...) {
    m_stream = 0;
    throw;
  }
}

void
CPI::Util::StreamFs::StreamFsReader::
openFs (Vfs * fs, const std::string & name)
    throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  if (m_stream) {
    throw std::string ("already open");
  }

  m_stream = 0;
  m_fs = fs;
  m_name = name;
  m_cwd = "/";
  m_pos = static_cast<unsigned long long> (-1);
  m_openFiles = 0;
  m_openIterators = 0;

  std::string authority = fs->nameToURI (name);
  m_baseURI  = "streamfs://";
  m_baseURI += CPI::Util::Uri::encode (authority);
  m_baseURI += "/";

  m_stream = m_fs->openReadonly (m_name, std::ios_base::binary);

  try {
    readTOC ();
  }
  catch (...) {
    try {
      m_fs->close (m_stream);
    }
    catch (...) {
    }

    m_stream = 0;
    m_fs = 0;

    throw;
  }
}

void
CPI::Util::StreamFs::StreamFsReader::
closeFs ()
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  cpiAssert (!m_openFiles);
  cpiAssert (!m_openIterators);

  m_toc.clear ();

  if (m_fs && m_stream) {
    std::istream * stream = m_stream;
    m_stream = 0;
    m_fs->close (stream);
  }
  else {
    m_stream = 0;
  }
}

/*
 * ----------------------------------------------------------------------
 * Read a Table of Contents from the end of the stream
 * ----------------------------------------------------------------------
 */

void
CPI::Util::StreamFs::StreamFsReader::
readTOC ()
  throw (std::string)
{
  m_pos = static_cast<unsigned long long> (-1);

  /*
   * Extract TOC position from the end of the file.
   */

  if (m_stream->seekg (-17, std::ios_base::end) < 0) {
    throw std::string ("can not seek to end");
  }

  std::string sTocPos;

  if (!std::getline (*m_stream, sTocPos).good()) {
    throw std::string ("can not read TOC position");
  }

  if (sTocPos.length() != 16) {
    throw std::string ("invalid TOC position");
  }

  char * endPtr;
  unsigned long long tocPos = std::strtoul (sTocPos.c_str(), &endPtr, 10);
  std::streamsize ptTocPos = CPI::Util::Misc::unsignedToStreamsize (tocPos);

  if (!endPtr || *endPtr || ptTocPos < 0) {
    throw std::string ("invalid TOC position");
  }

  /*
   * Seek to TOC.
   */

  m_stream->seekg (ptTocPos);

  if (!m_stream->good()) {
    throw std::string ("error seeking TOC");
  }

  /*
   * There should be a LF at the beginning.
   */

  if (m_stream->get() != '\n') {
    throw std::string ("error reading TOC");
  }

  /*
   * Read TOC.
   */

  while (42) {
    std::string fileName, sPos, sSize, sLm;

    std::getline (*m_stream, fileName);

    if (!m_stream->good()) {
      throw std::string ("error reading TOC");
    }

    if (fileName == "<End Of TOC>") {
      break;
    }

    std::getline (*m_stream, sPos);
    std::getline (*m_stream, sSize);
    std::getline (*m_stream, sLm);

    if (!m_stream->good()) {
      throw std::string ("error reading TOC");
    }

    char *epPos, *epSize, *epLm;

    unsigned long long pos =  std::strtoul (sPos.c_str(), &epPos, 10);
    unsigned long long size = std::strtoul (sSize.c_str(), &epSize, 10);
    unsigned long long lm =   std::strtoul (sLm.c_str(), &epLm, 10);

    if (!epPos || *epPos || !epSize || *epSize || !epLm || !epLm) {
      throw std::string ("error reading TOC");
    }

    Node & inode = m_toc[fileName];
    inode.pos = pos;
    inode.size = size;
    inode.lastModified = static_cast<std::time_t> (lm);
  }
}

/*
 * ----------------------------------------------------------------------
 * URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::StreamFs::StreamFsReader::
baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
CPI::Util::StreamFs::StreamFsReader::
nameToURI (const std::string & fileName) const
  throw (std::string)
{
  std::string an = absoluteName (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += CPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
CPI::Util::StreamFs::StreamFsReader::
URIToName (const std::string & uri) const
  throw (std::string)
{
  if (uri.length() < m_baseURI.length() ||
      uri.compare (0, m_baseURI.length(), m_baseURI) != 0 ||
      m_baseURI.compare (0, 24, "streamfs://[somestream]/") == 0) {
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
CPI::Util::StreamFs::StreamFsReader::
absoluteNameLocked (const std::string & name) const
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
CPI::Util::StreamFs::StreamFsReader::
cwd () const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);
  return m_cwd;
}

void
CPI::Util::StreamFs::StreamFsReader::
cd (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);
  m_cwd = absoluteName (fileName);
}

void
CPI::Util::StreamFs::StreamFsReader::
mkdir (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}

void
CPI::Util::StreamFs::StreamFsReader::
rmdir (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Iterator *
CPI::Util::StreamFs::StreamFsReader::
list (const std::string & dir, const std::string & pattern)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);
  std::string absDir = absoluteName (dir);
  m_openIterators++;
  return new StreamFsIterator (absDir, pattern, m_toc);
}

void
CPI::Util::StreamFs::StreamFsReader::
closeIterator (CPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  StreamFsIterator * sfi = dynamic_cast<StreamFsIterator *> (it);

  if (!sfi) {
    throw std::string ("invalid iterator");
  }

  delete sfi;

  cpiAssert (m_openIterators > 0);
  m_openIterators--;
}

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::StreamFs::StreamFsReader::
exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  std::string nn = absoluteName (fileName);

  /*
   * See if there is a file by this name
   */

  if (m_toc.find (nn) != m_toc.end ()) {
    if (isDir) {
      *isDir = false;
    }

    return true;
  }

  /*
   * Browse the file list, and see if there is a file with this prefix
   */

  TOC::iterator it;
  std::string::size_type nnlen = nn.length();

  for (it = m_toc.begin(); it != m_toc.end(); it++) {
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
CPI::Util::StreamFs::StreamFsReader::
size (const std::string & fileName)
  throw (std::string)
{
  std::string nn = absoluteName (fileName);
  TOC::iterator it = m_toc.find (nn);

  if (it == m_toc.end()) {
    throw std::string ("file not found");
  }

  return (*it).second.size;
}

std::time_t
CPI::Util::StreamFs::StreamFsReader::
lastModified (const std::string & fileName)
  throw (std::string)
{
  std::string nn = absoluteName (fileName);
  TOC::iterator it = m_toc.find (nn);

  if (it == m_toc.end()) {
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
CPI::Util::StreamFs::StreamFsReader::
open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

std::istream *
CPI::Util::StreamFs::StreamFsReader::
openReadonly (const std::string & fileName,
              std::ios_base::openmode)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  if (m_openFiles) {
    throw std::string ("opening more than one file not supported");
  }

  std::string nn = absoluteNameLocked (fileName);
  TOC::iterator it = m_toc.find (nn);

  if (it == m_toc.end()) {
    throw std::string ("file not found");
  }

  if ((*it).second.pos != m_pos) {
    std::streamsize newpos = CPI::Util::Misc::unsignedToStreamsize ((*it).second.pos);

    if (newpos < 0) {
      throw std::string ("invalid position");
    }

    m_stream->seekg (newpos);

    if (!m_stream->good()) {
      throw std::string ("can not seek file");
    }

    m_pos = (*it).second.pos;
  }

  m_openFiles++;

  return new StreamFsReaderStream (m_stream, (*it).second.pos, (*it).second.size);
}

std::ostream *
CPI::Util::StreamFs::StreamFsReader::
openWriteonly (const std::string &,
               std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

void
CPI::Util::StreamFs::StreamFsReader::
close (std::ios * str)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  StreamFsReaderStream * sfrs = dynamic_cast<StreamFsReaderStream *> (str);

  if (!sfrs) {
    throw std::string ("not opened by this fs");
  }

  /*
   * sfrs most probably has the fail bit set, from an attempt to read
   * beyond the end of file. tellg() doesn't like that.
   */

  sfrs->clear ();
  std::streamsize read = sfrs->tellg ();
  cpiAssert (read >= 0);
  m_pos += read;

  delete sfrs;

  cpiAssert (m_openFiles == 1);
  m_openFiles--;

  if (!m_stream->good()) {
    throw std::string ("input stream is not good");
  }
}

/*
 * ----------------------------------------------------------------------
 * File system operations
 * ----------------------------------------------------------------------
 */

void
CPI::Util::StreamFs::StreamFsReader::
remove (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}
