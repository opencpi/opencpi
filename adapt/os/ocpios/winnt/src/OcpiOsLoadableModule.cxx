
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
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
