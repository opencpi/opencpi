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

#include <iostream>
#include <string>


#include "ossie/cf.h"
#include "ossie/FileManager_impl.h"
#include "ossie/debug.h"
#include "ossie/ossieSupport.h"

FileManager_impl::FileManager_impl ():FileSystem_impl ()
{
    DEBUG(4, FileManager, "Entering constructor.");

    numMounts = 0;
    mount_table = new CF::FileManager::MountSequence(5);

    DEBUG(4, FileManager, "Leaving constructor.");
}

FileManager_impl::~FileManager_impl()

{
    DEBUG(4, FileManager, "In destructor.");
}

void
FileManager_impl::mount (const char *mountPoint,
                         CF::FileSystem_ptr _fileSystem)
throw (CORBA::SystemException, CF::InvalidFileName,
       CF::FileManager::InvalidFileSystem,
       CF::FileManager::MountPointAlreadyExists)
{
    DEBUG(4, FileManager, "Entering mount with " << mountPoint);

    if (CORBA::is_nil (_fileSystem))
        throw CF::FileManager::InvalidFileSystem ();

    CF::FileManager::MountType _mt;

    for (unsigned int i=0; i < mount_table->length(); i++) {
        if (strcmp(mountPoint, mount_table[i].mountPoint) == 0)
            throw CF::FileManager::MountPointAlreadyExists ();
    }

    numMounts++;
    mount_table->length(numMounts);

    mount_table[numMounts-1].mountPoint = CORBA::string_dup(mountPoint);
    mount_table[numMounts-1].fs = CF::FileSystem::_duplicate(_fileSystem);


    DEBUG(4, FileManager, "Leaving mount.");
}


void
FileManager_impl::unmount (const char *mountPoint)
throw (CORBA::SystemException, CF::FileManager::NonExistentMount)
{
    DEBUG(4, FileManager, "Entering unmount with " << mountPoint);

    for (unsigned int i = 0; i < numMounts; i++) {
        if (strcmp (mount_table[i].mountPoint, mountPoint) == 0) {
            DEBUG(4, FileManager, "Found mount point to delete.");
            for (unsigned int j = i; j < mount_table->length() - 1; ++j) ///\todo this leaks FileSystems etc (check)
                mount_table[j] = mount_table[j+1];

            mount_table->length(mount_table->length() - 1);
            numMounts--;
            DEBUG(4, FileManager, "Leaving unmount.");
            return;
        }
    }

    std::cerr << "Throwing CF::FileManager::NonExistentMount from FileManger unmount." << std::endl;
    throw CF::FileManager::NonExistentMount ();
}


void
FileManager_impl::remove (const char *fileName)
throw (CORBA::SystemException, CF::FileException, CF::InvalidFileName)
{
    DEBUG(4, FileManager, "Entering remove with " << fileName);

    if (fileName[0] != '/' || !ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileManager, "remove passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileManager::remove] Invalid file name");
    }

    unsigned int FSIndex(0);
    std::string FSPath;

    getFSandFSPath(fileName, FSIndex, FSPath);

    if (mount_table[FSIndex].fs->exists (FSPath.c_str()))
        mount_table[FSIndex].fs->remove (FSPath.c_str());

    DEBUG(4, FileManager, "Leaving remove.");
}


void
FileManager_impl::copy (const char *sourceFileName,
                        const char *destinationFileName)
throw (CORBA::SystemException, CF::InvalidFileName, CF::FileException)
{
    DEBUG(4, FileManager, "Entering copy with " << sourceFileName << " to " << destinationFileName);

    if (!ossieSupport::isValidFileName(sourceFileName) || !ossieSupport::isValidFileName(destinationFileName)) {
        DEBUG(7, FileManager, "copy passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileManager::copy] Invalid file name");
    }

    unsigned int srcFS(0), dstFS(0);
    std::string srcPath;
    std::string dstPath;

    getFSandFSPath(sourceFileName, srcFS, srcPath);
    getFSandFSPath(destinationFileName, dstFS, dstPath);


    if (!mount_table[srcFS].fs->exists (srcPath.c_str())) {
        DEBUG(3, FileManager, "Throwing exception from copy because source file does not exist.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::Copy] Invalid file name");
    }

    if (srcFS == dstFS) { // Check if copy is within one FileSystem

            mount_table[srcFS].fs->copy (srcPath.c_str(), dstPath.c_str());

        return;
    }

// Copy file across FileSystems

    CF::File_var srcFile;

        srcFile = mount_table[srcFS].fs->open(srcPath.c_str(), true);


    unsigned int srcSize = srcFile->sizeOf();

    if (srcSize == 0) { ///\todo Check spec to see why we throw if size == 0

            srcFile->close();

        throw CF::FileException ();
    }

    if (!mount_table[dstFS].fs->exists(dstPath.c_str())) {

            mount_table[dstFS].fs->create(dstPath.c_str());

    }

    CF::File_var dstFile;

        dstFile = mount_table[dstFS].fs->open (dstPath.c_str(), false);


    CF::OctetSequence_var data;


        srcFile->read (data, srcSize);



        dstFile->write (data);



        srcFile->close();



        dstFile->close();

}


CORBA::Boolean FileManager_impl::exists (const char *fileName)
throw (CORBA::SystemException, CF::InvalidFileName)
{
    DEBUG(4, FileManager, "Entering exists with " << fileName);

    if (!ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileManager, "exists passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileManager::exists] Invalid file name");
    }

    DEBUG(6, FileManager, "Running FS.exists for file " << fileName);

    for ( unsigned int i = 0; i < numMounts; i++ ) {
        if (mount_table[i].fs->exists (fileName) )
            return true;
    }
    return false;
}


CF::FileSystem::FileInformationSequence *
FileManager_impl::list (const char *pattern) throw (CORBA::SystemException,
        CF::FileException,
        CF::InvalidFileName)
{
    DEBUG(4, FileManager, "Entering list with " << pattern);

    if (pattern[0] != '/')
        throw CF::InvalidFileName(CF::CFEINVAL, "[FileManager::list] Relative path given.");

    CF::FileSystem::FileInformationType* fit_arr[numMounts];
    int fit_length[numMounts];
    int result_length = 0;
    int tmp_length = 0;
        for ( unsigned int i = 0; i < numMounts; i++ ) {
            DEBUG(4, FileManager, "Calling FileSystem->list");
            CF::FileSystem::FileInformationSequence_var fis = mount_table[i].fs->list (pattern);
            DEBUG(4, FileManager, "Returned from FileSystem->list");
            result_length += fis->length();
            DEBUG(4, FileManager, "Recording "<<result_length<<" matches");
            fit_length[i] = fis->length();
            fit_arr[i] = fis->get_buffer(true);
        }
        DEBUG(4, FileManager, "Creating FIS results");
        tmp_length = result_length;
        CF::FileSystem::FileInformationType *fit = new CF::FileSystem::FileInformationType[result_length];
        CF::FileSystem::FileInformationType *tmp;
        CF::FileSystem::FileInformationType *beg = fit;
        DEBUG(4, FileManager, "Adding to FIS results");
        for ( int j = 0; j < numMounts; j++ ) {
            std::cout << "[FileManager::list] In mount number " << j << "\n";
            tmp = fit_arr[j];
            for ( unsigned int k = 0; k < fit_length[j]; k++ ) {
                std::cout << "[FileManager::list] Adding file number " << k << "\n";
                *fit = *tmp;
                fit ++;
                tmp ++;
            }
        }
        fit = beg;
        CF::FileSystem::FileInformationSequence_var result =
            new CF::FileSystem::FileInformationSequence(result_length, result_length, fit);
        DEBUG(4, FileManager, "About to return from list");
        return result._retn();
}


CF::File_ptr FileManager_impl::create (const char *fileName) throw (CORBA::
        SystemException,
        CF::
        InvalidFileName,
        CF::
        FileException)
{
    DEBUG(4, FileManager, "Entering create with " << fileName);

    if (!ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileManager, "create passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileManager::create] Invalid file name");
    }

    unsigned int fileFS(0);
    std::string filePath;

    getFSandFSPath(fileName, fileFS, filePath);

    CF::File_var file_var;

        file_var = mount_table[fileFS].fs->create (filePath.c_str());


    return file_var._retn();
}


CF::File_ptr
FileManager_impl::open (const char *fileName, CORBA::Boolean read_Only)
throw (CORBA::SystemException,CF::InvalidFileName, CF::FileException)
{
    DEBUG(4, FileManager, "Entering open with " << fileName);

    if (!ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileManager, "open passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileManager::open] Invalid file name");
    }

//    unsigned int fileFS(0);
//    std::string filePath;

//    getFSandFSPath(fileName, fileFS, filePath);
    CF::File_var file_var;
    for ( unsigned int i = 0; i < numMounts; i++ ) {
        if ( mount_table[i].fs->exists(fileName) ) {
            try {
                file_var = mount_table[i].fs->open (fileName, read_Only);
            } catch ( CF::InvalidFileName &_ex ) {
                std::cout << "[FileManager::open] While opening file " << fileName << ": " << _ex.msg << "\n";
                throw CF::FileException(CF::CFNOTSET, "[FileManager::open] File not found in any mounted File Systems");
            } catch ( CF::FileException &_ex ) {
                std::cout << "[FileManager::open] While opening file " << fileName << ": " << _ex.msg << "\n";
                throw CF::FileException(CF::CFNOTSET, "[FileManager::open] File not found in any mounted File Systems");
            } catch ( ... ) {
                std::cout << "[FileManager::open] While opening file " << fileName << ": Unknown Exception\n";
                throw;
            }
            break;
        }
    }

    return file_var._retn();
}


void
FileManager_impl::mkdir (const char *directoryName)
throw (CORBA::SystemException, CF::FileException, CF::InvalidFileName)
{
    DEBUG(4, FileManager, "Entering mkdir with " << directoryName);

    if (!ossieSupport::isValidFileName(directoryName)) {
        DEBUG(7, FileManager, "mkdir passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileManager::mkdir] Invalid directory name");
    }

    unsigned int fileFS(0);
    std::string filePath;

    getFSandFSPath(directoryName, fileFS, filePath);


        mount_table[fileFS].fs->mkdir(filePath.c_str());


    DEBUG(4, FileManager, "Leaving mkdir.");
}


void
FileManager_impl::rmdir (const char *directoryName)
throw (CORBA::SystemException, CF::FileException, CF::InvalidFileName)
{
    DEBUG(4, FileManager, "Entering rmdir with " << directoryName);

    if (!ossieSupport::isValidFileName(directoryName)) {
        DEBUG(7, FileManager, "rmdir passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileManager::rmdir] Invalid directory name");
    }

    unsigned int fileFS(0);
    std::string filePath;

    getFSandFSPath(directoryName, fileFS, filePath);


        mount_table[fileFS].fs->rmdir (filePath.c_str());


    DEBUG(4, FileManager, "Leaving rmdir.");
}


void
FileManager_impl::query (CF::Properties & fileSysProperties)
throw (CORBA::SystemException, CF::FileSystem::UnknownFileSystemProperties)
{
    DEBUG(4, FileManager, "Entering query.");

#if 0
    unsigned int fileFS(0);
    char *filePath = new char[MAXPATHLEN];

    getFSandFSPath(fileName, fileFS, filePath);
#endif

    bool check;

    for (unsigned int i = 0; i < fileSysProperties.length (); i++) {
        check = false;

        if (strcmp (fileSysProperties[i].id, CF::FileSystem::SIZE) == 0) {
            CORBA::Long totalSize, temp;
            totalSize = 0;

            for (unsigned int j = 0; j < mount_table->length(); j++) {
                CF::DataType dt;
                dt.id = CORBA::string_dup ("SIZE");
                CF::Properties pr (2, 1, &dt, 0);


                    mount_table[j].fs->query (pr);


                CF::DataType * _dt = pr.get_buffer ();

                for (unsigned int k = 0; k < pr.length (); k++) {
                    _dt->value >>= temp;
                    totalSize = totalSize + temp;
                    _dt++;
                }

                fileSysProperties[i].value >>= temp;
                fileSysProperties[i].value <<= totalSize + temp;

                check = true;
            }
        }

        if (strcmp (fileSysProperties[i].id,
                    CF::FileSystem::AVAILABLE_SIZE) == 0) {
            CORBA::Long totalSize;
            totalSize = 0;

            for (unsigned int i = 0; i < mount_table->length(); i++) {
            }

            check = true;
        }

        if (!check)
            throw CF::FileSystem::UnknownFileSystemProperties ();

        ///\todo Add functionality to query ALL FileManager properties
    }
    DEBUG(4, FileManager, "Leaving query.");
}


CF::FileManager::MountSequence*
FileManager_impl::getMounts ()throw (CORBA::SystemException)
{
    DEBUG(4, FileManager, "Entering MountSequence.");
    CF::FileManager::MountSequence_var result = new CF::FileManager::MountSequence(mount_table);
    return result._retn();
}

void FileManager_impl::getFSandFSPath(const char *path, unsigned int &mountTableIndex, std::string &FSPath)
{
    DEBUG(5, FileManager, "Entering getFSandFSPath with path " << path);

    std::string tmpFSPath;

    unsigned int lastMatchLength(0);

    for (unsigned int i(0); i < numMounts; ++i) {
        unsigned int matchLength = pathMatches(path,mount_table[i].mountPoint, tmpFSPath);
        if ( matchLength > lastMatchLength) {
            mountTableIndex = i;
            lastMatchLength = matchLength;
            FSPath = tmpFSPath;
        }
    }
    DEBUG(5, FileManager, "Found mountIndex " << mountTableIndex << " and local path " << FSPath);
    std::cout << "[FileManager::getFSandFSPath] Found mountIndex " << mountTableIndex << " and local path " << FSPath << "\n";

}

unsigned int FileManager_impl::pathMatches(const char * path, const char *mPoint, std::string &FSPath)
{
    DEBUG(5, FileManager, "Entering pathMatches with path " << path << " mount point " << mPoint);

    unsigned n = 0, commonElements = 0;
    for (n = 0; *path && *mPoint && *path == *mPoint; n++, path++, mPoint++)
      if (n && *path == '/')
	commonElements++;
    FSPath = path;

    DEBUG(5, FileManager, "Path matches with " << commonElements << " path elements in common and remaining path " << FSPath);

    return commonElements;
}

