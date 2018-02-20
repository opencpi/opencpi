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

#ifndef OCPIUTILSTATICMEMFILE_H__
#define OCPIUTILSTATICMEMFILE_H__

/**
 * \file
 * \brief Abstraction of in-memory files.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiUtilMemFileChunk.h>
#include <istream>
#include <ctime>

namespace OCPI {
  namespace Util {
    namespace MemFs {

      /**
       * \brief Abstraction of in-memory files.
       *
       * StaticMemFile provides read-only access to an, in-memory file.
       * The openReadonly() operation returns an std::istream reading
       * from a static buffer.
       *
       * Data may be a contiguous chunk of memory (pointer and size),
       * or a set of memory chunks using a null-terminated array of
       * OCPI::Util::MemFs::MemFileChunk elements, i.e., the end of
       * the array is marked by an element whose "ptr" value is 0.
       * In the latter case, the discontinuity is not visible to
       * readers of the file, which will see a seamless stream of
       * data.  This is useful to get around compiler limitations
       * for the length of static strings.
       *
       * StaticMemFile objects can be "mounted" into a virtual file
       * system of type OCPI::Util::MemFs::StaticMemFs to be accessible
       * through the Vfs file system interface.
       *
       * Example:
       *
       * \code
       *   static const char filePtr1[] = "..."; // first chunk
       *   static const char filePtr2[] = "..."; // second chunk
       *
       *   MemFileChunk fileChunks[] = {
       *     { filePtr1, sizeof(filePtr1) },
       *     { filePtr2, sizeof(filePtr2) },
       *     { 0, 0 }
       *   };
       *
       *   // An in-memory file using a contiguous chunk of memory
       *   OCPI::Util::MemFs::StaticMemFile f1 ("Hello World!", 13);
       *
       *   // An in-memory file using a discontiguous chunk of memory
       *   OCPI::Util::MemFs::StaticMemFile f2 (fileChunks);
       * \endcode
       *
       * The off-line tool <tt>strfile.tcl</tt> allows "compiling"
       * a file into C++ code, so that it can be accessed via a
       * StaticMemFile object.  See OCPI::Util::MemFs for an example.
       */

      class StaticMemFile {
      public:
        /**
         * Constructor for a file whose data consists of a single chunk
         * of memory.
         *
         * \param[in] ptr   Pointer to the file's contents.
         * \param[in] size  The amount of data in the file, in octets.
         * \param[in] lastModified The timestamp of the last modification,
         *                  or -1 if unknown.
         */

        StaticMemFile (const char * ptr,
                       unsigned long long size,
                       std::time_t lastModified = (std::time_t) -1)
          throw ();

        /**
         * Constructor for a file whose data is in multiple chunks.
         *
         * \param[in] chunks Null-terminated array of MemFileChunk elements.
         * \param[in] lastModified The timestamp of the last modification,
         *              or -1 if unknown.
         */

        StaticMemFile (const OCPI::Util::MemFs::MemFileChunk * chunks,
                       std::time_t lastModified = (std::time_t) -1)
          throw ();

        /**
         * Destructor.
         */

        ~StaticMemFile ()
          throw ();

        /**
         * File size.
         *
         * \return The size, in octets.
         */

        unsigned long long size ()
          throw ();

        /**
         * Last modification timestamp.
         *
         * \return The last modification timestamp, or -1 if unknown.
         */

        std::time_t lastModified ()
          throw ();

        /**
         * Open the file.
         *
         * \return An object of type std::istream that can be used to
         *         read from the file.  This object must eventually be
         *         released using close().
         */

        std::istream * openReadonly ()
          throw ();

        /**
         * Release resources associated with an open stream.
         *
         * \param[in] str A stream object previously returned from
         *                openReadonly().
         */

        void close (std::ios * str)
          throw ();

        /** \cond */

      private:
        unsigned long long m_size;
        std::time_t m_lastModified;
        const char * m_ptr; // either ...
        const OCPI::Util::MemFs::MemFileChunk * m_chunks; // or

        /** \endcond */

      private:
        /**
         * Not implemented.
         */

        StaticMemFile (const StaticMemFile &);

        /**
         * Not implemented.
         */

        StaticMemFile & operator= (const StaticMemFile &);
      };

    }
  }
}

#endif
