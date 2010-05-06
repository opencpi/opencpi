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
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
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
#include "CpiOsPosixError.h"
#include "CpiOsPosixSocket.h"

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

CPI::OS::Socket::Socket ()
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = -1;
}

CPI::OS::Socket::Socket (const CPI::OS::uint64_t * opaque)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (int)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (int));
  o2fd (m_osOpaque) = o2fd (opaque);
}

CPI::OS::Socket::Socket (const Socket & other)
  throw ()
{
  o2fd (m_osOpaque) = o2fd (other.m_osOpaque);
}

CPI::OS::Socket &
CPI::OS::Socket::operator= (const Socket & other)
  throw ()
{
  o2fd (m_osOpaque) = o2fd (other.m_osOpaque);
  return *this;
}

CPI::OS::Socket::~Socket ()
  throw ()
{
}

unsigned long long
CPI::OS::Socket::recv (char * buffer, unsigned long long amount)
  throw (std::string)
{
  unsigned long count = static_cast<unsigned long> (amount);
  cpiAssert (static_cast<unsigned long long> (count) == amount);

  size_t ret = ::recv (o2fd (m_osOpaque), buffer, count, 0);

  if (ret == static_cast<size_t> (-1)) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  return static_cast<unsigned long long> (ret);
}

unsigned long long
CPI::OS::Socket::send (const char * data, unsigned long long amount)
  throw (std::string)
{
  unsigned long count = static_cast<unsigned long> (amount);
  cpiAssert (static_cast<unsigned long long> (count) == amount);

  size_t ret = ::send (o2fd (m_osOpaque), data, count, 0);

  if (ret == static_cast<size_t> (-1)) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  return static_cast<unsigned long long> (ret);
}

unsigned int
CPI::OS::Socket::getPortNo ()
  throw (std::string)
{
  struct sockaddr_in sin;
  socklen_t len = sizeof (sin);

  int ret = ::getsockname (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  return ntohs (sin.sin_port);
}

void
CPI::OS::Socket::getPeerName (std::string & peerHost,
                              unsigned int & peerPort)
  throw (std::string)
{
  struct sockaddr_in sin;
  socklen_t len = sizeof (sin);

  int ret = ::getpeername (o2fd (m_osOpaque), (struct sockaddr *) &sin, &len);

  if (ret != 0 || len != sizeof (sin)) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  CPI::OS::Posix::netDbLock ();

  struct hostent * hent =
    ::gethostbyaddr ((const char *) &sin.sin_addr.s_addr, 4,
                     sin.sin_family);

  if (hent && hent->h_name) {
    peerHost = hent->h_name;
  }
  else {
    peerHost = inet_ntoa (sin.sin_addr);
  }

  CPI::OS::Posix::netDbUnlock ();
  peerPort = ntohs (sin.sin_port);
}

void
CPI::OS::Socket::linger (bool opt)
  throw (std::string)
{
  struct linger lopt;
  lopt.l_onoff = opt ? 1 : 0;
  lopt.l_linger = 0;

  if (::setsockopt (o2fd (m_osOpaque), SOL_SOCKET, SO_LINGER,
                    (char *) &lopt, sizeof (struct linger)) != 0) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }   
}

void
CPI::OS::Socket::shutdown (bool sendingEnd)
  throw (std::string)
{
  if (::shutdown (o2fd (m_osOpaque), sendingEnd ? SHUT_WR : SHUT_RD) != 0) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }
}

void
CPI::OS::Socket::close ()
  throw (std::string)
{
  if (::close (o2fd (m_osOpaque))) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }
}

CPI::OS::Socket
CPI::OS::Socket::dup ()
  throw (std::string)
{
  int newfd = ::dup (o2fd (m_osOpaque));
  CPI::OS::uint64_t * fd2o = reinterpret_cast<CPI::OS::uint64_t *> (&newfd);
  return CPI::OS::Socket (fd2o);
}
