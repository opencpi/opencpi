
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


/*
 * Revision History:
 *
 *     06/04/2009 - Frank Pilhofer
 *                  - Also block SIGINT and SIGTERM in the reaper thread, as
 *                    these should only be delivered to the main thread.
 *                  - Unblock all signals after forking; the new process
 *                    inherits the signal mask.
 *
 *     04/09/2009 - Frank Pilhofer
 *                  - Bugfix: Use pthread_sigmask() instead of sigprocmask().
 *                  - Bugfix: Block SIGCHLD and SIGUSR1 in main thread.
 *                  - Bugfix: Handle multiple children with one SIGCHLD.
 */

#include <OcpiOsAssert.h>
#include <OcpiOsProcessManager.h>
#include <OcpiOsThreadManager.h>
#include <OcpiOsSizeCheck.h>
#include <OcpiOsDataTypes.h>
#include <OcpiOsMisc.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstdio>
#include <sys/wait.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#ifdef __APPLE__
#include <sys/time.h>
#endif
#include <errno.h>
#include "OcpiOsPosixError.h"

/*
 * ----------------------------------------------------------------------
 * In order to support both detach() and wait(), we run a single
 * thread to reap our zombie children and to figure out their exit
 * status.
 *
 * We keep a list of all running processes in a global vector, so
 * that the thread can manipulate the data.
 *
 * A mutex and a condition variable is used to signal that a process
 * has terminated, and that wait()ing threads shall reexamine the
 * vector.
 *
 * Pre-2.6 Linux makes life a bit more complicated with a pthreads bug
 * that causes threads to have different process ids.  That bug has the
 * side effect that waitpid() refuses to handle child processes that were
 * started by a different thread -- because the thread that calls
 * waitpid() does not appear to be the child's parent: it's pid does
 * not match the child's getppid() value.
 *
 * Even though Timesys Linux on the VPA-200 reports itself to be based
 * on a Linux 2.6.12.6 kernel, it still suffers from this bug, which has
 * been purged from all other versions of Linux 2.6 that I've seen.
 *
 * The workaround is that child processes must be started from the
 * reaper thread.  So the "started" flag is introduced; if the reaper
 * thread finds an item in the list of children with started==false,
 * it forks and starts the new process.
 *
 * When a ProcessManager wants to start a new process, it appends an
 * item to the list and then wakes up the reaper thread.
 * ----------------------------------------------------------------------
 */

namespace {

  struct ProcessData {
    ProcessData (const std::string & executable,
                 const OCPI::OS::ProcessManager::ParameterList & parameters);
    ~ProcessData ();

    bool started;
    bool detached;
    bool terminated;
    int argc;
    char ** argv;
    pid_t pid;
    int exitCode;
  };

  ProcessData::ProcessData (const std::string & executable,
                            const OCPI::OS::ProcessManager::ParameterList & parameters)
    : started (false),
      detached (false),
      terminated (false),
      argc (0),
      argv (0),
      pid (0)
  {
    unsigned int numParams = parameters.size();
    unsigned int i, l;
    char * p;

    argv = new char* [numParams + 2];

    l = executable.length();
    argv[0] = p = new char [l + 1];
    std::memcpy (p, executable.data(), l);
    p[l] = 0;

    for (i=0; i<numParams; i++) {
      l = parameters[i].length();
      argv[i+1] = p = new char [l + 1];
      std::memcpy (p, parameters[i].data(), l);
      p[l] = 0;
    }

    argv[i+1] = 0;
    argc = i+1;
  }

  ProcessData::~ProcessData ()
  {
    ocpiAssert (terminated);

    for (int ai=0; ai<argc; ai++) {
      delete argv[ai];
    }

    delete [] argv;
  }

  typedef std::vector<ProcessData *> ProcessDataSeq;

  bool g_isReaperRunning = false;
  pid_t g_reaperThreadPid = 0;
  unsigned int g_numRunningChildren = 0;
  pthread_cond_t g_pmDataCond = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t g_pmDataMutex = PTHREAD_MUTEX_INITIALIZER;
  OCPI::OS::ThreadManager g_reaperManager;
  ProcessDataSeq g_myChildren;

}

/*
 * ----------------------------------------------------------------------
 * The Reaper Thread
 *
 * Wait for a child to terminate, then update our list of children
 * with the information that it has exited, and signal all listeners
 * that may be stuck in wait().
 *
 * Exits when there are no more processes left to wait for.
 *
 * As explained above, the reaper thread is also responsible for
 * starting new processes.
 * ----------------------------------------------------------------------
 */

/*
 * Signal handler for SIGCHLD. It does not need to do anything, but
 * without it, SIGCHLD would be ignored, and on some systems, this
 * causes waitpid() to fail.
 */

namespace {
  void
  SigChldHandler (int)
  {
  }
}

void
reaperThread (void *)
  throw ()
{
  /*
   * Block SIGCHLD and SIGUSR1, so that we can atomically wait for both.
   */

  sigset_t noSigSet, clearSigSet, oldSigSet;

  sigemptyset (&noSigSet);
  sigemptyset (&clearSigSet);
  sigaddset (&noSigSet, SIGCHLD);
  sigaddset (&noSigSet, SIGUSR1);
  sigaddset (&noSigSet, SIGINT);
  sigaddset (&noSigSet, SIGTERM);
  sigaddset (&clearSigSet, SIGINT);
  sigaddset (&clearSigSet, SIGTERM);
  pthread_sigmask (SIG_BLOCK, &noSigSet, &oldSigSet);

  /*
   * Set signal handler.
   */

  struct sigaction act;
  act.sa_handler = SigChldHandler;
  act.sa_flags = SA_NOCLDSTOP;
  sigemptyset (&act.sa_mask);
  sigaction (SIGCHLD, &act, 0);
  sigaction (SIGUSR1, &act, 0);

  pthread_mutex_lock (&g_pmDataMutex);
  g_reaperThreadPid = getpid ();

 again:
  while (g_numRunningChildren) {
    ProcessDataSeq::iterator it;

    /*
     * See if there is any process that wants to start.
     */

    for (it = g_myChildren.begin(); it != g_myChildren.end(); it++) {
      ProcessData & pd = **it;

      if (!pd.started) {
        /*
         * Start this process!
         */

        pid_t cp = fork ();

        if (cp == 0) {
          /*
           * Undo our blocked signals.  The new process is created single-
           * threaded, so use sigprocmask here.
           */

          sigset_t clearSigSet;
          sigaddset (&clearSigSet, SIGCHLD);
          sigaddset (&clearSigSet, SIGUSR1);
          sigaddset (&clearSigSet, SIGINT);
          sigaddset (&clearSigSet, SIGTERM);
          sigprocmask (SIG_UNBLOCK, &clearSigSet, 0);

          /*
           * Child process, run the executable.
           */

          execv (pd.argv[0], pd.argv);

          /*
           * We get here only if the exec has failed.
           */

          exit (1);
        }
        else if (cp == -1) {
          /*
           * The fork has failed.  Clean up.
           */

          g_numRunningChildren--;
          pd.started = true;
          pd.terminated = true;
          pd.exitCode = -1;

          if (pd.detached) {
            /*
             * Nobody cares about this process.
             */

            delete *it;
          }
          else {
            /*
             * Wake up any threads that may be blocked in wait().
             */

            pthread_cond_broadcast (&g_pmDataCond);
          }

          /*
           * We don't care about this process any more.  Forget about it.
           */

          g_myChildren.erase (it);

          /*
           * The iterator may be invalidated.  And we may have no more
           * children.  Start from the top.
           */

          goto again;
        }

        /*
         * The new process was started successfully.
         */

        pd.started = true;
        pd.pid = cp;

        /*
         * Signal the parent that its child is alive and well.
         */

        pthread_cond_broadcast (&g_pmDataCond);
      }
    }

    /*
     * Wait for any childs to exit -- or for our parent to wake us.
     *
     * We can't just call waitpid(), there would be a race condition where
     * we could receive a SIGUSR1 wakeup signal after unlocking our mutex
     * but before entering waitpid(), resulting in that signal being
     * ignored.
     *
     * Instead, we call sigsuspend().  Above, we have blocked SIGCHLD and
     * SIGUSR1.  sigsuspend() atomically unblocks both and then waits for
     * the delivery of any signal.
     *
     * We can then discriminate between both by calling waitpid() with
     * the WNOHANG flag.  If that returns a pid, then a child has exited
     * and a SIGCHLD was delivered.  If it doesn't, then we must have
     * received SIGUSR1 or some other signal that's not for us.  There's
     * also the possibility of having received both signals simultaneously.
     */

    pid_t childPid;
    int exitCode;

    pthread_mutex_unlock (&g_pmDataMutex);

    sigsuspend (&clearSigSet);
    childPid = waitpid (-1, &exitCode, WNOHANG);

    pthread_mutex_lock (&g_pmDataMutex);

    /*
     * We may receive just one SIGCHLD even if multiple childs have exited.
     * Therefore, we need to loop here.
     */

    while (childPid != 0 && childPid != -1) {
      for (it = g_myChildren.begin(); it != g_myChildren.end(); it++) {
        if ((*it)->pid == childPid) {
          break;
        }
      }

      ocpiAssert (it != g_myChildren.end());
      ProcessData & pd = **it;

      g_numRunningChildren--;
      pd.terminated = true;
      pd.exitCode = exitCode;

      if (pd.detached) {
        /*
         * Nobody cares about this process.
         */

        delete *it;
      }
      else {
        /*
         * Wake up any threads that may be blocked in wait().
         */

        pthread_cond_broadcast (&g_pmDataCond);
      }

      /*
       * We don't care about this process any more.  Forget about it.
       */

      g_myChildren.erase (it);

      childPid = waitpid (-1, &exitCode, WNOHANG);
    }
  }

  /*
   * No more running processes, and thus no further need for a reaper
   * thread. Goodbye.
   */

  g_isReaperRunning = false; 
  g_reaperThreadPid = 0;

  pthread_mutex_unlock (&g_pmDataMutex);
  sigprocmask (SIG_SETMASK, &oldSigSet, 0);
}

/*
 * ----------------------------------------------------------------------
 * ProcessManager implementation
 * ----------------------------------------------------------------------
 */

inline
ProcessData *&
o2pd (OCPI::OS::uint64_t * ptr)
  throw ()
{
  return *reinterpret_cast<ProcessData **> (ptr);
}

OCPI::OS::ProcessManager::ProcessManager ()
  throw ()
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (ProcessData *)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (ProcessData *));
  o2pd (m_osOpaque) = 0;
}

OCPI::OS::ProcessManager::ProcessManager (const std::string & executable,
                                         const ParameterList & parameters)
  throw (std::string)
{
  ocpiAssert ((compileTimeSizeCheck<sizeof (m_osOpaque), sizeof (ProcessData)> ()));
  ocpiAssert (sizeof (m_osOpaque) >= sizeof (ProcessData));
  o2pd (m_osOpaque) = 0;
  start (executable, parameters);
}

OCPI::OS::ProcessManager::~ProcessManager ()
  throw ()
{
  ProcessData *& pdp = o2pd (m_osOpaque);

  if (pdp) {
    ocpiAssert (pdp->terminated);
    delete (pdp);
  }
}

void
OCPI::OS::ProcessManager::start (const std::string & executable,
                                const ParameterList & parameters)
  throw (std::string)
{
  ProcessData *& pdp = o2pd (m_osOpaque);

  if (pdp) {
    ocpiAssert (pdp->terminated);
    delete (pdp);
    pdp = 0;
  }

  /*
   * See if the executable exists; try to report this failure early.
   * Note that the executable may still appear or disappear between
   * now and the actual exec().  This will still be caught, but only
   * after creating the new process and lots of synchronization.
   */

  if (::access (executable.c_str(), X_OK)) {
    throw OCPI::OS::Posix::getErrorMessage (errno);
  }

  pdp = new ProcessData (executable, parameters);

  /*
   * Block SIGCHLD and SIGUSR1 in this thread.  They should go only to the
   * reaper thread.
   */

  sigset_t blockSigSet;
  sigemptyset (&blockSigSet);
  sigaddset (&blockSigSet, SIGCHLD);
  sigaddset (&blockSigSet, SIGUSR1);
  pthread_sigmask (SIG_BLOCK, &blockSigSet, 0);

  /*
   * Queue this information with the reaper thread.
   */

  pthread_mutex_lock (&g_pmDataMutex);

  g_numRunningChildren++;
  g_myChildren.push_back (pdp);

  /*
   * If the reaper thread is not running, start it.  Else, wake it.
   */

  if (!g_isReaperRunning) {
    g_isReaperRunning = true;
    g_reaperManager.start (reaperThread, 0);
    g_reaperManager.detach ();
  }
  else {
    ocpiAssert (g_reaperThreadPid);
    ::kill (g_reaperThreadPid, SIGUSR1);
  }

  /*
   * To avoid a race condition, we wait until the reaper thread has started
   * the new process.  (Otherwise, the user might, e.g., call pid() before
   * the reaper thread has set that field.)
   */

  volatile bool * isStarted = &pdp->started;

  while (!*isStarted) {
    pthread_cond_wait (&g_pmDataCond, &g_pmDataMutex);
  }

  pthread_mutex_unlock (&g_pmDataMutex);
}

unsigned long
OCPI::OS::ProcessManager::pid ()
  throw (std::string)
{
  ProcessData *& pdp = o2pd (m_osOpaque);

#if !defined (NDEBUG)
  ocpiAssert (pdp);
#endif

  return pdp->pid;
}

bool
OCPI::OS::ProcessManager::wait (unsigned long timeout)
  throw (std::string)
{
#if !defined (NDEBUG)
  ocpiAssert (o2pd (m_osOpaque));
#endif

  volatile ProcessData & pd = *o2pd (m_osOpaque);

  /*
   * Figure out the end of our deadline, if timeout != -1
   */

  struct timespec deadline;

  if (timeout != 0 && timeout != static_cast<unsigned long> (-1)) {
#ifdef __APPLE__
    struct timeval tv;
    gettimeofday(&tv, NULL);
    TIMEVAL_TO_TIMESPEC(&tv, &deadline);
#else
    clock_gettime (CLOCK_REALTIME, &deadline);
#endif
    deadline.tv_sec += timeout / 1000;
    if ((deadline.tv_nsec += (timeout%1000)*1000000) > 1000000000) {
      deadline.tv_sec++;
      deadline.tv_nsec -= 1000000000;
    }
  }

  /*
   * See if our process has exited in the meantime
   */

  pthread_mutex_lock (&g_pmDataMutex);

  /*
   * Wait for some process to finish for some time.  We must loop,
   * because we are awakened when any process exits.
   */

  while (!pd.terminated) {
    int res;

    if (timeout == 0) {
      res = ETIMEDOUT;
    }
    else if (timeout != static_cast<unsigned long> (-1)) {
      res = pthread_cond_timedwait (&g_pmDataCond, &g_pmDataMutex,
                                    &deadline);
    }
    else {
      res = pthread_cond_wait (&g_pmDataCond, &g_pmDataMutex);
    }

    if (res == ETIMEDOUT) {
      /*
       * We are past our deadline.
       */

      break;
    }

    ocpiAssert (res == 0);
  }

  pthread_mutex_unlock (&g_pmDataMutex);
  return pd.terminated;
}

int
OCPI::OS::ProcessManager::getExitCode ()
  throw (std::string)
{
#if !defined (NDEBUG)
  ocpiAssert (o2pd (m_osOpaque));
#endif

  ProcessData & pd = *o2pd (m_osOpaque);

  ocpiAssert (pd.terminated);

  if (!WIFEXITED (pd.exitCode)) {
    std::string reason;

    if (WIFSIGNALED (pd.exitCode)) {
      char tmp[16];
      std::sprintf (tmp, "%d", WTERMSIG (pd.exitCode));
      reason = "process killed by signal ";
      reason += tmp;
    }
    else {
      reason = "process terminated";
    }

    throw reason;
  }

  return WEXITSTATUS(pd.exitCode);
}

/*
 * There is a race condition here: maybe the process has just exited,
 * our reaper thread has performed the waitpid(), and is just about to
 * update the g_myChildren. In that case, kill() fails with errno==ESRCH.
 *
 * Nothing that we do can entirely resolve that race condition, and
 * thus there remains the remote (hopefully academic) possibility that
 * the PID was reused in the meantime, and we're about to kill some
 * unrelated process.
 *
 * To mitigate the race condition, we check if the process was already
 * reaped. The system hopefully won't reuse the PID in the time between
 * checking for the process' termination and our sending of the kill
 * signal.
 */

void
OCPI::OS::ProcessManager::shutdown ()
  throw (std::string)
{
#if !defined (NDEBUG)
  ocpiAssert (o2pd (m_osOpaque));
#endif

  volatile ProcessData & pd = *o2pd (m_osOpaque);

  pthread_mutex_lock (&g_pmDataMutex);

  int res = pd.terminated ? 0 : ::kill (pd.pid, SIGINT);
  int serrno = errno;

  pthread_mutex_unlock (&g_pmDataMutex);

  if (res != 0 && serrno != ESRCH)
    throw OCPI::OS::Posix::getErrorMessage (errno);
}

void
OCPI::OS::ProcessManager::kill ()
  throw (std::string)
{
#if !defined (NDEBUG)
  ocpiAssert (o2pd (m_osOpaque));
#endif

  volatile ProcessData & pd = *o2pd (m_osOpaque);


  pthread_mutex_lock (&g_pmDataMutex);

  int res = pd.terminated ? 0 : ::kill (pd.pid, SIGKILL);
  int serrno = errno;

  pthread_mutex_unlock (&g_pmDataMutex);

  if (res != 0 && serrno != ESRCH)
    throw OCPI::OS::Posix::getErrorMessage (errno);
}

void
OCPI::OS::ProcessManager::detach ()
  throw ()
{
  ProcessData *& pdp = o2pd (m_osOpaque);
  ocpiAssert (pdp);

  pthread_mutex_lock (&g_pmDataMutex);

  if (pdp->terminated) {
    /*
     * Process has already exited.  Clean up.
     */

    delete pdp;
  }
  else {
    pdp->detached = true;
  }

  pdp = 0;

  pthread_mutex_unlock (&g_pmDataMutex);
}
