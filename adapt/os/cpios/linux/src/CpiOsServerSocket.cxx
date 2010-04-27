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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "CpiOsPosixError.h"

inline
int &
o2fd (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<int *> (ptr);
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
  o2fd (m_osOpaque) = -1;
}

CPI::OS::ServerSocket::ServerSocket (unsigned int portNo, bool reuse)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = -1;
  bind (portNo, reuse);
}

CPI::OS::ServerSocket::~ServerSocket ()
  throw ()
{
  cpiAssert (o2fd (m_osOpaque) == -1);
}

void
CPI::OS::ServerSocket::bind (unsigned int portNo, bool reuse)
  throw (std::string)
{
  cpiAssert (o2fd (m_osOpaque) == -1);

  struct sockaddr_in sin;
  memset (&sin, 0, sizeof (struct sockaddr_in));

  sin.sin_family = AF_INET;
  sin.sin_port = htons (portNo);
  sin.sin_addr.s_addr = INADDR_ANY;

  int fileno = ::socket (PF_INET, SOCK_STREAM, 0);
  
  if (fileno < 0) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  int reuseopt = reuse ? 1 : 0;

  if (::setsockopt (fileno, SOL_SOCKET, SO_REUSEADDR,
		    (char *) &reuseopt, sizeof (int)) != 0) {
    int err = errno;
    ::close (fileno);
    throw CPI::OS::Posix::getErrorMessage (err);
  }
  
  if (::bind (fileno, (struct sockaddr *) &sin, sizeof (sin)) != 0) {
    int err = errno;
    ::close (fileno);
    throw CPI::OS::Posix::getErrorMessage (err);
  }

  if (::listen (fileno, DEFAULT_LISTEN_BACKLOG) != 0) {
    int err = errno;
    ::close (fileno);
    throw CPI::OS::Posix::getErrorMessage (err);
  }

  o2fd (m_osOpaque) = fileno;
}

unsigned int
CPI::OS::ServerSocket::getPortNo ()
  throw (std::string)
{
  cpiAssert (o2fd (m_osOpaque) != -1);

  struct sockaddr_in sin;
  socklen_t len = sizeof (sin);

  int ret = ::getsockname (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  return ntohs (sin.sin_port);
}

CPI::OS::Socket
CPI::OS::ServerSocket::accept ()
  throw (std::string)
{
  cpiAssert (o2fd (m_osOpaque) != -1);

  int newfd = ::accept (o2fd (m_osOpaque), 0, 0);

  if (newfd == -1) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  CPI::OS::uint64_t * fd2o = reinterpret_cast<CPI::OS::uint64_t *> (&newfd);
  return CPI::OS::Socket (fd2o);
}

bool
CPI::OS::ServerSocket::wait (unsigned long msecs)
  throw (std::string)
{
  cpiAssert (o2fd (m_osOpaque) != -1);

  struct timeval timeout;
  fd_set readfds;

  int fd = o2fd (m_osOpaque);

  /*
   * We want to wake up if the socket is closed in another thread.
   * Unfortunately, Linux is no help here: when the socket is closed
   * in one thread, it remains stubbornly stuck in select() until
   * the timeout expires. So, to cover this possibility, we must
   * poll, never waiting for too long. This polling does not seem
   * necessary on SunOs.
   */

 again:
  FD_ZERO (&readfds);
  FD_SET (fd, &readfds);

  if (msecs != static_cast<unsigned long> (-1)) {
    timeout.tv_sec = (msecs >= 1000) ? 1 : 0;
    timeout.tv_usec = (msecs % 1000) * 1000;
  }
  else {
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
  }

  int res = ::select (fd+1, &readfds, 0, 0, &timeout);

  if (res < 0) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  if (res == 0) {
    /*
     * Timeout expired.
     */

    if (msecs == static_cast<unsigned long> (-1)) {
      goto again;
    }
    else if (msecs > 1999) {
      msecs = 1000 * ((msecs/1000) - 1);
      goto again;
    }
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
  cpiAssert (o2fd (m_osOpaque) != -1);

  if (::close (o2fd (m_osOpaque))) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  o2fd (m_osOpaque) = -1;
}
