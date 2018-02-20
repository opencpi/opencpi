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

#include <OcpiOsAssert.h>
#include <OcpiOsSocket.h>
#include <OcpiOsClientSocket.h>
#include <OcpiOsDataTypes.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>
#include "OcpiOsWin32Socket.h"

OCPI::OS::Socket
OCPI::OS::ClientSocket::connect (const std::string & remoteHost,
                                unsigned int remotePort)
  throw (std::string)
{
  OCPI::OS::Win32::winSockInit ();

  struct sockaddr_in sin;
  std::memset (&sin, 0, sizeof (struct sockaddr_in));

  sin.sin_family = AF_INET;
  sin.sin_port = htons (remotePort);

  unsigned long addr = ::inet_addr (remoteHost.c_str());

  if (addr != INADDR_NONE) {
    memcpy (&sin.sin_addr.s_addr, &addr, 4);
  }
  else {
    struct hostent * hent = ::gethostbyname (remoteHost.c_str());

    if (hent) {
      memcpy (&sin.sin_addr.s_addr, hent->h_addr, hent->h_length);
    }
    else {
      throw OCPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
    }
  }

  int fileno = ::socket (PF_INET, SOCK_STREAM, 0);

  if (fileno < 0) {
    throw OCPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
  }

  if (::connect (fileno, (struct sockaddr *) &sin, sizeof (sin)) != 0) {
    throw OCPI::OS::Win32::getWinSockErrorMessage (WSAGetLastError());
  }

  OCPI::OS::uint64_t * fd2o = reinterpret_cast<OCPI::OS::uint64_t *> (&fileno);
  return OCPI::OS::Socket (fd2o);
}
