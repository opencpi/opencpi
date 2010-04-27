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
#include <CpiOsRWLock.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <windows.h>
#include "CpiOsWin32Error.h"

namespace {
  struct RWLockData {
    CRITICAL_SECTION mutex;
    unsigned long reading;
    HANDLE event;
  };
}

inline
RWLockData &
o2rwd (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<RWLockData *> (ptr);
}

CPI::OS::RWLock::RWLock ()
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (RWLockData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (RWLockData));
  RWLockData & rwd = o2rwd (m_osOpaque);
  InitializeCriticalSection (&rwd.mutex);
  if ((rwd.event = CreateEvent (0, 0, 0, 0)) == 0) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }
  rwd.reading = 0;
}

CPI::OS::RWLock::~RWLock ()
  throw ()
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  cpiAssert (!rwd.reading);
  DeleteCriticalSection (&rwd.mutex);
  CloseHandle (rwd.event);
}

void
CPI::OS::RWLock::rdLock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  EnterCriticalSection (&rwd.mutex);
  rwd.reading++;
  LeaveCriticalSection (&rwd.mutex);
}

bool
CPI::OS::RWLock::rdTrylock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  if (!TryEnterCriticalSection (&rwd.mutex)) {
    return false;
  }
  rwd.reading++;
  LeaveCriticalSection (&rwd.mutex);
  return true;
}

void
CPI::OS::RWLock::rdUnlock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  EnterCriticalSection (&rwd.mutex);

  cpiAssert (rwd.reading);
  rwd.reading--;

  SetEvent (rwd.event);
  LeaveCriticalSection (&rwd.mutex);
}

void
CPI::OS::RWLock::wrLock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  EnterCriticalSection (&rwd.mutex);

  while (rwd.reading) {
    LeaveCriticalSection (&rwd.mutex);
    WaitForSingleObject (rwd.event, INFINITE);
    EnterCriticalSection (&rwd.mutex);
  }
}

bool
CPI::OS::RWLock::wrTrylock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);

  if (!TryEnterCriticalSection (&rwd.mutex)) {
    return false;
  }

  if (rwd.reading) {
    LeaveCriticalSection (&rwd.mutex);
    return false;
  }

  return true;
}

void
CPI::OS::RWLock::wrUnlock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  SetEvent (rwd.event);
  LeaveCriticalSection (&rwd.mutex);
}
