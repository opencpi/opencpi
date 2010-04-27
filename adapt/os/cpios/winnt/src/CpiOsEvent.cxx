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
#include <CpiOsEvent.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <windows.h>
#include <string>
#include "CpiOsWin32Error.h"

inline
HANDLE &
o2h (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<HANDLE *> (ptr);
}

CPI::OS::Event::Event (bool initial)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HANDLE)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (HANDLE));
  if (((o2h (m_osOpaque)) = CreateEvent (0, 0, initial, 0)) == 0) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

CPI::OS::Event::~Event ()
  throw ()
{
  CloseHandle (o2h (m_osOpaque));
}

void
CPI::OS::Event::set ()
  throw (std::string)
{
  SetEvent (o2h (m_osOpaque));
}

void
CPI::OS::Event::wait ()
  throw (std::string)
{
  WaitForSingleObject (o2h (m_osOpaque), INFINITE);
}

bool
CPI::OS::Event::wait (unsigned int timeout)
  throw (std::string)
{
  if (WaitForSingleObject (o2h (m_osOpaque), timeout) == WAIT_TIMEOUT) {
    return false;
  }

  return true;
}

