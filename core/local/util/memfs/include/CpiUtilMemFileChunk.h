// -*- c++ -*-

#ifndef CPIUTILMEMFILECHUNK_H__
#define CPIUTILMEMFILECHUNK_H__

/**
 * \file
 * \brief The CPI::Util::MemFs::MemFileChunk data type.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

namespace CPI {
  namespace Util {
    namespace MemFs {

      /**
       * \brief The CPI::Util::MemFs::MemFileChunk data type.
       *
       * Represent one piece (chunk) of an in-memory file.
       *
       * The CPI::Util::MemFs::StaticMemFile class allows splitting
       * file content across multiple discontinuous chunks.  This
       * avoids compiler limits for the length of string literals.
       *
       * An array of MemFileChunks can be represented as a seamless
       * stream of data using CPI::Util::MemFs::StaticMemFile.
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
