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

/*
 * Revision History:
 *
 *     06/04/2009 - Frank Pilhofer
 *                  Signals should be delivered only to the main thread, so
 *                  block all the signals that we care about in any threads
 *                  that we create.
 */

#include <CpiOsAssert.h>
#include <CpiOsThreadManager.h>
#include <CpiOsSizeCheck.h>
#include <CpiOsDataTypes.h>
#include <string>
#include <signal.h>
#include <pthread.h>
#include "CpiOsPosixError.h"

namespace {
  struct ThreadData {
    bool running;
    pthread_t th;
  };
}

inline
ThreadData &
o2td (CPI::OS::uint64_t * ptr)
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

CPI::OS::ThreadManager::ThreadManager ()
  throw ()
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (ThreadData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (ThreadData));
#if !defined(NDEBUG)
  o2td (m_osOpaque).running = false;
#endif
}

CPI::OS::ThreadManager::ThreadManager (void (*func) (void *), void * opaque)
  throw (std::string)
{
  cpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (ThreadData)> ()));
  cpiAssert (sizeof (m_osOpaque) >= sizeof (ThreadData));
#if !defined(NDEBUG)
  o2td (m_osOpaque).running = false;
#endif
  start (func, opaque);
}

CPI::OS::ThreadManager::~ThreadManager ()
  throw ()
{
#if !defined(NDEBUG)
  // Note this assert will fail if neither a join nor detach has been invoked on the thread
  // NOTE: comment it out as error paths don't typically do the detach
  // cpiAssert (!o2td (m_osOpaque).running);
#endif
}

void
CPI::OS::ThreadManager::start (void (*func) (void *), void * opaque)
  throw (std::string)
{
  ThreadData & td = o2td (m_osOpaque);

#if !defined(NDEBUG)
  cpiAssert (!td.running);
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
    throw CPI::OS::Posix::getErrorMessage (res);
  }

  td.running = true;
  pthread_sigmask (SIG_SETMASK, &oldSigSet, 0);
}

void
CPI::OS::ThreadManager::join ()
  throw (std::string)
{
  ThreadData & td = o2td (m_osOpaque);
  int res;

#if !defined(NDEBUG)
  cpiAssert (td.running);
#endif

  void * dummy;
  if ((res = pthread_join (td.th, &dummy))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

#if !defined(NDEBUG)
  td.running = false;
#endif
}

void
CPI::OS::ThreadManager::detach ()
  throw (std::string)
{
  ThreadData & td = o2td (m_osOpaque);
  int res;

#if !defined(NDEBUG)
  cpiAssert (td.running);
#endif

  if ((res = pthread_detach (td.th))) {
    throw CPI::OS::Posix::getErrorMessage (res);
  }

#if !defined(NDEBUG)
  td.running = false;
#endif
}
