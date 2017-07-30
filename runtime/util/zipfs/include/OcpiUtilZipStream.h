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

#ifndef OCPIUTILZIPSTREAM_H__
#define OCPIUTILZIPSTREAM_H__

/**
 * \file
 * \brief Implementation of <em>std::istream</em> and <em>std::ostream</em> for ZIP files.
 *
 * Implementations of the <em>std::istream</em> and <em>std::ostream</em>
 * interfaces, which use the <em>minizip</em> API to read from or write to
 * files contained in a ZIP archive.
 *
 * Used internally within ZipFs.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <streambuf>
#include <iostream>
#include "zip.h"
#include "unzip.h"

namespace OCPI {
  namespace Util {
    namespace ZipFs {

      class ZipFs;

      /**
       * \brief Implementation of <em>std::streambuf</em> for reading from a ZIP file.
       */

      class zipibuf : public std::streambuf {
      public:
        zipibuf (unzFile = 0);
        ~zipibuf ();

        bool setFile (unzFile);

        /*
         * Implementation of the streambuf interface
         */

      protected:
        int_type underflow ();
        int_type uflow ();
        std::streamsize xsgetn (char *, std::streamsize);
        int_type pbackfail (int_type = std::streambuf::traits_type::eof());
        pos_type seekoff (off_type off, std::ios_base::seekdir way,
                          std::ios_base::openmode which);
        pos_type seekpos (pos_type pos, std::ios_base::openmode which);

        /*
         * Data
         */

      protected:
        /*
         * If havePending, pending is the next character in the stream.
         * Happens as a result of underflow().
         *
         * If !havePending && haveLast, pending is the last character
         * that was read. To be used by pbackfail(eof).
         */

        bool m_haveLast;
        bool m_havePending;
        int_type m_pending;
        unzFile m_zipFile;

      private:
        zipibuf (const zipibuf &);
        zipibuf & operator= (const zipibuf &);
      };

      /**
       * \brief Implementation of <em>std::streambuf</em> for writing to a ZIP file.
       */

      class zipobuf : public std::streambuf {
      public:
        zipobuf (zipFile = 0);
        ~zipobuf ();

        bool setFile (zipFile);

        /*
         * Implementation of the streambuf interface
         */

      protected:
        int_type overflow (int_type c = std::streambuf::traits_type::eof());
        std::streamsize xsputn (const char *, std::streamsize);

        /*
         * Data
         */
        
      protected:
        zipFile m_zipFile;
        
      private:
        zipobuf (const zipobuf &);
        zipobuf & operator= (const zipobuf &);
      };

      /**
       * \brief Implementation of <em>std::istream</em> for reading from a ZIP file.
       */

      /*
       * istream for reading from zip files
       */

      class zipistream : public std::istream {
      private:
        friend class ZipFs;

      public:
        zipistream (unzFile);
        ~zipistream ();

      protected:
        unzFile m_zip;
        zipibuf m_buf;

      private:
        zipistream (const zipistream &);
        zipistream & operator= (const zipistream &);
      };

      /**
       * \brief Implementation of <em>std::ostream</em> for writing to a ZIP file.
       */

      class zipostream : public std::ostream {
      private:
        friend class ZipFs;

      public:
        zipostream (zipFile);
        ~zipostream ();

      protected:
        zipFile m_zip;
        zipobuf m_buf;

      private:
        zipostream (const zipostream &);
        zipostream & operator= (const zipostream &);
      };

    }
  }
}

#endif
