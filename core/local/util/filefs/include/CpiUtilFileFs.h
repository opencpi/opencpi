// -*- c++ -*-

#ifndef CPIUTILFILEFS_H__
#define CPIUTILFILEFS_H__

/**
 * \file
 * \brief Vfs implementation based on the local file system.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiUtilVfs.h>
#include <CpiUtilVfsIterator.h>
#include <CpiOsMutex.h>
#include <iostream>
#include <string>
#include <ctime>

namespace CPI {
  namespace Util {
    /**
     * \brief Vfs implementation based on the local file system.
     */

    namespace FileFs {

      /**
       * \brief Vfs implementation based on the local file system.
       *
       * This class implements the CPI::Util::Vfs::Vfs file system
       * interface, based on the operating system's idea of a file
       * system.  All operations are delegated to the functions in
       * the CPI::OS::FileSystem namespace.
       *
       * Two difference with respect to the CPI::OS::FileSystem functions
       * are that each FileFs instance maintains its own root directory,
       * so that only a subtree is exposed, and that each FileFs instance
       * maintains its own working directory.
       */

      class FileFs : public CPI::Util::Vfs::Vfs {
      public:
	/**
	 * Constructor.
	 *
	 * \param[in] root The directory that is used as this file system's
	 *             root directory.  It will be prepended to all file
	 *             names that are accessed by this FileFs instance.
	 *             The parameter must be an absolute path name that
	 *             identifies an existing directory.  Since ".." is
	 *             not a valid path component, only files within the
	 *             \a root and its subdirectories will be accessible.
	 */

	FileFs (const std::string & root)
	  throw (std::string);

	/**
	 * Destructor.
	 */

	~FileFs ()
	  throw ();

	/**
	 * \name Map to and from file system paths.
	 */

	//@{

	/**
	 * Convert the name of a file within this file system to a path
	 * within the local file system.  Essentially, this prepends the
	 * root directory to the file name.  The path name can then be
	 * used with the functions in the CPI::OS::FileSystem namespace.
	 *
	 * \param[in] name The name of a file within this file system.
	 * \return    An absolute path name within the local file system.
	 *
	 * \throw std::string If name is not a valid file name.
	 */

	std::string nameToPath (const std::string & name) const
	  throw (std::string);

	/**
	 * Convert a local file system path name to a file name within
	 * this file system instance.
	 *
	 * \param[in] path A path name within the local file system.
	 * \return         A file name within this file system.
	 *
	 * \throw std::string If \a path is not a valid path name, or
	 * if it points to a location outside of this file system's
	 * root directory.
	 */

	std::string pathToName (const std::string & path) const
	  throw (std::string);

	//@}

	/**
	 * \name Map to and from native file names.
	 */

	//@{

	/**
	 * Convert the name of a file within this file system to a file
	 * name in native format, i.e., one that can be passed to system
	 * calls.  This is equivalent to
	 * CPI::OS::FileSystem::toNativeName (nameToPath(name));
	 *
	 * \param[in] name A file name within this file system.
	 * \return         An absolute file name in "native" format.
	 *
	 * \throw std::string If name is not a valid file name.
	 */

	std::string toNativeName (const std::string & name) const
	  throw (std::string);

	/**
	 * Convert a native file name to a file name within this file
	 * system. This is equivalent to
	 * pathToName (CPI::OS::FileSystem::fromNativeName (path));
	 *
	 * \param[in] path A file name in "native" format.
	 * \return         A file name within this file system.
	 *
	 * \throw std::string If path is not a valid native file name,
	 * or if it points to a location outside of this file system's
	 * root directory.
	 */

	std::string fromNativeName (const std::string & path) const
	  throw (std::string);
	
	//@}
	
	/**
	 * \name Implementation of the CPI::Util::Vfs::Vfs interface.
	 */

	//@{

	/*
	 * The operations below implement the Vfs interface. See the Vfs
	 * interface for more detail.
	 */

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

	void mkdir (const std::string &)
	  throw (std::string);

	void rmdir (const std::string &)
	  throw (std::string);

	/*
	 * Directory Listing
	 */

	CPI::Util::Vfs::Iterator * list (const std::string & dir,
					 const std::string & pattern = "*")
	  throw (std::string);

	void closeIterator (CPI::Util::Vfs::Iterator *)
	  throw (std::string);

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

	std::iostream * open (const std::string &, std::ios_base::openmode = std::ios_base::in | std::ios_base::out)
	  throw (std::string);

	std::istream * openReadonly (const std::string &, std::ios_base::openmode = std::ios_base::in)
	  throw (std::string);

	std::ostream * openWriteonly (const std::string &, std::ios_base::openmode = std::ios_base::out | std::ios_base::trunc)
	  throw (std::string);

	void close (std::ios *)
	  throw (std::string);

	/*
	 * File System Operations
	 */

	void move (const std::string &, Vfs *, const std::string &)
	  throw (std::string);

	void rename (const std::string &, const std::string &)
	  throw (std::string);

	void remove (const std::string &)
	  throw (std::string);

	//@}

      protected:
	/** \cond */
	static void testFilenameForValidity (const std::string &)
	  throw (std::string);

	std::string nativeFilename (const std::string &) const
	  throw (std::string);

	std::string absoluteNameLocked (const std::string &) const
	  throw (std::string);

      protected:
	std::string m_baseURI;
	std::string m_root;
	std::string m_cwd;
	mutable CPI::OS::Mutex m_lock;

	/** \endcond */

      private:
	/**
	 * Not implemented.
	 */

	FileFs (const FileFs &);

	/**
	 * Not implemented.
	 */

	FileFs & operator= (const FileFs &);
      };

    }
  }
}

#endif
