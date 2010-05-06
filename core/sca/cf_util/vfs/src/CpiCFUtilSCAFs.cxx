/*
 * Vfs implementation using an SCA FileSystem.
 *
 * Revision History:
 *
 *     06/10/2009 - Frank Pilhofer
 *                  GCC bug 40391 workaround.
 *
 *     10/17/2008 - Frank Pilhofer
 *                  64 bit fixes.
 *
 *     10/06/2008 - Frank Pilhofer
 *                  Fix seekg and seekp.
 *
 *     07/09/2008 - Frank Pilhofer
 *                  Bugfixes.
 *
 *     06/30/2008 - Frank Pilhofer
 *                  Initial version.
 */

#include <ctime>
#include <string>
#include <iostream>
#include <CpiOsAssert.h>
#include <CpiOsMutex.h>
#include <CpiUtilUri.h>
#include <CpiUtilVfs.h>
#include <CpiUtilMisc.h>
#include <CpiUtilAutoMutex.h>
#include <CpiUtilVfsIterator.h>
#include <CpiStringifyCorbaException.h>
#include <CpiCFUtilStringifyCFException.h>
#include <CF.h>
#include "CpiCFUtilSCAFs.h"

/*
 * ----------------------------------------------------------------------
 * Reading from or writing to a CF::File using the iostream interface.
 * ----------------------------------------------------------------------
 */

namespace CpiCFUtilSCAFs {
  class SCAFileStream : public std::iostream {
  public:
    class Buf : public std::streambuf {
    public:
      enum {
        BufferSize = 4096
      };

    public:
      Buf (CF::File_ptr file, std::ios_base::openmode mode);
      ~Buf ();

      int close ();

    protected:
      int_type underflow_common (bool);
      int_type underflow ();
      int_type uflow ();
      std::streamsize showmanyc ();
      std::streamsize xsgetn (char *, std::streamsize);
      int_type pbackfail (int_type = std::streambuf::traits_type::eof());

      int sync ();
      std::streamsize xsputn (const char *, std::streamsize);
      int_type overflow (int_type = std::streambuf::traits_type::eof());

      pos_type seekoff (off_type, std::ios_base::seekdir, std::ios_base::openmode = std::ios_base::in | std::ios_base::out);
      pos_type seekpos (pos_type, std::ios_base::openmode = std::ios_base::in | std::ios_base::out);

    protected:
      CF::File_var m_file;
      bool m_bufferInUse;
      CF::OctetSequence_var m_buffer;
      std::ios_base::openmode m_mode;
      pos_type m_bufferBeginPos;
      int_type m_putback;
    };

  public:
    SCAFileStream (CF::File_ptr file, std::ios_base::openmode mode)
      throw ();

    ~SCAFileStream ()
      throw ();

    int close ()
      throw ();

  protected:
    Buf m_buf;
  };
}

using namespace CpiCFUtilSCAFs;

SCAFileStream::
SCAFileStream (CF::File_ptr file, std::ios_base::openmode mode)
  throw ()
  : std::iostream (0),
    m_buf (file, mode)
{
  this->init (&m_buf);
}

SCAFileStream::
~SCAFileStream ()
  throw ()
{
}

int
SCAFileStream::
close ()
  throw ()
{
  return m_buf.close ();
}

SCAFileStream::Buf::
Buf (CF::File_ptr file, std::ios_base::openmode mode)
  : m_file (file),
    m_bufferInUse (false),
    m_mode (mode),
    m_bufferBeginPos (0),
    m_putback (traits_type::eof())
{
}

SCAFileStream::Buf::
~Buf ()
{
  sync ();
}

int
SCAFileStream::Buf::
close ()
{
  bool good = (sync () == 0);

  try {
    m_file->close ();
  }
  catch (...) {
    good = false;
  }

  return good ? 0 : -1;
}

std::streambuf::int_type
SCAFileStream::Buf::
underflow_common (bool bump)
{
  std::streambuf::int_type res;

  if (!traits_type::eq_int_type (m_putback, traits_type::eof())) {
    res = m_putback;

    if (bump) {
      m_putback = traits_type::eof ();

      /* Restore the read buffer */
      char * buf = reinterpret_cast<char *> (m_buffer->get_buffer ());
      CORBA::ULong bl = m_buffer->length ();
      setg (buf, buf, buf+bl);
    }

    return res;
  }

  cpiAssert (!gptr() || gptr() >= egptr());

  if (gptr()) {
    m_bufferBeginPos += m_buffer->length ();
  }
  else {
    if (sync() != 0) {
      setg (0, 0, 0);
      return traits_type::eof ();
    }
  }

  cpiAssert (!pptr());

  try {
    m_file->read (m_buffer.out(), BufferSize);
  }
  catch (...) {
    m_bufferInUse = false;
    setg (0, 0, 0);
    return traits_type::eof ();
  }

  m_bufferInUse = true;
  char * buf = reinterpret_cast<char *> (m_buffer->get_buffer ());
  CORBA::ULong bl = m_buffer->length ();

  if (!bl) {
    return traits_type::eof ();
  }

  res = traits_type::to_int_type (*buf);
  setg (buf, bump ? (buf+1) : buf, buf+bl);
  return res;
}

std::streambuf::int_type
SCAFileStream::Buf::
underflow ()
{
  return underflow_common (0);
}

std::streambuf::int_type
SCAFileStream::Buf::
uflow ()
{
  return underflow_common (1);
}

std::streamsize
SCAFileStream::Buf::
showmanyc ()
{
  int res = egptr() - gptr();

  if (!traits_type::eq_int_type (m_putback, traits_type::eof())) {
    res++;
  }

  return res;
}

std::streamsize
SCAFileStream::Buf::
xsgetn (char * s, std::streamsize n)
{
  std::streamsize count, remaining, total;
  char * ptr;

  remaining = n;
  total = 0;

  if (n && !traits_type::eq_int_type (m_putback, traits_type::eof())) {
    *s++ = traits_type::to_char_type (underflow_common(1));
    remaining--;
    total++;
  }

  while (remaining) {
    /*
     * If the readable area is greater than zero, use it. Otherwise
     * ask underflow to fix it.
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

    gbump (count);
    remaining -= count;
    total += count;
    s += count;
  }

  return total;
}

std::streambuf::int_type
SCAFileStream::Buf::
pbackfail (int_type c)
{
  if (!gptr()) {
    return traits_type::eof ();
  }

  if (gptr() > eback()) {
    return traits_type::eof ();
  }

  if (!m_bufferBeginPos) {
    return traits_type::eof ();
  }

  m_putback = c;
  setg (0, 0, 0);
  return traits_type::not_eof (c);
}

int
SCAFileStream::Buf::
sync ()
{
  if (pptr() && pptr() > pbase()) {
    unsigned int count = pptr() - pbase();

    cpiAssert (!gptr());
    cpiAssert (m_bufferInUse);
    cpiAssert (count <= m_buffer->length());

    if (count) {
      m_buffer->length (count);

      try {
        m_file->write (m_buffer.in());
      }
      catch (...) {
        return -1;
      }

      m_bufferBeginPos += count;
    }

    setp (0, 0);
  }
  else if (gptr()) {
    unsigned int count = gptr() - eback();
    m_bufferBeginPos += count;

    if (!traits_type::eq_int_type (m_putback, traits_type::eof())) {
      m_putback = traits_type::eof ();
      m_bufferBeginPos = m_bufferBeginPos - static_cast<off_type> (1);
    }

    setg (0, 0, 0);
  }

  return 0;
}

std::streambuf::int_type
SCAFileStream::Buf::
overflow (int_type c)
{
  cpiAssert (!pptr() || static_cast<unsigned int> (pptr() - pbase()) >= m_buffer->length());

  if (!((m_mode & std::ios_base::out) == std::ios_base::out)) {
    return traits_type::eof ();
  }

  if (sync() != 0) {
    return traits_type::eof ();
  }

  if (!m_bufferInUse) {
    m_bufferInUse = true;
    m_buffer = new CF::OctetSequence (BufferSize);
  }

  m_buffer->length (BufferSize);
  char * buf = reinterpret_cast<char *> (m_buffer->get_buffer ());
  setp (buf, buf+BufferSize);

  if (!traits_type::eq_int_type (c, traits_type::eof())) {
    *buf = traits_type::to_char_type (c);
    pbump (1);
  }

  return traits_type::not_eof (c);
}

std::streamsize
SCAFileStream::Buf::
xsputn (const char * s, std::streamsize n)
{
  std::streamsize count, remaining, total;
  char * ptr;

  remaining = n;
  total = 0;

  while (remaining) {
    /*
     * If the writable area is greater than zero, use it.  Otherwise
     * ask overflow to adjust it.
     */

    if (!(ptr = pptr()) || (count = epptr() - pptr()) == 0) {
      if (traits_type::eq_int_type (overflow(traits_type::eof()), traits_type::eof())) {
        return total;
      }

      ptr = pptr();
      count = epptr() - ptr;
    }

    if (count > remaining) {
      count = remaining;
    }

    traits_type::copy (ptr, s, count);

    pbump (count);
    remaining -= count;
    total += count;
    s += count;
  }

  return total;
}

std::streambuf::pos_type
SCAFileStream::Buf::
seekoff (off_type off, std::ios_base::seekdir way, std::ios_base::openmode mode)
{
  if (sync() != 0) {
    return static_cast<pos_type> (-1);
  }

  pos_type origin;

  switch (way) {
  case std::ios_base::beg:
    origin = 0;
    break;

  case std::ios_base::cur:
    origin = m_bufferBeginPos;
    if (gptr()) {
      origin += static_cast <off_type> (gptr() - eback());
    }
    break;

  case std::ios_base::end:
    try {
      origin = m_file->sizeOf ();
    }
    catch (...) {
      return static_cast<pos_type> (-1);
    }
    break;

  default: // Get rid of compiler warning
    break;
  }

  if (off < 0 && static_cast<pos_type> ((-off) > origin)) {
    return static_cast<pos_type> (-1);
  }

  pos_type newpos = origin + off;
  return seekpos (newpos, mode);
}

std::streambuf::pos_type
SCAFileStream::Buf::
seekpos (pos_type pos, std::ios_base::openmode)
{
  if (pos < 0) {
    return static_cast<pos_type> (-1);
  }

  if (!gptr() && !pptr()) {
    if (pos == m_bufferBeginPos) {
      return pos;
    }
  }

  if (gptr()) {
    pos_type curpos = m_bufferBeginPos + static_cast <off_type> (gptr() - eback());
    pos_type endpos = m_bufferBeginPos + static_cast <off_type> (egptr() - eback());

    if (pos >= m_bufferBeginPos && pos < endpos) {
      off_type index = pos - m_bufferBeginPos;
      setg (eback(), eback() + index, egptr());
      return pos;
    }
  }

  if (sync() != 0) {
    return static_cast<pos_type> (-1);
  }

  CORBA::ULong fp = static_cast<CORBA::ULong> (pos);

  try {
    m_file->setFilePointer (fp);
  }
  catch (...) {
    return static_cast<pos_type> (-1);
  }

  m_bufferBeginPos = pos;
  return pos;
}

/*
 * ----------------------------------------------------------------------
 * Iterator object for directory listings
 * ----------------------------------------------------------------------
 */

namespace {

  class SCAFsIterator : public CPI::Util::Vfs::Iterator {
  public:
    SCAFsIterator (CF::FileSystem::FileInformationSequence * fis,
                   const std::string & dir,
                   const std::string & pbd)
      throw ();
    ~SCAFsIterator ()
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
    CORBA::ULong m_index;
    CORBA::ULong m_fisLength;
    const std::string m_dir;
    const std::string m_pbd;
    CF::FileSystem::FileInformationSequence_var m_fis;
  };

}

SCAFsIterator::SCAFsIterator (CF::FileSystem::FileInformationSequence * fis,
                              const std::string & dir,
                              const std::string & pbd)
  throw ()
  : m_index (0),
    m_fisLength (fis->length()),
    m_dir (dir),
    m_pbd (pbd),
    m_fis (fis)
{
}

SCAFsIterator::~SCAFsIterator ()
  throw ()
{
}

bool
SCAFsIterator::end ()
  throw (std::string)
{
  while (m_index < m_fisLength) {
    const char * fileName = m_fis[m_index].name.in();
    cpiAssert (fileName);

    if (fileName[0] != '.') {
      break;
    }
    else if (fileName[1] == '.') {
      if (fileName[2] && fileName[2] != '/') {
        break;
      }
    }
    else if (fileName[1] && fileName[1] != '/') {
      break;
    }

    /*
     * Skip over this "." or ".." listing.
     */

    m_index++;
  }

  return (m_index >= m_fisLength);
}

bool
SCAFsIterator::next ()
  throw (std::string)
{
  if (m_index >= m_fisLength) {
    throw std::string ("attempt to move beyond the end of the SCAFsIterator");
  }

  m_index++;
  return !end();
}

std::string
SCAFsIterator::absoluteName ()
  throw (std::string)
{
  if (m_index >= m_fisLength) {
    throw std::string ("beyond the end of the SCAFsIterator");
  }

  /*
   * Need to truncate a directory name's slash at the end.
   */

  std::string fileName = m_fis[m_index].name.in();
  std::string::size_type n = fileName.length ();
  cpiAssert (n);

  if (fileName[n-1] == '/') {
    cpiAssert (n > 1);
    fileName = fileName.substr (0, n-1);
  }

  return CPI::Util::Vfs::joinNames (m_dir, fileName);
}

std::string
SCAFsIterator::relativeName ()
  throw (std::string)
{
  if (m_index >= m_fisLength) {
    throw std::string ("beyond the end of the SCAFsIterator");
  }

  /*
   * Need to truncate a directory name's slash at the end.
   */

  std::string fileName = m_fis[m_index].name.in();
  std::string::size_type n = fileName.length ();
  cpiAssert (n);

  if (fileName[n-1] == '/') {
    cpiAssert (n > 1);
    fileName = fileName.substr (0, n-1);
  }

  if (m_pbd.length()) {
    return CPI::Util::Vfs::joinNames (m_pbd, fileName);
  }

  return fileName;
}

bool
SCAFsIterator::isDirectory ()
  throw (std::string)
{
  if (m_index >= m_fisLength) {
    throw std::string ("beyond the end of the SCAFsIterator");
  }

  return (m_fis[m_index].kind != CF::FileSystem::PLAIN);
}

unsigned long long
SCAFsIterator::size ()
  throw (std::string)
{
  if (m_index >= m_fisLength) {
    throw std::string ("beyond the end of the SCAFsIterator");
  }

  return static_cast<unsigned long long> (m_fis[m_index].size);
}

std::time_t
SCAFsIterator::lastModified ()
  throw (std::string)
{
  if (m_index >= m_fisLength) {
    throw std::string ("beyond the end of the SCAFsIterator");
  }

  const CF::Properties & fprops = m_fis[m_index].fileProperties;

  for (CORBA::ULong pi=0; pi<fprops.length(); pi++) {
    const CF::DataType & fp = fprops[pi];

    if (std::strcmp (fp.id, "MODIFIED_TIME") == 0) {
      CORBA::ULongLong ts;
      if (!(fp.value >>= ts)) {
        throw std::string ("invalid timestamp");
      }

      return static_cast<std::time_t> (ts);
    }
  }

  return static_cast<std::time_t> (-1);
}

/*
 * ----------------------------------------------------------------------
 * Constructor and Destructor
 * ----------------------------------------------------------------------
 */

CPI::CFUtil::SCAFs::
SCAFs (CORBA::ORB_ptr orb,
       CF::FileSystem_ptr fs)
  throw (std::string)
  : m_cwd ("/")
{
  cpiAssert (!CORBA::is_nil (fs));

  try {
    m_orb = CORBA::ORB::_duplicate (orb);
    m_fs = CF::FileSystem::_duplicate (fs);

    CORBA::String_var fsIor = orb->object_to_string (m_fs);
    std::string fsIorString = fsIor.in ();
    cpiAssert (fsIorString.substr (0, 4) == "IOR:");

    m_baseURI = "scafs://";
    m_baseURI += fsIorString.substr (4);
    m_baseURI += "/";
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CORBAUtil::Misc::stringifyCorbaException (ex);
  }
}

CPI::CFUtil::SCAFs::
~SCAFs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::CFUtil::SCAFs::
baseURI () const
  throw ()
{
  return m_baseURI;
}

std::string
CPI::CFUtil::SCAFs::
nameToURI (const std::string & fileName) const
  throw (std::string)
{
  std::string an = absoluteName (fileName);
  std::string uri = m_baseURI.substr (0, m_baseURI.length() - 1);
  uri += CPI::Util::Uri::encode (an, "/");
  return uri;
}

std::string
CPI::CFUtil::SCAFs::
URIToName (const std::string & struri) const
  throw (std::string)
{
  CPI::Util::Uri uri (struri);

  if (uri.getScheme() != "scafs") {
    std::string reason = "scafs file system does not support \"";
    reason += uri.getScheme ();
    reason += "\" URIs";
    throw reason;
  }

  std::string ior = "IOR:";
  ior += uri.getAuthority ();

  try {
    CORBA::Object_var obj = m_orb->string_to_object (ior.c_str());

    if (!(m_fs->_is_equivalent (obj))) {
      throw std::string ("URI not local to this file system");
    }
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CORBAUtil::Misc::stringifyCorbaException (ex);
  }

  return CPI::Util::Uri::decode (uri.getPath ());
}

/*
 * ----------------------------------------------------------------------
 * Directory Management
 * ----------------------------------------------------------------------
 */

std::string
CPI::CFUtil::SCAFs::
cwd () const
  throw (std::string)
{
  CPI::Util::AutoMutex lock (m_lock);
  return m_cwd;
}

void
CPI::CFUtil::SCAFs::
cd (const std::string & name)
  throw (std::string)
{
  bool isDir, ex;

  ex = exists (name, &isDir);

  if (!ex) {
    throw std::string ("name does not exist");
  }
  else if (!isDir) {
    throw std::string ("not a directory");
  }

  CPI::Util::AutoMutex lock (m_lock);
  m_cwd = CPI::Util::Vfs::joinNames (m_cwd, name);
}

void
CPI::CFUtil::SCAFs::
mkdir (const std::string & name)
  throw (std::string)
{
  std::string absName = absoluteName (name);

  try {
    m_fs->mkdir (absName.c_str ());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }
}

void
CPI::CFUtil::SCAFs::
rmdir (const std::string & name)
  throw (std::string)
{
  std::string absName = absoluteName (name);

  try {
    m_fs->rmdir (absName.c_str ());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Iterator *
CPI::CFUtil::SCAFs::
list (const std::string & dir,
      const std::string & pattern)
  throw (std::string)
{
  std::string absName = absoluteName (dir);
  std::string pbd;


  if (pattern.find ('/') != std::string::npos) {
    pbd = CPI::Util::Vfs::directoryName (pattern);
  }

  std::string absPat = CPI::Util::Vfs::joinNames (absName, pattern);
  std::string absDir = CPI::Util::Vfs::directoryName (absPat);
  CF::FileSystem::FileInformationSequence_var fis;

  try {
    fis = m_fs->list (absPat.c_str());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }

  return new SCAFsIterator (fis._retn(), absDir, pbd);
}

void
CPI::CFUtil::SCAFs::
closeIterator (CPI::Util::Vfs::Iterator * it)
  throw (std::string)
{
  SCAFsIterator * sfi = dynamic_cast<SCAFsIterator *> (it);

  if (!sfi) {
    throw std::string ("invalid iterator");
  }

  delete sfi;
}

/*
 * ----------------------------------------------------------------------
 * File Information
 * ----------------------------------------------------------------------
 */

bool
CPI::CFUtil::SCAFs::
exists (const std::string & name, bool * isDir)
  throw (std::string)
{
  std::string absName = absoluteName (name);
  CF::FileSystem::FileInformationSequence_var fis;

  try {
    fis = m_fs->list (absName.c_str());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }

  if (!fis->length()) {
    return false;
  }

  cpiAssert (fis->length() == 1);

  if (isDir) {
    const CORBA::ULong index = 0;
    *isDir = (fis[index].kind != CF::FileSystem::PLAIN);
  }

  return true;
}

unsigned long long
CPI::CFUtil::SCAFs::
size (const std::string & name)
  throw (std::string)
{
  std::string absName = absoluteName (name);
  CF::FileSystem::FileInformationSequence_var fis;

  try {
    fis = m_fs->list (absName.c_str());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }

  if (!fis->length()) {
    throw std::string ("file not found");
  }

  cpiAssert (fis->length() == 1);
  const CORBA::ULong index = 0;
  return static_cast<unsigned long long> (fis[index].size);
}

std::time_t
CPI::CFUtil::SCAFs::
lastModified (const std::string & name)
  throw (std::string)
{
  std::string absName = absoluteName (name);
  CF::FileSystem::FileInformationSequence_var fis;

  try {
    fis = m_fs->list (absName.c_str());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }

  if (!fis->length()) {
    throw std::string ("file not found");
  }

  cpiAssert (fis->length() == 1);

  const CORBA::ULong index = 0;
  const CF::Properties & fprops = fis[index].fileProperties;

  for (CORBA::ULong pi=0; pi<fprops.length(); pi++) {
    const CF::DataType & fp = fprops[pi];

    if (std::strcmp (fp.id.in(), CF::FileSystem::MODIFIED_TIME_ID) == 0) {
      CORBA::ULongLong ts;
      if (!(fp.value >>= ts)) {
        throw std::string ("invalid timestamp");
      }

      return static_cast<std::time_t> (ts);
    }
  }

  return static_cast<std::time_t> (-1);
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
CPI::CFUtil::SCAFs::
open (const std::string & name,
      std::ios_base::openmode mode)
  throw (std::string)
{
  std::string absName = absoluteName (name);
  CF::File_var file;

  mode |= std::ios_base::in | std::ios_base::out;

  if ((mode & std::ios_base::trunc) == std::ios_base::trunc) {
    /*
     * CF::FileSystem::create() fails if the file already exists.  In that
     * case, we need to remove the file first.  (Subject to an unavoidable
     * race condition when someone else re-creates the file before we can.)
     */

    try {
      m_fs->remove (absName.c_str());
    }
    catch (...) {
      /* ignored */
    }

    try {
      file = m_fs->create (absName.c_str());
    }
    catch (const CORBA::Exception & ex) {
      throw CPI::CFUtil::stringifyCFException (ex);
    }

    if (CORBA::is_nil (file)) {
      throw std::string ("CF::FileSystem::create() failed");
    }
  }
  else {
    /*
     * CF::FileSystem::open() fails if the file does not exist.  In that
     * case, we must to create the file.  (Subject to an unavoidable race
     * condition when someone else creates the file before we can try
     * again.)
     */

    try {
      file = m_fs->open (absName.c_str(), 0);
    }
    catch (...) {
      file = CF::File::_nil ();
    }

    if (CORBA::is_nil (file)) {
      try {
        file = m_fs->create (absName.c_str());
      }
      catch (const CORBA::Exception & ex) {
        throw CPI::CFUtil::stringifyCFException (ex);
      }

      if (CORBA::is_nil (file)) {
        throw std::string ("CF::FileSystem::create() failed");
      }
    }
  }

  return new SCAFileStream (file._retn(), mode);
}

std::istream *
CPI::CFUtil::SCAFs::
openReadonly (const std::string & name,
              std::ios_base::openmode mode)
  throw (std::string)
{
  std::string absName = absoluteName (name);
  CF::File_var file;

  try {
    file = m_fs->open (absName.c_str(), 1);
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }

  if (CORBA::is_nil (file)) {
    throw std::string ("CF::FileSystem::open() failed");
  }

  return new SCAFileStream (file._retn(), mode);
}

std::ostream *
CPI::CFUtil::SCAFs::
openWriteonly (const std::string & name,
               std::ios_base::openmode mode)
  throw (std::string)
{
  return open (name, mode);
}

void
CPI::CFUtil::SCAFs::
close (std::ios * str)
  throw (std::string)
{
  SCAFileStream * sfstr = dynamic_cast<SCAFileStream *> (str);

  if (!sfstr) {
    throw std::string ("invalid stream");
  }

  bool good = (sfstr->close () == 0);
  delete sfstr;

  if (!good) {
    throw std::string ("error closing file");
  }
}

/*
 * ----------------------------------------------------------------------
 * File System Operations
 * ----------------------------------------------------------------------
 */

void
CPI::CFUtil::SCAFs::
copy (const std::string & oldName,
      Vfs * destFs,
      const std::string & newName)
  throw (std::string)
{
  /*
   * See if the target filesystem is an SCAFs. If yes, and if it points
   * to the same CF::FileSystem instance, we can use CF::FileSystem::copy.
   */

  SCAFs * destSCAFs = dynamic_cast<SCAFs *> (destFs);

  if (!destSCAFs) {
    Vfs::copy (oldName, destFs, newName);
    return;
  }

  bool isTheSame = false;

  try {
    isTheSame = m_fs->_is_equivalent (destSCAFs->m_fs);
  }
  catch (...) {
  }

  if (!isTheSame) {
    Vfs::move (oldName, destFs, newName);
    return;
  }

  std::string oldAbsName = absoluteName (oldName);
  std::string newAbsName = destSCAFs->absoluteName (newName);

  try {
    m_fs->copy (oldAbsName.c_str(), newAbsName.c_str());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }
}

void
CPI::CFUtil::SCAFs::
remove (const std::string & name)
  throw (std::string)
{
  std::string absName = absoluteName (name);

  try {
    m_fs->remove (absName.c_str ());
  }
  catch (const CORBA::Exception & ex) {
    throw CPI::CFUtil::stringifyCFException (ex);
  }
}

