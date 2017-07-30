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

#ifndef OCPIUTILMEMFILECHUNK_H__
#define OCPIUTILMEMFILECHUNK_H__

/**
 * \file
 * \brief The OCPI::Util::MemFs::MemFileChunk data type.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

namespace OCPI {
  namespace Util {
    namespace MemFs {

      /**
       * \brief The OCPI::Util::MemFs::MemFileChunk data type.
       *
       * Represent one piece (chunk) of an in-memory file.
       *
       * The OCPI::Util::MemFs::StaticMemFile class allows splitting
       * file content across multiple discontinuous chunks.  This
       * avoids compiler limits for the length of string literals.
       *
       * An array of MemFileChunks can be represented as a seamless
       * stream of data using OCPI::Util::MemFs::StaticMemFile.
       */

      struct MemFileChunk {
        /**
         * Pointer to the first octet of the chunk.
         */

        const char * ptr;

        /**
         * The number of octets in this chunk.
         */

        unsigned long long size;
      };

    }
  }
}

#endif
