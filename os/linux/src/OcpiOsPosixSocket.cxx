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

#include <string>
#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "OcpiOsPosixError.h"
#include "OcpiOsPosixSocket.h"

namespace {
  pthread_mutex_t netDbMutex = PTHREAD_MUTEX_INITIALIZER;
};

void
OCPI::OS::Posix::netDbLock ()
  throw (std::string)
{
  pthread_mutex_lock (&netDbMutex);
}

void
OCPI::OS::Posix::netDbUnlock ()
  throw (std::string)
{
  pthread_mutex_unlock (&netDbMutex);
}

std::string
OCPI::OS::Posix::getHostname ()
  throw (std::string)
{
  char buffer[1024];
  if (gethostname (buffer, 1024) != 0) {
    throw getErrorMessage (errno);
  }
  return buffer;
}

std::string
OCPI::OS::Posix::getFQDN ()
  throw (std::string)
{
  std::string localName = getHostname ();

  netDbLock ();
  struct hostent * hent = ::gethostbyname (localName.c_str ());

  if (!hent || !hent->h_name) {
    netDbUnlock ();
    throw std::string ("gethostbyname() failed1");
  }

  std::string fqdn = hent->h_name;
  netDbUnlock ();
  return fqdn;
}

std::string
OCPI::OS::Posix::getIPAddress ()
  throw (std::string)
{
  std::string localName = getHostname ();

  netDbLock ();
  struct hostent * hent = ::gethostbyname (localName.c_str ());

  if (!hent || !hent->h_name || !*hent->h_addr_list) {
    netDbUnlock ();
    throw std::string ("gethostbyname() failed2");
  }

  struct in_addr in;
  std::memcpy (&in.s_addr, *hent->h_addr_list, sizeof (in.s_addr));
  std::string address = inet_ntoa (in);
  netDbUnlock ();
  return address;
}

bool
OCPI::OS::Posix::isLocalhost (const std::string & name)
  throw (std::string)
{
  /*
   * Check whether the name is "localhost"
   */

  if (name.compare ("localhost") == 0) {
    return true;
  }

  /*
   * Check whether the name is my host name
   */

  std::string localName = getHostname ();

  if (name.compare (localName) == 0) {
    return true;
  }

  /*
   * Check whether the name matches any of my aliases
   */

  netDbLock ();
  struct hostent * hent = ::gethostbyname (localName.c_str ());

  if (!hent || !hent->h_name) {
    netDbUnlock ();
    throw std::string ("gethostbyname() failed3");
  }

  bool res = (name.compare (hent->h_name) == 0);

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

  netDbUnlock ();
  return res;
}

