// -*- c++ -*-

#ifndef CPIUTILVFSITERATOR_H__
#define CPIUTILVFSITERATOR_H__

/**
 * \file
 * \brief The CPI::Util::Vfs::Iterator abstract base class.
 *
 * This file defines the CPI::Util::Vfs::Iterator abstract base
 * class, to iterate over a set of files, usually the matches in
 * a directory search.
 *
 * Instances of this class are returned from
 * CPI::Util::Vfs::Vfs::list().
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
    namespace Vfs {

      /**
       * \brief Iterate over a set of files.
       *
       * The Iterator class is used to iterate over a set of files and
       * directories.  It does not have a public constructor.  Instances
       * of this class are returned from the
       * CPI::Util::Vfs::Vfs::list() operation, and must be deleted
       * using the CPI::Util::Vfs::Vfs::closeIterator() operation.
       */

      class Iterator {
      protected:
	/**
	 * Constructor
	 *
	 * Not user callable.  Objects of this type are created by
	 * and returned from CPI::Util::Vfs::Vfs::list().
	 */

	Iterator ()
	  throw ();

	/**
	 * Destructor
	 *
	 * Not user callable.  Objects of this type must be passed to
	 * CPI::Util::Vfs::Vfs::closeIterator() for destruction.
	 */

	virtual ~Iterator ()
	  throw ();

      public:
	/**
	 * Test whether the iterator has moved beyond the last
	 * element in the set of files.
	 *
	 * \return false if the iterator points to a file in the set.
	 *         true if the iterator has moved beyond the last element.
	 *         In this case, none of the other operations may be
	 *         called.
	 */

	virtual bool end ()
	  throw (std::string) = 0;

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

	virtual bool next ()
	  throw (std::string) = 0;

	/**
	 * The relative name of the file that the iterator points to.
	 *
	 * For iterators returned from CPI::Util::Vfs::Vfs::list(), the
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
      };

    }
  }
}

#endif
