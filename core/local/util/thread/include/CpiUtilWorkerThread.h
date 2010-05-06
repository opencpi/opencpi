// -*- c++ -*-

#ifndef CPI_UTIL_WORKER_THREAD_H__
#define CPI_UTIL_WORKER_THREAD_H__

/**
 * \file
 * \brief Worker thread for asynchronous job processing.
 *
 * Revision History:
 *
 *     02/23/2006 - Frank Pilhofer
 *                  Initial version.
 */

#include <string>
#include <CpiOsMutex.h>
#include <CpiOsSemaphore.h>
#include <CpiOsThreadManager.h>

namespace CPI {
  namespace Util {

    /**
     * \brief Worker thread for asynchronous job processing.
     *
     * This class implements a worker thread that can be used to schedule
     * asynchronous jobs.  Jobs can be submitted and are processed in a
     * separate thread.  The caller can then poll to see whether the job
     * has completed.
     *
     * Another way to look at it is that this class implements a one-thread
     * thread pool.
     *
     * The worker thread is kept alive even when no job is being run.  This
     * avoids the costs associated with thread creation and destruction.
     */

    class WorkerThread {
    public:
      /**
       * Constructor.
       *
       * \param[in] synchronous If true, then no separate thread is used,
       *                   but all jobs are processed synchronously.  This
       *                   is useful for debugging, to ensure that things
       *                   happen after one another.
       */

      WorkerThread (bool synchronous = false)
        throw (std::string);

      /*
       * Destructor. Stops the worker thread.
       */

      ~WorkerThread ()
        throw ();

      /**
       * Submit a job.  If the worker thread is currently busy, this blocks
       * until the previous jobs are finished.  If the worker thread is idle,
       * it starts processing the new job by calling its entrypoint.
       *
       * \param[in] job    The entrypoint for the job.
       * \param[in] opaque Parameter to pass to the entrypoint.
       */

      void start (void (*job) (void *), void * opaque)
        throw (std::string);

      /**
       * Block until the currently running job is complete (i.e., its
       * entry point returns) and the worker thread becomes idle.
       */

      void wait ()
        throw (std::string);

      /** \cond */

      /*
       * The thread procedure.
       */

      static
      void
      worker (void * opaque);

      /** \endcond */

    protected:
      /** \cond */
      /*
       * Info shared with the thread.
       */

      struct ThreadData {
        ThreadData ()
          throw (std::string);
        bool terminate;
        void (*job) (void *);
        void * opaque;
        CPI::OS::Semaphore jobPosted;
        CPI::OS::Semaphore jobComplete;
      };
      /** \endcond */

    protected:
      /** \cond */
      bool m_synchronous;
      CPI::OS::Mutex m_mutex;
      CPI::OS::ThreadManager m_threadManager;
      ThreadData m_threadData;
      /** \endcond */
    };

  }
}

#endif
