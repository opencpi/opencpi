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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <cstdlib>
#include <string>
#include <cstring>

#include "OcpiOsAssert.h"
#include "OcpiOsSocket.h"
#include "OcpiOsServerSocket.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiOsDataTypes.h"
#include "OcpiOsPosixError.h"

static inline int &o2fd(uint64_t *o) { return *(int*)o; }
//static inline const int &o2fd(const uint64_t *o) { return *(int*)o; }

namespace OCPI {
  namespace OS {
    const int DEFAULT_LISTEN_BACKLOG = 10;

int ServerSocket::fd() throw() {
  return o2fd(m_osOpaque);
}

void ServerSocket::getAddr(Ether::Address &addr) {
  int fd_tmp = o2fd(m_osOpaque);
  ocpiAssert (fd_tmp >= 0);

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(fd_tmp, (struct sockaddr *)&sin, &len))
    throw Posix::getErrorMessage (errno, "getsockname");
  ocpiAssert(sin.sin_family == AF_INET);
  addr.set(ntohs(sin.sin_port), sin.sin_addr.s_addr);
}


ServerSocket::
ServerSocket () throw ()
  : m_timeoutms(0) {
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = -1;
}

ServerSocket::
ServerSocket (uint16_t portNo, bool reuse ) throw (std::string)
  : m_timeoutms(0) {
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = -1;
  bind (portNo, reuse);
}

ServerSocket::~ServerSocket ()
  throw ()
{
  if (o2fd (m_osOpaque) >= 0)
    close();
  ocpiAssert (o2fd (m_osOpaque) == -1);
}

void ServerSocket::
bind(uint16_t portNo, bool reuse, bool udp, bool loopback) throw (std::string) {
  ocpiAssert (o2fd (m_osOpaque) == -1);

  union {
    struct sockaddr_in in;
    struct sockaddr sa;
  } sin;
  memset (&sin, 0, sizeof (sin));

  sin.in.sin_family = AF_INET;
#ifdef OCPI_OS_macos
  sin.in.sin_len = sizeof(sin);
#endif
  sin.in.sin_port = htons(portNo);
  sin.in.sin_addr.s_addr = htonl(loopback ? INADDR_LOOPBACK : INADDR_ANY); 

  int fileno = ::socket (PF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, udp ? IPPROTO_UDP : 0);
  if (fileno < 0)
    throw Posix::getErrorMessage (errno, "bind/socket");
  int reuseopt = reuse ? 1 : 0;
  socklen_t len = sizeof(sin);
  if (::setsockopt(fileno, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseopt, sizeof (int)) != 0 ||
      ::bind (fileno, &sin.sa, sizeof(sin)) != 0 ||
      ::getsockname(fileno, &sin.sa, &len) != 0 ||
      (!udp && ::listen (fileno, DEFAULT_LISTEN_BACKLOG) != 0)) {
    ::close (fileno);
    throw Posix::getErrorMessage(errno, "bind/setsockopt/listen");
  }
  o2fd (m_osOpaque) = fileno;
  ocpiDebug("Server Socket %p bound to \"%s\" port %u", this, inet_ntoa(sin.in.sin_addr),
	    ntohs(sin.in.sin_port));
  
}

uint16_t ServerSocket::
getPortNo() throw (std::string) {
  ocpiAssert (o2fd (m_osOpaque) != -1);

  struct sockaddr_in sin;
  socklen_t len = sizeof (sin);

  int ret = ::getsockname (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw Posix::getErrorMessage (errno);
  }

  return ntohs (sin.sin_port);
}

void ServerSocket::
accept (Socket &sock) throw (std::string) {
  ocpiAssert(o2fd(m_osOpaque) != -1);

  ocpiDebug("Socket accepting");
  int newfd = ::accept(o2fd(m_osOpaque), 0, 0);
  if (newfd == -1)
    throw Posix::getErrorMessage (errno, "server/accept");
  uint64_t opaque;
  o2fd(&opaque) = newfd;
  sock.setOpaque(&opaque);
}

bool
ServerSocket::wait (unsigned long msecs)
  throw (std::string)
{
  ocpiAssert (o2fd (m_osOpaque) != -1);

  struct timeval timeout;
  fd_set readfds;

  int pfd = o2fd (m_osOpaque);

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
  FD_SET (pfd, &readfds);

  if (msecs != static_cast<unsigned long> (-1)) {
    timeout.tv_sec = (msecs >= 1000) ? 1 : 0;
    timeout.tv_usec = (msecs % 1000) * 1000;
  }
  else {
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
  }

  ocpiDebug("Server Socket %p waiting", this);
  int res = ::select (pfd+1, &readfds, 0, 0, &timeout);
  ocpiDebug("Server Socket %p waiting, res: %d errno: %d", this, res, errno);

  if (res < 0) {
    throw Posix::getErrorMessage (errno);
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
ServerSocket::close ()
  throw (std::string)
{
  ocpiAssert (o2fd (m_osOpaque) != -1);

  if (::close (o2fd (m_osOpaque))) {
    throw Posix::getErrorMessage (errno);
  }

  o2fd (m_osOpaque) = -1;
}

// NOTE THIS CODE IS REPLICATED IN THE SOCKET CODE FOR DATAGRAMS
// FIXME SHARE CODE AT THE RIGHT LEVEL
size_t ServerSocket::
sendmsg (const void * iovect, unsigned int flags  ) throw (std::string) {
  const struct msghdr * iov = static_cast<const struct msghdr *>(iovect);
  ssize_t ret = ::sendmsg (o2fd (m_osOpaque), iov, flags);
  if (ret == -1)
    throw Posix::getErrorMessage (errno);
  return static_cast<size_t>(ret);
}

size_t ServerSocket::
sendto (const char * data, size_t amount, int flags,  char * src_addr, size_t addrlen)
  throw (std::string) {
  struct sockaddr * si_other = reinterpret_cast< struct sockaddr *>(src_addr);
  size_t ret = ::sendto (o2fd (m_osOpaque), data, amount, flags, si_other, (socklen_t)addrlen );
  if (ret == static_cast<size_t> (-1))
    throw Posix::getErrorMessage(errno);
  return static_cast<size_t>(ret);
}

size_t ServerSocket::
recvfrom(char  *buf, size_t amount, int flags,
	 char * src_addr, size_t * addrlen, unsigned timeoutms) throw (std::string) {
  if (timeoutms != m_timeoutms) {
    struct timeval tv;
    tv.tv_sec = timeoutms/1000;
    tv.tv_usec = (timeoutms % 1000) * 1000;
    ocpiDebug("[ServerSocket::recvfrom] Setting socket timeout to %u ms", timeoutms);
    if (setsockopt(o2fd (m_osOpaque), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
      throw Posix::getErrorMessage (errno);
    m_timeoutms = timeoutms;
  }
  struct sockaddr * si_other = reinterpret_cast< struct sockaddr *>(src_addr);
  ssize_t ret;
  ret= ::recvfrom (o2fd (m_osOpaque), buf, amount, flags, si_other, (socklen_t*)addrlen);
  if (ret == -1) {
    if (errno != EAGAIN && errno != EINTR)
      throw Posix::getErrorMessage(errno);
    return 0;
  }
  return static_cast<size_t> (ret);
}





}
}
