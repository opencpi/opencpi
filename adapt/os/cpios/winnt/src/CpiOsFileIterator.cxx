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
#include <windows.h>
#include "CpiOsWin32Error.h"

namespace {
  struct FileIteratorData {
    bool active;
    bool atend;
    std::string dir;
    std::string relPattern;
    std::string absPattern;
    std::string prefix;
    bool isSearch;
    // when isSearch
    HANDLE search;
    WIN32_FIND_DATA fileInfo;
    // when !isSearch, for listing drive names in the "root" directory
    DWORD drives;
    DWORD mask;
    char driveName[3];
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

    if (data.isSearch) {
      if (data.fileInfo.cFileName[0] == '.' && !data.fileInfo.cFileName[1]) {
        return false;
      }
      else if (data.fileInfo.cFileName[0] == '.' &&
               data.fileInfo.cFileName[1] == '.' &&
               !data.fileInfo.cFileName[2]) {
        return false;
      }

      return glob (data.fileInfo.cFileName, data.relPattern);
    }
    else {
      return glob (data.driveName, data.relPattern);
    }
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

  new (m_osOpaque) FileIteratorData;

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
    if (data.dir.length() == 1 && data.dir[0] == '/') {
      /*
       * Listing the "root" directory - list drive names
       */

      data.drives = GetLogicalDrives ();
      data.active = true;
      data.isSearch = false;
      data.mask = 1;
      data.driveName[0] = 'a';
      data.driveName[1] = ':';
      data.driveName[2] = 0;
      data.atend = false;

      if (!(data.drives & data.mask) || !matchingFileName (data)) {
        next ();
      }
    }
    else {
      /*
       * "normal" search
       */

      std::string nativePat = CPI::OS::FileSystem::toNativeName (data.absPattern);
      data.isSearch = true;
      data.active = true;

      data.search = FindFirstFile (nativePat.c_str(), &data.fileInfo);

      if (data.search == INVALID_HANDLE_VALUE) {
        data.atend = true;
      }
      else {
        data.atend = false;
      }

      /*
       * Skip over the '.' and '..' entries
       */

      if (!data.atend && !matchingFileName (data)) {
        next ();
      }
    }
  }

  return data.atend;
}

std::string
CPI::OS::FileIterator::relativeName ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if (data.isSearch) {
    if (data.prefix.empty()) {
      return data.fileInfo.cFileName;
    }

    return FileSystem::joinNames (data.prefix, data.fileInfo.cFileName);
  }
  else {
    return data.driveName;
  }
}

std::string
CPI::OS::FileIterator::absoluteName ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if (data.isSearch) {
    return FileSystem::joinNames (data.dir, data.fileInfo.cFileName);
  }
  else {
    return FileSystem::joinNames ("/", data.driveName);
  }
}

bool
CPI::OS::FileIterator::isDirectory ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if (data.isSearch) {
    return (data.fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
  }
  else {
    return true;
  }
}

unsigned long long
CPI::OS::FileIterator::size ()
  throw (std::string)
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if (!data.isSearch) {
    throw std::string ("disk drive does not have a size");
  }

  return ((unsigned long long) data.fileInfo.nFileSizeLow +
          ((unsigned long long) data.fileInfo.nFileSizeHigh << 32));
}

std::time_t
CPI::OS::FileIterator::lastModified ()
  throw (std::string)
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  if (!data.isSearch) {
    throw std::string ("disk drive does not have a timestamp");
  }

  SYSTEMTIME utcTime, localTime;

  if (!FileTimeToSystemTime (&data.fileInfo.ftLastWriteTime, &utcTime)) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }

  if (!SystemTimeToTzSpecificLocalTime (0, &utcTime, &localTime)) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }

  if (localTime.wYear < 1900) {
    throw std::string ("time before the year 1900 can not be represented");
  }

  struct tm myTime;
  myTime.tm_year  = localTime.wYear - 1900;
  myTime.tm_mon   = localTime.wMonth - 1;
  myTime.tm_mday  = localTime.wDay;
  myTime.tm_hour  = localTime.wHour;
  myTime.tm_min   = localTime.wMinute;
  myTime.tm_sec   = localTime.wSecond;
  myTime.tm_isdst = -1;

  time_t res = mktime (&myTime);

  if (res == (time_t) -1) {
    throw std::string ("time can not be represented");
  }

  return res;
}

bool
CPI::OS::FileIterator::next ()
  throw (std::string)
{
  FileIteratorData & data = o2fid (m_osOpaque);
  cpiAssert (data.active && !data.atend);

  /*
   * Skip over the '.' and '..' entries
   */

  do {
    if (data.isSearch) {
      if (!FindNextFile (data.search, &data.fileInfo)) {
        DWORD errCode = GetLastError();
      
        if (errCode != ERROR_NO_MORE_FILES) {
          throw CPI::OS::Win32::getErrorMessage (errCode);
        }
    
        data.atend = true;
      }
    }
    else {
      do {
        if (!(data.mask <<= 1)) {
          data.atend = true;
        }
        data.driveName[0]++;
      }
      while (!data.atend && !(data.mask & data.drives));
    }
  }
  while  (!data.atend && !matchingFileName (data));
  return !data.atend;
}

void
CPI::OS::FileIterator::close ()
  throw ()
{
  FileIteratorData & data = o2fid (m_osOpaque);

  if (data.isSearch) {
    if (data.active && data.search != INVALID_HANDLE_VALUE) {
      FindClose (data.search);
      data.search = INVALID_HANDLE_VALUE;
      data.atend = true;
    }
  }
  else {
    data.atend = true;
  }
}
