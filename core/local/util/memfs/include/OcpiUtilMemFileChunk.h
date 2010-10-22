
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
