// -*- c++ -*-

#ifndef CPICFUTILVFSFILESYSTEM_H__
#define CPICFUTILVFSFILESYSTEM_H__

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
#include <CpiUtilVfs.h>
#include <CpiUtilVfsIterator.h>
#include <CF_s.h>

namespace CPI {
  namespace CFUtil {
    /**
     * \brief SCA FileSystem using a Vfs.
     *
     * This class implements the CF::FileSystem interface by
     * delegating to a CPI::Util::Vfs::Vfs instance.
     */

    class VfsFileSystem : public POA_CF::FileSystem {
    public:
      VfsFileSystem (CORBA::ORB_ptr orb,
                     PortableServer::POA_ptr poa,
                     CPI::Util::Vfs::Vfs * fs,
                     bool adopt)
        throw ();

      /**
       * Destructor.
       */

      ~VfsFileSystem ()
        throw ();

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
      CORBA::ORB_var m_orb;
      PortableServer::POA_var m_poa;
      CPI::Util::Vfs::Vfs * m_fs;
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
