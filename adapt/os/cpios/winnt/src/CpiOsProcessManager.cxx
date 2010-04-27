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
#include <CpiOsProcessManager.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <string>
#include <vector>
#include <windows.h>
#include "CpiOsWin32Error.h"

/*
 * The PROCESS_INFORMATION structure contains a hProcess HANDLE that
 * we use to track the process' state: if hProcess is not zero, then
 * a process is running and not detached. If the process exits,
 * hProcess is reset to zero.
 *
 * After we waited for the process to exit, we put its exit code in
 * the struct's dwThreadId member, so that it remains available even
 * after closing the handles.
 */

inline
PROCESS_INFORMATION &
o2pi (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<PROCESS_INFORMATION *> (ptr);
}

CPI::OS::ProcessManager::ProcessManager ()
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (PROCESS_INFORMATION)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (PROCESS_INFORMATION));
#if !defined(NDEBUG)
  o2pi (m_osOpaque).hProcess = 0;
#endif
}

CPI::OS::ProcessManager::ProcessManager (const std::string & executable,
					 const ParameterList & parameters)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (PROCESS_INFORMATION)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (PROCESS_INFORMATION));
#if !defined(NDEBUG)
  o2pi (m_osOpaque).hProcess = 0;
#endif
  start (executable, parameters);
}

CPI::OS::ProcessManager::~ProcessManager ()
  throw ()
{
#if !defined(NDEBUG)
  cpiAssert (!o2pi (m_osOpaque).hProcess);
#endif
}

void
CPI::OS::ProcessManager::start (const std::string & executable,
				const ParameterList & parameters)
  throw (std::string)
{
  PROCESS_INFORMATION & pi = o2pi (m_osOpaque);

#if !defined(NDEBUG)
  cpiAssert (!pi.hProcess);
#endif

  /*
   * Construct command line
   */

  std::string commandLine = "\"";
  commandLine += executable;
  commandLine += '\"';

  for (ParameterList::const_iterator it = parameters.begin ();
       it != parameters.end(); it++) {
    const std::string & par = *it;

    /*
     * Put each parameter in quotes.
     */

    commandLine += " \"";
 
    /*
     * If a parameter contains a quotation mark, it must be quoted.
     *
     * A backslash must be quoted if it precedes a quotation mark, so
     * that it is not mistaken for a backslash that quotes the quotation
     * mark, or if it occurs at the end of the string, so that it is not
     * mistaken as quoting the quotation mark that we use to delimit the
     * parameter. However, a backslash in the middle of a string must
     * not be quoted, it is treated literally (as in "hello\world").
     * Windows sure has an interesting way of parsing the command line.
     */

    std::string::size_type parLen = par.length ();
    std::string::size_type quotPos = par.find_first_of ("\\\"");

    if (quotPos == std::string::npos) {
      commandLine += par;
    }
    else {
      for (std::string::size_type pos = 0; pos < parLen; pos++) {
	if (par[pos] == '\"' ||
	    (par[pos] == '\\' && pos+1 < parLen && par[pos+1] == '\"') ||
	    (par[pos] == '\\' && pos+1 == parLen)) {
	  commandLine += '\\';
	}
	commandLine += par[pos];
      }
    }

    commandLine += "\"";
  }

  /*
   * CreateProcess insists on modifying its lpCommandLine parameter
   */

  char * cl = new char [commandLine.length() + 1];
  commandLine.copy (cl, commandLine.length());
  cl[commandLine.length()] = 0;

  /*
   * Create process
   *
   * The CREATE_NEW_PROCESS_GROUP flag is used so that the process
   * can be selectively signaled using GenerateConsoleCtrlEvent (see
   * shutdown() below).
   */

  STARTUPINFO si;
  si.cb = sizeof (STARTUPINFO);
  si.lpReserved = 0;
  si.lpDesktop = 0;
  si.lpTitle = 0;
  si.dwFlags = 0;
  si.cbReserved2 = 0;
  si.lpReserved2 = 0;

  bool res = !!CreateProcess (executable.c_str (), cl,
			      0, 0, 0,
			      CREATE_NEW_PROCESS_GROUP,
			      0, 0, &si,
			      &pi);

  delete cl;

  if (!res) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

unsigned long
CPI::OS::ProcessManager::pid ()
  throw (std::string)
{
  return o2pi (m_osOpaque).dwProcessId;
}

bool
CPI::OS::ProcessManager::wait (unsigned long timeout)
  throw (std::string)
{
  PROCESS_INFORMATION & pi = o2pi (m_osOpaque);

#if !defined(NDEBUG)
  cpiAssert (pi.hProcess);
#endif

  DWORD flag = WaitForSingleObject (pi.hProcess, timeout);
  bool good = (flag == WAIT_ABANDONED || flag == WAIT_OBJECT_0);

  if (!good) {
    return false;
  }

  DWORD exitCode;

  if (!GetExitCodeProcess (pi.hProcess, &exitCode)) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }

  /*
   * Close handles, and put exit code into our data structure
   */

  CloseHandle (pi.hProcess);
  CloseHandle (pi.hThread);

#if !defined(NDEBUG)
  pi.hProcess = 0;
#endif
  pi.dwThreadId = exitCode;

  return true;
}

int
CPI::OS::ProcessManager::getExitCode ()
  throw (std::string)
{
  PROCESS_INFORMATION & pi = o2pi (m_osOpaque);

#if !defined(NDEBUG)
  cpiAssert (!pi.hProcess);
#endif

  /*
   * Requires that wait() returned true, see above.
   */

  return static_cast<int> (pi.dwThreadId);
}

void
CPI::OS::ProcessManager::shutdown ()
  throw (std::string)
{
  PROCESS_INFORMATION & pi = o2pi (m_osOpaque);

#if !defined(NDEBUG)
  cpiAssert (pi.hProcess);
#endif

  if (!GenerateConsoleCtrlEvent (CTRL_BREAK_EVENT, pi.dwProcessId)) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void
CPI::OS::ProcessManager::kill ()
  throw (std::string)
{
  PROCESS_INFORMATION & pi = o2pi (m_osOpaque);

#if !defined(NDEBUG)
  cpiAssert (pi.hProcess);
#endif

  if (!TerminateProcess (pi.hProcess, /* exit code */ 255)) {
    throw CPI::OS::Win32::getErrorMessage (GetLastError());
  }
}

void
CPI::OS::ProcessManager::detach ()
  throw ()
{
  PROCESS_INFORMATION & pi = o2pi (m_osOpaque);

#if !defined(NDEBUG)
  cpiAssert (pi.hProcess);
#endif

  CloseHandle (pi.hProcess);
  CloseHandle (pi.hThread);

#if !defined(NDEBUG)
  pi.hProcess = 0;
#endif
}
