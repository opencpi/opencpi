
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
#include <OcpiOsEvent.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "OcpiOsPosixError.h"
#ifdef __APPLE__
#include <sys/time.h>
#endif

namespace {
  struct EventData {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    volatile unsigned int value;
  };
}

inline
EventData &
o2ed (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<EventData *> (ptr);
}

OCPI::OS::Event::Event (bool initial)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (EventData)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (EventData));

  EventData & ed = o2ed (m_osOpaque);
  int res;

  if ((res = pthread_mutex_init (&ed.mutex, 0))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_cond_init (&ed.cond, 0))) {
    pthread_mutex_destroy (&ed.mutex);
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  ed.value = initial ? 1 : 0;
}

OCPI::OS::Event::~Event ()
  throw ()
{
  EventData & ed = o2ed (m_osOpaque);
  pthread_cond_destroy (&ed.cond);
  pthread_mutex_destroy (&ed.mutex);
}

void
OCPI::OS::Event::set ()
  throw (std::string)
{
  EventData & ed = o2ed (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&ed.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  ed.value = 1;

  if ((res = pthread_cond_signal (&ed.cond))) {
    pthread_mutex_unlock (&ed.mutex);
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_mutex_unlock (&ed.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
}

void
OCPI::OS::Event::wait ()
  throw (std::string)
{
  EventData & ed = o2ed (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&ed.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  while (!ed.value) {
    if ((res = pthread_cond_wait (&ed.cond, &ed.mutex))) {
      pthread_mutex_unlock (&ed.mutex);
      throw OCPI::OS::Posix::getErrorMessage (res);
    }
  }

  ed.value = 0;

  if ((res = pthread_mutex_unlock (&ed.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
}

bool
OCPI::OS::Event::wait (unsigned int timeout)
  throw (std::string)
{
  struct timespec absTimeout;
  EventData & ed = o2ed (m_osOpaque);
  int res = 0;
#ifndef __APPLE__
  if ((res = clock_gettime (CLOCK_REALTIME, &absTimeout)) != 0) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
#else
  struct timeval tv;
  if ((res = gettimeofday(&tv, NULL)) != 0) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
  TIMEVAL_TO_TIMESPEC(&tv, &absTimeout);
#endif

  if ((absTimeout.tv_nsec += ((timeout % 1000) * 1000000)) >= 1000000000) {
    absTimeout.tv_sec += (timeout / 1000) + 1;
    absTimeout.tv_nsec -= 1000000000;
  }
  else {
    absTimeout.tv_sec += (timeout / 1000);
  }

  if ((res = pthread_mutex_lock (&ed.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  while (!ed.value) {
    if ((res = pthread_cond_timedwait (&ed.cond, &ed.mutex, &absTimeout))) {
      pthread_mutex_unlock (&ed.mutex);

      if (res == ETIMEDOUT) {
        return false;
      }

      throw OCPI::OS::Posix::getErrorMessage (res);
    }
  }

  ed.value = 0;

  if ((res = pthread_mutex_unlock (&ed.mutex))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  return true;
}

