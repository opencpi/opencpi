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

// -*- c++ -*-

#ifndef CPIOSTHREADMANAGER_H__
#define CPIOSTHREADMANAGER_H__

/**
 * \file
 *
 * \brief Start and manage threads.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Use 64-bit type for our opaque data, to ensure
 *                  alignment.
 *
 *     04/20/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <CpiOsDataTypes.h>
#include <string>

namespace CPI {
  namespace OS {

    /**
     * \brief Start and manage threads.
     *
     * \note In VxWorks DKM, system defaults are used for the new task's
     * priority, options and stack size.  These system defaults can be
     * configured using the kernel symbols <em>spTaskPriority</em>,
     * <em>spTaskOptions</em> and <em>spTaskStackSize</em>, respectively.
     * Some applications may find the default stack size of 20000 bytes
     * insufficient and may want to redefine the default.
     */

    class ThreadManager {
    public:
      /**
       * Constructor: Initializes the instance, but does not start a thread.
       */

      ThreadManager ()
        throw ();

      /**
       * Constructor: Initializes the instance, then calls
       *              #start (\a func, \a opaque).
       *
       * \throw std::string See start().
       */

      ThreadManager (void (*func) (void *), void * opaque)
        throw (std::string);

      /**
       * Destructor.
       *
       * \pre This instance is not managing a thread.
       */

      ~ThreadManager ()
        throw ();

      /**
       * Starts a thread, and manages it.
       *
       * \param[in] func   The function to run in the new thread. It is passed
       *                   \a opaque as it single parameter. The thread ends
       *                   when the function returns.
       * \param[in] opaque A parameter to pass to \a func in the new thread.
       *
       * \throw std::string Operating system error creating a new thread.
       *
       * \pre No thread is being managed.
       * \post A thread is being managed.
       */

      void start (void (*func) (void *), void * opaque)
        throw (std::string);

      /**
       * Waits for the completion of a thread.
       *
       * Blocks until the thread's \a func function returns.
       *
       * \throw std::string Operating system error waiting for thread
       * termination.
       *
       * \pre A thread is being managed.
       * \post The thread has terminated.  No thread is being managed.
       */

      void join ()
        throw (std::string);

      /**
       * Cease management of the managed thread.
       *
       * "Forgets" about the started thread, which will continue to
       * run.
       *
       * \throw std::string Operating system error detaching the thread.
       *
       * \pre A thread is being managed.
       * \post No thread is being managed.
       */

      void detach ()
        throw (std::string);

    private:
      CPI::OS::uint64_t m_osOpaque[2];

    private:
      /**
       * Not implemented.
       */

      ThreadManager (const ThreadManager &);

      /**
       * Not implemented.
       */

      ThreadManager & operator= (const ThreadManager &);
    };

  }
}

#endif
