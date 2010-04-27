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
#include <CpiOsSemaphore.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <climits>
#include <string>
#include <windows.h>
#include "CpiOsWin32Error.h"

inline
HANDLE &
o2h (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<HANDLE *> (ptr);
}

CPI::OS::Semaphore::Semaphore (unsigned int initial)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HANDLE)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (HANDLE));

  if (((o2h (m_osOpaque)) = CreateSemaphore (0, initial, LONG_MAX, 0)) == 0) {
    std::string reason = "error creating semaphore: ";
    reason += CPI::OS::Win32::getErrorMessage (GetLastError());
    throw reason;
  }
}

CPI::OS::Semaphore::~Semaphore ()
  throw ()
{
  CloseHandle (o2h (m_osOpaque));
}

void
CPI::OS::Semaphore::post ()
  throw (std::string)
{
  ReleaseSemaphore (o2h (m_osOpaque), 1, 0);
}

void
CPI::OS::Semaphore::wait ()
  throw (std::string)
{
  WaitForSingleObject (o2h (m_osOpaque), INFINITE);
}

