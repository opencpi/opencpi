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

#include <OcpiOsAssert.h>
#include <OcpiOsSpinLock.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <pthread.h>
#include <errno.h>
#include "OcpiOsPosixError.h"

#ifdef __APPLE__
#include <libkern/OSAtomic.h>
typedef volatile OSSpinLock pthread_spinlock_t;
#endif
/*
 * Linux does implement spinlocks as defined in "Advanced Realtime Threads"
 * (an optional part of the Single Unix Specification), so let's use them.
 * The only caveat is that the code must be compiled with -D_XOPEN_SOURCE=600.
 */

inline
pthread_spinlock_t *
o2pm (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return reinterpret_cast<pthread_spinlock_t *> (ptr);
}

OCPI::OS::SpinLock::SpinLock ()
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (pthread_spinlock_t)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (pthread_spinlock_t));

#ifndef __APPLE__
  int res;
  if ((res = pthread_spin_init (o2pm (m_osOpaque), PTHREAD_PROCESS_PRIVATE))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
#else
  *o2pm(m_osOpaque) = 0;
#endif
}

OCPI::OS::SpinLock::~SpinLock ()
  throw ()
{
#ifndef __APPLE__
  pthread_spin_destroy (o2pm (m_osOpaque));
#endif
}

void
OCPI::OS::SpinLock::lock ()
  throw (std::string)
{
#ifndef __APPLE__
  int res;
  if ((res = pthread_spin_lock (o2pm (m_osOpaque)))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
#else
  OSSpinLockLock(o2pm(m_osOpaque));
#endif
}

bool
OCPI::OS::SpinLock::trylock ()
  throw (std::string)
{
#ifndef __APPLE__
  int res = pthread_spin_trylock (o2pm (m_osOpaque));
  if (res != 0 && res != EBUSY) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
  return ((res == 0) ? true : false);
#else
  return OSSpinLockTry(o2pm(m_osOpaque));
#endif
}

void
OCPI::OS::SpinLock::unlock ()
  throw (std::string)
{
#ifndef __APPLE__
  int res;
  if ((res = pthread_spin_unlock (o2pm (m_osOpaque)))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
#else
  OSSpinLockUnlock(o2pm(m_osOpaque));
#endif
}
