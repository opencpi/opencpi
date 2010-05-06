#include <ctime>
#include <string>
#include <cctype>
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
#include <CF_s.h>
#include "CpiCFUtilLegacyErrorNumbers.h"
#include "CpiCFUtilVfsFileSystem.h"

/*
 * ----------------------------------------------------------------------
 * Reading from or writing to an iostream using the CF::File interface.
 * ----------------------------------------------------------------------
 */

namespace {
  class VfsFile : public POA_CF::File {
  public:
    VfsFile (PortableServer::POA_ptr poa,
             CPI::Util::Vfs::Vfs * fs,
             const std::string & fileName,
             std::istream * in,
             std::ostream * out)
      throw ();

    ~VfsFile ()
      throw ();

    char * fileName ()
      throw (CORBA::SystemException);

    CORBA::ULong filePointer ()
      throw (CORBA::SystemException);

    void read (CF::OctetSequence_out data,
               CORBA::ULong length)
      throw (CF::File::IOException,
             CORBA::SystemException);

    void write (const CF::OctetSequence & data)
      throw (CF::File::IOException,
             CORBA::SystemException);

    CORBA::ULong sizeOf ()
      throw (CF::FileException,
             CORBA::SystemException);

    void close ()
      throw (CF::FileException,
             CORBA::SystemException);

    void setFilePointer (CORBA::ULong filePointer)
      throw (CF::File::InvalidFilePointer,
             CF::FileException,
             CORBA::SystemException);

  protected:
    PortableServer::POA_var m_poa;
    CPI::Util::Vfs::Vfs * m_fs;
    const std::string m_fileName;
    std::istream * m_in;
    std::ostream * m_out;
  };
}

VfsFile::
VfsFile (PortableServer::POA_ptr poa,
         CPI::Util::Vfs::Vfs * fs,
         const std::string & fileName,
         std::istream * in,
         std::ostream * out)
  throw ()
  : m_poa (PortableServer::POA::_duplicate (poa)),
    m_fs (fs),
    m_fileName (fileName),
    m_in (in),
    m_out (out)
{
  cpiAssert (m_in);
}

VfsFile::
~VfsFile ()
  throw ()
{
}

char *
VfsFile::
fileName ()
  throw (CORBA::SystemException)
{
  return CORBA::string_dup (m_fileName.c_str());
}

CORBA::ULong
VfsFile::
filePointer ()
  throw (CORBA::SystemException)
{
  std::ios::pos_type pos;

  if (m_in) {
    pos = m_in->tellg ();
  }
  else {
    pos = m_out->tellp ();
  }

  if (pos == static_cast<std::ios::pos_type> (-1)) {
    throw CORBA::INTERNAL ();
  }

  return static_cast<CORBA::ULong> (pos);
}

void
VfsFile::
read (CF::OctetSequence_out data,
      CORBA::ULong length)
  throw (CF::File::IOException,
         CORBA::SystemException)
{
  CF::OctetSequence_var buf = new CF::OctetSequence (length);
  buf->length (length);

  char * pbuf = reinterpret_cast<char *> (buf->get_buffer ());

  m_in->read (pbuf, length);

  if (m_in->bad()) {
    CF::File::IOException ioe;
    ioe.errorNumber = CF::CF_EIO;
    ioe.msg = "Input stream is bad";
    throw ioe;
  }

  buf->length (m_in->gcount());
  data = buf._retn ();
}

void
VfsFile::
write (const CF::OctetSequence & data)
  throw (CF::File::IOException,
         CORBA::SystemException)
{
  if (!m_out) {
    CF::File::IOException ioe;
    ioe.errorNumber = CF::CF_EBADF;
    ioe.msg = "File is not open for writing";
    throw ioe;
  }

  m_out->write (reinterpret_cast<const char *> (data.get_buffer ()),
                data.length ());

  if (m_out->bad()) {
    CF::File::IOException ioe;
    ioe.errorNumber = CF::CF_EIO;
    ioe.msg = "Output stream is bad";
    throw ioe;
  }
}

CORBA::ULong
VfsFile::
sizeOf ()
  throw (CF::FileException,
         CORBA::SystemException)
{
  unsigned long long size;

  try {
    size = m_fs->size (m_fileName);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  if (size >= 1ull<<32) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFBIG;
    fe.msg = "File size exceeds range";
    throw fe;
  }

  return static_cast<CORBA::ULong> (size);
}

void
VfsFile::
close ()
  throw (CF::FileException,
         CORBA::SystemException)
{
  PortableServer::ObjectId_var myId = m_poa->servant_to_id (this);
  m_poa->deactivate_object (myId.in());

  try {
    m_fs->close (m_in);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }
}

void
VfsFile::
setFilePointer (CORBA::ULong filePointer)
  throw (CF::File::InvalidFilePointer,
         CF::FileException,
         CORBA::SystemException)
{
  /*
   * The setFilePointer operation shall raise the InvalidFilePointer
   * exception when the value of the filePointer parameter exceeds
   * the file size.
   */

  unsigned long long size;

  try {
    size = m_fs->size (m_fileName);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  if (filePointer > size) {
    throw CF::File::InvalidFilePointer ();
  }

  m_in->seekg (filePointer);

  if (m_in->fail()) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = "Failed to set file pointer";
    throw fe;
  }
}

/*
 * ----------------------------------------------------------------------
 * Constructor and Destructor
 * ----------------------------------------------------------------------
 */

CPI::CFUtil::VfsFileSystem::
VfsFileSystem (CORBA::ORB_ptr orb,
               PortableServer::POA_ptr poa,
               CPI::Util::Vfs::Vfs * fs,
               bool adopt)
  throw ()
  : m_orb (CORBA::ORB::_duplicate (orb)),
    m_poa (PortableServer::POA::_duplicate (poa)),
    m_fs (fs),
    m_adopted (adopt)
{
}

CPI::CFUtil::VfsFileSystem::
~VfsFileSystem ()
  throw ()
{
  if (m_adopted) {
    delete m_fs;
  }
}

/*
 * ----------------------------------------------------------------------
 * Implementation of the CF::FileSystem interface.
 * ----------------------------------------------------------------------
 */

void
CPI::CFUtil::VfsFileSystem::
remove (const char * fileName)
  throw (CF::InvalidFileName,
         CF::FileException,
         CORBA::SystemException)
{
  std::string name (fileName);
  testFileName (fileName);

  try {
    m_fs->remove (fileName);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }
}

void
CPI::CFUtil::VfsFileSystem::
copy (const char * sourceFileName,
      const char * destinationFileName)
  throw (CF::InvalidFileName,
         CF::FileException,
         CORBA::SystemException)
{
  std::string sourceName (sourceFileName);
  std::string destName (destinationFileName);

  testFileName (sourceName);
  testFileName (destName);

  try {
    m_fs->copy (sourceName, m_fs, destName);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }
}

CORBA::Boolean
CPI::CFUtil::VfsFileSystem::
exists (const char * fileName)
  throw (CF::InvalidFileName,
         CORBA::SystemException)
{
  std::string name (fileName);
  CORBA::Boolean res;

  testFileName (fileName);

  try {
    res = (m_fs->exists (fileName)) ? 1 : 0;
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  return res;
}

CF::FileSystem::FileInformationSequence *
CPI::CFUtil::VfsFileSystem::
list (const char * pattern)
  throw (CF::InvalidFileName,
         CF::FileException,
         CORBA::SystemException)
{
  std::string pat (pattern);
  testFileName (pat, true);

  std::string dir = CPI::Util::Vfs::directoryName (pat);
  std::string bp = CPI::Util::Vfs::relativeName (pat);

  CPI::Util::Vfs::Iterator * vit;

  try {
    vit = m_fs->list (dir, bp);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  CF::FileSystem::FileInformationSequence_var fis =
    new CF::FileSystem::FileInformationSequence;
  CORBA::ULong index = 0;

  try {
    while (!vit->end()) {
      fis->length (index + 1);
      CF::FileSystem::FileInformationType & fi = fis[index];
      std::string fn = vit->relativeName ();

      if (vit->isDirectory()) {
        fn += '/';
        fi.name = fn.c_str ();
        fi.kind = CF::FileSystem::DIRECTORY;
        fi.size = 0;
      }
      else {
        fi.name = fn.c_str ();
        fi.kind = CF::FileSystem::PLAIN;
        fi.size = vit->size ();
      }

      std::time_t lm = vit->lastModified ();

      if (lm != static_cast<std::time_t> (-1)) {
        fi.fileProperties.length (1);
        fi.fileProperties[0].id = CF::FileSystem::MODIFIED_TIME_ID;
        fi.fileProperties[0].value <<= static_cast<CORBA::ULong> (lm);
      }

      index++;
      vit->next ();
    }
  }
  catch (const std::string & oops) {
    try {
      m_fs->closeIterator (vit);
    }
    catch (...) {
      cpiAssert (0);
    }

    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  try {
    m_fs->closeIterator (vit);
  }
  catch (...) {
    cpiAssert (0);
  }

  return fis._retn ();
}

CF::File_ptr
CPI::CFUtil::VfsFileSystem::
create (const char * fileName)
  throw (CF::InvalidFileName,
         CF::FileException,
         CORBA::SystemException)
{
  std::string name (fileName);
  testFileName (name);

  /*
   * The create operation shall raise the CF FileException if the file
   * already exists.  Note: there is an inherent race condition here.
   */

  bool ex;

  try {
    ex = m_fs->exists (name);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  if (ex) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EEXIST;
    fe.msg = "File exists";
    throw fe;
  }

  std::iostream * ios;

  try {
    ios = m_fs->open (name, std::ios_base::binary);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  CF::File_var cf;

  try {
    VfsFile * vf = new VfsFile (m_poa, m_fs, name, ios, ios);
    PortableServer::ObjectId_var oid = m_poa->activate_object (vf);
    CORBA::Object_var obj = m_poa->id_to_reference (oid.in());
    CF::File_var cf = CF::File::_narrow (obj);
    cpiAssert (!CORBA::is_nil (cf));
    vf->_remove_ref ();
  }
  catch (const CORBA::Exception & ex) {
    std::string oops = CPI::CORBAUtil::Misc::stringifyCorbaException (ex);
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  return cf._retn ();
}

CF::File_ptr
CPI::CFUtil::VfsFileSystem::
open (const char * fileName,
      CORBA::Boolean read_Only)
  throw (CF::InvalidFileName,
         CF::FileException,
         CORBA::SystemException)
{
  std::string name (fileName);
  testFileName (name);

  /*
   * The open operation shall raise the CF FileException if the file
   * does not exist.  Note: there is an inherent race condition here.
   */

  bool ex;

  try {
    ex = m_fs->exists (name);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  if (!ex) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_ENOENT;
    fe.msg = "File does not exit";
    throw fe;
  }

  std::iostream * ios;
  std::istream * is;

  try {
    if (read_Only) {
      is = m_fs->openReadonly (name, std::ios_base::binary);
      ios = 0;
    }
    else {
      is = ios = m_fs->open (name, std::ios_base::binary);
    }
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  CF::File_var cf;

  try {
    VfsFile * vf = new VfsFile (m_poa, m_fs, name, is, ios);
    PortableServer::ObjectId_var oid = m_poa->activate_object (vf);
    CORBA::Object_var obj = m_poa->id_to_reference (oid.in());
    cf = CF::File::_narrow (obj);
    cpiAssert (!CORBA::is_nil (cf));
    vf->_remove_ref ();
  }
  catch (const CORBA::Exception & ex) {
    std::string oops = CPI::CORBAUtil::Misc::stringifyCorbaException (ex);
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }

  return cf._retn ();
}

void
CPI::CFUtil::VfsFileSystem::
mkdir (const char * directoryName)
  throw (CF::InvalidFileName,
         CF::FileException,
         CORBA::SystemException)
{
  std::string name (directoryName);
  testFileName (name);

  try {
    m_fs->mkdir (name);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }
}

void
CPI::CFUtil::VfsFileSystem::
rmdir (const char * directoryName)
  throw (CF::InvalidFileName,
         CF::FileException,
         CORBA::SystemException)
{
  std::string name (directoryName);
  testFileName (name);

  try {
    m_fs->rmdir (name);
  }
  catch (const std::string & oops) {
    CF::FileException fe;
    fe.errorNumber = CF::CF_EFAULT;
    fe.msg = oops.c_str ();
    throw fe;
  }
}

void
CPI::CFUtil::VfsFileSystem::
query (CF::Properties & fileSystemProperties)
  throw (CF::FileSystem::UnknownFileSystemProperties,
         CORBA::SystemException)
{
  CF::FileSystem::UnknownFileSystemProperties ufsp;
  CORBA::ULong np = fileSystemProperties.length ();
  CORBA::ULong numUnknownProps = 0;

  /*
   * The SCA requires us to support the SIZE and AVAILABLE_SPACE file
   * system properties.  But the Vfs interface does not have anything
   * equivalent.  So we just report them as 0.
   */

#if defined (CPI_USES_SCA22)
  /* CF 2.2 IDL misspells AVAILABLE_SPACE */
#define AVAILABLE_SPACE AVAILABLE_SIZE
#endif

  for (CORBA::ULong i=0; i<np; i++) {
    CF::DataType & prop = fileSystemProperties[i];

    if (std::strcmp (prop.id.in(), CF::FileSystem::SIZE) == 0) {
      CORBA::ULongLong size = 0;
      prop.value <<= size;
    }
    else if (std::strcmp (prop.id.in(), CF::FileSystem::AVAILABLE_SPACE) == 0) {
      CORBA::ULongLong avail = 0;
      prop.value <<= avail;
    }
    else {
      ufsp.invalidProperties.length (numUnknownProps + 1);
      ufsp.invalidProperties[numUnknownProps++] = prop;
    }
  }

  if (numUnknownProps) {
    throw ufsp;
  }
}

/*
 * Test whether a file name is valid or not.  Throws an exception if it
 * isn't.  The various FileSystem operations "shall raise the InvalidFileName
 * exception when the filename is not a valid file name or not an absolute
 * pathname."
 *
 * If isPattern is true, then wildcard characters are allowed in the last
 * path component (for the "list" operation).
 *
 * From SCA 2.2.2: "Valid individual filenames and directory names shall be
 * 40 characters or less.  Valid characters for a filename or directory name
 * are the 62 alphanumeric characters (Upper, and lowercase letters and the
 * numbers 0 to 9) in addition to the '.' (period), '_' (underscore) and '-'
 * (hyphen) characters.  The filenames '.' ("dot") and ".." ("dot-dot") are
 * invalid in the context of a file sytem."
 *
 * "Valid pathnames are structured according to the POSIX specification
 * whose valid characters include the "/" (forward slash) character in
 * addition to the valid filename characters.  A valid pathname may consist
 * of a single filename.  A valid pathname shall not exceed 1024 characters.
 */

void
CPI::CFUtil::VfsFileSystem::
testFileName (const std::string & fileName, bool isPattern)
  throw (CF::InvalidFileName)
{
  if (!fileName.length()) {
    CF::InvalidFileName ifn;
    ifn.errorNumber = CF::CF_EINVAL;
    ifn.msg = "Null file name";
    throw ifn;
  }

  if (fileName.length() > 1024) {
    CF::InvalidFileName ifn;
    ifn.errorNumber = CF::CF_ENAMETOOLONG;
    ifn.msg = "Path name exceeds 1024 characters";
    throw ifn;
  }

  if (fileName[0] != '/') {
    CF::InvalidFileName ifn;
    ifn.errorNumber = CF::CF_EINVAL;
    ifn.msg = "Not an absolute pathname";
    throw ifn;
  }

  std::string::size_type pcBeg = 0;
  std::string::size_type pcEnd = fileName.find ('/', 1);
  std::string::size_type pcl;
  std::string pc;

  while (pcBeg != std::string::npos) {
    if (pcEnd == std::string::npos) {
      pc = fileName.substr (pcBeg+1);
    }
    else {
      pc = fileName.substr (pcBeg+1, pcEnd-pcBeg-1);
    }

    pcl = pc.length ();

    if (pcl > 40) {
      CF::InvalidFileName ifn;
      ifn.errorNumber = CF::CF_ENAMETOOLONG;
      ifn.msg = "Path component exceeds 40 characters";
      throw ifn;
    }

    if ((pcl == 1 && pc[0] == '.') ||
        (pcl == 2 && pc[0] == '.' && pc[1] == '.')) {
      CF::InvalidFileName ifn;
      ifn.errorNumber = CF::CF_EINVAL;
      ifn.msg = "Dot or dot-dot path component encountered";
      throw ifn;
    }

    for (std::string::size_type pci=0; pci<pcl; pci++) {
      char c = pc[pci];

      if (!isalnum (c) && c != '.' && c != '_' && c != '-') {
        if (pcEnd == std::string::npos && isPattern) {
          /*
           * Last path component in a pattern.  Allow wildcard characters.
           */

          if (c != '?' && c != '*') {
            CF::InvalidFileName ifn;
            ifn.errorNumber = CF::CF_EINVAL;
            ifn.msg = "Invalid file name character";
            throw ifn;
          }
        }
      }
    }

    pcBeg = pcEnd;
    pcEnd = fileName.find ('/', pcBeg+1);
  }

  /*
   * If we got this far without throwing an exception, the file name is
   * acceptable.
   */
}
