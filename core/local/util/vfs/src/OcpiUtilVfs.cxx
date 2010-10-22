
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

#include <OcpiUtilVfs.h>
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

/*
 * ----------------------------------------------------------------------
 * File Name Helpers
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Vfs::joinNames (const std::string & dir,
                           const std::string & name)
  throw (std::string)
{
  return OCPI::OS::FileSystem::joinNames (dir, name);
}

std::string
OCPI::Util::Vfs::directoryName (const std::string & name)
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
OCPI::Util::Vfs::relativeName (const std::string & name)
  throw (std::string)
{
  return OCPI::OS::FileSystem::relativeName (name);
}

/*
 * ----------------------------------------------------------------------
 * Vfs Constructor and Destructor
 * ----------------------------------------------------------------------
 */

OCPI::Util::Vfs::Vfs::Vfs ()
  throw ()
{
}

OCPI::Util::Vfs::Vfs::~Vfs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::Util::Vfs::Vfs::absoluteName (const std::string & name) const
  throw (std::string)
{
  return OCPI::Util::Vfs::joinNames (cwd(), name);
}

std::string
OCPI::Util::Vfs::Vfs::directoryName (const std::string & name) const
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
OCPI::Util::Vfs::Vfs::copy (const std::string & src,
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
OCPI::Util::Vfs::Vfs::rename (const std::string & src,
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
OCPI::Util::Vfs::Vfs::move (const std::string & src,
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
