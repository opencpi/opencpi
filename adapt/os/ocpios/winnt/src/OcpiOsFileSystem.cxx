
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


#include <OcpiOsAssert.h>
#include <OcpiOsFileSystem.h>
#include <OcpiOsFileIterator.h>
#include <string>
#include <cctype>
#include <ctime>
#include <windows.h>
#include "OcpiOsWin32Error.h"

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
OCPI::OS::FileSystem::toNativeName (const std::string & name)
  throw (std::string)
{
  std::string::size_type slashPos = name.find ('/');

  if (slashPos == std::string::npos) {
    return name;
  }

  std::string nativeName;

  if (slashPos == 0) {
    /*
     * Absolute name.
     *
     * If the first path component is a single character followed by
     * a colon, then it's a drive name. Else, it's a UNC path.
     */

    if (name.length() == 1) {
      throw std::string ("invalid file name");
    }

    slashPos = name.find ('/', 1);

    if (slashPos == std::string::npos) {
      /*
       * A single path component only
       */

      if (name.length() == 3 && std::isalpha (name[1]) && name[2] == ':') {
        /*
         * Drive name only, making into, e.g., c:\
         */
        nativeName = name.substr (1);
        nativeName += "\\";
      }
      else {
        /*
         * Host name only, making into \\host\\
         */

        nativeName = "\\\\";
        nativeName += name.substr (1);
        nativeName += '\\';
      }

      return nativeName;
    }

    /*
     * Absolute name with more than one path component
     */

    if (slashPos == 3 && std::isalpha (name[1]) && name[2] == ':') {
      /*
       * First path component is a drive name
       */
      nativeName = name.substr (1, 2);
    }
    else {
      /*
       * First path component is a host name
       */
      nativeName = "\\\\";
      nativeName += name.substr (1, slashPos - 1);
    }
  }
  else if (slashPos == std::string::npos) {
    /*
     * Relative name, single path component
     */

    return name;
  }
  else {
    /*
     * Relative name
     */

    nativeName = name.substr (0, slashPos);
  }

  /*
   * None of the FileSytem operations generate a file name that ends in
   * a slash.
   */

  if (name.length() == slashPos + 1) {
    throw std::string ("invalid file name: should not end in '/'");
  }

  /*
   * At this point, name contains the first path component, and slashPos
   * points to the first slash after the first name component.
   */

  /*
   * Just copy the rest of the name onto the nativeName, and replace
   * all forward slashes by backward slashes.
   */

  nativeName += '\\';
  nativeName += name.substr (slashPos + 1);

  while ((slashPos = nativeName.find ('/', slashPos + 1)) != std::string::npos) {
    nativeName[slashPos] = '\\';
  }

  return nativeName;
}

std::string
OCPI::OS::FileSystem::fromNativeName (const std::string & nativeName)
  throw (std::string)
{
  const std::string slashes = "/\\";
  std::string name, prefix;
  std::string::size_type slashPos;

  if (nativeName.length() == 0) {
    throw std::string ("empty filename");
  }

  if (nativeName.length() > 1 &&
      std::isalpha (nativeName[0]) &&
      nativeName[1] == ':') {
    /*
     * An absolute name starting with a drive letter
     */

    if (nativeName.length() > 2 &&
        nativeName[2] != '/' && 
        nativeName[2] != '\\') {
      throw std::string ("invalid file name: expected (back) slash after drive name");
    }

    prefix = "/";
    prefix += nativeName.substr (0, 2);
    slashPos = 2;
  }
  else if (nativeName.length() > 1 &&
           (nativeName[0] == '\\' && nativeName[1] == '\\' ||
            nativeName[0] == '/' && nativeName[1] == '/')) {
    /*
     * A UNC file name
     */

    if (nativeName.length() == 2 ||
        nativeName[2] == '\\' ||
        nativeName[2] == '/') {
      throw std::string ("invalid file name: expected host name after '\\\\'");
    }

    slashPos = nativeName.find_first_of (slashes, 2);

    if (slashPos == std::string::npos) {
      name = "/";
      name += nativeName.substr (2);
      return name;
    }

    prefix = "/";
    prefix += nativeName.substr (2, slashPos - 2);
  }
  else {
    slashPos = nativeName.find_first_of (slashes);

    /*
     * A relative name
     */

    if (slashPos == 0) {
      /*
       * Starts with a slash? Should we assume c:\? I don't think so --
       * such assumptions are a bad thing.
       */

      std::string reason = "invalid file name: native name may not start with '";
      reason += nativeName[0];
      reason += "': missing drive information";
      throw reason;
    }

    /*
     * See if the first (or only) path component is '.' or '..'
     */

    if ((slashPos == 1 ||
         (slashPos == std::string::npos && nativeName.length() == 1)) &&
        nativeName[0] == '.') {
      name = cwd ();
    }
    else if ((slashPos == 2 ||
              (slashPos == std::string::npos && nativeName.length() == 2)) &&
             nativeName[0] == '.' && nativeName[1] == '.') {
      std::string currentDir = cwd ();

      ocpiAssert (currentDir.length() && currentDir[0] == '/');
      std::string::size_type secondSlash = currentDir.find_first_of (slashes, 1);

      if (secondSlash == std::string::npos) {
        throw std::string ("invalid file name: '..' in top-level directory");
      }

      std::string::size_type prevSlashPos = currentDir.rfind ('/');
      name = currentDir.erase (prevSlashPos);
    }
    else if (slashPos != std::string::npos) {
      name = nativeName.substr (0, slashPos);
    }
    else if (slashPos == std::string::npos) {
      /*
       * File name has a single path component only
       */

      return nativeName;
    }
  }

  /*
   * At this point, name is empty or contains the first path component,
   * and slashPos points to the first backslash after the first name
   * component.
   */

  /*
   * Process all other path components
   */

  do {
    std::string::size_type oldSlashPos = slashPos;
    std::string::size_type newSlashPos;

    newSlashPos = slashPos = nativeName.find_first_of (slashes, slashPos + 1);

    if (newSlashPos == std::string::npos) {
      newSlashPos = nativeName.length();
    }

    if (newSlashPos - oldSlashPos == 1) {
      /*
       * Ignore a slash at the end
       */
      if (slashPos == std::string::npos) {
        break;
      }

      throw std::string ("empty path component");
    }
    else if (newSlashPos - oldSlashPos == 2 &&
             nativeName[oldSlashPos+1] == '.') {
      /*
       * A path component of "." - ignore
       */
      continue;
    }
    else if (newSlashPos - oldSlashPos == 3 &&
             nativeName[oldSlashPos+1] == '.' &&
             nativeName[oldSlashPos+2] == '.') {
      /*
       * If the name is empty, and if it was a relative name, then
       * back up beyond the current working directory.
       */

      if (name.empty()) {
        if (!prefix.empty()) {
          throw std::string ("unprocessible '..' path component");
        }

        std::string currentDir = cwd ();

        /*
         * Everything up to the second slash becomes the immutable
         * prefix. This way, we never "back up" twice.
         */

        ocpiAssert (currentDir.length() && currentDir[0] == '/');
        std::string::size_type secondSlash = currentDir.find ('/', 1);

        if (secondSlash == std::string::npos) {
          throw std::string ("unprocessible '..' path component");
        }

        prefix = currentDir.substr (0, secondSlash);
        name = currentDir.substr (secondSlash+1);
      }

      /*
       * Truncate the previous path component
       */

      std::string::size_type prevSlashPos = name.rfind ('/');

      if (prevSlashPos == std::string::npos) {
        name.erase ();
      }
      else {
        name.erase (prevSlashPos);
      }

      continue;
    }

    /*
     * Concatenate new path component
     */

    if (name.length()) {
      name += '/';
    }

    name += nativeName.substr (oldSlashPos + 1, newSlashPos - oldSlashPos - 1);
  }
  while (slashPos != std::string::npos);

  /*
   * Concatenate prefix and name
   */

  if (!prefix.empty()) {
    std::string fullName = prefix;
    if (!name.empty()) {
      fullName += '/';
    }
    fullName += name;
    return fullName;
  }

  if (name.empty()) {
    /*
     * Refers to the current directory.
     */

    return cwd ();
  }

  return name;
}

/*
 * ----------------------------------------------------------------------
 * File Name Helpers
 * ----------------------------------------------------------------------
 */

std::string
OCPI::OS::FileSystem::joinNames (const std::string & dir,
                                const std::string & name)
  throw (std::string)
{
  ocpiAssert (name.length() > 0);

  if (name[0] == '/') {
    return name;
  }

  ocpiAssert (dir.length() > 0);

  std::string absName = dir;

  if (dir.length() != 1 || dir[0] != '/') {
    absName += '/';
  }

  absName += name;
  return absName;
}

std::string
OCPI::OS::FileSystem::absoluteName (const std::string & name)
  throw (std::string)
{
  return joinNames (cwd(), name);
}

std::string
OCPI::OS::FileSystem::directoryName (const std::string & name)
  throw (std::string)
{
  std::string::size_type slashPos = name.rfind ('/');

  if (slashPos == std::string::npos) {
    return cwd ();
  }

  if (slashPos == 0) {
    return "/";
  }

  return name.substr (0, slashPos);
}

std::string
OCPI::OS::FileSystem::relativeName (const std::string & name)
  throw (std::string)
{
  std::string::size_type slashPos = name.rfind ('/');

  if (slashPos == std::string::npos) {
    return name;
  }

  if (slashPos == 0 && name.length() == 1) {
    /*
     * The relative name of "/" is "/".
     */
    return "/";
  }

  return name.substr (slashPos + 1);
}

std::string
OCPI::OS::FileSystem::getPathElement (std::string & path,
                                     bool ignoreInvalid,
                                     char separator)
  throw (std::string)
{
  std::string::size_type colPos;
  std::string firstElement, res;

  if (!separator) {
    separator = ';';
  }

 again:
  colPos = path.find (separator);

  if (colPos == std::string::npos) {
    firstElement = path;
    path.clear ();
  }
  else {
    firstElement = path.substr (0, colPos);
    path = path.substr (colPos + 1);
  }

  if (firstElement.empty()) {
    if (path.empty()) {
      return res;
    }

    goto again;
  }

  try {
    res = fromNativeName (firstElement);
  }
  catch (const std::string &) {
    if (!ignoreInvalid) {
      throw;
    }

    goto again;
  }

  return res;
}

/*
 * ----------------------------------------------------------------------
 * Directory Management
 * ----------------------------------------------------------------------
 */

std::string
OCPI::OS::FileSystem::cwd ()
  throw (std::string)
{
  char buffer[1024];

  if (GetCurrentDirectory (1024, buffer) >= 1024) {
    throw std::string ("wow, current directory is longer than 1024 chars");
  }

  return fromNativeName (buffer);
}

void
OCPI::OS::FileSystem::cd (const std::string & name)
  throw (std::string)
{
  std::string nativeName = toNativeName (name);

  nativeName += '\\';

  if (!SetCurrentDirectory (nativeName.c_str())) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void
OCPI::OS::FileSystem::mkdir (const std::string & name)
  throw (std::string)
{
  std::string nativeName = toNativeName (name);

  if (!CreateDirectory (nativeName.c_str(), 0)) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void
OCPI::OS::FileSystem::rmdir (const std::string & name)
  throw (std::string)
{
  std::string nativeName = toNativeName (name);

  if (!RemoveDirectory (nativeName.c_str())) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

/*
 * ----------------------------------------------------------------------
 * Directory Listing
 * ----------------------------------------------------------------------
 */

OCPI::OS::FileIterator
OCPI::OS::FileSystem::list (const std::string & dir,
                           const std::string & pattern)
  throw (std::string)
{
  return FileIterator (dir, pattern);
}

/*
 * ----------------------------------------------------------------------
 * File Information
 * ----------------------------------------------------------------------
 */

/*
 * Maybe these should use the Win32 functions instead ...
 */

bool
OCPI::OS::FileSystem::exists (const std::string & name, bool * isDir)
  throw ()
{
  /*
   * Special case: the "root" directory "/" exists -- it is a
   * directory that contains the drive names. (toNativeName()
   * thinks that "/" is an invalid file name.)
   */

  if (name.length() == 1 && name[0] == '/') {
    if (isDir) {
      *isDir = true;
    }

    return true;
  }

  std::string nativeName = toNativeName (name);
  DWORD fa = GetFileAttributes (nativeName.c_str());

  if (fa == INVALID_FILE_ATTRIBUTES) {
    return false;
  }

  if (isDir) {
    *isDir = (fa & FILE_ATTRIBUTE_DIRECTORY) ? true : false;
  }

  return true;
}

unsigned long long
OCPI::OS::FileSystem::size (const std::string & name)
  throw (std::string)
{
  std::string nativeName = toNativeName (name);

  HANDLE fh = CreateFile (nativeName.c_str (),
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          0,
                          OPEN_EXISTING,
                          0,
                          0);

  if (fh == INVALID_HANDLE_VALUE) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }

  DWORD fileSizeLow, fileSizeHigh, errCode;

  fileSizeLow = GetFileSize (fh, &fileSizeHigh);

  if (fileSizeLow == INVALID_FILE_SIZE &&
      (errCode = GetLastError()) != NO_ERROR) {
    CloseHandle (fh);
    throw OCPI::OS::Win32::getErrorMessage (errCode);
  }

  CloseHandle (fh);

  return ((unsigned long long) fileSizeLow +
          ((unsigned long long) fileSizeHigh << 32));
}

std::time_t
OCPI::OS::FileSystem::lastModified (const std::string & name)
  throw (std::string)
{
  std::string nativeName = toNativeName (name);
  
  HANDLE fh = CreateFile (nativeName.c_str (),
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          0,
                          OPEN_EXISTING,
                          0,
                          0);

  if (fh == INVALID_HANDLE_VALUE) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }

  FILETIME fileTime;

  if (!GetFileTime (fh, 0, 0, &fileTime)) {
    DWORD errCode = GetLastError ();
    CloseHandle (fh);
    throw OCPI::OS::Win32::getErrorMessage (errCode);
  }

  CloseHandle (fh);

  SYSTEMTIME utcTime, localTime;

  if (!FileTimeToSystemTime (&fileTime, &utcTime)) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }

  if (!SystemTimeToTzSpecificLocalTime (0, &utcTime, &localTime)) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
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

/*
 * ----------------------------------------------------------------------
 * File System Operations
 * ----------------------------------------------------------------------
 */

void
OCPI::OS::FileSystem::rename (const std::string & srcName,
                             const std::string & destName)
  throw (std::string)
{
  std::string srcNativeName = toNativeName (srcName);
  std::string destNativeName = toNativeName (destName);

  if (!MoveFile (srcNativeName.c_str(), destNativeName.c_str())) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void
OCPI::OS::FileSystem::remove (const std::string & name)
  throw (std::string)
{
  std::string nativeName = toNativeName (name);

  if (!DeleteFile (nativeName.c_str())) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
}
