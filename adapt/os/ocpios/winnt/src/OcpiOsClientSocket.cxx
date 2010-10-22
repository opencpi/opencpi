
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
#include <OcpiOsClientSocket.h>
#include <OcpiOsDataTypes.h>
#include <string>
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
