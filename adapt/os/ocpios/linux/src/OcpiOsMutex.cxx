
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
#include <OcpiOsMutex.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <pthread.h>
#include <errno.h>
#include "OcpiOsPosixError.h"

inline
pthread_mutex_t *
o2pm (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return reinterpret_cast<pthread_mutex_t *> (ptr);
}

OCPI::OS::Mutex::Mutex (bool recursive)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (pthread_mutex_t)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (pthread_mutex_t));

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
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  pthread_mutexattr_destroy (&ma);
}

OCPI::OS::Mutex::~Mutex ()
  throw ()
{
  pthread_mutex_destroy (o2pm (m_osOpaque));
}

void
OCPI::OS::Mutex::lock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_mutex_lock (o2pm (m_osOpaque)))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
}

bool
OCPI::OS::Mutex::trylock ()
  throw (std::string)
{
  int res = pthread_mutex_trylock (o2pm (m_osOpaque));
  if (res != 0 && res != EBUSY) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
  return ((res == 0) ? true : false);
}

void
OCPI::OS::Mutex::unlock ()
  throw (std::string)
{
  int res;
  if ((res = pthread_mutex_unlock (o2pm (m_osOpaque)))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }
}
