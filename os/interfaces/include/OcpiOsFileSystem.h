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

#ifndef OCPIOSFILESYSTEM_H__
#define OCPIOSFILESYSTEM_H__

/**
 * \file
 *
 * \brief Access to the local file system.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string.h>
#include <string>
#include <ctime>
#include "OcpiOsFileIterator.h"

namespace OCPI {
  namespace OS {

    /**
     * \brief Access to the local file system.
     *
     * These functions abstract from local file name conventions
     * such as the path separator and drive names. The root directory
     * is "/", and '/' is used as the path separator. The toNativeName()
     * and fromNativeName() operations can be used to map a file name
     * between the native and the common format, e.g., for file names
     * that need to be passed to or are received from system calls.
     * Also, user inputs (from the command line, user interaction or
     * configuration files) are usually in native format and must be
     * mapped to the common format for processing.
     *
     * All operations, with the sole exception of fromNativeName(),
     * expect file names in "common" format.
     *
     * This file system abstraction does not consider "." or ".." to
     * be special. They may not be used as path components, except
     * in inputs to the fromNativeName() function.
     *
     * The file system maintains a "current working directory".
     * Relative file names, i.e., names that do not start with '/',
     * are interpreted to the current directory.
     *
     * There is only one file system that is shared by all threads
     * in the process. Multithreaded software needs to be careful
     * when using relative names, if the current working directory
     * may be changed by a different thread.
     */

    namespace FileSystem {

      struct FileId {
	uint64_t m_opaque[2];
	inline bool operator==(const FileId &rhs) const {
	  return memcmp(this, &rhs, sizeof(FileId)) == 0;
	}
	inline bool operator!=(const FileId &rhs) const {
	  return memcmp(this, &rhs, sizeof(FileId)) != 0;
	}
	// Used by STL set containers, etc.
	inline bool operator<(const FileId &rhs) const {
	  return memcmp(this, &rhs, sizeof(FileId)) < 0;
	}
      };
      /**
       * \name File Name Mapping
       *
       * Maps between the file names used by the FileSystem abstraction
       * and native file names, which can be passed to system calls or
       * other functions that require native names, such as
       * ProcessManager::start(), or std::fstream.
       *
       * E.g., on Windows, the native name for "/c:/Frank/sceos" is
       * "c:\Frank\sceos".
       */

      //@{

      /**
       * Maps a file name in "common" format to the "native" format.
       *
       * \param[in] name A file name in "common" format.
       * \return An equivalent file name in "native" format.
       * \throw std::string If \a name is an invalid common file name.
       */

      std::string toNativeName (const std::string & name)
        throw (std::string);

      /**
       * Maps a file name in "native" format to the "common" format.
       * If \a name is a relative name, it is interpreted according to
       * the current working directory.
       *
       * \param[in] name A file name in "native" format.
       * \return An equivalent file name in "common" format.
       * \throw std::string If \a name is an invalid native file name.
       */

      std::string fromNativeName (const std::string & name)
        throw (std::string);

      //@}

      /**
       * \name File Name Helpers
       *
       * These are convenience functions for composing and dissecting
       * file names from and to their path components.  They do not
       * check file existence.
       */

      //@{

      /**
       * Concatenates two path names.
       *
       * \param[in] dir A directory name. The name may be absolute or
       *            relative.
       * \param[in] name A file name. The name may be absolute or
       *            relative, and may contain multiple path components.
       * \return    The result of interpreting \a name relative to
       *            the \a dir directory. The resulting file name is
       *            absolute if \a name is absolute or if \a dir is
       *            absolute.
       */

      std::string joinNames (const std::string & dir,
                             const std::string & name)
        throw (std::string);
      const char * joinNames(const std::string &dir,
			     const std::string &name,
			     std::string &joined)
        throw (std::string);

      /**
       * Returns an absolute file name for a file name.
       *
       * \param[in] name A file name. If relative, it is interpreted relative
       *                 to the current working directory. If absolute, it is
       *                 returned unmodified.
       * \return         The absolute file name corresponding to \a name.
       *
       * \note Equivalent to #joinNames (cwd(), name).
       */

      std::string absoluteName (const std::string & name)
        throw (std::string);

      /**
       * Returns all but the last path component of a file name.
       *
       * \param[in] name A file name.
       * \return    All but the last path component of \a name. Returns
       *            cwd() if \a name contains a single path component
       *            only.
       */

      std::string directoryName (const std::string & name)
        throw (std::string);

      /**
       * Returns a file name's last path component.
       *
       * \param[in] name  A file name.
       * \return    The file name's last path component, which may be
       *            the same as \a name if it has a single path component
       *            only.
       */

      std::string relativeName (const std::string & name)
        throw (std::string);

      /**
       * Allows iterating over elements in a string that is composed of a
       * set of native file names that are separated by a special character,
       * e.g., like the PATH environment variable.
       *
       * \param[in,out] path The (remainder of the) path. This string should
       *               initially be initialized, e.g., from the PATH
       *               environment variable. The first element in the
       *               path is split off, converted to a native name,
       *               and returned. The remainder, which may be empty,
       *               is again assigned to the \a path variable. Thus,
       *               consecutive calls with the same variable yield
       *               the directories in the path.  If \a path is returned
       *               as empty, then there are no more directories in
       *               the path, and the iteration is complete.
       * \param[in] ignoreInvalid Whether invalid path elements, i.e.,
       *               file names that are extracted from the \a path but
       *               cannot be converted to native format (because
       *               fromNativeName() raises an exception) are
       *               ignored.  If false, then an exception is raised
       *               if an invalid file name is encountered.
       * \param[in] separator The character that separates file names in the
       *               \a path variable.  If NIL ('\\0'), then the system's
       *               default character for the PATH variable is used.
       * \return       The first directory from the \a path variable, in
       *               common format, or an empty string if \a path is
       *               empty, or if \a ignoreInvalid is true and \a path
       *               contains no more valid elements.
       *
       * \throw std::string If an invalid path element is encountered and
       * if the \a ignoreInvalid parameter is false.  The \a path variable is
       * still updated, so the caller may continue iterating over the
       * remaining elements.
       */

      std::string getPathElement (std::string & path,
                                  bool ignoreInvalid = true,
                                  char separator = '\0')
        throw (std::string);

      //@}

      /**
       * \name Directory Management
       */

      //@{

      /**
       * Returns the current working directory.
       *
       * \return The absolute name of the current working directory.
       */

      std::string cwd ()
        throw (std::string);

      /**
       * Changes the current working directory.
       *
       * \param[in] name  The desired directory to consider the new working
       *                  directory.
       *
       * \throw std::string If \a name does not exist, is not accessible,
       * or is not a directory.
       */

      void cd (const std::string & name)
        throw (std::string);

      /**
       * Create a directory.
       *
       * \param[in] name The name of the new directory.
       *
       * \throw std::string If \a name already exists, if the parent
       * directory (i.e., directoryName (name)) does not exist or is
       * not a directory, or if the parent directory is not writable.
       */

      void mkdir (const std::string & name, bool existsOk = false)
        throw (std::string);

      /**
       * Remove a directory.
       *
       * \param[in] name The name of the directory to remove.
       *
       * \throw Raises an exception of type std::string if \a name does
       * not exist or is not a directory, or if the parent directory is
       * not writable.
       *
       * \pre The directory shall be empty.
       */

      void rmdir (const std::string & name)
        throw (std::string);

      //@}

      /**
       * \name Directory Listing
       */

      //@{

      /**
       * Iterate over directory contents that match a search pattern.
       *
       * \param[in] dir     The base directory for the search.
       * \param[in] pattern A "glob"-style pattern. The pattern must be
       *               relative. It may contain multiple path components,
       *               but only the last may contain wildcard characters.
       * \return       A FileIterator object that can be used to iterate
       *               over the (potentially empty set of) matched files.
       *
       * \throw std::string If \a pattern is not relative, if it contains
       * wildcard characters in non-final path copmonents, or if
       * #directoryName(#joinNames(\a dir,\a pattern)) does not exist or
       * is not a directory.
       */

      FileIterator list (const std::string & dir = cwd(),
                         const std::string & pattern = "*")
        throw (std::string);

      //@}

      /**
       * \name File Information
       */

      //@{

      /**
       * Determines if a file or directory exists.
       *
       * \param[in] name   A file name.
       * \param[out] isDir If not null, and if the file exists, then true
       *              is returned if the file name identifies a directory,
       *              or false if the file name identifies a plain file.
       * \return      true if the file name exists in the file system,
       *              false if it does not exist.
       */

      bool exists (const char *name, bool * isDir = NULL, uint64_t *size = 0,
		   std::time_t *mtime = 0, FileId *id = NULL) throw();
      inline bool exists (const std::string & name, bool * isDir = 0,
                   uint64_t *size = 0, std::time_t *mtime = 0, FileId *id = NULL)
        throw () {
	return exists(name.c_str(), isDir, size, mtime, id);
      }

      /**
       * Returns the size of a file.
       *
       * \param[in] name   A file name.
       * \return           The size of the file, in octets (bytes).
       *
       * \throw std::string If \a name does not exist or is not a plain file.
       */

      unsigned long long size (const std::string & name)
        throw (std::string);

      /**
       * Returns the timestamp of the last modification.
       *
       * \param[in] name   A file name.
       * \return           The timestamp of the last modification.
       *
       * \throw std::string If \a name does not exist.
       */

      std::time_t lastModified (const std::string & name)
        throw (std::string);

      //@}

      /**
       * \name File System Operations
       */

      //@{

      /**
       * Rename a file.
       *
       * \param[in] srcName  The original file name.
       * \param[in] destName The new name for the file.
       *
       * \throw std::string If the file cannot be renamed.
       */

      void rename (const std::string & srcName,
                   const std::string & destName)
        throw (std::string);

      /**
       * Copy a file.
       *
       * \param[in] srcName  The original file name.
       * \param[in] destName The name for the new copy of the file.
       *
       * \throw std::string If the file cannot be copied.
       */

      void copy (const std::string & srcName,
                   const std::string & destName)
        throw (std::string);

      /**
       * Remove a file.
       *
       * \param[in] name A file name.
       *
       * \throw std::string if the file cannot be removed.
       */

      void remove (const std::string & name)
        throw (std::string);

      //@}
      extern const char *slashes;

      class Dir;
      Dir *openDir(const std::string &dir) throw(std::string);

      class Dir {
	intptr_t m_opaque;
	std::string m_name;
      public:
	Dir(const std::string &dir) throw (std::string);
	bool next(std::string &s, bool &isDir) throw(std::string);
	~Dir() throw();
      };
    }
  }
}

#endif
