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

#ifndef OCPIUTILVFSUTIL_H__
#define OCPIUTILVFSUTIL_H__

/**
 * \file
 * \brief Various helpers related to files and file systems.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>

namespace OCPI {
  namespace Util {
    namespace Vfs {

      class Vfs;

      /**
       * Determine if a file is an XML document.
       *
       * \param[in] fs       The file system containing a file.
       * \param[in] fileName The name of the file within the file system.
       * \return             Returns true if the file is determined to be
       *                     an XML document. Returns false if the file is
       *                     not an XML document.
       *
       * \throw std::string If the file can not be opened for reading.
       *
       * \note Uses OCPI::Util::Misc::isXmlDocument().
       */

      bool isXMLDocument (Vfs * fs, const std::string & fileName)
        throw (std::string);

      /**
       * \name Recursive Copying and Deleting
       */

      //@{

      /**
       * Recursively copy files and subdirectories to another directory.
       *
       * All files in the source directory that match a pattern are copied
       * into the destination directory.
       *
       * \param[in] srcFs   The source file system.
       * \param[in] srcDir  The name of a directory within the source file
       *                    system.
       * \param[in] destFs  The destination file system, which may be the
       *                    same as \a srcFs.
       * \param[in] destDir The name of a directory within the destination
       *                    file system.
       * \param[in] pattern A glob-style pattern; only files whose names
       *                    match the pattern are copied.
       *
       * \throw std::string In case of failure.
       */

      void copyFilesRecursively (Vfs * srcFs, const std::string & srcDir,
                                 Vfs * destFs, const std::string & destDir,
                                 const std::string & pattern = "*")
        throw (std::string);

      /**
       * Recursively copies a directory, including its files and
       * subdirectories, to another directory.
       *
       * The difference to copyFilesRecursively() is that \a destDir is
       * created before copying the contents of \a srcDir into it.
       *
       * \param[in] srcFs   The source file system.
       * \param[in] srcDir  The name of a directory within the source file
       *                    system.
       * \param[in] destFs  The destination file system, which may be the
       *                    same as \a srcFs.
       * \param[in] destDir The name of a directory within the destination
       *                    file system.
       * \param[in] pattern A glob-style pattern; only files whose names
       *                    match the pattern are copied.
       *
       * \throw std::string In case of failure.
       */

      void copyDirectoryRecursively (Vfs * srcFs, const std::string & srcDir,
                                     Vfs * destFs, const std::string & destDir,
                                     const std::string & pattern = "*")
        throw (std::string);

      /**
       * Recursively deletes all files matching a pattern.
       *
       * \param[in] fs      A file system.
       * \param[in] name    The name of a directory within the source file
       *                    system.
       * \param[in] pattern A glob-stype pattern.  All files matching this
       *                    pattern will be deleted.
       * \param[in] deleteEmptyDirectories Whether to remove directories
       *                    that become empty after all matching files are
       *                    deleted.
       *
       * \throw std::string In case of failure.
       */

      void removeFilesRecursively (Vfs * fs, const std::string & name,
                                   const std::string & pattern = "*",
                                   bool deleteEmptyDirectories = true)
        throw (std::string);

      /**
       * Recursively delete all files matching a pattern.
       *
       * If \a deleteEmptyDirectories is true, and the toplevel \a name
       * directory becomes empty, it is deleted as well.
       *
       * \param[in] fs      A file system.
       * \param[in] name    The name of a directory within the source file
       *                    system.
       * \param[in] pattern A glob-stype pattern.  All files matching this
       *                    pattern will be deleted.
       * \param[in] deleteEmptyDirectories Whether to remove directories
       *                    that become empty after all matching files are
       *                    deleted.
       *
       * \throw std::string In case of failure.
       */

      void removeDirectoryRecursively (Vfs * fs, const std::string & name,
                                       const std::string & pattern = "*",
                                       bool deleteEmptyDirectories = true)
        throw (std::string);

      //@}

      /**
       * \brief Scheduled removal of temporary files.
       *
       * Removes a file or a directory from its destructor, unless removal
       * is canceled.  This can be used, e.g., for temporary files, or to
       * clean up after failure: if something goes wrong (exception or
       * premature return), the file is removed; if everything goes well,
       * removal can be canceled to keep the file.
       */

      class EventualEraser {
      public:

        /**
         * Default constructor.
         *
         * A file can later be scheduled for removal using
         * #eventuallyErase().
         */

        EventualEraser ()
          throw ();

        /**
         * Constructor.
         *
         * Calls #eventuallyErase (\a fs, \a name, \a isFile).
         */

        EventualEraser (Vfs * fs, const std::string & name,
                        bool isFile = true)
          throw ();

        /**
         * Destructor.
         *
         * Attempts to erase a file or directory, if eventuallyErase()
         * was called and cancel() was not called afterwards.  Any error
         * deleting the file or directory is ignored.
         */

        ~EventualEraser ()
          throw ();

        /**
         * Schedule removal of a file or directory.  Arranges for the
         * destructor to remove the file or directory.
         *
         * \param[in] fs      A file system.
         * \param[in] name    The name of a file or directory within that file
         *                    system, which is to be removed eventually.
         * \param[in] isFile  Whether \a name identifies a file name (true) or
         *                    a directory (false).
         *
         * \throw std::string If this instance is already "in use."
         */

        void eventuallyErase (Vfs * fs, const std::string & name,
                              bool isFile = true)
          throw (std::string);

        /**
         * Cancel the eventual removal of the file.
         */

        void cancel ()
          throw ();

      protected:
        Vfs * m_fs;
        std::string m_name;
        bool m_isFile;
      };

    }
  }
}

#endif
