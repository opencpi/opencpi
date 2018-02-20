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

/*
 * Revision History:
 *
 *     06/04/2009 - Frank Pilhofer
 *                  Signals should be delivered only to the main thread, so
 *                  block all the signals that we care about in any threads
 *                  that we create.
 */

#include <OcpiOsAssert.h>
#include <OcpiOsThreadManager.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <string>
#include <signal.h>
#include <pthread.h>
#include "OcpiOsPosixError.h"

namespace {
  struct ThreadData {
    bool running;
    pthread_t th;
  };
}

inline
ThreadData &
o2td (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<ThreadData *> (ptr);
}

/*
 * Entry point for the new thread. This is necessary because of
 * a difference in signatures: we use a function w/o return, while
 * Posix wants a function returning void*.
 *
 * Have to use heap for the RunThreadParams to avoid race conditions,
 * in case the creating thread immediately calls detach() and the
 * destructor, before the new thread gets a chance to run.
 */

namespace {
  struct RunThreadParams {
    void (*func) (void *);
    void * opaque;
  };

  void *
  RunThread (void * arg)
    throw ()
  {
    RunThreadParams * p = static_cast<RunThreadParams *> (arg);
    void (*func) (void *) = p->func;
    void * opaque = p->opaque;
    delete p;
    func (opaque);
    return 0;
  }
}

OCPI::OS::ThreadManager::ThreadManager ()
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (ThreadData)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (ThreadData));
#if !defined(NDEBUG)
  o2td (m_osOpaque).running = false;
#endif
}

OCPI::OS::ThreadManager::ThreadManager (void (*func) (void *), void * opaque)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (ThreadData)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (ThreadData));
#if !defined(NDEBUG)
  o2td (m_osOpaque).running = false;
#endif
  start (func, opaque);
}

OCPI::OS::ThreadManager::~ThreadManager ()
  throw ()
{
#if !defined(NDEBUG)
  // Note this assert will fail if neither a join nor detach has been invoked on the thread
  // NOTE: comment it out as error paths don't typically do the detach
  // ocpiAssert (!o2td (m_osOpaque).running);
#endif
}

void
OCPI::OS::ThreadManager::start (void (*func) (void *), void * opaque)
  throw (std::string)
{
  ThreadData & td = o2td (m_osOpaque);

#if !defined(NDEBUG)
  ocpiAssert (!td.running);
#endif

  RunThreadParams * p = new RunThreadParams;
  p->func = func;
  p->opaque = opaque;

  /*
   * We want these signals delivered to the main thread only.  Block them
   * in the child thread.  To avoid a race condition, we block the signals
   * here, create the thread (which inherits blocked signals), then restore
   * the signal mask.
   */

  sigset_t blockSigSet, oldSigSet;
  sigemptyset (&blockSigSet);
  sigaddset (&blockSigSet, SIGCHLD);
  sigaddset (&blockSigSet, SIGUSR1);
  sigaddset (&blockSigSet, SIGINT);
  sigaddset (&blockSigSet, SIGTERM);
  pthread_sigmask (SIG_BLOCK, &blockSigSet, &oldSigSet);

  int res;
  if ((res = pthread_create (&td.th, 0, RunThread, p))) {
    pthread_sigmask (SIG_SETMASK, &oldSigSet, 0);
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

  td.running = true;
  pthread_sigmask (SIG_SETMASK, &oldSigSet, 0);
}

void
OCPI::OS::ThreadManager::join ()
  throw (std::string)
{
  ThreadData & td = o2td (m_osOpaque);
  int res;

  if (!td.running) // since we don't return anything its ok to join something that never ran
    return;
  void * dummy;
  if ((res = pthread_join (td.th, &dummy))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

#if !defined(NDEBUG)
  td.running = false;
#endif
}

void
OCPI::OS::ThreadManager::detach ()
  throw (std::string)
{
  ThreadData & td = o2td (m_osOpaque);
  int res;

#if !defined(NDEBUG)
  ocpiAssert (td.running);
#endif

  if ((res = pthread_detach (td.th))) {
    throw OCPI::OS::Posix::getErrorMessage (res);
  }

#if !defined(NDEBUG)
  td.running = false;
#endif
}
