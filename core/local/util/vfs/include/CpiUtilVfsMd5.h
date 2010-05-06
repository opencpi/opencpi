// -*- c++ -*-

#ifndef CPIUTILVFSMD5_H__
#define CPIUTILVFSMD5_H__

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

namespace CPI {
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
