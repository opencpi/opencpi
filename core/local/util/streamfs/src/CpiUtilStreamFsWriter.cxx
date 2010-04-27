/*
 * Aggregate a set of files into a single data stream.
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
#include "CpiUtilStreamFsWriter.h"

/*
 * ----------------------------------------------------------------------
 * StreamFsWriterStream: std::ostream implementation for StreamFs
 * ----------------------------------------------------------------------
 */

namespace CpiUtilStreamFsWriter {

  class StreamFsWriterStream : public std::ostream {
  protected:
    class StreamBuf : public std::streambuf {
    public:
      StreamBuf (std::ostream *);
      ~StreamBuf ();

    protected:
      int sync ();
      pos_type seekoff (off_type off, std::ios_base::seekdir way,
			std::ios_base::openmode which);
      int_type overflow (int_type c);
      std::streamsize xsputn (const char *, std::streamsize);

    protected:
      std::ostream * m_stream;
      unsigned long long m_payload;
    };

  public:
    StreamFsWriterStream (std::ostream *, const std::string & name);
    ~StreamFsWriterStream ();

    const std::string & fileName () const;

  protected:
    StreamBuf m_buf;
    std::string m_name;
  };

  StreamFsWriterStream::StreamBuf::
  StreamBuf (std::ostream * str)
    : m_stream (str),
      m_payload (0)
  {
  }

  StreamFsWriterStream::StreamBuf::
  ~StreamBuf ()
  {
  }

  int
  StreamFsWriterStream::StreamBuf::
  sync ()
  {
    m_stream->flush ();
    return m_stream->good() ? 0 : -1;
  }

  std::streambuf::pos_type
  StreamFsWriterStream::StreamBuf::
  seekoff (off_type off, std::ios_base::seekdir way,
	   std::ios_base::openmode)
  {
    if (way != std::ios_base::cur || off != 0) {
      return -1;
    }

    return CPI::Util::Misc::unsignedToStreamsize (m_payload);
  }

  std::streambuf::int_type
  StreamFsWriterStream::StreamBuf::
  overflow (int_type i)
  {
    if (traits_type::eq_int_type (i, traits_type::eof())) {
      return traits_type::not_eof (i);
    }

    m_stream->put (i);

    if (m_stream->fail()) {
      return traits_type::eof ();
    }

    m_payload++;
    return i;
  }

  std::streamsize
  StreamFsWriterStream::StreamBuf::
  xsputn (const char * data, std::streamsize count)
  {
    if (count <= 0) {
      return 0;
    }

    m_stream->write (data, count);

    if (m_stream->fail()) {
      return 0;
    }

    m_payload += count;
    return count;
  }

  StreamFsWriterStream::StreamFsWriterStream (std::ostream * str,
					      const std::string & name)
    : std::ostream (0),
      m_buf (str),
      m_name (name)
  {
    this->init (&m_buf);
  }

  StreamFsWriterStream::~StreamFsWriterStream ()
  {
  }

  const std::string &
  StreamFsWriterStream::fileName () const
  {
    return m_name;
  }

}

using namespace CpiUtilStreamFsWriter;

/*
 * ----------------------------------------------------------------------
 * StreamFsWriter
 * ----------------------------------------------------------------------
 */

CPI::Util::StreamFs::StreamFsWriter::
StreamFsWriter ()
  throw ()
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
}

CPI::Util::StreamFs::StreamFsWriter::
StreamFsWriter (std::ostream * stream)
  throw (std::string)
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
  openFs (stream);
}

CPI::Util::StreamFs::StreamFsWriter::
StreamFsWriter (Vfs * fs, const std::string & name)
  throw (std::string)
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
  openFs (fs, name);
}

CPI::Util::StreamFs::StreamFsWriter::
~StreamFsWriter ()
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
CPI::Util::StreamFs::StreamFsWriter::
openFs (std::ostream * stream)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  if (m_stream) {
    throw std::string ("already open");
  }

  m_stream = stream;
  m_fs = 0;
  m_cwd = "/";
  m_pos = 0;
  m_openFiles = 0;
  m_baseURI = "streamfs://[somestream]/";
}

void
CPI::Util::StreamFs::StreamFsWriter::
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
  m_pos = 0;
  m_openFiles = 0;

  std::string authority = fs->nameToURI (name);
  m_baseURI  = "streamfs://";
  m_baseURI += CPI::Util::Uri::encode (authority);
  m_baseURI += "/";

  m_stream = m_fs->openWriteonly (m_name, std::ios_base::binary | std::ios_base::trunc);
}

void
CPI::Util::StreamFs::StreamFsWriter::
closeFs ()
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  cpiAssert (!m_openFiles);

  if (m_stream) {
    std::ostream * omstream = m_stream;

    try {
      dumpTOC ();
    }
    catch (...) {
      m_stream = 0;

      if (m_fs) {
	try {
	  m_fs->close (omstream);
	}
	catch (...) {
	}
      }

      throw;
    }

    m_stream = 0;

    if (m_fs) {
      m_fs->close (omstream);
    }
  }
}

/*
 * ----------------------------------------------------------------------
 * Write a Table of Contents at the end of the stream
 * ----------------------------------------------------------------------
 *
 * Four text lines are written for every file:
 * - The absolute file name.
 * - The position of the file's first octet in the stream.
 * - The size of the file, in octets.
 * - The last modification timestamp.
 *
 * At the very end, in the last 17 octets of the stream, the position
 * of the table of contents is written, with a final LF.
 *
 * Thus, a reader can seek to the end minus 17 octets, read the TOC's
 * position, then seek to the TOC, read the TOC, and then start reading
 * files.
 */

void
CPI::Util::StreamFs::StreamFsWriter::
dumpTOC ()
  throw (std::string)
{
  /*
   * Current position, i.e., the position of the TOC, is in m_pos.
   */

  for (TOC::iterator it=m_toc.begin(); it!=m_toc.end() && m_stream->good(); it++) {
    std::string pos =
      CPI::Util::Misc::unsignedToString ((*it).second.pos);
    std::string size =
      CPI::Util::Misc::unsignedToString ((*it).second.size);
    std::string lm =
      CPI::Util::Misc::unsignedToString (static_cast<unsigned long long> ((*it).second.lastModified));

    m_stream->put ('\n');
    m_stream->write ((*it).first.data(), (*it).first.length());
    m_stream->put ('\n');
    m_stream->write (pos.data(), pos.length());
    m_stream->put ('\n');
    m_stream->write (size.data(), size.length());
    m_stream->put ('\n');
    m_stream->write (lm.data(), lm.length());
  }

  std::string tp =
    CPI::Util::Misc::unsignedToString (m_pos, 10, 16, ' ');

  m_stream->write ("\n<End Of TOC>\n", 14);
  m_stream->write (tp.data(), tp.length());
  m_stream->put ('\n');

  if (!m_stream->good()) {
    throw std::string ("output stream not good");
  }
}

/*
 * ----------------------------------------------------------------------
 * URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::StreamFs::StreamFsWriter::
baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
CPI::Util::StreamFs::StreamFsWriter::
nameToURI (const std::string & fileName) const
  throw (std::string)
{
  std::string an = absoluteName (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += CPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
CPI::Util::StreamFs::StreamFsWriter::
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
CPI::Util::StreamFs::StreamFsWriter::
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
CPI::Util::StreamFs::StreamFsWriter::
cwd () const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);
  return m_cwd;
}

void
CPI::Util::StreamFs::StreamFsWriter::
cd (const std::string & fileName)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);
  m_cwd = absoluteName (fileName);
}

void
CPI::Util::StreamFs::StreamFsWriter::
mkdir (const std::string &)
  throw (std::string)
{
  /* no-op */
}

void
CPI::Util::StreamFs::StreamFsWriter::
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
CPI::Util::StreamFs::StreamFsWriter::
list (const std::string &, const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}

void
CPI::Util::StreamFs::StreamFsWriter::
closeIterator (CPI::Util::Vfs::Iterator *)
  throw (std::string)
{
  cpiAssert (0);
}

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::StreamFs::StreamFsWriter::
exists (const std::string &, bool *)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return false;
}

unsigned long long
CPI::Util::StreamFs::StreamFsWriter::
size (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

std::time_t
CPI::Util::StreamFs::StreamFsWriter::
lastModified (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
CPI::Util::StreamFs::StreamFsWriter::
open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

std::istream *
CPI::Util::StreamFs::StreamFsWriter::
openReadonly (const std::string &,
	      std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

std::ostream *
CPI::Util::StreamFs::StreamFsWriter::
openWriteonly (const std::string & fileName,
	       std::ios_base::openmode)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  if (m_openFiles) {
    throw std::string ("opening more than one file not supported");
  }

  if (!m_stream->good()) {
    throw std::string ("output stream is not good");
  }

  std::string nn = absoluteNameLocked (fileName);
  TOC::iterator it = m_toc.find (nn);

  if (it != m_toc.end()) {
    throw std::string ("file exists");
  }

  Node & inode = m_toc[nn];
  inode.pos = m_pos;
  m_openFiles++;

  return new StreamFsWriterStream (m_stream, nn);
}

void
CPI::Util::StreamFs::StreamFsWriter::
close (std::ios * str)
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_mutex);

  StreamFsWriterStream * sfws = dynamic_cast<StreamFsWriterStream *> (str);

  if (!sfws) {
    throw std::string ("not opened by this fs");
  }

  std::string fileName = sfws->fileName ();
  TOC::iterator it = m_toc.find (fileName);
  cpiAssert (it != m_toc.end());

  std::streamsize fileSize = sfws->tellp ();
  cpiAssert (fileSize >= 0);

  delete sfws;

  if (!m_stream->good()) {
    m_toc.erase (it);
    throw std::string ("output stream is not good");
  }

  m_pos += fileSize;
  (*it).second.size = fileSize;
  (*it).second.lastModified = std::time (0);

  cpiAssert (m_openFiles == 1);
  m_openFiles--;
}

/*
 * ----------------------------------------------------------------------
 * File system operations
 * ----------------------------------------------------------------------
 */

void
CPI::Util::StreamFs::StreamFsWriter::
remove (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}

