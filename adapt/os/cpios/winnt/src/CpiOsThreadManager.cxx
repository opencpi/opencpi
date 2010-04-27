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
#include <CpiOsThreadManager.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
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

CPI::OS::ThreadManager::ThreadManager ()
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HANDLE)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (HANDLE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}

CPI::OS::ThreadManager::ThreadManager (void (*func) (void *), void * opaque)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HANDLE)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (HANDLE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
  start (func, opaque);
}

CPI::OS::ThreadManager::~ThreadManager ()
  throw ()
{
#if !defined(NDEBUG)
  cpiAssert (!o2h (m_osOpaque));
#endif
}

void
CPI::OS::ThreadManager::start (void (*func) (void *), void * opaque)
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (!o2h (m_osOpaque));
#endif

  RunThreadParams * p = new RunThreadParams;
  p->func = func;
  p->opaque = opaque;

  if (!(o2h (m_osOpaque) = CreateThread (0, 0, RunThread, p, 0, 0))) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void
CPI::OS::ThreadManager::join ()
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2h (m_osOpaque));
#endif

  WaitForSingleObject (o2h (m_osOpaque), INFINITE);
  CloseHandle (o2h (m_osOpaque));

#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}

void
CPI::OS::ThreadManager::detach ()
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2h (m_osOpaque));
#endif

  CloseHandle (o2h (m_osOpaque));

#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}
