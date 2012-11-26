
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

#ifndef OCPIUTILSTREAMFSREADER_H__
#define OCPIUTILSTREAMFSREADER_H__

/**
 * \file
 * \brief Extract a set of files from a single data stream.
 *
 * Revision History:
 *
 *     05/27/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <iostream>
#include <string>
#include <ctime>
#include <map>
#include <OcpiOsMutex.h>
#include "OcpiUtilVfs.h"

namespace OCPI {
  namespace Util {
    namespace StreamFs {

      /**
       * \brief Extract a set of files from a single data stream.
       *
       * Extracts a set of files from a single data stream as written
       * by OCPI::Util::StreamFs::StreamFsWriter.
       *
       * Only one file can be open for reading at a time.  Files can
       * not be written, erased or modified.
       *
       * The class is single-threaded only, not reentrant.
       *
       * If files are read in the same sequence as they were written,
       * then no seeking of the data stream is required after reading
       * the table of contents (which requires 3 seek operations, one
       * forward and two backwards).
       */

      class StreamFsReader : public OCPI::Util::Vfs::Vfs {
      public:
        /**
         * Constructor.
         *
         * Must call open().
         */

        StreamFsReader ()
          throw ();

        /**
         * Constructor.
         *
         * Calls #openFs (\a stream).
         *
         * \param[in] stream  The stream to read from.
         *
         * \throw std::string Read error, or invalid file.
         *
         * \pre \a stream shall be open for reading in binary mode.
         * \pre \a stream shall be seekable.
         * \post \a stream shall not be manipulated directly throughout
         * the lifetime of this object.
         */

        StreamFsReader (std::istream * stream)
          throw (std::string);

        /**
         * Constructor.
         *
         * Calls #openFs (\a fs, \a name).
         *
         * \param[in] fs    A file system instance that contains the file.
         * \param[in] name  The name of the file in the file system to
         *                  read from.
         *
         * \throw std::string If the file can not be opened for reading.
         * \throw std::string Read error, or invalid file.
         */

        StreamFsReader (OCPI::Util::Vfs::Vfs * fs, const std::string & name)
          throw (std::string);

        /**
         * Destructor.
         *
         * Calls closeFs(), ignoring any errors.
         */

        ~StreamFsReader ()
          throw ();

        /**
         * \name Opening and closing stream file systems.
         */

        //@{

        /**
         * Sets the stream to read the file system contents from.
         *
         * \param[in] stream  The stream to read from.
         *
         * \throw std::string Read error, or invalid file.
         *
         * \pre \a stream shall be open for reading in binary mode.
         * \post \a stream shall not be manipulated directly throughout
         * the lifetime of this object.
         */

        void openFs (std::istream * stream)
          throw (std::string);

        /**
         * Opens a file to read the file system contents from.
         *
         * \param[in] fs    A file system instance that contains the file.
         * \param[in] name  The name of the file in the file system to
         *                  read from.
         *
         * \throw std::string If the file can not be opened for reading.
         * \throw std::string Read error, or invalid file.
         */

        void openFs (OCPI::Util::Vfs::Vfs * fs, const std::string & name)
          throw (std::string);

        /**
         * Closes the file system.
         *
         * \throw std::string Propagated from OCPI::Util::Vfs::Vfs::close().
         */

        void closeFs ()
          throw (std::string);

        //@}

        /**
         * \name Implementation of the OCPI::Util::Vfs::Vfs interface.
         */

        //@{

        /*
         * File Name URI Mapping
         */

        std::string baseURI () const
          throw ();

        std::string nameToURI (const std::string &) const
          throw (std::string);

        std::string URIToName (const std::string &) const
          throw (std::string);

        /*
         * Directory Management
         */

        std::string cwd () const
          throw (std::string);

        void cd (const std::string &)
          throw (std::string);

        /**
         * Not supported by this file system.
         */

        void mkdir (const std::string &)
          throw (std::string);

        /**
         * Not supported by this file system.
         */

        void rmdir (const std::string &)
          throw (std::string);

#if 0
        /*
         * Directory Listing
         */

        OCPI::Util::Vfs::Iterator * list (const std::string & dir,
                                         const std::string & pattern = "*")
          throw (std::string);

        void closeIterator (OCPI::Util::Vfs::Iterator *)
          throw (std::string);
#endif
        /*
         * File Information
         */

        bool exists (const std::string &, bool * = 0)
          throw (std::string);

        unsigned long long size (const std::string &)
          throw (std::string);

        std::time_t lastModified (const std::string &)
          throw (std::string);

        /*
         * File I/O
         */

        /**
         * Not supported by this file system.
         */

        std::iostream * open (const std::string &, std::ios_base::openmode = std::ios_base::in | std::ios_base::out)
          throw (std::string);

        std::istream * openReadonly (const std::string &, std::ios_base::openmode = std::ios_base::in)
          throw (std::string);

        /**
         * Not supported by this file system.
         */

        std::ostream * openWriteonly (const std::string &, std::ios_base::openmode = std::ios_base::out | std::ios_base::trunc)
          throw (std::string);

        void close (std::ios *)
          throw (std::string);

        /*
         * File System Operations. Not implemented.
         */

        /**
         * Not supported by this file system.
         */

        void remove (const std::string &)
          throw (std::string);

        //@}

      protected:
        /** \cond */

        void readTOC ()
          throw (std::string);

        std::string absoluteNameLocked (const std::string &) const
          throw (std::string);

      public:
        struct Node {
          unsigned long long pos;
          unsigned long long size;
          std::time_t lastModified;
        };

        typedef std::map<std::string, Node> TOC;

      protected:
        Vfs * m_fs;
        std::string m_name;
        std::istream * m_stream;
        unsigned long long m_pos;

        TOC m_toc;
        std::string m_cwd;
        std::string m_baseURI;

        unsigned long m_openFiles;
        unsigned long m_openIterators;
        mutable OCPI::OS::Mutex m_mutex;
        /** \endcond */

      private:
        /**
         * Not implemented.
         */

        StreamFsReader (const StreamFsReader &);

        /**
         * Not implemented.
         */

        StreamFsReader & operator= (const StreamFsReader &);
      };

    }
  }
}

#endif
