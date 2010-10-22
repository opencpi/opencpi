
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
#include <OcpiOsSpinLock.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <windows.h>

inline
LONG *
o2l (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return reinterpret_cast<LONG *> (ptr);
}

OCPI::OS::SpinLock::SpinLock ()
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (LONG)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (LONG));
  *o2l (m_osOpaque) = 0;
}

OCPI::OS::SpinLock::~SpinLock ()
  throw ()
{
}

void
OCPI::OS::SpinLock::lock ()
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
OCPI::OS::SpinLock::trylock ()
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
OCPI::OS::SpinLock::unlock ()
  throw (std::string)
{
  InterlockedDecrement (o2l (m_osOpaque));
}
