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
#include <CpiOsMutex.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <string>
#include <windows.h>

/*
 * ----------------------------------------------------------------------
 * A Windows CriticalSection is always recursive, so we do some
 * bookkeeping that allows us to complain about attempts to re-lock
 * a non-recursive Mutex.
 * ----------------------------------------------------------------------
 */

namespace {
  struct MutexData {
    CRITICAL_SECTION cs;
    bool recursive;
    unsigned long count;
  };
}

inline
MutexData &
o2md (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<MutexData *> (ptr);
}

CPI::OS::Mutex::Mutex (bool recursive)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (MutexData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (MutexData));
  MutexData & md = o2md (m_osOpaque);
  InitializeCriticalSection (&md.cs);
  md.recursive = recursive;
  md.count = 0;
}

CPI::OS::Mutex::~Mutex ()
  throw ()
{
  MutexData & md = o2md (m_osOpaque);
  DeleteCriticalSection (&md.cs);
}

void
CPI::OS::Mutex::lock ()
  throw (std::string)
{
  MutexData & md = o2md (m_osOpaque);
  EnterCriticalSection (&md.cs);
  if (md.count && !md.recursive) {
    LeaveCriticalSection (&md.cs);
    throw std::string ("attempt to re-lock non-recursive mutex");
  }
  md.count++;
}

bool
CPI::OS::Mutex::trylock ()
  throw (std::string)
{
  MutexData & md = o2md (m_osOpaque);
  if (!TryEnterCriticalSection (&md.cs)) {
    return false;
  }
  if (md.count && !md.recursive) {
    LeaveCriticalSection (&md.cs);
    throw std::string ("attempt to re-lock non-recursive mutex");
  }
  md.count++;
  return true;
}

void
CPI::OS::Mutex::unlock ()
  throw (std::string)
{
  MutexData & md = o2md (m_osOpaque);
  md.count--;
  LeaveCriticalSection (&md.cs);
}
