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

#include <OcpiUtilVfs.h>
#include <OcpiUtilVfsIterator.h>
#include <OcpiOsFileSystem.h>
#include <iostream>
#include <string>

/*
 * Default buffer size for copying
 */

namespace {
  enum {
    DEFAULT_BUFFER_SIZE = 16384
  };
}

namespace OCPI {
  namespace Util {
    namespace Vfs {
/*
 * ----------------------------------------------------------------------
 * File Name Helpers
 * ----------------------------------------------------------------------
 */

std::string
joinNames (const std::string & dir,
                           const std::string & name)
  throw (std::string)
{
  return OCPI::OS::FileSystem::joinNames (dir, name);
}

std::string
directoryName (const std::string & name)
  throw (std::string)
{
  std::string::size_type slashPos = name.rfind ('/');

  if (slashPos == std::string::npos) {
    throw std::string ("single path component");
  }

  if (slashPos == 0) {
    return "/";
  }

  return name.substr (0, slashPos);
}

std::string
relativeName (const std::string & name)
  throw (std::string)
{
  return OCPI::OS::FileSystem::relativeName (name);
}

/*
 * ----------------------------------------------------------------------
 * Vfs Constructor and Destructor
 * ----------------------------------------------------------------------
 */

Vfs::Vfs ()
  throw ()
{
}

Vfs::~Vfs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
Vfs::absoluteName (const std::string & name) const
  throw (std::string)
{
  return OCPI::Util::Vfs::joinNames (cwd(), name);
}

std::string
Vfs::Vfs::directoryName (const std::string & name) const
  throw (std::string)
{
  return OCPI::Util::Vfs::directoryName (absoluteName (name));
}

/*
 * ----------------------------------------------------------------------
 * File System Operations
 * ----------------------------------------------------------------------
 */

/*
 * The default implementation of copy copies the file, chunk by chunk
 */

void
Vfs::copy (const std::string & src,
                           Vfs * destfs,
                           const std::string & dest)
  throw (std::string)
{
  std::istream * srcStream = openReadonly (src, std::ios_base::binary);
  std::ostream * destStream;

  try {
    destStream = destfs->openWriteonly (dest, std::ios_base::trunc | std::ios_base::binary);
  }
  catch (...) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    throw;
  }

  try {
    char buffer[DEFAULT_BUFFER_SIZE];

    while (!srcStream->eof() && srcStream->good() && destStream->good()) {
      srcStream->read (buffer, DEFAULT_BUFFER_SIZE);
      destStream->write (buffer, srcStream->gcount());
    }
  }
  catch (...) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw;
  }

  if (srcStream->bad()) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw std::string ("error reading source file");
  }

  if (!destStream->good()) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw std::string ("error writing target file");
  }

  try {
    close (srcStream);
  }
  catch (...) {
    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw;
  }

  try {
    destfs->close (destStream);
  }
  catch (...) {
    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw;
  }
}

/*
 * Default implementation of rename
 */

void
Vfs::rename (const std::string & src,
                             const std::string & dest)
  throw (std::string)
{
  copy (src, this, dest);
  remove (src);
}

/*
 * The default implementation of move is naive.
 */

void
Vfs::move (const std::string & src,
                           Vfs * destfs,
                           const std::string & dest)
  throw (std::string)
{
  if (this == destfs) {
    rename (src, dest);
  }
  else {
    copy (src, destfs, dest);
    remove (src);
  }
}
Iterator * Vfs::list (const std::string & dir,
		      const std::string & pattern,
		      bool recursive)
  throw (std::string) {
  return new Iterator(*this, dir, pattern.c_str(), recursive);
}

Dir::Dir(Vfs &fs, const std::string &name)
  : m_parent(NULL), m_fs(fs), m_name(name) {
}
Dir::~Dir() throw() {
}
Dir *Dir::pushDir(const std::string &s) throw(std::string) {
  Dir &child = m_fs.openDir(joinNames(m_name, s));
  child.m_parent = this;
  return &child;
}
Dir *Dir::popDir() {
  Dir *parent = m_parent;
  delete this;
  return parent;
}

#if 0
void OCPI::Util::Vfs::Vfs::closeIterator (Iterator * it)
  throw (std::string) {
  delete it;
}
#endif
    }
  }
}
