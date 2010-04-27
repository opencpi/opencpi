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
#include <CpiOsLoadableModule.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
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
o2vp (CPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<void **> (ptr);
}

CPI::OS::LoadableModule::LoadableModule ()
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (void *)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (void *));
#if !defined(NDEBUG)
  o2vp (m_osOpaque) = 0;
#endif
}

CPI::OS::LoadableModule::LoadableModule (const std::string & fileName)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (void *)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (void *));
#if !defined(NDEBUG)
  o2vp (m_osOpaque) = 0;
#endif
  open (fileName);
}

CPI::OS::LoadableModule::~LoadableModule ()
  throw ()
{
#if !defined(NDEBUG)
  cpiAssert (!o2vp (m_osOpaque));
#endif
}

void
CPI::OS::LoadableModule::open (const std::string & fileName)
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (!o2vp (m_osOpaque));
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
CPI::OS::LoadableModule::getSymbol (const std::string & functionName)
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2vp (m_osOpaque));
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
CPI::OS::LoadableModule::close ()
  throw (std::string)
{
#if !defined(NDEBUG)
  cpiAssert (o2vp (m_osOpaque));
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
