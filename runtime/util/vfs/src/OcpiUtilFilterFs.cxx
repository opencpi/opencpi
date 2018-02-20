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

#include <OcpiUtilFilterFs.h>
#include <OcpiUtilVfs.h>
#include <iostream>
#include <string>

/*
 * ----------------------------------------------------------------------
 * FilterFS implementation
 * ----------------------------------------------------------------------
 */

OCPI::Util::Vfs::FilterFs::FilterFs (OCPI::Util::Vfs::Vfs & delegatee)
  throw ()
  : m_delegatee (delegatee)
{
}

OCPI::Util::Vfs::FilterFs::~FilterFs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name URI Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Vfs::FilterFs::baseURI () const
  throw ()
{
  return m_delegatee.baseURI ();
}

std::string
OCPI::Util::Vfs::FilterFs::nameToURI (const std::string & fileName) const
  throw (std::string)
{
  return m_delegatee.nameToURI (fileName);
}

std::string
OCPI::Util::Vfs::FilterFs::URIToName (const std::string & struri) const
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
OCPI::Util::Vfs::FilterFs::cwd () const
  throw (std::string)
{
  return m_delegatee.cwd ();
}

void
OCPI::Util::Vfs::FilterFs::cd (const std::string & fileName)
  throw (std::string)
{
  access (fileName, std::ios_base::in, true);
  m_delegatee.cd (fileName);
}

void
OCPI::Util::Vfs::FilterFs::mkdir (const std::string & fileName)
  throw (std::string)
{
  std::string dirName = directoryName (fileName);
  access (dirName, std::ios_base::out, true);
  m_delegatee.mkdir (fileName);
}

void
OCPI::Util::Vfs::FilterFs::rmdir (const std::string & fileName)
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

#if 0
OCPI::Util::Vfs::Iterator *
OCPI::Util::Vfs::FilterFs::list (const std::string & dir,
                                const std::string & pattern)
  throw (std::string)
{
  std::string absDirName = absoluteName (dir);
  std::string absPatName = OCPI::Util::Vfs::joinNames (dir, pattern);
  std::string patDirName = OCPI::Util::Vfs::directoryName (absPatName);
  access (patDirName, std::ios_base::in, true);
  return m_delegatee.list (dir, pattern);
}

void
OCPI::Util::Vfs::FilterFs::closeIterator (Iterator * it)
  throw (std::string)
{
  return m_delegatee.closeIterator (it);
}
#endif
/*
 * ----------------------------------------------------------------------
 * File information
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::Vfs::FilterFs::exists (const std::string & fileName, bool * isDir)
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  std::string dirName = OCPI::Util::Vfs::directoryName (absName);
  access (dirName, std::ios_base::in, true);
  return m_delegatee.exists (fileName, isDir);
}

unsigned long long
OCPI::Util::Vfs::FilterFs::size (const std::string & fileName)
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  std::string dirName = OCPI::Util::Vfs::directoryName (absName);
  access (dirName, std::ios_base::in, true);
  return m_delegatee.size (fileName);
}

std::time_t
OCPI::Util::Vfs::FilterFs::lastModified (const std::string & fileName)
  throw (std::string)
{
  std::string absName = absoluteName (fileName);
  std::string dirName = OCPI::Util::Vfs::directoryName (absName);
  access (dirName, std::ios_base::in, true);
  return m_delegatee.lastModified (fileName);
}

/*
 * ----------------------------------------------------------------------
 * File I/O
 * ----------------------------------------------------------------------
 */

std::iostream *
OCPI::Util::Vfs::FilterFs::open (const std::string & fileName,
                                std::ios_base::openmode mode)
  throw (std::string)
{
  access (fileName, mode, false);
  return m_delegatee.open (fileName, mode);
}

std::istream *
OCPI::Util::Vfs::FilterFs::openReadonly (const std::string & fileName,
                                        std::ios_base::openmode mode)
  throw (std::string)
{
  access (fileName, mode & ~(std::ios_base::out), false);
  return m_delegatee.openReadonly (fileName, mode);
}

std::ostream *
OCPI::Util::Vfs::FilterFs::openWriteonly (const std::string & fileName,
                                         std::ios_base::openmode mode)
  throw (std::string)
{
  access (fileName, mode & ~(std::ios_base::in), false);
  return m_delegatee.openWriteonly (fileName, mode);
}

void
OCPI::Util::Vfs::FilterFs::close (std::ios * str)
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
OCPI::Util::Vfs::FilterFs::copy (const std::string & oldName,
                                OCPI::Util::Vfs::Vfs * destfs,
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
OCPI::Util::Vfs::FilterFs::move (const std::string & oldName,
                                OCPI::Util::Vfs::Vfs * destfs,
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
OCPI::Util::Vfs::FilterFs::rename (const std::string & oldName,
                                  const std::string & newName)
  throw (std::string)
{
  access (oldName, std::ios_base::in | std::ios_base::trunc, false);
  access (newName, std::ios_base::out | std::ios_base::trunc, false);
  m_delegatee.rename (oldName, newName);
}

void
OCPI::Util::Vfs::FilterFs::remove (const std::string & fileName)
  throw (std::string)
{
  access (fileName, std::ios_base::trunc, false);
  m_delegatee.remove (fileName);
}
