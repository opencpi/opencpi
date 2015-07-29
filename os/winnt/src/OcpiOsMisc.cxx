
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


#include <OcpiOsMisc.h>
#include <string>
#include <windows.h>
#include "OcpiOsWin32Socket.h"
#include "OcpiOsWin32DumpStack.h"

void
OCPI::OS::sleep (unsigned long msecs)
  throw (std::string)
{
  Sleep (msecs);
}

unsigned long
OCPI::OS::getProcessId ()
  throw ()
{
  return static_cast<unsigned long> (GetCurrentProcessId ());
}

unsigned long
OCPI::OS::getThreadId ()
  throw ()
{
  return static_cast<unsigned long> (GetCurrentThreadId ());
}

std::string
OCPI::OS::getHostname ()
  throw (std::string)
{
  return Win32::getHostname ();
}

std::string
OCPI::OS::getFQDN ()
  throw (std::string)
{
  return Win32::getFQDN ();
}

std::string
OCPI::OS::getIPAddress ()
  throw (std::string)
{
  return Win32::getIPAddress ();
}

bool
OCPI::OS::isLocalhost (const std::string & name)
  throw (std::string)
{
  return Win32::isLocalhost (name);
}

namespace {

  /*
   * Win32 insists on having a Ctrl-C Handler that takes a DWORD
   * parameter and returns a bool parameter
   */

  void (*g_userHandler) (void) = 0;

  BOOL WINAPI
  Win32CtrlCHandler (DWORD type)
    throw ()
  {
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT) {
      return 0;
    }

    g_userHandler ();
    return 1;
  }

}

void
OCPI::OS::setCtrlCHandler (void (*handler) (void))
  throw ()
{
  if (!g_userHandler) {
    g_userHandler = handler;
    SetConsoleCtrlHandler (Win32CtrlCHandler, 1);
  }
  else {
    g_userHandler = handler;
  }
}

