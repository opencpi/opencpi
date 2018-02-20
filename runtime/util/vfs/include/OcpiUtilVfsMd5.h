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

// -*- c++ -*-

#ifndef OCPIUTILVFSMD5_H__
#define OCPIUTILVFSMD5_H__

#include <iostream>
#include <string>

/**
 * \file
 * \brief Compute MD5 checksums for files.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

namespace OCPI {
  namespace Util {
    namespace Vfs {

      class Vfs;

      /**
       * \name Compute MD5 checksums for files
       */

      //@{

      /**
       * Compute MD5 checksum for data in a stream.
       *
       * \param[in] stream The stream to read data from.  It should be open
       *                   in binary mode.
       * \param[in] count  The maximum number of octets to read from the
       *                   stream.  If -1, or if larger than the remaining
       *                   amount of data in the stream, reads up to the
       *                   end of the stream.
       * \return           The MD5 checksum, as a 32 character hexadecimal
       *                   string.
       *
       * \throw std::string If reading from the stream fails.
       */

      std::string md5 (std::istream * stream, std::streamsize count = -1)
        throw (std::string);

      /**
       * Compute MD5 checksum for a file.
       *
       * \param[in] fs    The file system containing a file.
       * \param[in] name  The name of the file within the file system.
       * \return          The MD5 checksum, as a 32 character hexadecimal
       *                  string.
       *
       * \throw std::string If the file can not be opened for reading, or
       * if reading from the file fails.
       */

      std::string md5 (Vfs * fs, const std::string & name)
        throw (std::string);

      //@}

    }
  }
}

#endif
