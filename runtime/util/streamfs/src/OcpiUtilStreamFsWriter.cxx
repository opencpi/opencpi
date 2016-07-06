
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
#include <OcpiOsAssert.h>
#include <OcpiOsMutex.h>
#include <OcpiUtilUri.h>
#include <OcpiUtilMisc.h>
#include <OcpiUtilAutoMutex.h>
#include "OcpiUtilVfs.h"
#include "OcpiUtilVfsIterator.h"
#include "OcpiUtilStreamFsWriter.h"

/*
 * ----------------------------------------------------------------------
 * StreamFsWriterStream: std::ostream implementation for StreamFs
 * ----------------------------------------------------------------------
 */

namespace OcpiUtilStreamFsWriter {

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

    return OCPI::Util::unsignedToStreamsize (m_payload);
  }

  std::streambuf::int_type
  StreamFsWriterStream::StreamBuf::
  overflow (int_type i)
  {
    if (traits_type::eq_int_type (i, traits_type::eof())) {
      return traits_type::not_eof (i);
    }

    m_stream->put ((traits_type::char_type)i);

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

using namespace OcpiUtilStreamFsWriter;

/*
 * ----------------------------------------------------------------------
 * StreamFsWriter
 * ----------------------------------------------------------------------
 */

OCPI::Util::StreamFs::StreamFsWriter::
StreamFsWriter ()
  throw ()
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
}

OCPI::Util::StreamFs::StreamFsWriter::
StreamFsWriter (std::ostream * stream)
  throw (std::string)
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
  openFs (stream);
}

OCPI::Util::StreamFs::StreamFsWriter::
StreamFsWriter (Vfs * fs, const std::string & name)
  throw (std::string)
  : m_fs (0),
    m_stream (0),
    m_cwd ("/")
{
  openFs (fs, name);
}

OCPI::Util::StreamFs::StreamFsWriter::
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
OCPI::Util::StreamFs::StreamFsWriter::
openFs (std::ostream * stream)
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_mutex);

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
OCPI::Util::StreamFs::StreamFsWriter::
openFs (Vfs * fs, const std::string & name)
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_mutex);

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
  m_baseURI += OCPI::Util::Uri::encode (authority);
  m_baseURI += "/";

  m_stream = m_fs->openWriteonly (m_name, std::ios_base::binary | std::ios_base::trunc);
}

void
OCPI::Util::StreamFs::StreamFsWriter::
closeFs ()
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_mutex);

  ocpiAssert (!m_openFiles);

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
OCPI::Util::StreamFs::StreamFsWriter::
dumpTOC ()
  throw (std::string)
{
  /*
   * Current position, i.e., the position of the TOC, is in m_pos.
   */

  for (TOC::iterator it=m_toc.begin(); it!=m_toc.end() && m_stream->good(); it++) {
    std::string pos =
      OCPI::Util::unsignedToString ((*it).second.pos);
    std::string sizeStr =
      OCPI::Util::unsignedToString ((*it).second.size);
    std::string lm =
      OCPI::Util::unsignedToString (static_cast<unsigned long long> ((*it).second.lastModified));

    m_stream->put ('\n');
    m_stream->write ((*it).first.data(), (*it).first.length());
    m_stream->put ('\n');
    m_stream->write (pos.data(), pos.length());
    m_stream->put ('\n');
    m_stream->write (sizeStr.data(), sizeStr.length());
    m_stream->put ('\n');
    m_stream->write (lm.data(), lm.length());
  }

  std::string tp =
    OCPI::Util::unsignedToString (m_pos, 10, 16, ' ');

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
OCPI::Util::StreamFs::StreamFsWriter::
baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
OCPI::Util::StreamFs::StreamFsWriter::
nameToURI (const std::string & fileName) const
  throw (std::string)
{
  std::string an = absoluteName (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += OCPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
OCPI::Util::StreamFs::StreamFsWriter::
URIToName (const std::string & uri) const
  throw (std::string)
{
  if (uri.length() < m_baseURI.length() ||
      uri.compare (0, m_baseURI.length(), m_baseURI) != 0 ||
      m_baseURI.compare (0, 24, "streamfs://[somestream]/") == 0) {
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
OCPI::Util::StreamFs::StreamFsWriter::
absoluteNameLocked (const std::string & name) const
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
OCPI::Util::StreamFs::StreamFsWriter::
cwd () const
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  return m_cwd;
}

void
OCPI::Util::StreamFs::StreamFsWriter::
cd (const std::string & fileName)
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_mutex);
  m_cwd = absoluteName (fileName);
}

void
OCPI::Util::StreamFs::StreamFsWriter::
mkdir (const std::string &)
  throw (std::string)
{
  /* no-op */
}

void
OCPI::Util::StreamFs::StreamFsWriter::
rmdir (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}

#if 0
/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

OCPI::Util::Vfs::Iterator *
OCPI::Util::StreamFs::StreamFsWriter::
list (const std::string &, const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}

void
OCPI::Util::StreamFs::StreamFsWriter::
closeIterator (OCPI::Util::Vfs::Iterator *)
  throw (std::string)
{
  ocpiAssert (0);
}
#endif
/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::StreamFs::StreamFsWriter::
exists (const std::string &, bool *)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return false;
}

unsigned long long
OCPI::Util::StreamFs::StreamFsWriter::
size (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

std::time_t
OCPI::Util::StreamFs::StreamFsWriter::
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
OCPI::Util::StreamFs::StreamFsWriter::
open (const std::string &, std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

std::istream *
OCPI::Util::StreamFs::StreamFsWriter::
openReadonly (const std::string &,
              std::ios_base::openmode)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
  return 0;
}

std::ostream *
OCPI::Util::StreamFs::StreamFsWriter::
openWriteonly (const std::string & fileName,
               std::ios_base::openmode)
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_mutex);

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
OCPI::Util::StreamFs::StreamFsWriter::
close (std::ios * str)
  throw (std::string)
{
  OCPI::Util::AutoMutex lock (m_mutex);

  StreamFsWriterStream * sfws = dynamic_cast<StreamFsWriterStream *> (str);

  if (!sfws) {
    throw std::string ("not opened by this fs");
  }

  std::string fileName = sfws->fileName ();
  TOC::iterator it = m_toc.find (fileName);
  ocpiAssert (it != m_toc.end());

  std::streamoff fileSize = sfws->tellp ();
  ocpiAssert (fileSize >= 0);

  delete sfws;

  if (!m_stream->good()) {
    m_toc.erase (it);
    throw std::string ("output stream is not good");
  }

  m_pos += fileSize;
  (*it).second.size = fileSize;
  (*it).second.lastModified = std::time (0);

  ocpiAssert (m_openFiles == 1);
  m_openFiles--;
}

/*
 * ----------------------------------------------------------------------
 * File system operations
 * ----------------------------------------------------------------------
 */

void
OCPI::Util::StreamFs::StreamFsWriter::
remove (const std::string &)
  throw (std::string)
{
  throw std::string ("not supported on this file system");
}

