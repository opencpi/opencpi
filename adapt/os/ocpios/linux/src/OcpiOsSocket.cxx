
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
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "OcpiOsPosixError.h"
#include "OcpiOsPosixSocket.h"

static inline
int &
o2fd (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<int *> (ptr);
}

static inline
const int &
o2fd (const OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<const int *> (ptr);
}

int OCPI::OS::Socket::fd() throw() {
  return o2fd(m_osOpaque);
}
OCPI::OS::Socket::Socket ()
  throw ()
{
  m_timeoutms = 0;
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = -1;
}

OCPI::OS::Socket::Socket (const OCPI::OS::uint64_t * opaque)
  throw (std::string)
{
  m_temporary = true;
  m_timeoutms = 0;
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = o2fd (opaque);
}

OCPI::OS::Socket::Socket (const Socket & other)
  throw ()
{
  m_timeoutms = 0;
  m_temporary = false;
  o2fd (m_osOpaque) = o2fd (other.m_osOpaque);
}

OCPI::OS::Socket &
OCPI::OS::Socket::operator= (const Socket & other)
  throw ()
{
  m_timeoutms = 0;
  m_temporary = false;
  o2fd (m_osOpaque) = o2fd (other.m_osOpaque);
  return *this;
}

OCPI::OS::Socket::~Socket ()
  throw ()
{
  if (!m_temporary && o2fd (m_osOpaque) != -1)
    close();
}

unsigned long long
OCPI::OS::Socket::recv (char * buffer, unsigned long long amount, unsigned timeoutms)
  throw (std::string)
{
  if (timeoutms != m_timeoutms) {
    struct timeval tv;
    tv.tv_sec = timeoutms/1000;
    tv.tv_usec = (timeoutms % 1000) * 1000;
    ocpiDebug("Setting socket timeout to %u ms", timeoutms);
    if (setsockopt(o2fd (m_osOpaque), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
      throw OCPI::OS::Posix::getErrorMessage (errno);
    m_timeoutms = timeoutms;
  }
  unsigned long count = static_cast<unsigned long> (amount);
  ocpiAssert (static_cast<unsigned long long> (count) == amount);

  ssize_t ret;
  while ((ret = ::recv (o2fd (m_osOpaque), buffer, count, 0)) == -1 && errno == EINTR)
    ;
  if (ret == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) // timeout errors
      return ULLONG_MAX;
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  //  printf("recv %p %p %d\n", this, buffer, o2fd(m_osOpaque));
  return static_cast<unsigned long long> (ret);
}


unsigned long long 
OCPI::OS::Socket::
recvfrom(char  *buf, unsigned long long amount, int flags,
	 char * src_addr, unsigned long * addrlen, unsigned timeoutms)
  throw (std::string)
{

  if (timeoutms != m_timeoutms) {
    struct timeval tv;
    tv.tv_sec = timeoutms/1000;
    tv.tv_usec = (timeoutms % 1000) * 1000;
    ocpiDebug("Setting socket timeout to %u ms", timeoutms);
    if (setsockopt(o2fd (m_osOpaque), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
      throw OCPI::OS::Posix::getErrorMessage (errno);
    m_timeoutms = timeoutms;
  }


  struct sockaddr * si_other = reinterpret_cast< struct sockaddr *>(src_addr);
  unsigned long count = static_cast<unsigned long> (amount);
  ocpiAssert (static_cast<unsigned long long> (count) == amount);

  ssize_t ret;
  ret= ::recvfrom (o2fd (m_osOpaque), buf, count, flags, si_other, (socklen_t*)addrlen);
  if (ret == -1) {
    if (errno != EAGAIN && errno != EINTR)
      throw OCPI::OS::Posix::getErrorMessage(errno);
    return 0;
  }

  //  printf("recv %p %p %d\n", this, buffer, o2fd(m_osOpaque));
  return static_cast<unsigned long long> (ret);

}


unsigned long long
OCPI::OS::Socket::sendto (const char * data, unsigned long long amount,
			  int flags,  char * src_addr, unsigned long addrlen)			  
  throw (std::string)
{
  struct sockaddr * si_other = reinterpret_cast< struct sockaddr *>(src_addr);
  unsigned long count = static_cast<unsigned long> (amount);
  ocpiAssert (static_cast<unsigned long long> (count) == amount);

  size_t ret = ::sendto (o2fd (m_osOpaque), data, count, flags, si_other, (socklen_t)addrlen );

  if (ret == static_cast<size_t> (-1)) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  return static_cast<unsigned long long> (ret);
}


unsigned long long
OCPI::OS::Socket::send (const char * data, unsigned long long amount)
  throw (std::string)
{
  unsigned long count = static_cast<unsigned long> (amount);
  ocpiAssert (static_cast<unsigned long long> (count) == amount);

  size_t ret = ::send (o2fd (m_osOpaque), data, count, 
#ifdef OCPI_OS_darwin
		       0 // darwin uses setsockopt for this
#else
		       MSG_NOSIGNAL
#endif		       
		       );

  if (ret == static_cast<size_t> (-1)) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  return static_cast<unsigned long long> (ret);
}

unsigned long long
OCPI::OS::Socket::sendmsg (const void * iovect, unsigned int flags  )
  throw (std::string)
{
  const struct msghdr * iov = static_cast<const struct msghdr *>(iovect);

  size_t ret = ::sendmsg (o2fd (m_osOpaque), iov, flags);

  if (ret == static_cast<size_t> (-1)) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  return static_cast<unsigned long long> (ret);
}




unsigned int
OCPI::OS::Socket::getPortNo ()
  throw (std::string)
{
  struct sockaddr_in sin;
  socklen_t len = sizeof (sin);

  int ret = ::getsockname (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  return ntohs (sin.sin_port);
}

void
OCPI::OS::Socket::getPeerName (std::string & peerHost,
                              unsigned int & peerPort)
  throw (std::string)
{
  struct sockaddr_in sin;
  socklen_t len = sizeof (sin);

  int ret = ::getpeername (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  OCPI::OS::Posix::netDbLock ();

  struct hostent * hent =
    ::gethostbyaddr ((const char *) &sin.sin_addr.s_addr, 4,
                     sin.sin_family);

  if (hent && hent->h_name) {
    peerHost = hent->h_name;
  }
  else {
    peerHost = inet_ntoa (sin.sin_addr);
  }

  OCPI::OS::Posix::netDbUnlock ();
  peerPort = ntohs (sin.sin_port);
}

void
OCPI::OS::Socket::linger (bool opt)
  throw (std::string)
{
  struct linger lopt;
  lopt.l_onoff = opt ? 1 : 0;
  lopt.l_linger = 0;

  if (::setsockopt (o2fd (m_osOpaque), SOL_SOCKET, SO_LINGER,
                    (char *) &lopt, sizeof (struct linger)) != 0) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }   
#ifdef OCPI_OS_darwin
  int x = 1;
  if (::setsockopt (o2fd (m_osOpaque), SOL_SOCKET, SO_NOSIGPIPE,
                    (void *) &x, sizeof (x)) != 0) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }   
#endif
}

void
OCPI::OS::Socket::shutdown (bool sendingEnd)
  throw (std::string)
{
  if (::shutdown (o2fd (m_osOpaque), sendingEnd ? SHUT_WR : SHUT_RD) != 0) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }
}

void
OCPI::OS::Socket::close ()
  throw (std::string)
{
  if (::close (o2fd (m_osOpaque))) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }
  o2fd (m_osOpaque) = -1;
}

OCPI::OS::Socket
OCPI::OS::Socket::dup ()
  throw (std::string)
{
  int newfd = ::dup (o2fd (m_osOpaque));
  OCPI::OS::uint64_t * fd2o = reinterpret_cast<OCPI::OS::uint64_t *> (&newfd);
  return OCPI::OS::Socket (fd2o);
}
