// -*- c++ -*-

#ifndef CPIUTILURIFS_H__
#define CPIUTILURIFS_H__

/**
 * \file
 * \brief Delegate file access to mounted file systems, using URIs as file names.
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
#include <vector>
#include <map>

namespace CPI {
  namespace Util {
    namespace Vfs {

      /**
       * \brief Delegate file access to mounted file systems, using URIs as file names.
       *
       * This class implements the CPI::Util::Vfs::Vfs interface.
       *
       * URIs are directly used as file names, so that URI path components
       * make up the file name's path components, e.g.,
       * "/http/www.mc.com/index.html".
       *
       * All access is delegated to a set of "mounted" secondary Vfs
       * instances, based on whether a secondary Vfs supports the URI,
       * as determined by its URIToName() operation.
       */

      class UriFs : public CPI::Util::Vfs::Vfs {
      public:
        /**
         * Constructor.
         *
         * Initializes the UriFs instance with an empty set of delegatees.
         */

        UriFs ()
          throw ();

        /**
         * Destructor.
         *
         * Deletes all adopted delegatees.
         */

        ~UriFs ()
          throw ();

        /**
         * Mounts a delegatee.
         *
         * File access is delegated to \a delegatee if the
         * URIToName() operation confirms that the file is located in
         * the \a delegatee's file system.
         *
         * \param[in] delegatee A secondary Vfs instance to delegate
         *                      accesses to, if the delegatee covers
         *                      the namespace that a file is in. 
         * \param[in] adopt     Whether the delegatee is to be adopted.
         *                      If true, the delegatee is deleted when
         *                      unmounted or by the destructor.
         */

        void mount (CPI::Util::Vfs::Vfs * delegatee, bool adopt = false)
          throw (std::string);

        /**
         * Unmount a delegatee.
         *
         * If the delegatee was adopted during mount(), it is deleted.
         *
         * \param[in] delegatee The Vfs instance to unmount.
         */

        void unmount (CPI::Util::Vfs::Vfs * delegatee)
          throw (std::string);

        /**
         * \name Implementation of the CPI::Util::Vfs::Vfs interface.
         *
         * These operations are implemented by UriFs, delegating to
         * the appropriate delegatee.
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

        Iterator * list (const std::string & dir,
                         const std::string & pattern = "*")
          throw (std::string);

        void closeIterator (Iterator *)
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

        void copy (const std::string &, CPI::Util::Vfs::Vfs *, const std::string &)
          throw (std::string);

        void move (const std::string &, CPI::Util::Vfs::Vfs *, const std::string &)
          throw (std::string);

        void remove (const std::string &)
          throw (std::string);

        //@}

      protected:
        /** \cond */
        struct MountPoint {
          bool adopted;
          std::string baseURI;
          CPI::Util::Vfs::Vfs * fs;
        };

        typedef std::vector<MountPoint> MountPoints;
        typedef std::map<CPI::Util::Vfs::Iterator *, CPI::Util::Vfs::Vfs *> OpenIterators;
        typedef std::map<std::ios *, CPI::Util::Vfs::Vfs *> OpenFiles;
    
      protected:
        std::string absoluteNameLocked (const std::string &) const
          throw (std::string);
        std::string absoluteNameToURI (const std::string &) const
          throw (std::string);
        CPI::Util::Vfs::Vfs * findFs (const std::string &, std::string &) const
          throw (std::string);

      protected:
        std::string m_cwd;
        MountPoints m_mountPoints;
        OpenIterators m_openIterators;
        OpenFiles m_openFiles;
        mutable CPI::OS::RWLock m_lock;

        /** \endcond */

      private:
        /**
         * Not implemented.
         */

        UriFs (const UriFs &);

        /**
         * Not implemented.
         */

        UriFs & operator= (const UriFs &);
      };

    }
  }
}

#endif
