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
#include <OcpiOsSemaphore.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <climits>
#include <string>
#include <windows.h>
#include "OcpiOsWin32Error.h"

inline
HANDLE &
o2h (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<HANDLE *> (ptr);
}

OCPI::OS::Semaphore::Semaphore (unsigned int initial)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HANDLE)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (HANDLE));

  if (((o2h (m_osOpaque)) = CreateSemaphore (0, initial, LONG_MAX, 0)) == 0) {
    std::string reason = "error creating semaphore: ";
    reason += OCPI::OS::Win32::getErrorMessage (GetLastError());
    throw reason;
  }
}

OCPI::OS::Semaphore::~Semaphore ()
  throw ()
{
  CloseHandle (o2h (m_osOpaque));
}

void
OCPI::OS::Semaphore::post ()
  throw (std::string)
{
  ReleaseSemaphore (o2h (m_osOpaque), 1, 0);
}

void
OCPI::OS::Semaphore::wait ()
  throw (std::string)
{
  WaitForSingleObject (o2h (m_osOpaque), INFINITE);
}

