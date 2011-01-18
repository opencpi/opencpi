
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


#include <OcpiOsSpinLock.h>
#include "OcpiOsPosixError.h"
#include <cerrno>
#include <cstdio>
#include <string>
#include <cstring>
#include <pthread.h>

namespace {
  pthread_mutex_t gemMutex = PTHREAD_MUTEX_INITIALIZER;
};

std::string
OCPI::OS::Posix::getErrorMessage (int errorCode)
  throw ()
{
  /*
   * std::strerror is not reentrant
   */

  pthread_mutex_lock (&gemMutex);
  const char * message = std::strerror (errorCode);
  std::string res;

  if (!message) {
    char tmp[32];
    std::sprintf (tmp, "error %d", errorCode);
    res = tmp;
  }
  else {
    res = message;
  }

  pthread_mutex_unlock (&gemMutex);
  return res;
}
