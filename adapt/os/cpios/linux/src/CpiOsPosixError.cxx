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

#include <CpiOsSpinLock.h>
#include "CpiOsPosixError.h"
#include <cerrno>
#include <cstdio>
#include <string>
#include <pthread.h>

namespace {
  pthread_mutex_t gemMutex = PTHREAD_MUTEX_INITIALIZER;
};

std::string
CPI::OS::Posix::getErrorMessage (int errorCode)
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
