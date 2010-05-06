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
#include <CpiOsEvent.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "CpiOsPosixError.h"
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
o2ed (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<EventData *> (ptr);
}

CPI::OS::Event::Event (bool initial)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (EventData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (EventData));

  EventData & ed = o2ed (m_osOpaque);
  int res;

  if ((res = pthread_mutex_init (&ed.mutex, 0))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_cond_init (&ed.cond, 0))) {
    pthread_mutex_destroy (&ed.mutex);
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  ed.value = initial ? 1 : 0;
}

CPI::OS::Event::~Event ()
  throw ()
{
  EventData & ed = o2ed (m_osOpaque);
  pthread_cond_destroy (&ed.cond);
  pthread_mutex_destroy (&ed.mutex);
}

void
CPI::OS::Event::set ()
  throw (std::string)
{
  EventData & ed = o2ed (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&ed.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  ed.value = 1;

  if ((res = pthread_cond_signal (&ed.cond))) {
    pthread_mutex_unlock (&ed.mutex);
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_mutex_unlock (&ed.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

void
CPI::OS::Event::wait ()
  throw (std::string)
{
  EventData & ed = o2ed (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&ed.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  while (!ed.value) {
    if ((res = pthread_cond_wait (&ed.cond, &ed.mutex))) {
      pthread_mutex_unlock (&ed.mutex);
      throw CPI::OS::Posix::getErrorMessage (res);
    }
  }

  ed.value = 0;

  if ((res = pthread_mutex_unlock (&ed.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

bool
CPI::OS::Event::wait (unsigned int timeout)
  throw (std::string)
{
  struct timespec absTimeout;
  EventData & ed = o2ed (m_osOpaque);
  int res;

#ifndef __APPLE__
  if (clock_gettime (CLOCK_REALTIME, &absTimeout) != 0) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
#else
  struct timeval tv;
  if ((res = gettimeofday(&tv, NULL)) != 0) {
    throw CPI::OS::Posix::getErrorMessage (res);
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
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  while (!ed.value) {
    if ((res = pthread_cond_timedwait (&ed.cond, &ed.mutex, &absTimeout))) {
      pthread_mutex_unlock (&ed.mutex);

      if (res == ETIMEDOUT) {
        return false;
      }

      throw CPI::OS::Posix::getErrorMessage (res);
    }
  }

  ed.value = 0;

  if ((res = pthread_mutex_unlock (&ed.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  return true;
}

