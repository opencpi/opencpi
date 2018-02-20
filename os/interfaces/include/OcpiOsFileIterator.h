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

#ifndef OCPIOSFILEITERATOR_H__
#define OCPIOSFILEITERATOR_H__

/**
 * \file
 *
 * \brief Iterates over a list of files and subdirectories in a directory.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Use 64-bit type for our opaque data, to ensure
 *                  alignment.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiOsDataTypes.h>
#include <string>
#include <ctime>

namespace OCPI {
  namespace OS {

    /**
     * \brief Iterates over a list of files and subdirectories in a directory.
     *
     * Iterate over a list of file names in the file system that match a
     * "glob"-style pattern.  FileIterator instances can be constructed
     * manually, and are returned from the FileSystem::list() operation.
     *
     * The end() operation should be called after construction, before
     * calling any of the other operations (with the exception of close()),
     * to ensure that the list of files is not empty.  Otherwise, if there
     * were no matching files, the other operations would fail.
     *
     * The ordering of files and subdirectories is implementation dependent.
     *
     * \note On Windows, networked shares are not included when listing
     * files in the root directory.  The root directory of a networked
     * share can not be listed.  E.g., listing the directory "/ad-fpilhofe1"
     * will fail even though the host may export shares.
     */

    class FileIterator {
    public:
      /**
       * Constructor: Initializes the FileIterator to iterate over all
       * files in directory \a dir matching the glob-style \a pattern.
       *
       * \param[in] dir The base directory in the file system for the
       *                search.
       * \param[in] pattern A glob-style pattern. This pattern may contain
       *               multiple path components, but only the last path
       *               component may contain wildcard characters.
       * \throw std::string If the directory name or pattern are invalid
       *               file names, if the directory does not exist, or
       *               if the directory can not be read.
       */

      FileIterator (const std::string & dir, const std::string & pattern)
        throw (std::string);

      /**
       * Copy constructor. The new instance will iterate over the same
       * set of files as the original, restarting from the first.
       *
       * \param[in] other Another FileIterator object.
       */

      FileIterator (const FileIterator & other)
        throw (std::string);

      /**
       * Destructor.
       *
       * \pre The close() operation must be called before destructing
       * the object.
       */

      ~FileIterator ()
        throw ();

      /**
       * Assignment operator. This instance will then iterate over the
       * same set of files as the original, restarting from the first.
       *
       * \param[in] other Another FileIterator object.
       */

      FileIterator & operator= (const FileIterator & other)
        throw (std::string);

      /**
       * Tests whether the iterator has moved beyond the last file,
       * including the case when the list of files was empty to begin
       * with.
       *
       * \return Returns false when there are more files matching the
       * search pattern. Returns true when there are no more matches.
       */

      bool end ()
        throw (std::string);

      /**
       * \return The relative file name of the matching
       * file. The name is relative to the \a dir directory that was
       * passed to the constructor.
       * \pre end() is false.
       */

      std::string relativeName ()
        throw ();
      const char *relativeName(std::string &)
        throw ();

      /**
       * \return The absolute file name of the matching file.
       * \pre end() is false.
       */

      std::string absoluteName ()
        throw ();
      const char *absoluteName(std::string &)
        throw ();

      /**
       * \return true if the current match identifies a
       * subdirectory. false if the match is for a plain file.
       * \pre end() is false.
       */

      bool isDirectory ()
        throw ();

      /**
       * \return size() returns the size of this file, in bytes.
       * \pre end() is false.
       * \pre isDirectory() is false.
       */

      unsigned long long size ()
        throw (std::string);

      /**
       * \return The last-modification timestamp for
       * the current file or directory, if available.
       * \pre end() is false.
       */

      std::time_t lastModified ()
        throw (std::string);

      /**
       * Advances the iterator to the next file in the list.
       *
       * \return true if the iterator finds another file that matches
       * the search pattern. false if there are no more matches.
       * \pre end() is false.
       * \post next() == ! end()
       */

      bool next ()
        throw (std::string);

      /**
       * Closes the iterator and releases associated resources. An
       * iterator must be closed before it can be destructed.
       */

      void close ()
        throw ();

    private:
      OCPI::OS::uint64_t m_osOpaque[256];
    };

  }
}

#endif
