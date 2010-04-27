// -*- c++ -*-

#ifndef CPIUTILZIPFS_H__
#define CPIUTILZIPFS_H__

/**
 * \file
 * \brief Vfs implementation based on the contents of a Zip file.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiUtilVfs.h>
#include <CpiOsRWLock.h>
#include <iostream>
#include <string>
#include <ctime>
#include <map>

namespace CPI {
  namespace Util {
    /**
     * \brief Defines the CPI::Util::ZipFs::ZipFs class.
     */

    namespace ZipFs {

      /**
       * \brief Vfs implementation based on the contents of a Zip file.
       *
       * This class implements the CPI::Util::Vfs::Vfs file system
       * interface, representing the contents of a Zip file.
       *
       * \note Streams returned by openReadonly() or openWriteonly() are
       * not seekable.  The ZIP file format does not allow for in-place
       * modification, thus open() always fails.
       *
       * \note Only a single file can be open for writing at any time.
       *
       * \note Zip files do not have a directory structure, "directories"
       * are only implied by file names that consist of mulitple path
       * components. Thus, mkdir() and cd() do not have a way of checking
       * that a directory exists, and always succeed.  rmdir() and exists()
       * simply check whether any file has a given prefix.
       */

      class ZipFs : public CPI::Util::Vfs::Vfs {
      public:
	/**
	 * Constructor.
	 *
	 * Must call openZip() to open a Zip file.
	 */

	ZipFs ()
	  throw ();

	/**
	 * Constructor.
	 *
	 * Calls #openZip (\a fs, \a name, \a mode, \a adopt, \a keepOpen).
	 *
	 * \param[in] fs   The file system containing the Zip file.
	 * \param[in] name The name of the Zip file in the file system.
	 * \param[in] mode Whether to allow reading, writing or both.
	 * \param[in] adopt Whether to adopt the \a fs.
	 * \param[in] keepOpen Whether to cache the Zip file handle.
	 *
	 * \throw std::string See openZip().
	 *
	 * \note See openZip() for more details.
	 */

	ZipFs (CPI::Util::Vfs::Vfs * fs, const std::string & name,
	       std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
	       bool adopt = false,
	       bool keepOpen = false)
	  throw (std::string);

	/**
	 * Destructor.
	 *
	 * Calls closeZip().
	 */

	~ZipFs ()
	  throw ();

	/**
	 * \name Opening and closing Zip files.
	 */

	//@{

	/**
	 * Opens a Zip file.
	 *
	 * If (\a mode & std::ios_base::trunc), a new empty ZIP file is
	 * created; an existing file with that name is overwritten.
	 *
	 * \param[in] fs   The file system containing the Zip file.
	 * \param[in] name The name of the Zip file in the file system.
	 * \param[in] mode Whether to allow reading (std::ios_base::in),
	 *                 writing (std::ios_base::out) or both
	 *                 both (std::ios_base::in|std::ios_base::out).
	 * \param[in] adopt Whether to adopt the \a fs.  If true, \a fs
	 *                 is deleted when the zip file is closed.
	 * \param[in] keepOpen If false, then the Zip file is closed
	 *                 whenever no file within the Zip file is being
	 *                 read or written. If set to true, then the Zip
	 *                 file handle is cached and the Zip file is only
	 *                 re-opened when switching between reading and
	 *                 writing files.
	 *
	 * \pre No Zip file is currently open in this instance.
	 *
	 * \pre If !(\a mode & std::ios_base::trunc), \a fileName must
	 * identify an existing  ZIP file within the \a fs file system.
	 *
	 * \pre To read from Zip files, the stream that is returned from
	 * \a fs->openReadonly (\a name) shall be randomly seekable.
	 *
	 * \pre To write to Zip files, the stream that is returned from
	 * \a fs->open (\a name) shall be randomly seekable, and it shall
	 * be possible to modify the directory that contains the Zip file.
	 *
	 * \post A Zip file is open.
	 *
	 * \note Setting \a keepOpen to true can significantly reduce I/O
	 * when reading or writing multiple files, because the underlying
	 * zlib library will re-read the Zip file's table of contents
	 * every time the Zip file is re-opened.  However, if \a keepOpen
	 * is set to true, an error flushing written data that would
	 * normally be result in close() raising an exception, may be
	 * delayed until the next file access.  In particular, this may
	 * cause closeZip() to raise an exception.
	 *
	 * \note The ZIP file format by itself does not support
	 * file removal.  File deletion is accomplished by copying the
	 * contents of the ZIP file, minus the to-be deleted file, to
	 * a new temporary file, deleting the original ZIP, and then
	 * renaming the temporary file.  So these operations need to
	 * be supported by the file system that holds the ZIP file.
	 * Writing to an existing file involves deleting that file
	 * first.
	 */

	void openZip (CPI::Util::Vfs::Vfs * fs, const std::string & name,
		      std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
		      bool adopt = false,
		      bool keepOpen = false)
	  throw (std::string);

	/**
	 * Close this Zip file.
	 *
	 * \throw std::string If an I/O error occurs closing the file.
	 *
	 * \pre A Zip file is open.
	 * \post No Zip file is open.
	 */

	void closeZip ()
	  throw (std::string);

	//@}

	/**
	 * \name Implementation of the CPI::Util::Vfs::Vfs interface.
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

	/**
	 * \note ZipFs does not support open().  It always throws an exception.
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

	void rename (const std::string &, const std::string &)
	  throw (std::string);

	void remove (const std::string &)
	  throw (std::string);

	//@}

	/** \cond */

      protected:
	static void testFilenameForValidity (const std::string &)
	  throw (std::string);

	std::string nativeFilename (const std::string &)
	  throw ();

      protected:
	void updateContents ()
	  throw (std::string);

	std::string absoluteNameLocked (const std::string &) const
	  throw (std::string);

	void removeLocked (const std::string &)
	  throw (std::string);

      protected:
	/*
	 * Our base URI
	 */

	std::string m_baseURI;

	/*
	 * File system that contains the ZIP file, and its name
	 */

	std::ios_base::openmode m_mode;
	CPI::Util::Vfs::Vfs * m_fs;
	std::string m_zipFileName;
	bool m_adopted;
	bool m_keepOpen;

	/*
	 * Open zip file handles, if m_keepOpen is true.  The types are
	 * zipFile and unzFile, respectively, but we use void pointers
	 * so that we don't have to drag in zip.h and unzip.h.
	 */

	void * m_zipFile;
	void * m_unzFile;

	/*
	 * Our "working directory" within the ZIP file
	 */

	std::string m_cwd;

	/*
	 * Table of Contents
	 */

      public:
	struct FileInfo {
	  unsigned long long size;
	  std::time_t lastModified;
	};

	typedef std::map<std::string, FileInfo> FileInfos;

      protected:
	FileInfos m_contents;

	/*
	 * Implementation of the I/O API for zip/unzip
	 */

	void * m_zFileFuncsOpaque[8];

	/*
	 * For reentrancy.
	 */

	mutable CPI::OS::RWLock m_lock;

	/** \endcond */

      private:
	/**
	 * Not implemented.
	 */

	ZipFs (const ZipFs &);

	/**
	 * Not implemented.
	 */

	ZipFs & operator= (const ZipFs &);
      };

    }
  }
}

#endif
