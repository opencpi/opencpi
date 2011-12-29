
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

#ifndef OCPICFUTILVFSFILESYSTEM_H__
#define OCPICFUTILVFSFILESYSTEM_H__

/**
 * \file
 * \brief SCA FileSystem using a Vfs.
 *
 * Revision History:
 *
 *     07/01/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <ctime>
#include <string>
#include <iostream>

#ifndef __cf_hh__
#ifndef __CF_hh__
#include "CF_s.h"
#endif
#endif
#include "OcpiUtilVfs.h"
#include "OcpiUtilVfsIterator.h"

namespace OCPI {
  namespace CFUtil {
    /**
     * \brief SCA FileSystem using a Vfs.
     *
     * This class implements the CF::FileSystem interface by
     * delegating to a OCPI::Util::Vfs::Vfs instance.
     */

    class VfsFileSystem : virtual public POA_CF::FileSystem {
    public:
      VfsFileSystem (//CORBA::ORB_ptr orb,
                     PortableServer::POA_ptr poa,
                     OCPI::Util::Vfs::Vfs * fs = NULL,
                     bool adopt = true)
        throw ();

      /**
       * Destructor.
       */

      ~VfsFileSystem ();

      /**
       * \name Implementation of the CF::FileSystem interface.
       */

      //@{

      void remove (const char * fileName)
        throw (CF::InvalidFileName,
               CF::FileException,
               CORBA::SystemException);

      void copy (const char * sourceFileName,
                 const char * destinationFileName)
        throw (CF::InvalidFileName,
               CF::FileException,
               CORBA::SystemException);

      CORBA::Boolean exists (const char * fileName)
        throw (CF::InvalidFileName,
               CORBA::SystemException);

      CF::FileSystem::FileInformationSequence * list (const char * pattern)
        throw (CF::InvalidFileName,
               CF::FileException,
               CORBA::SystemException);

      CF::File_ptr create (const char * fileName)
        throw (CF::InvalidFileName,
               CF::FileException,
               CORBA::SystemException);

      CF::File_ptr open (const char * fileName,
                         CORBA::Boolean read_Only)
        throw (CF::InvalidFileName,
               CF::FileException,
               CORBA::SystemException);

      void mkdir (const char * directoryName)
        throw (CF::InvalidFileName,
               CF::FileException,
               CORBA::SystemException);

      void rmdir (const char * directoryName)
        throw (CF::InvalidFileName,
               CF::FileException,
               CORBA::SystemException);

      void query (CF::Properties & fileSystemProperties)
        throw (CF::FileSystem::UnknownFileSystemProperties,
               CORBA::SystemException);

      //@}

    protected:
      static void testFileName (const std::string & fileName, bool isPattern = 0)
        throw (CF::InvalidFileName);

    protected:
      //      CORBA::ORB_var m_orb;
      PortableServer::POA_var m_poa;
      OCPI::Util::Vfs::Vfs * m_fs;
      bool m_adopted;

    private:
      /**
       * Not implemented.
       */

      VfsFileSystem (const VfsFileSystem &);

      /**
       * Not implemented.
       */

      VfsFileSystem & operator= (const VfsFileSystem &);
    };

  }
}

#endif
