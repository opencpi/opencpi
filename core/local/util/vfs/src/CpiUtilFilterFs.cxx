#include <CpiUtilFilterFs.h>
#include <CpiUtilVfs.h>
#include <iostream>
#include <string>

/*
 * ----------------------------------------------------------------------
 * FilterFS implementation
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::FilterFs::FilterFs (CPI::Util::Vfs::Vfs & delegatee)
  throw ()
  : m_delegatee (delegatee)
{
}

CPI::Util::Vfs::FilterFs::~FilterFs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Vfs::FilterFs::baseURI () const
  throw ()
{
  return m_delegatee.baseURI ();
}

std::string
CPI::Util::Vfs::FilterFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  return m_delegatee.nameToURI (fileName);
}

std::string
CPI::Util::Vfs::FilterFs::URIToName (const std::string & struri) const
  throw (std::string)
{
  return m_delegatee.URIToName (struri);
}

/*
 * ----------------------------------------------------------------------
 * Directory Management
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Vfs::FilterFs::cwd () const
  throw (std::string)
{
  return m_delegatee.cwd ();
}

void
CPI::Util::Vfs::FilterFs::cd (const std::string & fileName)
  throw (std::string)
{
  access (fileName, std::ios_base::in, true);
  m_delegatee.cd (fileName);
}

void
CPI::Util::Vfs::FilterFs::mkdir (const std::string & fileName)
  throw (std::string)
{
  std::string dirName = directoryName (fileName);
  access (dirName, std::ios_base::out, true);
  m_delegatee.mkdir (fileName);
}

void
CPI::Util::Vfs::FilterFs::rmdir (const std::string & fileName)
  throw (std::string)
{
  std::string dirName = directoryName (fileName);
  access (dirName, std::ios_base::out, true);
  m_delegatee.rmdir (fileName);
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Iterator *
CPI::Util::Vfs::FilterFs::list (const std::string & dir,
				const std::string & pattern)
  throw (std::string)
{
  std::string absDirName = absoluteName (dir);
  std::string absPatName = CPI::Util::Vfs::joinNames (dir, pattern);
  std::string patDirName = CPI::Util::Vfs::directoryName (absPatName);
  access (patDirName, std::ios_base::in, true);
  return m_delegatee.list (dir, pattern);
}

void
CPI::Util::Vfs::FilterFs::closeIterator (Iterator * it)
  throw (std::string)
{
  return m_delegatee.closeIterator (it);
}

/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::Vfs::FilterFs::exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  std::string dirName = CPI::Util::Vfs::directoryName (absName);
  access (dirName, std::ios_base::in, true);
  return m_delegatee.exists (fileName, isDir);
}

unsigned long long
CPI::Util::Vfs::FilterFs::size (const std::string & fileName)
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  std::string dirName = CPI::Util::Vfs::directoryName (absName);
  access (dirName, std::ios_base::in, true);
  return m_delegatee.size (fileName);
}

std::time_t
CPI::Util::Vfs::FilterFs::lastModified (const std::string & fileName)
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  std::string dirName = CPI::Util::Vfs::directoryName (absName);
  access (dirName, std::ios_base::in, true);
  return m_delegatee.lastModified (fileName);
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
CPI::Util::Vfs::FilterFs::open (const std::string & fileName,
				std::ios_base::openmode mode)
  throw (std::string)
{
  access (fileName, mode, false);
  return m_delegatee.open (fileName, mode);
}

std::istream *
CPI::Util::Vfs::FilterFs::openReadonly (const std::string & fileName,
					std::ios_base::openmode mode)
  throw (std::string)
{
  access (fileName, mode & ~(std::ios_base::out), false);
  return m_delegatee.openReadonly (fileName, mode);
}

std::ostream *
CPI::Util::Vfs::FilterFs::openWriteonly (const std::string & fileName,
					 std::ios_base::openmode mode)
  throw (std::string)
{
  access (fileName, mode & ~(std::ios_base::in), false);
  return m_delegatee.openWriteonly (fileName, mode);
}

void
CPI::Util::Vfs::FilterFs::close (std::ios * str)
  throw (std::string)
{
  m_delegatee.close (str);
}

/*
 * ----------------------------------------------------------------------
 * File system operations
 * ----------------------------------------------------------------------
 */

void
CPI::Util::Vfs::FilterFs::copy (const std::string & oldName,
				CPI::Util::Vfs::Vfs * destfs,
				const std::string & newName)
  throw (std::string)
{
  access (oldName, std::ios_base::in, false);

  FilterFs * destfilterfs = dynamic_cast<FilterFs *> (destfs);

  if (destfilterfs) {
    destfilterfs->access (newName, std::ios_base::out | std::ios_base::trunc, false);
    m_delegatee.copy (oldName, &destfilterfs->m_delegatee, newName);
  }
  else {
    m_delegatee.copy (oldName, destfs, newName);
  }
}

void
CPI::Util::Vfs::FilterFs::move (const std::string & oldName,
				CPI::Util::Vfs::Vfs * destfs,
				const std::string & newName)
  throw (std::string)
{
  access (oldName, std::ios_base::in | std::ios_base::trunc, false);

  FilterFs * destfilterfs = dynamic_cast<FilterFs *> (destfs);

  if (destfilterfs) {
    destfilterfs->access (newName, std::ios_base::out | std::ios_base::trunc, false);
    m_delegatee.move (oldName, &destfilterfs->m_delegatee, newName);
  }
  else {
    m_delegatee.move (oldName, destfs, newName);
  }
}

void
CPI::Util::Vfs::FilterFs::rename (const std::string & oldName,
				  const std::string & newName)
  throw (std::string)
{
  access (oldName, std::ios_base::in | std::ios_base::trunc, false);
  access (newName, std::ios_base::out | std::ios_base::trunc, false);
  m_delegatee.rename (oldName, newName);
}

void
CPI::Util::Vfs::FilterFs::remove (const std::string & fileName)
  throw (std::string)
{
  access (fileName, std::ios_base::trunc, false);
  m_delegatee.remove (fileName);
}
