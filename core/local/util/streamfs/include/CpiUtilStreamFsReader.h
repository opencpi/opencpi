// -*- c++ -*-

#ifndef CPIUTILSTREAMFSREADER_H__
#define CPIUTILSTREAMFSREADER_H__

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
#include <CpiOsMutex.h>
#include "CpiUtilVfs.h"

namespace CPI {
  namespace Util {
    namespace StreamFs {

      /**
       * \brief Extract a set of files from a single data stream.
       *
       * Extracts a set of files from a single data stream as written
       * by CPI::Util::StreamFs::StreamFsWriter.
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

      class StreamFsReader : public CPI::Util::Vfs::Vfs {
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

	StreamFsReader (CPI::Util::Vfs::Vfs * fs, const std::string & name)
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

	void openFs (CPI::Util::Vfs::Vfs * fs, const std::string & name)
	  throw (std::string);

	/**
	 * Closes the file system.
	 *
	 * \throw std::string Propagated from CPI::Util::Vfs::Vfs::close().
	 */

	void closeFs ()
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
	mutable CPI::OS::Mutex m_mutex;
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
