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

#include <CpiOsMutex.h>
#include <cstdio>
#include <cstring>
#include <winsock2.h>
#include "CpiOsWin32Socket.h"

namespace {
  unsigned int winSockInitCount = 0;
  CPI::OS::Mutex wsicMutex;
}

void
CPI::OS::Win32::winSockInit ()
  throw (std::string)
{
  wsicMutex.lock ();

  if (winSockInitCount++ == 0) {
    WORD wVersionRequested;
    WSADATA wsaData;
    
    wVersionRequested = MAKEWORD (2, 0);
    
    if (WSAStartup (wVersionRequested, &wsaData)) {
      std::string reason = "WSAStartup failed: ";
      reason += getWinSockErrorMessage (WSAGetLastError());
      wsicMutex.unlock ();
      throw reason;
    }
  }

  wsicMutex.unlock ();
}

void
CPI::OS::Win32::winSockFini ()
  throw (std::string)
{
  wsicMutex.lock ();

  if (--winSockInitCount == 0) {
    if (WSACleanup ()) {
      std::string reason = "WSACleanup failed: ";
      reason += getWinSockErrorMessage (WSAGetLastError());
      wsicMutex.unlock ();
      throw reason;
    }
  }

  wsicMutex.unlock ();
}

std::string
CPI::OS::Win32::getHostname ()
  throw (std::string)
{
  winSockInit ();

  char buffer[1024];
  if (gethostname (buffer, 1024) != 0) {
    std::string reason = getWinSockErrorMessage (WSAGetLastError());
    winSockFini ();
    throw reason;
  }

  winSockFini ();
  return buffer;
}

std::string
CPI::OS::Win32::getFQDN ()
  throw (std::string)
{
  winSockInit ();

  std::string localName;

  try {
    localName = getHostname ();
  }
  catch (...) {
    winSockFini ();
    throw;
  }

  struct hostent * hent = ::gethostbyname (localName.c_str ());

  if (!hent || !hent->h_name) {
    std::string reason = getWinSockErrorMessage (WSAGetLastError());
    winSockFini ();
    throw reason;
  }

  std::string fqdn = hent->h_name;
  winSockFini ();
  return fqdn;
}

std::string
CPI::OS::Win32::getIPAddress ()
  throw (std::string)
{
  winSockInit ();

  std::string localName;

  try {
    localName = getHostname ();
  }
  catch (...) {
    winSockFini ();
    throw;
  }

  struct hostent * hent = ::gethostbyname (localName.c_str ());

  if (!hent || !hent->h_name || !*hent->h_addr_list) {
    std::string reason = getWinSockErrorMessage (WSAGetLastError());
    winSockFini ();
    throw reason;
  }

  struct in_addr in;
  std::memcpy (&in.s_addr, *hent->h_addr_list, sizeof (in.s_addr));
  std::string address = inet_ntoa (in);
  winSockFini ();
  return address;
}

bool
CPI::OS::Win32::isLocalhost (const std::string & name)
  throw (std::string)
{
  /*
   * Check whether the name is "localhost"
   */

  if (name.compare ("localhost") == 0) {
    return true;
  }

  winSockInit ();

  /*
   * Check whether the name is my host name
   */

  std::string localName;

  try {
    localName = getHostname ();
  }
  catch (...) {
    winSockFini ();
    throw;
  }

  bool res = (name.compare (localName) == 0);

  if (!res) {
    /*
     * Check whether the name matches any of my aliases
     */

    struct hostent * hent = ::gethostbyname (localName.c_str ());

    if (!hent || !hent->h_name) {
      std::string reason = getWinSockErrorMessage (WSAGetLastError());
      winSockFini ();
      throw reason;
    }

    res = (name.compare (hent->h_name) == 0);

    if (!res) {
      char ** aliases = hent->h_aliases;

      while (aliases && *aliases) {
        if (name.compare (*aliases) == 0) {
          res = true;
          break;
        }
        aliases++;
      }
    }
  }

  winSockFini ();
  return res;
}

namespace {
  struct WinSockErrorDescription {
    int code;
    const char * name;
    const char * desc;
  };

  WinSockErrorDescription winSockErrors[] = {
    { WSAEINTR,       "WSAEINTR",       "Interrupted function call" },
    { WSAEACCES,      "WSAEACCES",      "Permission denied" },
    { WSAEFAULT,      "WSAEFAULT",      "Bad address" },
    { WSAEINVAL,      "WSAEINVAL",      "Invalid argument" },
    { WSAEMFILE,      "WSAEMFILE",      "Too many open files" },
    { WSAEWOULDBLOCK, "WSAEWOULDBLOCK", "Resource temporarily unavailable" },
    { WSAEINPROGRESS, "WSAEINPROGRESS", "Operation now in progress" },
    { WSAEALREADY,    "WSAEALREADY",    "Operation already in progress" },
    { WSAENOTSOCK,    "WSAENOTSOCK",    "Socket operation on nonsocket" },
    { WSAEDESTADDRREQ,"WSAEDESTADDRREQ","Destination address required" },
    { WSAEMSGSIZE,    "WSAEMSGSIZE",    "Message too long" },
    { WSAEPROTOTYPE,  "WSAEPROTOTYPE",  "Protocol wrong type for socket" },
    { WSAENOPROTOOPT, "WSAENOPROTOOPT", "Bad protocol option" },
    { WSAEPROTONOSUPPORT, "WSAEPROTONOSUPPORT", "Protocol not supported" },
    { WSAESOCKTNOSUPPORT, "WSAESOCKTNOSUPPORT", "Socket type not supported" },
    { WSAEOPNOTSUPP,  "WSAEOPNOTSUPP",  "Operation not supported" },
    { WSAEPFNOSUPPORT,"WSAEPFNOSUPPORT","Protocol family not supported" },
    { WSAEAFNOSUPPORT,"WSAEAFNOSUPPORT","Address family not supported by protocol family" },
    { WSAEADDRINUSE,  "WSAEADDRINUSE",  "Address already in use" },
    { WSAEADDRNOTAVAIL, "WSAEADDRNOTAVAIL", "Cannot assign requested address" },
    { WSAENETDOWN,    "WSAENETDOWN",    "Network is down" },
    { WSAENETUNREACH, "WSAENETUNREACH", "Network is unreachable" },
    { WSAENETRESET,   "WSAENETRESET",   "Network dropped connection on reset" },
    { WSAECONNABORTED,"WSAECONNABORTED","Software caused connection abort" },
    { WSAECONNRESET,  "WSAECONNRESET",  "Connection reset by peer" },
    { WSAENOBUFS,     "WSAENOBUFS",     "No buffer space available" },
    { WSAEISCONN,     "WSAEISCONN",     "Socket is already connected" },
    { WSAENOTCONN,    "WSAENOTCONN",    "Socket is not connected" },
    { WSAESHUTDOWN,   "WSAESHUTDOWN",   "Cannot send after socket shutdown" },
    { WSAETIMEDOUT,   "WSAETIMEDOUT",   "Connection timed out" },
    { WSAECONNREFUSED,"WSAECONNREFUSED","Connection refused" },
    { WSAEHOSTDOWN,   "WSAEHOSTDOWN",   "Host is down" },
    { WSAEHOSTUNREACH,"WSAEHOSTUNREACH","No route to host" },
    { WSAEPROCLIM,    "WSAEPROCLIM",    "Too many processes" },
    { WSASYSNOTREADY, "WSASYSNOTREADY", "Network subsystem is unavailable" },
    { WSAVERNOTSUPPORTED, "WSAVERNOTSUPPORTED", "Winsock.dll version out of range" },
    { WSANOTINITIALISED, "WSANOTINITIALISED", "Successful WSAStartup not yet performed" },
    { WSAEDISCON,     "WSAEDISCON",     "Graceful shutdown in progress" },
    { WSATYPE_NOT_FOUND, "WSATYPE_NOT_FOUND", "The specified class was not found" },
    { WSAHOST_NOT_FOUND, "WSAHOST_NOT_FOUND", "Host not found" },
    { WSATRY_AGAIN,   "WSATRY_AGAIN",   "Nonauthoritative host not found" },
    { WSANO_RECOVERY, "WSANO_RECOVERY", "This is a nonrecoverable error" },
    { WSANO_DATA,     "WSANO_DATA",     "Valid name, no data record of requested type" },
    { WSA_INVALID_HANDLE, "WSA_INVALID_HANDLE", "Specified event object handle is invalid" },
    { WSA_INVALID_PARAMETER, "WSA_INVALID_PARAMETER", "One or more parameters are invalid" },
    { WSA_IO_INCOMPLETE, "WSA_IO_INCOMPLETE", "Overlapped I/O event object not in signaled state" },
    { WSA_IO_PENDING, "WSA_IO_PENDING", "Overlapped operations will complete later" },
    { WSA_NOT_ENOUGH_MEMORY, "WSA_NOT_ENOUGH_MEMORY", "Insufficient memory available" },
    { WSA_OPERATION_ABORTED, "WSA_OPERATION_ABORTED", "Overlapped operation aborted" },
#if 0
    /*
     * These don't seem to exist?
     * They're documented, but the compiler complains.
     */
    { WSAINVALIDPROCTABLE, "WSAINVALIDPROCTABLE", "Invalid procedure table from service provider" },
    { WSAINVALIDPROVIDER, "WSAINVALIDPROVIDER", "Invalid service provider version number" },
    { WSAPROVIDERFAILEDINIT, "WSAPROVIDERFAILEDINIT", "Unable to initialize a service provider" },
#endif
    { WSASYSCALLFAILURE, "WSASYSCALLFAILURE", "System call failure" },
    { 0, 0, 0 }
  };
}

std::string
CPI::OS::Win32::getWinSockErrorMessage (int errorCode)
  throw ()
{
  WinSockErrorDescription * edi = winSockErrors;

  while (edi->name) {
    if (errorCode == edi->code) {
      std::string reason = edi->desc;
      reason += " (";
      reason += edi->name;
      reason += ")";
      return reason;
    }
    edi++;
  }

  char tmp[32];
  std::sprintf (tmp, "error %d", errorCode);
  return tmp;
}

