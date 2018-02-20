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
#include <OcpiOsLoadableModule.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <cassert>
#include <string>
#include <windows.h>
#include "OcpiOsWin32Error.h"

inline
HMODULE &
o2h (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<HMODULE *> (ptr);
}

OCPI::OS::LoadableModule::LoadableModule ()
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HMODULE)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (HMODULE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}

OCPI::OS::LoadableModule::LoadableModule (const std::string & fileName)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (HMODULE)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (HMODULE));
#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
  open (fileName);
}

OCPI::OS::LoadableModule::~LoadableModule ()
  throw ()
{
#if !defined(NDEBUG)
  ocpiAssert (!o2h (m_osOpaque));
#endif
}

void
OCPI::OS::LoadableModule::open (const std::string & fileName)
  throw (std::string)
{
  if (!(o2h (m_osOpaque) = LoadLibrary (fileName.c_str()))) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void *
OCPI::OS::LoadableModule::getSymbol (const std::string & functionName)
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (o2h (m_osOpaque));
#endif

  FARPROC addr;

  if (!(addr = GetProcAddress (o2h (m_osOpaque), functionName.c_str()))) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }

  return reinterpret_cast<void *> (reinterpret_cast<int> (addr));
}

void
OCPI::OS::LoadableModule::close ()
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (o2h (m_osOpaque));
#endif

  if (!FreeLibrary (o2h (m_osOpaque))) {
    throw OCPI::OS::Win32::getErrorMessage (GetLastError());
  }

#if !defined(NDEBUG)
  o2h (m_osOpaque) = 0;
#endif
}
