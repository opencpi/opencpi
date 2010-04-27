// -*- c++ -*-

#ifndef CPICFUTILSCAFS_H__
#define CPICFUTILSCAFS_H__

/**
 * \file
 * \brief Vfs implementation using an SCA FileSystem.
 *
 * Revision History:
 *
 *     06/30/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <ctime>
#include <string>
#include <iostream>
#include <CpiOsMutex.h>
#include <CpiUtilVfs.h>
#include <CpiUtilVfsIterator.h>
#include <CF.h>

namespace CPI {
  namespace CFUtil {
    /**
     * \brief Vfs implementation using an SCA FileSystem.
     *
     * This class implements the CPI::Util::Vfs::Vfs file system by
     * delegating to an SCA CF::FileSystem instance.
     */

    class SCAFs : public CPI::Util::Vfs::Vfs {
    public:
      SCAFs (CORBA::ORB_ptr orb, CF::FileSystem_ptr fs)
	throw (std::string);

      /**
       * Destructor.
       */

      ~SCAFs ()
	throw ();

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

      void copy (const std::string &, Vfs *, const std::string &)
	throw (std::string);

      void remove (const std::string &)
	throw (std::string);

      //@}

    protected:
      std::string m_baseURI;
      std::string m_cwd;
      mutable CPI::OS::Mutex m_lock;
      CORBA::ORB_var m_orb;
      CF::FileSystem_var m_fs;

      /** \endcond */

    private:
      /**
       * Not implemented.
       */

      SCAFs (const SCAFs &);

      /**
       * Not implemented.
       */

      SCAFs & operator= (const SCAFs &);
    };

  }
}

#endif
