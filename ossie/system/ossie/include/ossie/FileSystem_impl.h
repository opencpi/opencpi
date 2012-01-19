/****************************************************************************

Copyright 2004, 2007, Virginia Polytechnic Institute and State University

This file is part of the OSSIE Core Framework.

OSSIE Core Framework is free software; you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

OSSIE Core Framework is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with OSSIE Core Framework; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

****************************************************************************/

/* SCA */
#ifndef __FILESYSTEM_IMPL__
#define __FILESYSTEM_IMPL__

#include "ossie/cf.h"
#include "ossie/ossieSupport.h"
#ifdef USE_OPENCPI_FS

#include "OcpiCFUtilVfsFileSystem.h"

class FileSystem_impl : OCPI::CFUtil::VfsFileSystem, virtual public POA_CF::FileSystem {
 public:
  // The standalone constructor to create a file system based on a URL
  FileSystem_impl(const char *url);
  FileSystem_impl();
  ~FileSystem_impl();
  static bool isValidPath(const char *path);
  static void joinPath(const char *base, const char *rel, std::string &result);
};
#else

#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>

#include "ossie/ossiecf.h"
#include "ossie/File_impl.h"
#include "ossie/ossieSupport.h"

class OSSIECF_API FileSystem_impl:public virtual POA_CF::FileSystem

{
public:
    FileSystem_impl ();
    FileSystem_impl (const char *_root);
    ~ FileSystem_impl ();

    static bool isValidPath(const char *path);

    static void joinPath(const char *base, const char *rel, std::string &result);

    void remove (const char *fileName)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);

    void copy (const char *sourceFileName, const char *destinationFileName)
    throw (CF::FileException, CF::InvalidFileName, CORBA::SystemException);

    void mkdir (const char *directoryName)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);

    void rmdir (const char *directoryName)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);

    void query (CF::Properties & fileSysProperties)
    throw (CF::FileSystem::UnknownFileSystemProperties, CORBA::SystemException);
    CORBA::Boolean exists (const char *fileName)
    throw (CF::InvalidFileName, CORBA::SystemException);

    CF::File_ptr create (const char *fileName)
    throw (CF::FileException, CF::InvalidFileName, CORBA::SystemException);

    CF::File_ptr open (const char *fileName, CORBA::Boolean read_Only)
    throw (CF::FileException, CF::InvalidFileName, CORBA::SystemException);

    CF::FileSystem::FileInformationSequence *list (const char *pattern)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);

private:
    FileSystem_impl (const FileSystem_impl & _fsi);
    FileSystem_impl operator= (FileSystem_impl _fsi);

    void init ();
    void removeDirectory(const boost::filesystem::path &dirPath, bool doRemove);
    void recursiveList(const boost::filesystem::path& dirPath, const char* pattern, CF::FileSystem::FileInformationSequence_var& fis);

    boost::filesystem::path root;

    ossieSupport::ORB *orb;

    class fileInfo
    {
    public:
        std::string fileName;
        CF::File_var servant;
    };
    std::vector<fileInfo> files;

};                                                /* END CLASS DEFINITION FileSystem */
#endif                                            /* end of else of ifdef USE_OPENCPI_FS */
#endif                                            /* __FILESYSTEM__ */
