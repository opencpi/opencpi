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

#include <cassert>
#include <string>
#include <pthread.h>
#include <dlfcn.h>
#include "ocpi-config.h"
#include "OcpiOsAssert.h"
#include "OcpiOsLoadableModule.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiOsDataTypes.h"

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
  o2vp (m_osOpaque) = 0;
}

OCPI::OS::LoadableModule::LoadableModule (const std::string & fileName, bool global)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (void *)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (void *));
#if !defined(NDEBUG)
  o2vp (m_osOpaque) = 0;
#endif
  open (fileName, global);
}

OCPI::OS::LoadableModule::~LoadableModule ()
  throw ()
{
  if (o2vp (m_osOpaque))
    close();
#if !defined(NDEBUG)
  ocpiAssert (!o2vp (m_osOpaque));
#endif
}

void
OCPI::OS::LoadableModule::open (const std::string & fileName, bool global)
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (!o2vp (m_osOpaque));
#endif

  std::string error;
  void *handle = load(fileName.c_str(), global, error);

  if (!(o2vp (m_osOpaque) = handle))
    throw error;
}

// Static: used with the class as well as usable standalone
// Return handle.  Or null and set error
void *
OCPI::OS::LoadableModule::load(const char *fileName, bool global, std::string &error) throw () {
  error = "";
  pthread_mutex_lock (&g_slMutex);
  void *handle = dlopen (fileName, RTLD_NOW | (global ? RTLD_GLOBAL : RTLD_LOCAL));
  if (!handle) {
    error = "error loading \"";
    error += fileName;
    error += "\": ";
    error += dlerror();
  }
  pthread_mutex_unlock (&g_slMutex);
  return handle;
}

const char *
OCPI::OS::LoadableModule::suffix() throw() {
#ifdef OCPI_OS_macos
  return "dylib";
#else
  return "so";
#endif
}


void *
OCPI::OS::LoadableModule::getSymbol (const char *functionName)
  throw (std::string)
{
#if !defined(NDEBUG)
  ocpiAssert (o2vp (m_osOpaque));
#endif

  pthread_mutex_lock (&g_slMutex);

  void * addr;
  if (!(addr = dlsym(o2vp (m_osOpaque), functionName))) {
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
