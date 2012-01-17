/****************************************************************************

Copyright 2004, 2007 Virginia Polytechnic Institute and State University

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

#ifndef __FILEMANAGER__IMPL
#define __FILEMANAGER__IMPL

#include "ossiecf.h"

#include "FileSystem_impl.h"
class OSSIECF_API FileManager_impl:public virtual POA_CF::FileManager,  public FileSystem_impl
{
public:
    FileManager_impl ();

    ~FileManager_impl ();

    void
    mount (const char *mountPoint, CF::FileSystem_ptr _fileSystem)
    throw (CORBA::SystemException, CF::InvalidFileName,
           CF::FileManager::InvalidFileSystem,
           CF::FileManager::MountPointAlreadyExists);

    void
    unmount (const char *mountPoint)
    throw (CF::FileManager::NonExistentMount, CORBA::SystemException);
    void
    mkdir (const char *directoryName)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);
    void
    rmdir (const char *directoryName)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);
    void
    query (CF::Properties & fileSysProperties)
    throw (CF::FileSystem::UnknownFileSystemProperties, CORBA::SystemException);

    void
    remove (const char *fileName)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);
    void
    copy (const char *sourceFileName, const char *destinationFileName)
    throw (CF::FileException, CF::InvalidFileName, CORBA::SystemException);
    CORBA::Boolean exists (const char *fileName)
    throw (CF::InvalidFileName, CORBA::SystemException);
    CF::File_ptr create (const char *fileName)
    throw (CF::FileException, CF::InvalidFileName, CORBA::SystemException);
    CF::File_ptr open (const char *fileName, CORBA::Boolean read_Only)
    throw (CF::FileException, CF::InvalidFileName, CORBA::SystemException);
    CF::FileSystem::FileInformationSequence* list (const char *pattern)
    throw (CF::InvalidFileName, CF::FileException, CORBA::SystemException);
    CF::FileManager::MountSequence* getMounts ()throw (CORBA::SystemException);

private:
    void getFSandFSPath(const char *path, unsigned int &mountTableIndex, std::string &FSPath);
    unsigned int pathMatches(const char *path, const char *mPoint, std::string &FSPath);

    CF::FileManager::MountSequence_var mount_table;

    FileSystem_impl* rootFileSys_serv;
    CF::FileSystem_var rootFileSys;
    unsigned int numMounts;

};                  /* END CLASS DEFINITION FileManager */
#endif              /* __FILEMANAGER__ */
