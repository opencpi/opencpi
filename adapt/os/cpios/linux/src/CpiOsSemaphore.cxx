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
#include <CpiOsSemaphore.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <cassert>
#include <pthread.h>
#include "CpiOsPosixError.h"

namespace {
  struct SemaphoreData {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    volatile unsigned int value;
  };
}

inline
SemaphoreData &
o2sd (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<SemaphoreData *> (ptr);
}

CPI::OS::Semaphore::Semaphore (unsigned int initial)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (SemaphoreData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (SemaphoreData));

  SemaphoreData & sd = o2sd (m_osOpaque);
  int res;

  if ((res = pthread_mutex_init (&sd.mutex, 0))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_cond_init (&sd.cond, 0))) {
    pthread_mutex_destroy (&sd.mutex);
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  sd.value = initial;
}

CPI::OS::Semaphore::~Semaphore ()
  throw ()
{
  SemaphoreData & sd = o2sd (m_osOpaque);
  pthread_cond_destroy (&sd.cond);
  pthread_mutex_destroy (&sd.mutex);
}

void
CPI::OS::Semaphore::post ()
  throw (std::string)
{
  SemaphoreData & sd = o2sd (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&sd.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  sd.value++;

  if ((res = pthread_cond_signal (&sd.cond))) {
    pthread_mutex_unlock (&sd.mutex);
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  if ((res = pthread_mutex_unlock (&sd.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

void
CPI::OS::Semaphore::wait ()
  throw (std::string)
{
  SemaphoreData & sd = o2sd (m_osOpaque);
  int res;

  if ((res = pthread_mutex_lock (&sd.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  while (!sd.value) {
    if ((res = pthread_cond_wait (&sd.cond, &sd.mutex))) {
      pthread_mutex_unlock (&sd.mutex);
      throw CPI::OS::Posix::getErrorMessage (res);
    }
  }

  if (--sd.value) {
    if ((res = pthread_cond_signal (&sd.cond))) {
      pthread_mutex_unlock (&sd.mutex);
      throw CPI::OS::Posix::getErrorMessage (res);
    }
  }

  if ((res = pthread_mutex_unlock (&sd.mutex))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

