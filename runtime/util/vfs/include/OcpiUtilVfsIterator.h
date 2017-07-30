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

#ifndef OCPIUTILVFSITERATOR_H__
#define OCPIUTILVFSITERATOR_H__

/**
 * \file
 * \brief The OCPI::Util::Vfs::Iterator abstract base class.
 *
 * This file defines the OCPI::Util::Vfs::Iterator abstract base
 * class, to iterate over a set of files, usually the matches in
 * a directory search.
 *
 * Instances of this class are returned from
 * OCPI::Util::Vfs::Vfs::list().
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <iostream>
#include <string>
#include <ctime>
#include "OcpiUtilVfs.h"

namespace OCPI {
  namespace Util {
    namespace Vfs {

      /**
       * \brief Iterate over a set of files.
       *
       * The Iterator class is used to iterate over a set of files and
       * directories.  It does not have a public constructor.  Instances
       * of this class are returned from the
       * OCPI::Util::Vfs::Vfs::list() operation, and must be deleted
       * using the OCPI::Util::Vfs::Vfs::closeIterator() operation.
       */

      class Iterator {
	friend class Vfs;
	Vfs &m_fs;
	std::string m_dirName, m_pattern;
	bool m_recursive;
	Dir *m_dir;
	size_t m_skipLength;
      public:
        /**
         * Constructor
         *
         * Not user callable.  Objects of this type are created by
         * and returned from OCPI::Util::Vfs::Vfs::list().
         */

        Iterator (Vfs &fs, const std::string &dir, const char *pattern = "*", bool recursive = false)
          throw (std::string);

        /**
         * Destructor
         *
         * Not user callable.  Objects of this type must be passed to
         * OCPI::Util::Vfs::Vfs::closeIterator() for destruction.
         */

        virtual ~Iterator ()
          throw ();

      public:
#if 0
        /**
         * Test whether the iterator has moved beyond the last
         * element in the set of files.
         *
         * \return false if the iterator points to a file in the set.
         *         true if the iterator has moved beyond the last element.
         *         In this case, none of the other operations may be
         *         called.
         */

        virtual bool end()
          throw (std::string) = 0;
#endif
        /**
         * Move the iterator to the next element in the set.
         *
         * \return true if the iterator, after moving to the next element,
         *         still points to an element in the set. Returns false
         *         if the iterator has moved beyond the last element.
         *
         * \pre end() is false.
         * \post next() == ! end()
         */

        virtual bool next(std::string &, bool &isDir)
          throw (std::string);

#if 0
        /**
         * The relative name of the file that the iterator points to.
         *
         * For iterators returned from OCPI::Util::Vfs::Vfs::list(), the
         * \a dir parameter indicates the base directory that the relative
         * name is relative to.
         *
         * \return The file name of the current item in the set, relative
         *         to the iterator's base directory, as determined during
         *         the iterator's creation.
         *
         * \pre end() is false.
         */

        virtual std::string relativeName ()
          throw (std::string) = 0;

        /*
         * The absolute name of the file that the iterator points to.
         *
         * \return The absolute file name of the current item in the set.
         *
         * \pre end() is false.
         */

        virtual std::string absoluteName ()
          throw (std::string) = 0;

        /**
         * Indicate whether the current item is a plain file or a directory.
         *
         * \return false if the current item is a plain file.
         *         true if the current item is a directory.
         *
         * \pre end() is false.
         */

        virtual bool isDirectory ()
          throw (std::string) = 0;

        /**
         * The size of the current item, if it is a plain file.
         *
         * \return The size of the current item, in octets.
         *
         * \pre end() is false.
         * \pre isDirectory() is false.
         */

        virtual unsigned long long size ()
          throw (std::string) = 0;

        /**
         * The last modification timestamp of the current item.
         *
         * \return The last modification timestamp, if available.
         *
         * \throw std::string If the timestamp can not be determined,
         * e.g., if the file system does not keep timestamps.
         *
         * \pre end() is false.
         */

        virtual std::time_t lastModified ()
          throw (std::string) = 0;
#endif
      };

    }
  }
}

#endif
