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
#include <CpiOsMutex.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <pthread.h>
#include <errno.h>
#include "CpiOsPosixError.h"

inline
pthread_mutex_t *
o2pm (CPI::OS::uint64_t * ptr)
  throw ()
{
  return reinterpret_cast<pthread_mutex_t *> (ptr);
}

CPI::OS::Mutex::Mutex (bool recursive)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (pthread_mutex_t)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (pthread_mutex_t));

  int pttype;

  if (recursive) {
    pttype = PTHREAD_MUTEX_RECURSIVE;
  }
  else {
#ifdef NDEBUG
    pttype = PTHREAD_MUTEX_NORMAL;
#else
    pttype = PTHREAD_MUTEX_ERRORCHECK;
#endif
  }

  pthread_mutexattr_t ma;
  pthread_mutexattr_init (&ma);
  pthread_mutexattr_settype (&ma, pttype);

  int res;
  if ((res = pthread_mutex_init (o2pm (m_osOpaque), &ma))) {
    pthread_mutexattr_destroy (&ma);
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  pthread_mutexattr_destroy (&ma);
}

CPI::OS::Mutex::~Mutex ()
  throw ()
{
  pthread_mutex_destroy (o2pm (m_osOpaque));
}

void
CPI::OS::Mutex::lock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_mutex_lock (o2pm (m_osOpaque)))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

bool
CPI::OS::Mutex::trylock ()
  throw (std::string)
{
  int res = pthread_mutex_trylock (o2pm (m_osOpaque));
  if (res != 0 && res != EBUSY) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
  return ((res == 0) ? true : false);
}

void
CPI::OS::Mutex::unlock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_mutex_unlock (o2pm (m_osOpaque)))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}
