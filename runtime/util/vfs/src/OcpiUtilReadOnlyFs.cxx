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

#include <OcpiUtilReadOnlyFs.h>

/*
 * ----------------------------------------------------------------------
 * ReadOnlyFS implementation
 * ----------------------------------------------------------------------
 */

OCPI::Util::Vfs::ReadOnlyFs::ReadOnlyFs (OCPI::Util::Vfs::Vfs & delegatee)
  throw ()
  : FilterFs (delegatee)
{
}

OCPI::Util::Vfs::ReadOnlyFs::~ReadOnlyFs ()
  throw ()
{
}

void
OCPI::Util::Vfs::ReadOnlyFs::access (const std::string &,
                                    std::ios_base::openmode mode,
                                    bool)
  throw (std::string)
{
  if ((mode & std::ios_base::out) || (mode & std::ios_base::trunc)) {
    throw std::string ("readonly file system");
  }
}
