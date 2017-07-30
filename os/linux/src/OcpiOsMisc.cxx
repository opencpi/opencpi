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

/*
 * Revision History:
 *
 *     04/09/2009 - Frank Pilhofer
 *                  Bugfix: Also handle SIGTERM in setCtrlCHandler, since
 *                  that is the signal that kill(1) sends by default.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef OCPI_OS_macos
#include <mach-o/dyld.h>
#endif
#ifdef OCPI_OS_linux
#endif 
#include "OcpiOsMisc.h"
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

    if (res < 0 && errno != EINTR) {
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
void OCPI::OS::setError(std::string &error, const char *fmt, ...)
  throw() {
  int incoming_errno = errno;
  char *err0, *err1;
  va_list ap;
  va_start(ap, fmt);
  int len = vasprintf(&err0, fmt, ap);
  if (len > 0) {
    len = asprintf(&err1, "%s (%s [%d])", err0, strerror(incoming_errno), incoming_errno);
    free(err0);
  }
  if (len > 0) {
    error = err1;
    free(err1);
  } else {
    error = "OCPI::OS::setError unexpected allocation error calling [v]asprintf()";
  }
  va_end(ap);
}

void OCPI::OS::
getExecFile(std::string &file) {
  uint32_t bufsize = 1000;
  char *buf = new char[bufsize];
#ifdef OCPI_OS_macos
  if (_NSGetExecutablePath(buf, &bufsize)) {
    delete [] buf;
    buf = new char[bufsize];
    if (_NSGetExecutablePath(buf, &bufsize))
      throw "Unexpected exec file failure on MacOS";
  } 
#endif
#ifdef OCPI_OS_linux
  ssize_t n;
  while ((n = readlink("/proc/self/exe", buf, bufsize-1)) > 0 && n >= (ssize_t)bufsize - 1) {
    delete [] buf;
    bufsize *= 2;
    buf = new char[bufsize];
  }
  if (n <= 0) {
    delete[] buf;
    throw "Unexpected exec file failure on Linux";
  }
  buf[n] = 0;
#endif 
  file = buf;
  delete [] buf;
}
