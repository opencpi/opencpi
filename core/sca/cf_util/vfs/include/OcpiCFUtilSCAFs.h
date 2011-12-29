
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

// -*- c++ -*-

#ifndef OCPICFUTILSCAFS_H__
#define OCPICFUTILSCAFS_H__

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
#include <OcpiOsMutex.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilVfsIterator.h>
#include <CF.h>

namespace OCPI {
  namespace CFUtil {
    /**
     * \brief Vfs implementation using an SCA FileSystem.
     *
     * This class implements the OCPI::Util::Vfs::Vfs file system by
     * delegating to an SCA CF::FileSystem instance.
     */

    class SCADir;
    class SCAFs : public OCPI::Util::Vfs::Vfs {
      friend class SCADir;
    public:
      SCAFs (CORBA::ORB_ptr orb, CF::FileSystem_ptr fs)
        throw (std::string);

      /**
       * Destructor.
       */

      ~SCAFs ()
        throw ();

      /**
       * \name Implementation of the OCPI::Util::Vfs::Vfs interface.
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

#if 0
      OCPI::Util::Vfs::Iterator * list (const std::string & dir,
                                       const std::string & pattern = "*")
        throw (std::string);

      void closeIterator (OCPI::Util::Vfs::Iterator *)
        throw (std::string);
#endif
      OCPI::Util::Vfs::Dir &openDir(const std::string &name) throw(std::string);
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
      mutable OCPI::OS::Mutex m_lock;
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
