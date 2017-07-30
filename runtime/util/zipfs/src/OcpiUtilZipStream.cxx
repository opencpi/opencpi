/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Implementation of std::istream and std::ostream for ZIP files.
 *
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  Bugfix: don't pass pointer to uninitialized member to
 *                  std::iostream.
 *
 *     02/03/2009 - Frank Pilhofer
 *                  Use smaller buffer in seekpos().  It is allocated on the
 *                  stack, and VxWorks doesn't have much to spare.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiUtilZipStream.h>
#include <streambuf>
#include <iostream>
#include "zip.h"
#include "unzip.h"

/*
 * ----------------------------------------------------------------------
 *
 * I/O streams that read from or write to ZIP files.
 *
 * ----------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------
 * streambuf for reading from zip files
 * ----------------------------------------------------------------------
 */

OCPI::Util::ZipFs::zipibuf::
zipibuf (unzFile f)
  : m_haveLast (false),
    m_havePending (false),
    m_zipFile (f)
{
}

OCPI::Util::ZipFs::zipibuf::
~zipibuf ()
{
}

bool
OCPI::Util::ZipFs::zipibuf::
setFile (unzFile f)
{
  m_zipFile = f;
  m_haveLast = false;
  m_havePending = false;
  return true;
}

std::streambuf::int_type
OCPI::Util::ZipFs::zipibuf::
underflow ()
{
  if (m_havePending) {
    return m_pending;
  }

  if (!m_zipFile || unzeof (m_zipFile)) {
    return traits_type::eof ();
  }

  char c;
  if (unzReadCurrentFile (m_zipFile, &c, 1) != 1) {
    return traits_type::eof ();
  }

  m_havePending = true;
  return (m_pending = traits_type::to_int_type (c));
}

std::streambuf::int_type
OCPI::Util::ZipFs::zipibuf::
uflow ()
{
  if (m_havePending) {
    m_haveLast = true;
    m_havePending = false;
    return m_pending;
  }

  if (!m_zipFile || unzeof (m_zipFile)) {
    return traits_type::eof ();
  }

  char c;
  if (unzReadCurrentFile (m_zipFile, &c, 1) != 1) {
    return traits_type::eof ();
  }

  m_haveLast = true;
  return (m_pending = traits_type::to_int_type (c));
}

std::streamsize
OCPI::Util::ZipFs::zipibuf::
xsgetn (char * buffer, std::streamsize count)
{
  std::streamsize total = 0;

  if (!count || !m_zipFile) {
    return 0;
  }

  if (m_havePending) {
    m_havePending = false;
    *buffer++ = traits_type::to_char_type (m_pending);
    count--;
    total++;
  }

  while (count && !unzeof (m_zipFile)) {
    int amount = unzReadCurrentFile (m_zipFile, buffer, (unsigned)count);

    if (amount < 0) {
      break;
    }

    count -= amount;
    total += amount;
    buffer += amount;
  }

  if (total > 0) {
    m_haveLast = true;
    m_pending = traits_type::to_int_type (buffer[-1]);
  }

  return total;
}

std::streambuf::int_type
OCPI::Util::ZipFs::zipibuf::
pbackfail (int_type c)
{
  if (m_havePending || !m_zipFile || unzeof (m_zipFile)) {
    return traits_type::eof ();
  }

  if (traits_type::eq_int_type (c, traits_type::eof())) {
    if (!m_haveLast) {
      return traits_type::eof ();
    }
    m_havePending = true;
    return traits_type::not_eof (c);
  }

  m_pending = c;
  m_havePending = true;
  return c;
}

std::streambuf::pos_type
OCPI::Util::ZipFs::zipibuf::
seekoff (off_type off, std::ios_base::seekdir way,
         std::ios_base::openmode mode)
{
  if (!m_zipFile) {
    return static_cast<pos_type> (-1);
  }

  /*
   * Figure out the origin for the offset, according to "way".
   */

  pos_type origin=0;

  switch (way) {
  case std::ios_base::beg:
    origin = 0;
    break;

  case std::ios_base::cur:
    origin = unztell (m_zipFile);
    if (origin > 0 && m_havePending) {
      origin -= static_cast<off_type> (1);
    }
    if (!off) {
      return origin;
    }
    break;

  case std::ios_base::end:
    {
      unz_file_info fileInfo;

      if (unzGetCurrentFileInfo (m_zipFile, &fileInfo, 0, 0, 0, 0, 0, 0) != UNZ_OK) {
        return static_cast<pos_type> (-1);
      }

      origin = static_cast<pos_type> (fileInfo.uncompressed_size);

      if (origin < 0) {
        return static_cast<pos_type> (-1);
      }
      break;

    default:  // get rid of the compiler warnings
      break;
    
    }
  }

  if (off < 0 && -off > origin) {
    return static_cast<pos_type> (-1);
  }

  pos_type ptpos = origin + off;
  return seekpos (ptpos, mode);
}

std::streambuf::pos_type
OCPI::Util::ZipFs::zipibuf::
seekpos (pos_type pos, std::ios_base::openmode)
{
  if (!m_zipFile) {
    return static_cast<pos_type> (-1);
  }

  if (pos < 0) {
    return static_cast<pos_type> (-1);
  }

  pos_type curpos = unztell (m_zipFile);

  if (m_havePending) {
    if (pos == (curpos-static_cast<off_type> (1))) {
      return pos;
    }
    else if (pos == curpos) {
      m_havePending = false;
      return pos;
    }
  }

  if (pos < curpos) {
    /*
     * Re-open the current file, i.e., "seek to the beginning."
     */

    if (unzCloseCurrentFile (m_zipFile) != UNZ_OK) {
      return static_cast<pos_type> (-1);
    }

    if (unzOpenCurrentFile (m_zipFile) != UNZ_OK) {
      return static_cast<pos_type> (-1);
    }

    curpos = 0;
  }

  /*
   * The only way to seek forward is to read all data inbetween.
   */

  if (curpos < pos) {
    char buffer[4096];

    while (curpos < pos) {
      pos_type count = ((pos - curpos) < 4096) ? (pos - curpos) : 4096;
      int amount = unzReadCurrentFile (m_zipFile, buffer, (unsigned)count);

      if (amount < 0) {
        return static_cast<pos_type> (-1);
      }

      curpos += amount;
    }
  }

  m_haveLast = false;
  m_havePending = false;
  return pos;
}

/*
 * ----------------------------------------------------------------------
 * istream for reading from zip files
 * ----------------------------------------------------------------------
 */

OCPI::Util::ZipFs::zipistream::
zipistream (unzFile uzf)
  : std::istream (0),
    m_buf (uzf)
{
  this->init (&m_buf);
  m_zip = uzf;
}

OCPI::Util::ZipFs::zipistream::
~zipistream ()
{
}

/*
 * ----------------------------------------------------------------------
 * streambuf for writing to zip files
 * ----------------------------------------------------------------------
 */

OCPI::Util::ZipFs::zipobuf::
zipobuf (zipFile f)
{
  m_zipFile = f;
}

OCPI::Util::ZipFs::zipobuf::
~zipobuf ()
{
}

bool
OCPI::Util::ZipFs::zipobuf::
setFile (zipFile f)
{
  m_zipFile = f;
  return true;
}

std::streambuf::int_type
OCPI::Util::ZipFs::zipobuf::
overflow (std::streambuf::int_type i)
{
  if (!m_zipFile) {
    return traits_type::eof ();
  }

  if (traits_type::eq_int_type (i, traits_type::eof())) {
    return traits_type::not_eof (i);
  }

  char c = traits_type::to_char_type (i);

  if (zipWriteInFileInZip (m_zipFile, &c, 1) != ZIP_OK) {
    return traits_type::eof ();
  }

  return i;
}

std::streamsize
OCPI::Util::ZipFs::zipobuf::
xsputn (const char * buffer, std::streamsize count)
{
  if (!buffer || !count || !m_zipFile) {
    return 0;
  }

  if (zipWriteInFileInZip (m_zipFile, buffer, (unsigned)count) != ZIP_OK) {
    return 0;
  }

  return count;
}

/*
 * ----------------------------------------------------------------------
 * ostream for writing to zip files
 * ----------------------------------------------------------------------
 */

OCPI::Util::ZipFs::zipostream::
zipostream (zipFile zf)
  : std::ostream (0),
    m_buf (zf)
{
  this->init (&m_buf);
  m_zip = zf;
}

OCPI::Util::ZipFs::zipostream::
~zipostream ()
{
}

