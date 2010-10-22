
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


/*
 * Revision History:
 *
 *     04/09/2009 - Frank Pilhofer
 *                  Bugfix: Also handle SIGTERM in setCtrlCHandler, since
 *                  that is the signal that kill(1) sends by default.
 */

#include <OcpiOsMisc.h>
#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include "OcpiOsPosixSocket.h"
#include "OcpiOsPosixError.h"

void
OCPI::OS::sleep (unsigned long msecs)
  throw (std::string)
{
  if (msecs) {
    struct timeval timeout;
    timeout.tv_sec = msecs / 1000;
    timeout.tv_usec = (msecs % 1000) * 1000;

    int res = ::select (0, 0, 0, 0, &timeout);

    // NOTE: This needs to be made smarter since select can get kicked off from a signal !!

    if (res < 0) {
      throw OCPI::OS::Posix::getErrorMessage (errno);
    }
  }
  else {
    sched_yield ();
  }
}

unsigned long
OCPI::OS::getProcessId ()
  throw ()
{
  return static_cast<unsigned long> (getpid());
}

unsigned long
OCPI::OS::getThreadId ()
  throw ()
{
#ifdef __APPLE__
  return (unsigned long) pthread_self();
#else
  return static_cast<unsigned long> (pthread_self());
#endif
}

std::string
OCPI::OS::getHostname ()
  throw (std::string)
{
  return Posix::getHostname ();
}

std::string
OCPI::OS::getFQDN ()
  throw (std::string)
{
  return Posix::getFQDN ();
}

std::string
OCPI::OS::getIPAddress ()
  throw (std::string)
{
  return Posix::getIPAddress ();
}

bool
OCPI::OS::isLocalhost (const std::string & name)
  throw (std::string)
{
  return Posix::isLocalhost (name);
}

namespace {

  /*
   * Posix wants a signal handler that takes an integer parameter.
   */

  void (*g_userHandler) (void) = 0;

  void
  PosixCtrlCHandler (int)
    throw ()
  {
    g_userHandler ();
  }

}

void
OCPI::OS::setCtrlCHandler (void (*handler) (void))
  throw ()
{
  if (!g_userHandler) {
    g_userHandler = handler;
    struct sigaction act;
    act.sa_handler = PosixCtrlCHandler;
    act.sa_flags = 0;
    sigemptyset (&act.sa_mask);
    sigaction (SIGINT, &act, 0);
    sigaction (SIGTERM, &act, 0);
  }
  else {
    g_userHandler = handler;
  }
}

