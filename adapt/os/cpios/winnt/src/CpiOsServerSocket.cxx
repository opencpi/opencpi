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
#include <CpiOsSocket.h>
#include <CpiOsServerSocket.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <cstdlib>
#include <string>
#include <winsock2.h>
#include "CpiOsWin32Socket.h"

inline
int &
o2fd (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<int *> (ptr);
}

inline
const int &
o2fd (const CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<const int *> (ptr);
}

namespace {
  enum {
    DEFAULT_LISTEN_BACKLOG = 10
  };
}

CPI::OS::ServerSocket::ServerSocket ()
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  CPI::OS::Win32::winSockInit ();
#if !defined(NDEBUG)
  o2fd (m_osOpaque) = -1;
#endif
}

CPI::OS::ServerSocket::ServerSocket (unsigned int portNo, bool reuse)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  CPI::OS::Win32::winSockInit ();
#if !defined(NDEBUG)
  o2fd (m_osOpaque) = -1;
#endif
  bind (portNo, reuse);
}

CPI::OS::ServerSocket::~ServerSocket ()
  throw ()
{
  CPI::OS::Win32::winSockFini ();
#if !defined(NDEBUG)
  cpiAssert (o2fd (m_osOpaque) == -1);
#endif
}

void
CPI::OS::ServerSocket::bind (unsigned int portNo, bool reuse)
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2fd (m_osOpaque) == -1);
#endif

  struct sockaddr_in sin;
  memset (&sin, 0, sizeof (struct sockaddr_in));

  sin.sin_family = AF_INET;
  sin.sin_port = htons (portNo);
  sin.sin_addr.s_addr = INADDR_ANY;

  int fileno = ::socket (PF_INET, SOCK_STREAM, 0);
  
  if (fileno < 0) {
    throw CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
  }

  int reuseopt = reuse ? 1 : 0;

  if (::setsockopt (fileno, SOL_SOCKET, SO_REUSEADDR,
                    (char *) &reuseopt, sizeof (int)) != 0) {
    std::string reason =
      CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
    ::closesocket (fileno);
    throw reason;
  }
  
  if (::bind (fileno, (struct sockaddr *) &sin, sizeof (sin)) != 0) {
    std::string reason =
      CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
    ::closesocket (fileno);
    throw reason;
  }

  if (::listen (fileno, DEFAULT_LISTEN_BACKLOG) != 0) {
    std::string reason =
      CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
    ::closesocket (fileno);
    throw reason;
  }

  o2fd (m_osOpaque) = fileno;
}

unsigned int
CPI::OS::ServerSocket::getPortNo ()
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2fd (m_osOpaque) != -1);
#endif

  struct sockaddr_in sin;
  int len = sizeof (sin);

  int ret = ::getsockname (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
  }

  return ntohs (sin.sin_port);
}

CPI::OS::Socket
CPI::OS::ServerSocket::accept ()
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2fd (m_osOpaque) != -1);
#endif

  int newfd = ::accept (o2fd (m_osOpaque), 0, 0);

  if (newfd == INVALID_SOCKET) {
    throw CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
  }

  // by convention
  CPI::OS::Win32::winSockInit ();

  CPI::OS::uint64_t * fd2o = reinterpret_cast<CPI::OS::uint64_t *> (&newfd);
  return CPI::OS::Socket (fd2o);
}

bool
CPI::OS::ServerSocket::wait (unsigned long msecs)
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2fd (m_osOpaque) != -1);
#endif

  struct timeval timeout;
  fd_set readfds;

  int fd = o2fd (m_osOpaque);

 again:
  FD_ZERO (&readfds);
  FD_SET (fd, &readfds);

  if (msecs != static_cast<unsigned long> (-1)) {
    timeout.tv_sec = msecs / 1000;
    timeout.tv_usec = (msecs % 1000) * 1000;
  }
  else {
    timeout.tv_sec = 3600000; /* 1 hour */
    timeout.tv_usec = 0;
  }

  int res = ::select (fd+1, &readfds, 0, 0, &timeout);

  if (res < 0) {
    throw CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
  }

  if (msecs == static_cast<unsigned long> (-1) && res == 0) {
    goto again;
  }

  /*
   * Maybe we were woken up by a close()? Then we must raise an exception,
   * so that the caller does not continue to call accept().
   */

  if (o2fd (m_osOpaque) == -1) {
    throw std::string ("socket closed");
  }

  return res ? true : false;
}

void
CPI::OS::ServerSocket::close ()
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2fd (m_osOpaque) != -1);
#endif

  if (::closesocket (o2fd (m_osOpaque))) {
    throw CPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
  }

#if !defined(NDEBUG)
  o2fd (m_osOpaque) = -1;
#endif
}
