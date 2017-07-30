/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <OcpiOsAssert.h>
#include <OcpiOsRWLock.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <windows.h>
#include "OcpiOsWin32Error.h"

namespace {
  struct RWLockData {
    CRITICAL_SECTION mutex;
    unsigned long reading;
    HANDLE event;
  };
}

inline
RWLockData &
o2rwd (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<RWLockData *> (ptr);
}

OCPI::OS::RWLock::RWLock ()
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (RWLockData)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (RWLockData));
  RWLockData & rwd = o2rwd (m_osOpaque);
  InitializeCriticalSection (&rwd.mutex);
  if ((rwd.event = CreateEvent (0, 0, 0, 0)) == 0) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
  rwd.reading = 0;
}

OCPI::OS::RWLock::~RWLock ()
  throw ()
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  ocpiAssert (!rwd.reading);
  DeleteCriticalSection (&rwd.mutex);
  CloseHandle (rwd.event);
}

void
OCPI::OS::RWLock::rdLock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  EnterCriticalSection (&rwd.mutex);
  rwd.reading++;
  LeaveCriticalSection (&rwd.mutex);
}

bool
OCPI::OS::RWLock::rdTrylock ()
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
OCPI::OS::RWLock::rdUnlock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  EnterCriticalSection (&rwd.mutex);

  ocpiAssert (rwd.reading);
  rwd.reading--;

  SetEvent (rwd.event);
  LeaveCriticalSection (&rwd.mutex);
}

void
OCPI::OS::RWLock::wrLock ()
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
OCPI::OS::RWLock::wrTrylock ()
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
OCPI::OS::RWLock::wrUnlock ()
  throw (std::string)
{
  RWLockData & rwd = o2rwd (m_osOpaque);
  SetEvent (rwd.event);
  LeaveCriticalSection (&rwd.mutex);
}
