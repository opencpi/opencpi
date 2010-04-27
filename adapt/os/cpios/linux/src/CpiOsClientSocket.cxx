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
#include <CpiOsClientSocket.h>
#include <CpiOsDataTypes.h>
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

CPI::OS::Socket
CPI::OS::ClientSocket::connect (const std::string & remoteHost,
				unsigned int remotePort)
  throw (std::string)
{
  struct sockaddr_in sin;
  std::memset (&sin, 0, sizeof (struct sockaddr_in));

  sin.sin_family = AF_INET;
  sin.sin_port = htons (remotePort);

  if (!::inet_aton (remoteHost.c_str(), &sin.sin_addr)) {
    CPI::OS::Posix::netDbLock ();

    struct hostent * hent = ::gethostbyname (remoteHost.c_str());

    if (hent) {
      memcpy (&sin.sin_addr.s_addr, hent->h_addr, hent->h_length);
    }
    else {
      int err = h_errno;
      CPI::OS::Posix::netDbUnlock ();

      switch (err) {
      case HOST_NOT_FOUND:
	throw std::string ("unknown host");
	break;

      case NO_ADDRESS:
	throw std::string ("host has no address");
	break;

      default:
	throw std::string ("gethostbyname() failed");
      }

      // should not be here
      cpiAssert (0);
    }

    CPI::OS::Posix::netDbUnlock ();
  }

  int fileno = ::socket (PF_INET, SOCK_STREAM, 0);

  if (fileno < 0) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  if (::connect (fileno, (struct sockaddr *) &sin, sizeof (sin)) != 0) {
    throw CPI::OS::Posix::getErrorMessage (errno);
  }

  CPI::OS::uint64_t * fd2o = reinterpret_cast<CPI::OS::uint64_t *> (&fileno);
  return CPI::OS::Socket (fd2o);
}
