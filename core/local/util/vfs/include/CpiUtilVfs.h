// -*- c++ -*-

#ifndef CPIUTILVFS_H__
#define CPIUTILVFS_H__

/**
 * \file
 * \brief The CPI::Util::Vfs::Vfs abstract base class for the file system API.
 *
 * This file defines the CPI::Util::Vfs::Vfs abstract base class,
 * providing a file system API.
 *
 * Implementations of the Vfs interface map file accesses to a
 * concrete file system.
 *
 * This header file also defines some helper functions.
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

namespace CPI {
  namespace Util {

    /**
     * \namespace CPI::Util::Vfs
     * \brief The Virtual File System abstraction.
     */

    namespace Vfs {

      class Vfs;
      class Iterator;

      /**
       * \name File Name Helpers
       */

      //@{

      /**
       * Concatenate two path names.
       *
       * \param[in] dir  A directory name. The name may or may not be absolute.
       * \param[in] name A file name. The name may be relative or absolute, and
       *                 may contain multiple path components.
       * \return    Returns the result of interpreting \a name relative to
       *            the \a dir directory. The resulting file name is absolute
       *            if \a name is absolute or if \a dir is absolute.
       * \throw std::string If \a dir or \a name are invalid filenames.
       */

      std::string joinNames (const std::string & dir,
			     const std::string & name)
	throw (std::string);

      /**
       * Return all but the last path component of a file name.
       *
       * \param[in] name A file name.
       * \return    All but the last path component of \a name.
       *
       * \throw std::string If \a name contains a single path component
       * only, or if it is an invalid file name.
       */

      std::string directoryName (const std::string & name)
	throw (std::string);

      /**
       * Return a file name's last path component.
       *
       * \param[in] name A file name.
       * \return    The last path component of \a name, which may be
       *            the same as \a name if it consists of a single path
       *            component only.
       * \throw std::string If \a name is an invalid file name.
       */

      std::string relativeName (const std::string & name)
	throw (std::string);

      //@}

      /**
       * \brief The abstract base class for all virtual file systems.
       *
       * A file system is a collection of files, which may be readable,
       * writable, or both.  File systems may have a directory hierarchy.
       * File names consist of a number of path components separated by
       * the "/" forward slash character.  File names that start with
       * a forward slash are called absolute file names, otherwise they
       * are called relative file names.  A relative file name is usually
       * relative to a specific directory.  Note that a relative name
       * can still contain more than one path component.
       *
       * Each instance of a file system maintains a "current" directory.
       * If the same VFS instance is shared between multiple threads,
       * names that are relative to the current working directory should
       * be used with care, if other threads might change the file system's
       * working directory concurrently.
       *
       * The root directory of each file system is named "/".  (In the
       * Windows file system-based implementation, the root directory
       * contains sub-directories for all of the local drives, as
       * identified by a drive letter and a colon, e.g., "/C:".)
       *
       * File streams are provided as C++ iostreams.  It is implementation
       * dependent whether streams are seekable.
       *
       * File systems can be identified by a common base URI.  Files and
       * directories within a file system can be identified by an absolute
       * URI.
       *
       * Implementations of this interface may have certain limitations
       * and may not support all features.  Check their manual for details.
       *
       * See also the CPI::Util::Vfs namespace.
       */

      class Vfs {
      public:
	/**
	 * Constructor.
	 *
	 * Not user callable.
	 */

	Vfs ()
	  throw ();

	/**
	 * Destructor.
	 */

	virtual ~Vfs ()
	  throw ();

	/**
	 * \name File Name URI Mapping
	 *
	 * The functions in this section map between URIs and file names.
	 */

	//@{

	/**
	 * Get the <em>base URI</em> for this file system.  The file
	 * system is contained entirely within the name space defined
	 * by the base URI.
	 *
	 * \return The base URI.
	 */

	virtual std::string baseURI () const 
	  throw () = 0;

	/**
	 * Map the name of a file within this file system to a URI.
	 *
	 * \param[in] name A file name.
	 * \return An equivalent URI corresponding to the file.
	 * \throw std::string If \a name is not a valid file name.
	 */

	virtual std::string nameToURI (const std::string & name) const
	  throw (std::string) = 0;

	/**
	 * Map a URI to the name of a file, if it identifies a file
	 * within this file system.
	 *
	 * \param[in] uri A URI.
	 * \return The name of a file within this file system that
	 *         corresponds to the URI.
	 *
	 * \throw std::string If the file system does not cover the
	 * name space that the URI references.
	 */

	virtual std::string URIToName (const std::string & uri) const
	  throw (std::string) = 0;

	//@}

	/**
	 * \name File Name Mapping
	 *
	 * Map between absolute and relative names.
	 */

	//@{

	/**
	 * Returns an absolute file name.
	 * Equivalent to CPI::Util::Vfs::joinNames (cwd(), name).
	 *
	 * \param[in] name A file name.  If relative, it is interpreted
	 *            relative to the current working directory.  If
	 *            absolute, it is returned unmodified.
	 * \return The absolute file name corresponding to \a name.
	 *
	 * \throw std::string If name is not a valid file name.
	 */

	virtual std::string absoluteName (const std::string & name) const
	  throw (std::string);

	/**
	 * Returns all but the last path component of a file name.
	 * Equivalent to CPI::Util::Vfs::directoryName (absoluteName (name)).
	 *
	 * \param[in] name A file name.
	 * \return All but the last path component of \a name.  Returns
	 *            cwd() if \a name contains a single path component
	 *            only.
	 * \throw std::string If \a name is an invalid file name.
	 */

	virtual std::string directoryName (const std::string & name) const
	  throw (std::string);

	//@}

	/**
	 * \name Directory Management
	 */

	//@{

	/**
	 * Returns the current working directory.
	 *
	 * \return The absolute path of the current working directory.
	 */

	virtual std::string cwd () const
	  throw (std::string) = 0;

	/**
	 * Change the current working directory.
	 *
	 * \param[in] dir The name of a directory to consider the new working
	 *                directory.
	 *
	 * \throw std::string If \a dir does not exist, is not a directory,
	 * or is not accessible.
	 *
	 * \note "." and ".." are not recognized as special names.  To change
	 * to the parent directory, use cd (directoryName (cwd())).
	 */

	virtual void cd (const std::string & dir)
	  throw (std::string) = 0;

	/**
	 * Create a new directory.
	 *
	 * \param[in] dir The name of a directory to create.
	 *
	 * \throw std::string If creation of the new directory fails,
	 * e.g., if it already exists, or if the parent directory does
	 * not exist or is not accessible.
	 */

	virtual void mkdir (const std::string & dir)
	  throw (std::string) = 0;

	/**
	 * Remove a directory.
	 *
	 * \param[in] dir The name of a directory to remove.  The directory
	 *                must be empty, i.e., not contain any files or
	 *                subdirectories.
	 *
	 * \throw std::string If removal fails, e.g., if it is not empty,
	 * or if the parent directory is not accessible.
	 */

	virtual void rmdir (const std::string & dir)
	  throw (std::string) = 0;

	//@}

	/**
	 * \name Directory Listing
	 */

	//@{

	/**
	 * Create an iterator to iterate over a set of files in
	 * a directory whose file names match a pattern.
	 *
	 * The \a pattern may consist of multiple path components, but only
	 * the last path component may contain wildcard characters.
	 *
	 * An empty set of matches is not an error, but will return an
	 * "empty" iterator, whose end() operation returns true.
	 *
	 * \param[in] dir The name of a directory to consider the base for
	 *                the iterator. The iterator object will report
	 *                relative file names against this base directory.
	 * \param[in] pattern A glob-style pattern.
	 * \return        A pointer to an iterator object. The iterator
	 *                object must eventually be released by passing it
	 *                to the closeIterator() operation.
	 *
	 * \throw std::string If the directory is not accessible, or if
	 * the pattern is not valid.
	 */

	virtual Iterator * list (const std::string & dir,
				 const std::string & pattern = "*")
	  throw (std::string) = 0;

	/**
	 * Close an iterator, and release associated resources.
	 *
	 * \param[in] it A pointer to an iterator object that was returned
	 *               from a call to list() on the same Vfs instance.
	 *
	 * \throw std::string If the iterator is not valid, or was not
	 * returned by a list() operation on this Vfs instance.
	 */

	virtual void closeIterator (Iterator * it)
	  throw (std::string) = 0;

	//@}

	/**
	 * \name File Information
	 */

	//@{

	/**
	 * Determine if a file or directory exists.
	 *
	 * \param[in] name   A file name.
	 * \param[out] isDir If not null, and if the file exists, then true
	 *              is returned if the file name identifies a directory,
	 *              or false if the file name identifies a plain file.
	 * \return      true if the file name exists in the file system,
	 *              false if it does not exist.
	 */

	virtual bool exists (const std::string & name, bool * isDir = 0)
	  throw (std::string) = 0;

	/**
	 * Returns the size of a file.
	 *
	 * \param[in] name  A file name.
	 * \return          The size of the file, in octets (bytes).
	 *
	 * \throw std::string If \a name does not exist or is not a
	 * plain file.
	 */

	virtual unsigned long long size (const std::string & name)
	  throw (std::string) = 0;

	/**
	 * Returns the timestamp of the last modification.
	 *
	 * \param[in] name   A file name.
	 * \return           The timestamp of the last modification.
	 *
	 * \throw std::string If "name" does not exist or is not accessible.
	 */

	virtual std::time_t lastModified (const std::string & name)
	  throw (std::string) = 0;

	//@}

	/**
	 * \name File I/O
	 */

	//@{

	/**
	 * Open a file for reading and writing.
	 *
	 * \param[in] name The name of a file to open.
	 * \param[in] mode Flags with various effects on the open stream. See
	 *                 the documentation for std::ios_base::openmode for
	 *                 more detail.
	 * \return         A stream to access the file. Must eventually be
	 *                 passed to the close() operation.
	 *
	 * \throw std::string If \a name does not exist or is not accessible.
	 */

	virtual std::iostream * open (const std::string & name, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
	  throw (std::string) = 0;

	/**
	 * Open a file for reading.
	 *
	 * \param[in] name The name of a file to open.
	 * \param[in] mode Flags with various effects on the open stream. See
	 *                 the documentation for std::ios_base::openmode for
	 *                 more detail.
	 * \return         A stream to access the file. Must eventually be
	 *                 passed to the close() operation.
	 *
	 * \throw std::string If \a name does not exist or is not accessible.
	 */

	virtual std::istream * openReadonly (const std::string & name, std::ios_base::openmode mode = std::ios_base::in)
	  throw (std::string) = 0;

	/**
	 * Open a file for writing.
	 *
	 * \param[in] name The name of a file to open.
	 * \param[in] mode Flags with various effects on the open stream. See
	 *                 the documentation for std::ios_base::openmode for
	 *                 more detail.
	 * \return         A stream to access the file. Must eventually be
	 *                 passed to the close() operation.
	 *
	 * \throw std::string If \a name does not exist or is not accessible.
	 */

	virtual std::ostream * openWriteonly (const std::string & name, std::ios_base::openmode mode = std::ios_base::out | std::ios_base::trunc)
	  throw (std::string) = 0;

	/**
	 * Close a stream, and release associated resources.
	 *
	 * \param[in] stream A stream that was returned from open(),
	 *                   openReadonly() or openWriteonly().
	 *
	 * \throw std::string If an error occurs while closing.  Some file
	 * system implementations may not detect errors until the data is
	 * flushed from a cache and synchronized with the storage during
	 * the close() operation.  Despite the exception, the stream is
	 * closed.
	 */

	virtual void close (std::ios * stream)
	  throw (std::string) = 0;

	//@}

	/**
	 * \name File System Operations
	 */

	//@{

	/**
	 * Copy a file.
	 *
	 * \param[in] srcName The name of the original file, in this file
	 *                    system.
	 * \param[in] destFs  The target file system, which may be the
	 *                    same as this file system.
	 * \param[in] destName The name of the copy, in the target
	 *                    filesystem.
	 *
	 * \throw std::string If an error occurs.
	 *
	 * \note Behavior is undefined if \a srcName identifies a directory.
	 *
	 * \note The default implementation opens the source file for reading,
	 * the destination file for writing, and copies all data from one
	 * stream to the other.
	 */

	virtual void copy (const std::string & srcName, Vfs * destFs, const std::string & destName)
	  throw (std::string);

	/**
	 * Move a file.
	 *
	 * \param[in] srcName The name of the original file, in this file
	 *                    system.
	 * \param[in] destFs  The target file system, which may be the same
	 *                    as this file system.
	 * \param[in] destName The new name of the file, in the target
	 *                    filesystem.
	 *
	 * \throw std::string std::string If an error occurs.
	 *
	 * \note Behavior is undefined if \a srcName identifies a directory.
	 *
	 * \note The default implementation of calls rename(), if \a this
	 * and \a destFs are equal, else it first calls
	 * copy(\a srcName, \a destFs, \a destName), then
	 * remove(\a srcName).
	 */

	virtual void move (const std::string & srcName, Vfs * destFs, const std::string & destName)
	  throw (std::string);

	/**
	 * Rename a file.
	 *
	 * \param[in] srcName The name of the original file.
	 * \param[in] destName The new name of the file.
	 *
	 * \throw std::string If an error occurs.
	 *
	 * \note Behavior is undefined if \a srcName identifies a directory.
	 *
	 * \note Equivalent to move (\a srcName, \a this, \a destName).
	 */

	virtual void rename (const std::string & srcName, const std::string & destName)
	  throw (std::string);

	/**
	 * Remove a file.
	 *
	 * \param[in] name The name of the file.
	 *
	 * \throw std::string if an error occurs.
	 *
	 * \note Behavior is undefined if \a srcName identifies a directory.
	 */

	virtual void remove (const std::string & name)
	  throw (std::string) = 0;

	//@}

      private:
	/**
	 * Not implemented.
	 */

	Vfs (const Vfs &);

	/**
	 * Not implemented.
	 */

	Vfs & operator= (const Vfs &);
      };

    }
  }
}

#endif
