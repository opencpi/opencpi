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
#include <CpiOsLoadableModule.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <cassert>
#include <string>
#include <windows.h>
#include "CpiOsWin32Error.h"

inline
HMODULE &
o2h (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<HMODULE *> (ptr);
}

CPI::OS::LoadableModule::LoadableModule ()
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HMODULE)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (HMODULE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}

CPI::OS::LoadableModule::LoadableModule (const std::string & fileName)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HMODULE)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (HMODULE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
  open (fileName);
}

CPI::OS::LoadableModule::~LoadableModule ()
  throw ()
{
#if !defined(NDEBUG)
  cpiAssert (!o2h (m_osOpaque));
#endif
}

void
CPI::OS::LoadableModule::open (const std::string & fileName)
  throw (std::string)
{
  if (!(o2h (m_osOpaque) = LoadLibrary (fileName.c_str()))) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void *
CPI::OS::LoadableModule::getSymbol (const std::string & functionName)
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2h (m_osOpaque));
#endif

  FARPROC addr;

  if (!(addr = GetProcAddress (o2h (m_osOpaque), functionName.c_str()))) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }

  return reinterpret_cast<void *> (reinterpret_cast<int> (addr));
}

void
CPI::OS::LoadableModule::close ()
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2h (m_osOpaque));
#endif

  if (!FreeLibrary (o2h (m_osOpaque))) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }

#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}
