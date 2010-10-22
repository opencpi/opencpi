
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
#include <OcpiOsLoadableModule.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <cassert>
#include <string>
#include <pthread.h>
#include <dlfcn.h>

/*
 * We need a mutex, because dlerror() is not thread-safe.
 */

namespace {
  pthread_mutex_t g_slMutex = PTHREAD_MUTEX_INITIALIZER;
}

inline
void *&
o2vp (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<void **> (ptr);
}

OCPI::OS::LoadableModule::LoadableModule ()
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (void *)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (void *));
#if !defined(NDEBUG)
  o2vp (m_osOpaque) = 0;
#endif
}

OCPI::OS::LoadableModule::LoadableModule (const std::string & fileName)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (void *)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (void *));
#if !defined(NDEBUG)
  o2vp (m_osOpaque) = 0;
#endif
  open (fileName);
}

OCPI::OS::LoadableModule::~LoadableModule ()
  throw ()
{
#if !defined(NDEBUG)
  ocpiAssert (!o2vp (m_osOpaque));
#endif
}

void
OCPI::OS::LoadableModule::open (const std::string & fileName)
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (!o2vp (m_osOpaque));
#endif

  pthread_mutex_lock (&g_slMutex);

  if (!(o2vp (m_osOpaque) = dlopen (fileName.c_str(), RTLD_LAZY | RTLD_LOCAL))) {
    std::string reason = dlerror ();
    pthread_mutex_unlock (&g_slMutex);
    throw reason;
  }

  pthread_mutex_unlock (&g_slMutex);
}

void *
OCPI::OS::LoadableModule::getSymbol (const std::string & functionName)
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (o2vp (m_osOpaque));
#endif

  pthread_mutex_lock (&g_slMutex);

  void * addr;
  if (!(addr = dlsym (o2vp (m_osOpaque), functionName.c_str()))) {
    std::string reason = dlerror ();
    pthread_mutex_unlock (&g_slMutex);
    throw reason;
  }

  pthread_mutex_unlock (&g_slMutex);
  return addr;
}

void
OCPI::OS::LoadableModule::close ()
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (o2vp (m_osOpaque));
#endif

  pthread_mutex_lock (&g_slMutex);

  if (dlclose (o2vp (m_osOpaque))) {
    std::string reason = dlerror ();
    pthread_mutex_unlock (&g_slMutex);
    throw reason;
  }

  pthread_mutex_unlock (&g_slMutex);

#if !defined(NDEBUG)
  o2vp (m_osOpaque) = 0;
#endif
}
