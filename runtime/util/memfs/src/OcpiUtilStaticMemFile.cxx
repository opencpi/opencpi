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
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  GCC bug 40391 workaround.
 *
 *     11/05/2008 - Frank Pilhofer
 *                  Avoid compiler warning.
 *
 *     10/17/2008 - Frank Pilhofer
 *                  64 bit fixes.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiUtilStaticMemFile.h>
#include <OcpiUtilMemFileChunk.h>
#include <OcpiUtilMisc.h>
#include <streambuf>
#include <istream>
#include <ctime>

/*
 * ----------------------------------------------------------------------
 *
 * MemIStream, an implementation of std::istream that accesses a specific
 * chunk of memory, e.g. MemIStream ("hello world", 12);
 *
 * ----------------------------------------------------------------------
 */

namespace OcpiUtilStaticMemFile {
  class MemIStream : public std::istream {
  protected:
    class MemIStreamBuf : public std::streambuf {
    public:
      MemIStreamBuf (const char *, unsigned long long);
      MemIStreamBuf (const OCPI::Util::MemFs::MemFileChunk *);
      ~MemIStreamBuf ();

    protected:
      int_type underflow_common (bool);
      int_type underflow ();
      int_type uflow ();
      std::streamsize showmanyc ();
      std::streamsize xsgetn (char *, std::streamsize);
      int_type pbackfail (int_type = std::streambuf::traits_type::eof());
      pos_type seekoff (off_type, std::ios_base::seekdir, std::ios_base::openmode = std::ios_base::in | std::ios_base::out);
      pos_type seekpos (pos_type, std::ios_base::openmode = std::ios_base::in | std::ios_base::out);

    protected:
      const char * m_ptr;
      unsigned long long m_size;
      const OCPI::Util::MemFs::MemFileChunk * m_chunk;
      const OCPI::Util::MemFs::MemFileChunk * m_chunks;
      unsigned long long m_firstByteInChunk;

    private:
      MemIStreamBuf (const MemIStreamBuf &);
      MemIStreamBuf & operator= (const MemIStreamBuf &);
    };

  public:
    MemIStream (const char *, unsigned long long);
    MemIStream (const OCPI::Util::MemFs::MemFileChunk *);
    ~MemIStream ();

  protected:
    MemIStreamBuf m_buf;

  private:
    MemIStream (const MemIStream &);
    MemIStream & operator= (const MemIStream &);
  };

}

using namespace OcpiUtilStaticMemFile;

MemIStream::MemIStreamBuf::MemIStreamBuf (const char * ptr,
                                          unsigned long long size)
  : m_ptr (ptr),
    m_size (size),
    m_chunks (0)
{
  char * ncptr = const_cast<char *> (m_ptr);
  setg (ncptr, ncptr, ncptr + m_size);
}

MemIStream::MemIStreamBuf::MemIStreamBuf (const OCPI::Util::MemFs::MemFileChunk * chunks)
  : m_ptr (0),
    m_size (0),
    m_chunk (chunks),
    m_chunks (chunks),
    m_firstByteInChunk (0)
{
  char * ncptr = const_cast<char *> (m_chunk->ptr);
  setg (ncptr, ncptr, ncptr + m_chunk->size);
}

MemIStream::MemIStreamBuf::~MemIStreamBuf ()
{
}

std::streambuf::int_type
MemIStream::MemIStreamBuf::underflow_common (bool bump)
{
  unsigned long long curpos = gptr() - eback();
  std::streambuf::int_type res;

  if (m_ptr) {
    if (curpos >= m_size) {
      return traits_type::eof ();
    }

    res = traits_type::to_int_type (m_ptr[curpos]);

    if (bump) {
      curpos++;
    }

    char * ncptr = const_cast<char *> (m_ptr);
    setg (ncptr, ncptr + curpos, ncptr + m_size);
  }
  else {
    if (curpos >= m_chunk->size) {
      if (m_chunk->size) {
        m_firstByteInChunk += m_chunk->size;
        m_chunk++;
        curpos = 0;
      }
      if (!m_chunk->size) {
        return traits_type::eof ();
      }
    }

    res = traits_type::to_int_type (m_chunk->ptr[curpos]);

    if (bump) {
      curpos++;
    }

    char * ncptr = const_cast<char *> (m_chunk->ptr);
    setg (ncptr, ncptr + curpos, ncptr + m_chunk->size);
  }

  return res;
}

std::streambuf::int_type
MemIStream::MemIStreamBuf::underflow ()
{
  return underflow_common (0);
}

std::streambuf::int_type
MemIStream::MemIStreamBuf::uflow ()
{
  return underflow_common (1);
}

std::streamsize
MemIStream::MemIStreamBuf::showmanyc ()
{
  return egptr() - gptr();
}

std::streamsize
MemIStream::MemIStreamBuf::xsgetn (char * s, std::streamsize n)
{
  std::streamsize count, remaining, total;
  char * ptr;

  remaining = n;
  total = 0;

  while (remaining) {
    /*
     * If the readable area is greater than zero, use it. Otherwise
     * ask underflow to adjust it.
     */

    if (!(ptr = gptr()) || (count = egptr() - gptr()) == 0) {
      if (traits_type::eq_int_type (underflow(), traits_type::eof())) {
        return total;
      }
      ptr = gptr();
      count = egptr() - ptr;
    }

    if (count > remaining) {
      count = remaining;
    }

    traits_type::copy (s, ptr, count);

    gbump ((int)count);
    remaining -= count;
    total += count;
    s += count;
  }

  return total;
}

std::streambuf::int_type
MemIStream::MemIStreamBuf::pbackfail (int_type c)
{
  if (m_ptr) {
    /*
     * We have a buffer. The only reason why pbackfail() should be called
     * is if c!=*gptr(). We don't allow that.
     */
    return traits_type::eof ();
  }
  else {
    if (gptr() > eback()) {
      /*
       * We are in the middle of a buffer. The only reason why
       * pbackfail() should be called is if c!=*gptr(). We don't
       * allow that.
       */
      return traits_type::eof ();
    }

    if (m_chunk == m_chunks) {
      return traits_type::eof ();
    }

    const OCPI::Util::MemFs::MemFileChunk * prevChunk = m_chunk - 1;
    unsigned long long curpos = m_chunk->size - 1;

    if (prevChunk->size == 0) {
      return traits_type::eof ();
    }

    if (traits_type::eq_int_type (c, traits_type::to_int_type (prevChunk->ptr[curpos]))) {
      return traits_type::eof ();
    }

    m_chunk = prevChunk;
    m_firstByteInChunk -= m_chunk->size;

    char * ncptr = const_cast<char *> (m_chunk->ptr);
    setg (ncptr, ncptr + curpos, ncptr + m_chunk->size);
  }

  return traits_type::not_eof (c);
}

std::streambuf::pos_type
MemIStream::MemIStreamBuf::seekoff (off_type off, std::ios_base::seekdir way,
                                    std::ios_base::openmode mode)
{
  if (!gptr() || !egptr() || !eback()) {
    return static_cast<pos_type> (-1);
  }

  unsigned long long origin = 0;

  switch (way) {
  case std::ios_base::beg:
    origin = 0;
    break;

  case std::ios_base::cur:
    if (m_ptr) {
      origin = gptr() - eback();
    }
    else {
      origin = m_firstByteInChunk + (gptr() - eback());
    }
    break;

  case std::ios_base::end:
    if (m_ptr) {
      origin = m_size;
    }
    else {
      const OCPI::Util::MemFs::MemFileChunk * cp = m_chunk;
      origin = m_firstByteInChunk;
      while (cp->size) {
        origin += (cp++)->size;
      }
    }
    break;

  default:  // get rid of the compiler warnings
    break;
  }

  if (off < 0 && (static_cast<unsigned long long> (-off) > origin)) {
    return static_cast<pos_type> (-1);
  }

  unsigned long long newpos = origin + off;
  pos_type ptpos = OCPI::Util::unsignedToStreamsize (newpos);

  if (ptpos == static_cast<pos_type> (-1)) {
    return ptpos;
  }

  return seekpos (ptpos, mode);
}

std::streambuf::pos_type
MemIStream::MemIStreamBuf::seekpos (pos_type ptpos, std::ios_base::openmode)
{
  if (ptpos < 0) {
    return static_cast<pos_type> (-1);
  }

  unsigned long long pos = ptpos;

  if (m_ptr) {
    if (pos > m_size) {
      return static_cast<pos_type> (-1);
    }

    char * ncptr = const_cast<char *> (m_ptr);
    setg (ncptr, ncptr + pos, ncptr + m_size);
  }
  else {
    if (pos < m_firstByteInChunk) {
      m_chunk = m_chunks;
      m_firstByteInChunk = 0;
    }

    while (pos >= m_firstByteInChunk + m_chunk->size && m_chunk->size) {
      m_firstByteInChunk += m_chunk->size;
      m_chunk++;
    }

    if (pos > m_firstByteInChunk + m_chunk->size) {
      return static_cast<pos_type> (-1);
    }

    unsigned long long curpos = pos - m_firstByteInChunk;

    char * ncptr = const_cast<char *> (m_chunk->ptr);
    setg (ncptr, ncptr + curpos, ncptr + m_chunk->size);
  }

  return ptpos;
}

MemIStream::MemIStream (const char * ptr,
                        unsigned long long size)
  : std::istream (0),
    m_buf (ptr, size)
{
  this->init (&m_buf);
}

MemIStream::MemIStream (const OCPI::Util::MemFs::MemFileChunk * chunks)
  : std::istream (0),
    m_buf (chunks)
{
  this->init (&m_buf);
}

MemIStream::~MemIStream ()
{
}

/*
 * ----------------------------------------------------------------------
 * StaticMemFile
 * ----------------------------------------------------------------------
 */

OCPI::Util::MemFs::StaticMemFile::
StaticMemFile (const char * ptr,
               unsigned long long a_size,
               std::time_t a_lastModified)
  throw ()
  : m_size (a_size),
    m_lastModified (a_lastModified),
    m_ptr (ptr),
    m_chunks (0)
{
}

OCPI::Util::MemFs::StaticMemFile::
StaticMemFile (const OCPI::Util::MemFs::MemFileChunk * chunks,
               std::time_t a_lastModified)
  throw ()
  : m_lastModified (a_lastModified),
    m_ptr (0),
    m_chunks (chunks)
{
}

OCPI::Util::MemFs::StaticMemFile::
~StaticMemFile ()
  throw ()
{
}

unsigned long long
OCPI::Util::MemFs::StaticMemFile::size ()
  throw ()
{
  unsigned long long res;

  if (m_ptr) {
    res = m_size;
  }
  else {
    const OCPI::Util::MemFs::MemFileChunk * chunk = m_chunks;
    res = 0;
    while (chunk->size) {
      res += chunk++->size;
    }
  }

  return res;
}

std::time_t
OCPI::Util::MemFs::StaticMemFile::lastModified ()
  throw ()
{
  return m_lastModified;
}

std::istream *
OCPI::Util::MemFs::StaticMemFile::openReadonly ()
  throw ()
{
  std::istream * str;

  if (m_ptr) {
    str = new MemIStream (m_ptr, m_size);
  }
  else {
    str = new MemIStream (m_chunks);
  }

  return str;
}

void
OCPI::Util::MemFs::StaticMemFile::close (std::ios * str)
  throw ()
{
  delete str;
}

