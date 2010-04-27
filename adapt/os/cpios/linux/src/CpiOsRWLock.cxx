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
#include <CpiOsRWLock.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <cassert>
#include <pthread.h>
#include <errno.h>
#include "CpiOsPosixError.h"

inline
pthread_rwlock_t *
o2prw (CPI::OS::uint64_t * ptr)
  throw ()
{
  return reinterpret_cast<pthread_rwlock_t *> (ptr);
}

CPI::OS::RWLock::RWLock ()
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (pthread_rwlock_t)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (pthread_rwlock_t));

  int res;
  if ((res = pthread_rwlock_init (o2prw (m_osOpaque), 0))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

CPI::OS::RWLock::~RWLock ()
  throw ()
{
  pthread_rwlock_destroy (o2prw (m_osOpaque));
}

void
CPI::OS::RWLock::rdLock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_rwlock_rdlock (o2prw (m_osOpaque)))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

bool
CPI::OS::RWLock::rdTrylock ()
  throw (std::string)
{
  int res = pthread_rwlock_tryrdlock (o2prw (m_osOpaque));
  if (res != 0 && res != EBUSY) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
  return ((res == 0) ? true : false);
}

void
CPI::OS::RWLock::rdUnlock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_rwlock_unlock (o2prw (m_osOpaque)))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

void
CPI::OS::RWLock::wrLock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_rwlock_wrlock (o2prw (m_osOpaque)))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}

bool
CPI::OS::RWLock::wrTrylock ()
  throw (std::string)
{
  int res = pthread_rwlock_trywrlock (o2prw (m_osOpaque));
  if (res != 0 && res != EBUSY) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
  return ((res == 0) ? true : false);
}

void
CPI::OS::RWLock::wrUnlock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_rwlock_unlock (o2prw (m_osOpaque)))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }
}
