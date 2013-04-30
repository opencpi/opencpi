
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
#include <OcpiOsSocket.h>
#include <OcpiOsServerSocket.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "OcpiOsPosixError.h"

inline
int &
o2fd (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<int *> (ptr);
}

namespace {
  enum {
    DEFAULT_LISTEN_BACKLOG = 10
  };
}

int OCPI::OS::ServerSocket::fd() throw() {
  return o2fd(m_osOpaque);
}
OCPI::OS::ServerSocket::ServerSocket ()
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = -1;
}

OCPI::OS::ServerSocket::ServerSocket (unsigned int portNo, bool reuse )
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = -1;
  bind (portNo, reuse);
}

OCPI::OS::ServerSocket::~ServerSocket ()
  throw ()
{
  if (o2fd (m_osOpaque) >= 0)
    close();
  ocpiAssert (o2fd (m_osOpaque) == -1);
}

OCPI::OS::Socket
OCPI::OS::ServerSocket::bind (unsigned int portNo, bool reuse, bool udp )
  throw (std::string)
{
  ocpiAssert (o2fd (m_osOpaque) == -1);

  struct sockaddr_in sin;
  memset (&sin, 0, sizeof (struct sockaddr_in));

  sin.sin_family = AF_INET;
#ifdef OCPI_OS_macos
  sin.sin_len = sizeof(sin);
#endif
  sin.sin_port = htons (portNo);
  sin.sin_addr.s_addr = INADDR_ANY;

  int fileno;
  if ( ! udp ) {
    fileno = ::socket (PF_INET, SOCK_STREAM, 0);
  }
  else {
    fileno = ::socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  }
  
  if (fileno < 0) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  int reuseopt = reuse ? 1 : 0;

  if (::setsockopt (fileno, SOL_SOCKET, SO_REUSEADDR,
                    (char *) &reuseopt, sizeof (int)) != 0) {
    int err = errno;
    ::close (fileno);
    throw OCPI::OS::Posix::getErrorMessage (err);
  }
  
  if (::bind (fileno, (struct sockaddr *) &sin, sizeof (sin)) != 0) {
    int err = errno;
    ::close (fileno);
    throw OCPI::OS::Posix::getErrorMessage (err);
  }

  if ( ! udp ) {
    if (::listen (fileno, DEFAULT_LISTEN_BACKLOG) != 0) {
      int err = errno;
      ::close (fileno);
      throw OCPI::OS::Posix::getErrorMessage (err);
    }
  }

  o2fd (m_osOpaque) = fileno;

  OCPI::OS::uint64_t * fd2o = reinterpret_cast<OCPI::OS::uint64_t *> (&fileno);
  return OCPI::OS::Socket( fd2o );
}

unsigned int
OCPI::OS::ServerSocket::getPortNo ()
  throw (std::string)
{
  ocpiAssert (o2fd (m_osOpaque) != -1);

  struct sockaddr_in sin;
  socklen_t len = sizeof (sin);

  int ret = ::getsockname (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  return ntohs (sin.sin_port);
}

OCPI::OS::Socket
OCPI::OS::ServerSocket::accept ()
  throw (std::string)
{
  ocpiAssert (o2fd (m_osOpaque) != -1);

  
  int newfd = ::accept (o2fd (m_osOpaque), 0, 0);
  if (newfd == -1) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }
  OCPI::OS::uint64_t * fd2o = reinterpret_cast<OCPI::OS::uint64_t *> (&newfd);
  return OCPI::OS::Socket (fd2o);
}

bool
OCPI::OS::ServerSocket::wait (unsigned long msecs)
  throw (std::string)
{
  ocpiAssert (o2fd (m_osOpaque) != -1);

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
    throw OCPI::OS::Posix::getErrorMessage (errno);
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
OCPI::OS::ServerSocket::close ()
  throw (std::string)
{
  ocpiAssert (o2fd (m_osOpaque) != -1);

  if (::close (o2fd (m_osOpaque))) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  o2fd (m_osOpaque) = -1;
}
