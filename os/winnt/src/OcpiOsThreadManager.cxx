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
#include <OcpiOsThreadManager.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
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

/*
 * Entry point for the new thread. This is necessary because of
 * a difference in signatures: we use a function w/o return, while
 * Win32 wants a function returning DWORD.
 *
 * Have to use heap for the RunThreadParams to avoid race conditions,
 * in case the creating thread immediately calls detach() and the
 * destructor, before the new thread gets a chance to run.
 */

namespace {
  struct RunThreadParams {
    void (*func) (void *);
    void * opaque;
  };

  DWORD WINAPI
  RunThread (void * arg)
    throw ()
  {
    RunThreadParams * p = static_cast<RunThreadParams *> (arg);
    void (*func) (void *) = p->func;
    void * opaque = p->opaque;
    delete p;
    func (opaque);
    return 0;
  }
}

OCPI::OS::ThreadManager::ThreadManager ()
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HANDLE)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (HANDLE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}

OCPI::OS::ThreadManager::ThreadManager (void (*func) (void *), void * opaque)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HANDLE)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (HANDLE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
  start (func, opaque);
}

OCPI::OS::ThreadManager::~ThreadManager ()
  throw ()
{
#if !defined(NDEBUG)
  ocpiAssert (!o2h (m_osOpaque));
#endif
}

void
OCPI::OS::ThreadManager::start (void (*func) (void *), void * opaque)
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (!o2h (m_osOpaque));
#endif

  RunThreadParams * p = new RunThreadParams;
  p->func = func;
  p->opaque = opaque;

  if (!(o2h (m_osOpaque) = CreateThread (0, 0, RunThread, p, 0, 0))) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void
OCPI::OS::ThreadManager::join ()
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (o2h (m_osOpaque));
#endif

  WaitForSingleObject (o2h (m_osOpaque), INFINITE);
  CloseHandle (o2h (m_osOpaque));

#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}

void
OCPI::OS::ThreadManager::detach ()
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (o2h (m_osOpaque));
#endif

  CloseHandle (o2h (m_osOpaque));

#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}
