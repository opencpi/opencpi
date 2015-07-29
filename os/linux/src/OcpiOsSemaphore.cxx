
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
#include <OcpiOsSemaphore.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <cassert>
#include <pthread.h>
#include "OcpiOsPosixError.h"

namespace {
  struct SemaphoreData {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    volatile unsigned int value;
  };
}

inline
SemaphoreData &
o2sd (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<SemaphoreData *> (ptr);
}

OCPI::OS::Semaphore::Semaphore (unsigned int initial)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (SemaphoreData)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (SemaphoreData));

  SemaphoreData & sd = o2sd (m_osOpaque);
  int res;

  if ((res = pthread_mutex_init (&sd.mutex, 0))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_cond_init (&sd.cond, 0))) {
    pthread_mutex_destroy (&sd.mutex);
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  sd.value = initial;
}

OCPI::OS::Semaphore::~Semaphore ()
  throw ()
{
  SemaphoreData & sd = o2sd (m_osOpaque);
  pthread_cond_destroy (&sd.cond);
  pthread_mutex_destroy (&sd.mutex);
}

void
OCPI::OS::Semaphore::post ()
  throw (std::string)
{
  SemaphoreData & sd = o2sd (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&sd.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  sd.value++;

  if ((res = pthread_cond_signal (&sd.cond))) {
    pthread_mutex_unlock (&sd.mutex);
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_mutex_unlock (&sd.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
}

void
OCPI::OS::Semaphore::wait ()
  throw (std::string)
{
  SemaphoreData & sd = o2sd (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&sd.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  while (!sd.value) {
    if ((res = pthread_cond_wait (&sd.cond, &sd.mutex))) {
      pthread_mutex_unlock (&sd.mutex);
      throw OCPI::OS::Posix::getErrorMessage (res);
    }
  }

  if (--sd.value) {
    if ((res = pthread_cond_signal (&sd.cond))) {
      pthread_mutex_unlock (&sd.mutex);
      throw OCPI::OS::Posix::getErrorMessage (res);
    }
  }

  if ((res = pthread_mutex_unlock (&sd.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
}

