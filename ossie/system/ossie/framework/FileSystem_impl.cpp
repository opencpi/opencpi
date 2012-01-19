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

#include <string>
#include "ossie/FileSystem_impl.h"

#ifdef USE_OPENCPI_FS
#include "OcpiUtilFileFs.h"
#include "OcpiCFUtilVfsFileSystem.h"


FileSystem_impl::FileSystem_impl()
: OCPI::CFUtil::VfsFileSystem(ossieSupport::ORB::poa,
			      new OCPI::Util::FileFs::FileFs())
{
}
FileSystem_impl::FileSystem_impl(const char *root)
: OCPI::CFUtil::VfsFileSystem(ossieSupport::ORB::poa,
			      new OCPI::Util::FileFs::FileFs(root))
{
}

FileSystem_impl::~FileSystem_impl(){}

// static
bool FileSystem_impl::isValidPath(const char *path) {
  try {
    std::string s(path);
    testFileName(s);
  } catch (...) {
    return false;
  }
  return true;
}

// static
void FileSystem_impl::joinPath(const char *base, const char *rel, std::string &result) {
  result = OCPI::Util::Vfs::joinNames(OCPI::Util::Vfs::directoryName(base), rel);
}

#else
/* SCA */

#include <iostream>
#include <fnmatch.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/exception.hpp>
#ifndef BOOST_VERSION
#include <boost/version.hpp>
#endif

#if BOOST_VERSION < 103500
#  include <boost/filesystem/cerrno.hpp>
#else
#  include <boost/cerrno.hpp>
#endif

namespace fs = boost::filesystem;

#include "ossie/debug.h"

FileSystem_impl::FileSystem_impl ()
{
    DEBUG(6, FileSystem, "In constructor.");

    if (fs::path::default_name_check_writable())
        fs::path::default_name_check(fs::portable_posix_name);

    root = fs::initial_path();

    init ();
}


FileSystem_impl::FileSystem_impl (const char *_root)
{
    DEBUG(6, FileSystem, "In constructor with " << _root);

    root = _root;

    init ();
}


void
FileSystem_impl::init ()
{
    DEBUG(6, FileSystem, "In init()");

    if (fs::path::default_name_check_writable())
      fs::path::default_name_check(fs::portable_posix_name);
}

FileSystem_impl::~FileSystem_impl ()
{
    DEBUG(6, FileSystem, "In destructor.");
}

// Filename utility functions as static member functions of FileSystem_impl, 
// implemented using boost here, to limit boost dependency to here
// static
bool FileSystem_impl::isValidPath(const char *fileName) {
    try {
        fs::path testPath(fileName, fs::portable_posix_name);
    } catch (...) {
        return false;
    }

    return true;
}
// static
void FileSystem_impl::joinPath(const char *spdFile, const char *name, std::string &fileName) {
    fs::path spdPath(spdFile);

    fs::path filePath = spdPath.branch_path() / name;

    fileName = filePath.string();
}


void FileSystem_impl::remove (const char *fileName) throw (CORBA::SystemException, CF::FileException, CF::InvalidFileName)
{
    DEBUG(6, FileSystem, "In remove with " << fileName);

    if (!ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileSystem, "remove passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::remove] Invalid file name");
    }

    fs::path fname(root / fileName);

    DEBUG(6, FileSystem, "About to remove file " << fname.string());
    bool exists = fs::remove(fname);

    if (!exists) {
        DEBUG(6, FileSystem, "Attempt to remove non-existant file.");
        throw (CF::FileException (CF::CFEEXIST, "[FileSystem_impl::remove] Error removing file from file system"));
    }
}

void FileSystem_impl::copy (const char *sourceFileName, const char *destinationFileName) throw (CORBA::SystemException, CF::InvalidFileName, CF::FileException)
{
    DEBUG(6, FileSystem, "In copy from " << sourceFileName << " to " << destinationFileName);

    if (sourceFileName[0] != '/' || destinationFileName[0] != '/' || !ossieSupport::isValidFileName(sourceFileName) || !ossieSupport::isValidFileName(destinationFileName)) {
        DEBUG(7, FileSystem, "copy passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::copy] Invalid file name");
    }

    try {
        fs::path sFile(root / sourceFileName);
        fs::path dFile(root / destinationFileName);

        if (fs::is_directory(sFile))
            return;

        fs::copy_file(sFile, dFile);
    } catch (const fs::filesystem_error &ex) {
#if BOOST_VERSION < 103400
        if (ex.error() == fs::not_found_error)
#elif BOOST_VERSION < 103500
        if (ex.system_error() == ENOENT)
#else
        if (ex.code().value() == ENOENT)
#endif
            throw CF::FileException(CF::CFENOENT, ex.what());
    }
}

CORBA::Boolean FileSystem_impl::exists (const char *fileName)
throw (CORBA::SystemException, CF::InvalidFileName)
{
    DEBUG(6, FileSystem, "In exists with " << fileName);

    if (fileName[0] != '/' || !ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileSystem, "exists passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::exists] Invalid file name");
    }

    fs::path fname(root / fileName);

    DEBUG(9, FileSystem, "Check for file " << fname.string());

    return(fs::exists(fname));
}

void FileSystem_impl::recursiveList(const fs::path& dirPath, const char* pattern, CF::FileSystem::FileInformationSequence_var& fis)
{
    DEBUG(4, FileSystem, "Entering recursiveList")

    try {
        fs::path filePath(pattern, fs::no_check);
        std::string searchPattern(filePath.leaf());
        unsigned int idx = fis->length();
        DEBUG(4, FileSystem, "In list with path " << dirPath.string() << ", and pattern " << searchPattern);
        fs::directory_iterator end_itr; // past the end
        for (fs::directory_iterator itr(dirPath); itr != end_itr; ++itr) {
            if (fs::is_directory(*itr)) recursiveList(*itr, pattern, fis);
            DEBUG(9, FileSystem, "In list checking file " << itr->leaf());
            if (fnmatch(searchPattern.c_str(), itr->leaf().c_str(), 0) == 0) {
                DEBUG(9, FileSystem, "Match in list with " << itr->leaf());
                fis->length(idx + 1);

                if (fs::is_directory(*itr)) {
                    std::string tmp(itr->leaf());
                    tmp += "/";
                    fis[idx].name = CORBA::string_dup(tmp.c_str());
                    fis[idx].kind = CF::FileSystem::DIRECTORY;
                    fis[idx].size = 0;
                } else {
                    std::string full_path = dirPath.string();
                    full_path += "/";
                    full_path += itr->leaf();
                    fis[idx].name = CORBA::string_dup(full_path.c_str());
                    fis[idx].kind = CF::FileSystem::PLAIN;
                    fis[idx].size = fs::file_size(*itr);
                }
                /// \todo fix file creation time and last access time
                CF::Properties prop;
                prop.length(3);
                prop[0].id = CORBA::string_dup(CF::FileSystem::CREATED_TIME_ID);
                prop[0].value <<= fs::last_write_time(*itr);
                prop[1].id = CORBA::string_dup(CF::FileSystem::MODIFIED_TIME_ID);
                prop[1].value <<= fs::last_write_time(*itr);
                prop[2].id = CORBA::string_dup(CF::FileSystem::LAST_ACCESS_TIME_ID);
                prop[2].value <<= fs::last_write_time(*itr);
                fis[idx].fileProperties = prop;
                ++idx;
            }
        }
        DEBUG(4, FileSystem, "About to return from list.");
    } catch (const fs::filesystem_error &ex) {
#if BOOST_VERSION < 103400
        DEBUG(9, FileSystem, "Caught exception in list, error_code " << ex.error());
        if (ex.error() == fs::other_error)
#elif BOOST_VERSION < 103500
        DEBUG(9, FileSystem, "Caught exception in list, error_code " << ex.system_error());
        if (ex.system_error() == EINVAL)
#else
        DEBUG(9, FileSystem, "Caught exception in list, error_code " << ex.code().value());
        if (ex.code().value() == EINVAL)
#endif
            throw CF::InvalidFileName(CF::CFEINVAL, ex.what());
        throw CF::FileException(CF::CFNOTSET, ex.what());
    }
    DEBUG(4, FileSystem, "Leaving recursiveList")
}

/// \todo: modify to search the pattern as a regular expression
CF::FileSystem::FileInformationSequence* FileSystem_impl::list (const char *pattern) throw (CORBA::SystemException, CF::FileException, CF::InvalidFileName)
{
    DEBUG(6, FileSystem, "In list with " << pattern);

    const fs::path rootDirPath(root);
    CF::FileSystem::FileInformationSequence_var result = new CF::FileSystem::FileInformationSequence;
    result->length(0);

    try {
      recursiveList(rootDirPath, pattern, result);
    } catch (const fs::filesystem_error &ex) {
      // Convert boost fs errors into SCA FileSystem errors
#if BOOST_VERSION < 103400
      DEBUG(9, FileManager, "Caught exception in list, error_code " << ex.error());
      if (ex.error() == fs::other_error)
#elif BOOST_VERSION < 103500
      DEBUG(9, FileManager, "Caught exception in list, error_code " << ex.system_error());
      if (ex.system_error() == EINVAL)
#else
      DEBUG(9, FileManager, "Caught exception in list, error_code " << ex.code().value()); ;
      if (ex.code().value() == EINVAL)
#endif
	throw CF::InvalidFileName(CF::CFEINVAL, ex.what());
      throw CF::FileException(CF::CFNOTSET, ex.what());
    }
    return result._retn();
}


CF::File_ptr FileSystem_impl::create (const char *fileName) throw (CORBA::SystemException, CF::InvalidFileName, CF::FileException)
{
    DEBUG(6, FileSystem, "In create with " << fileName);

    if (!ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileSystem, "create passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::create] Invalid file name");
    }

    if (exists(fileName)) {
        DEBUG(6, FileSystem, "FileName exists in create, throwing exception.");
        throw CF::FileException(CF::CFEEXIST, "File exists.");
    }

    File_impl *file = new File_impl (fileName, root, false, true);
    CF::File_var fileServant = file->_this();

    fileInfo newFile;
    newFile.fileName = fileName;
    newFile.servant = fileServant;
    files.push_back(newFile);

    return fileServant._retn();
}

CF::File_ptr FileSystem_impl::open (const char *fileName, CORBA::Boolean read_Only) throw (CORBA::SystemException, CF::InvalidFileName, CF::FileException)
{
    DEBUG(6, FileSystem, "In open with " << fileName);

    if (!ossieSupport::isValidFileName(fileName)) {
        DEBUG(7, FileSystem, "open passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::open] Invalid file name");
    }


    if (!exists(fileName)) {
        DEBUG(6, FileSystem, "FileName does not exist in open, throwing exception.");
        throw CF::FileException(CF::CFEEXIST, "[FileSystem::open] File does not exist.");
    }

//    fs::path filePath(root / fileName);

    File_impl *file = new File_impl (fileName, root, read_Only, false);
    CF::File_var fileServant = file->_this();

    fileInfo newFile;
    newFile.fileName = fileName;
    newFile.servant = fileServant;
    files.push_back(newFile);

    return fileServant._retn();
}


void FileSystem_impl::mkdir (const char *directoryName) throw (CORBA::SystemException, CF::FileException, CF::InvalidFileName)
{
    DEBUG(6, FileSystem, "In mkdir with " << directoryName);

    if (!ossieSupport::isValidFileName(directoryName)) {
        DEBUG(7, FileSystem, "mkdir passed bad filename, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::mkdir] Invalid file name");
    }

    fs::path dirPath(root / directoryName);

    if (fs::exists(dirPath))
        throw CF::FileException (CF::CFEEXIST, "[FileSystem::mkdir] Directory exists.");

    fs::path::iterator walkPath(dirPath.begin());
    fs::path currentPath;
    while (walkPath != dirPath.end()) {
        DEBUG(9, FileSystem, "Walking path to create directories, current path " << currentPath.string());
        currentPath /= *walkPath;
        if (!fs::exists(currentPath)) {
            DEBUG(9, FileSystem, "Creating directory " << currentPath.string());
            try {
                fs::create_directory(currentPath);
            } catch (...) {
                throw CF::FileException (CF::CFENFILE, "[FileSystem::mkdir] Failed to create directory");
            }
        }
        ++walkPath;
    }
}

void FileSystem_impl::removeDirectory(const fs::path &dirPath, bool doRemove)
{

    fs::directory_iterator end_itr; // past the end
    for (fs::directory_iterator itr(dirPath); itr != end_itr; ++itr) {
        if (fs::is_directory(*itr))
            removeDirectory(*itr, doRemove);
        else {
            DEBUG(7, FileSystem, "Directory not empty in rmdir.");
            throw CF::FileException();
        }
    }

    if (doRemove)
        fs::remove(dirPath);
}

void FileSystem_impl::rmdir (const char *directoryName) throw (CORBA::SystemException, CF::FileException, CF::InvalidFileName)
{
    DEBUG(6, FileSystem, "In rmdir with " << directoryName);

    if (!ossieSupport::isValidFileName(directoryName)) {
        DEBUG(7, FileSystem, "rmdir passed bad directory name, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::rmdir] Invalid directory name");
    }

    fs::path dirPath(root / directoryName);

    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        DEBUG(7, FileSystem, "rmdir passed non_existant name or name is not a directory, throwing exception.");
        throw CF::InvalidFileName (CF::CFEINVAL, "[FileSystem::rmdir] Invalid directory name");
    }

// See the JTAP test for rmdir to understand this
    removeDirectory(dirPath, false); // Test for only empty directories
    removeDirectory(dirPath, true);  // Only empty directories, remove them all
}


void FileSystem_impl::query (CF::Properties & fileSysProperties) throw (CORBA::SystemException, CF::FileSystem::UnknownFileSystemProperties)
{
    DEBUG(6, FileSystem, "In query");
#if 0  ///\todo Implement query operations
    bool check;

    for (unsigned int i = 0; i < fileSysProperties.length (); i++) {
        check = false;
        if (strcmp (fileSysProperties[i].id, CF::FileSystem::SIZE) == 0) {
            struct stat fileStat;
            stat (root, &fileStat);
//	    fileSysProperties[i].value <<= fileStat.st_size;  /// \bug FIXME
            check = true;
        }
        if (strcmp (fileSysProperties[i].id,
                    CF::FileSystem::AVAILABLE_SIZE) == 0) {
//to complete
        }
        if (!check)
            throw CF::FileSystem::UnknownFileSystemProperties ();
    }
#endif
}

///\todo Implement File object reference clean up.
#endif /* end of else of ifdef USE_OPENCPI_FS */
