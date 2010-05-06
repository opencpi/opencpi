// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

#include <CpiOsAssert.h>
#include <CpiOsFileSystem.h>
#include <CpiOsFileIterator.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <string>
#include <ctime>
#include <new>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "CpiOsPosixError.h"

namespace {
  struct FileIteratorData {
    bool active;
    bool atend;
    std::string dir;
    std::string relPattern;
    std::string absPattern;
    std::string prefix;
    DIR * search;
    struct dirent * dirInfo;
    struct stat fileInfo;
  };

  bool
  glob (const std::string & str,
        const std::string & pat)
    throw ()
  {
    int strIdx = 0, strLen = str.length ();
    int patIdx = 0, patLen = pat.length ();
    const char * name = str.data ();
    const char * pattern = pat.data ();

    while (strIdx < strLen && patIdx < patLen) {
      if (*pattern == '*') {
        pattern++;
        patIdx++;
        while (strIdx < strLen) {
          if (glob (name, pattern)) {
            return true;
          }
          strIdx++;
          name++;
        }
        return (patIdx < patLen) ? false : true;
      }
      else if (*pattern == '?' || *pattern == *name) {
        pattern++;
        patIdx++;
        name++;
        strIdx++;
      }
      else {
        return false;
      }
    }
    
    while (*pattern == '*' && patIdx < patLen) {
      pattern++;
      patIdx++;
    }
    
    if (patIdx < patLen || strIdx < strLen) {
      return false;
    }
    
    return true;
  }

  bool
  matchingFileName (FileIteratorData & data)
    throw ()
  {
    cpiAssert (!data.atend);

    if (data.dirInfo->d_name[0] == '.' && !data.dirInfo->d_name[1]) {
      return false;
    }
    else if (data.dirInfo->d_name[0] == '.' &&
             data.dirInfo->d_name[1] == '.' &&
             !data.dirInfo->d_name[2]) {
      return false;
    }

    return glob (data.dirInfo->d_name, data.relPattern);
  }
}

inline
FileIteratorData &
o2fid (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<FileIteratorData *> (ptr);
}

inline
const FileIteratorData &
o2fid (const CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<const FileIteratorData *> (ptr);
}

CPI::OS::FileIterator::FileIterator (const std::string & dir,
                                     const std::string & pattern)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (FileIteratorData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (FileIteratorData));

  std::string absoluteDirName = FileSystem::absoluteName (dir);
  std::string absolutePatName = FileSystem::joinNames (absoluteDirName, pattern);

  new (m_osOpaque) FileIteratorData ();

  FileIteratorData & data = o2fid (m_osOpaque);
  data.active = false;
  data.dir = FileSystem::directoryName (absolutePatName);
  data.relPattern = FileSystem::relativeName (absolutePatName);
  data.absPattern = absolutePatName;

  if (pattern.find ('/') != std::string::npos) {
    data.prefix = FileSystem::directoryName (pattern);
  }

  /*
   * This makes sure that the directory exists and initializes the
   * members to point to the first file, if it exists.
   */

  try {
    end ();
  }
  catch (...) {
    data.FileIteratorData::~FileIteratorData ();
    throw;
  }
}

CPI::OS::FileIterator::FileIterator (const FileIterator & other)
  throw (std::string)
{
  const FileIteratorData & otherData = o2fid (other.m_osOpaque);

  new (m_osOpaque) FileIteratorData;
  FileIteratorData & data = o2fid (m_osOpaque);
  data.active = false;
  data.dir = otherData.dir;
  data.relPattern = otherData.relPattern;
  data.absPattern = otherData.absPattern;
  data.prefix = otherData.prefix;

  /*
   * This makes sure that the directory exists and initializes the
   * members to point to the first file, if it exists.
   */

  try {
    end ();
  }
  catch (...) {
    data.FileIteratorData::~FileIteratorData ();
    throw;
  }
}

CPI::OS::FileIterator::~FileIterator ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);
  data.FileIteratorData::~FileIteratorData ();
}

CPI::OS::FileIterator &
CPI::OS::FileIterator::operator= (const FileIterator & other)
  throw (std::string)
{
  const FileIteratorData & otherData = o2fid (other.m_osOpaque);
  FileIteratorData & data = o2fid (m_osOpaque);
  data.active = false;
  data.dir = otherData.dir;
  data.relPattern = otherData.relPattern;
  data.absPattern = otherData.absPattern;
  data.prefix = otherData.prefix;
  return *this;
}

bool
CPI::OS::FileIterator::end ()
  throw (std::string)
{
  FileIteratorData & data = o2fid (m_osOpaque);

  if (!data.active) {
    std::string nativeDir = FileSystem::toNativeName (data.dir);
    data.active = true;

    if (!(data.search = opendir (nativeDir.c_str()))) {
      throw CPI::OS::Posix::getErrorMessage (errno);
    }

    data.atend = false;

    /*
     * Skip to the first matching file
     */

    next ();
  }

  return data.atend;
}

std::string
CPI::OS::FileIterator::relativeName ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if (data.prefix.empty()) {
    return data.dirInfo->d_name;
  }

  return FileSystem::joinNames (data.prefix, data.dirInfo->d_name);
}

std::string
CPI::OS::FileIterator::absoluteName ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);
  return FileSystem::joinNames (data.dir, data.dirInfo->d_name);
}

bool
CPI::OS::FileIterator::isDirectory ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if ((data.fileInfo.st_mode & S_IFDIR) == S_IFDIR) {
    return true;
  }

  return false;
}

unsigned long long
CPI::OS::FileIterator::size ()
  throw (std::string)
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if ((data.fileInfo.st_mode & S_IFREG) != S_IFREG) {
    throw std::string ("not a regular file");
  }

  return data.fileInfo.st_size;
}

std::time_t
CPI::OS::FileIterator::lastModified ()
  throw (std::string)
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);
  return data.fileInfo.st_mtime;
}

bool
CPI::OS::FileIterator::next ()
  throw (std::string)
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  while (!data.atend) {
    if (!(data.dirInfo = readdir (data.search))) {
      data.atend = true;
      break;
    }

    if (!matchingFileName (data)) {
      continue;
    }

    std::string absoluteFileName = FileSystem::joinNames (data.dir, data.dirInfo->d_name);
    std::string nativeName = FileSystem::toNativeName (absoluteFileName);

    if (stat (nativeName.c_str(), &data.fileInfo)) {
      continue;
    }

    /*
     * Good match
     */

    break;
  }

  return !data.atend;
}

void
CPI::OS::FileIterator::close ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);

  if (data.active && data.search) {
    closedir (data.search);
    data.search = 0;
    data.atend = true;
  }
}
