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

#include <CpiOsMisc.h>
#include <string>
#include <windows.h>
#include "CpiOsWin32Socket.h"
#include "CpiOsWin32DumpStack.h"

void
CPI::OS::sleep (unsigned long msecs)
  throw (std::string)
{
  Sleep (msecs);
}

unsigned long
CPI::OS::getProcessId ()
  throw ()
{
  return static_cast<unsigned long> (GetCurrentProcessId ());
}

unsigned long
CPI::OS::getThreadId ()
  throw ()
{
  return static_cast<unsigned long> (GetCurrentThreadId ());
}

std::string
CPI::OS::getHostname ()
  throw (std::string)
{
  return Win32::getHostname ();
}

std::string
CPI::OS::getFQDN ()
  throw (std::string)
{
  return Win32::getFQDN ();
}

std::string
CPI::OS::getIPAddress ()
  throw (std::string)
{
  return Win32::getIPAddress ();
}

bool
CPI::OS::isLocalhost (const std::string & name)
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
CPI::OS::setCtrlCHandler (void (*handler) (void))
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

