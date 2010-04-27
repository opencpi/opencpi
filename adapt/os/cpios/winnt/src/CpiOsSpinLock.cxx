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
#include <CpiOsSpinLock.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <windows.h>

inline
LONG *
o2l (CPI::OS::uint64_t * ptr)
  throw ()
{
  return reinterpret_cast<LONG *> (ptr);
}

CPI::OS::SpinLock::SpinLock ()
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (LONG)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (LONG));
  *o2l (m_osOpaque) = 0;
}

CPI::OS::SpinLock::~SpinLock ()
  throw ()
{
}

void
CPI::OS::SpinLock::lock ()
  throw (std::string)
{
  LONG * ptr = o2l (m_osOpaque);
  LONG count;

  while ((count = InterlockedIncrement (ptr)) != 1) {
    InterlockedDecrement (ptr);
    Sleep (0);
  }
}

bool
CPI::OS::SpinLock::trylock ()
  throw (std::string)
{
  LONG * ptr = o2l (m_osOpaque);
  LONG count;

  if ((count = InterlockedIncrement (ptr)) != 1) {
    InterlockedDecrement (ptr);
    return false;
  }

  return true;
}

void
CPI::OS::SpinLock::unlock ()
  throw (std::string)
{
  InterlockedDecrement (o2l (m_osOpaque));
}
