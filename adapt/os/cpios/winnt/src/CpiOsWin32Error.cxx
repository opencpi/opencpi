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

#include "CpiOsWin32Error.h"
#include <windows.h>
#include <cstdio>
#include <string>

std::string
CPI::OS::Win32::getErrorMessage (unsigned long errorCode)
  throw ()
{
  char lpMsgBuf[1024];

  if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL,
                     errorCode,
                     MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     lpMsgBuf, 1024,
                     NULL)) {
    /*
     * Get rid of line break at the end.
     */

    char * ptr = lpMsgBuf;

    while (*ptr && *ptr != '\r' && *ptr != '\n') {
      ptr++;
    }

    *ptr = '\0';
  }
  else {
    std::sprintf (lpMsgBuf, "error %lu", errorCode);
  }

  return lpMsgBuf;
}
