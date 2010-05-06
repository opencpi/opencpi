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

#ifndef CPIOSSPINLOCK_H__
#define CPIOSSPINLOCK_H__

/**
 * \file
 *
 * \brief A class for mutual exclusion between threads.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Use 64-bit type for our opaque data, to ensure
 *                  alignment.
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiOsDataTypes.h>
#include <string>

namespace CPI {
  namespace OS {

    /**
     * \brief A class for mutual exclusion between threads.
     *
     * Allows for synchronization between different threads within the
     * same process, e.g., to protect access to shared data.  A single
     * instance is shared between the threads that want to synchronize.
     *
     * A spinlock is very similar to a mutex.  However, it is more
     * light-weight, as it does not require a signaling mechanism between
     * the threads, and does not involve the operating system.  Spinlocks
     * are used when the shared resource is likely to be held only briefly
     * by another thread that is running concurrently.  A thread that
     * wants to acquire a spinlock "spins" in a busy-wait loop for the
     * other thread to release its lock.
     *
     * Spinlocks are particularly attractive on SMP and multi-core
     * systems, where the other thread executes concurrently, is not
     * affected by the busy-waiting ("spinning") of the current thread,
     * and is likely to release the shared resource shortly.  In a
     * single-CPU, single-core environment, spinlocks are implemented
     * to yield the current thread's timeslice if the resource is not
     * available, so that the other thread has a chance to run.  Ideally,
     * threads should not be preempted while holding a spinlock.
     *
     * Spinlocks do not guarantee fair scheduling, while mutexes do.  A
     * thread may repeatedly lock the same spinlock without other threads
     * having a chance of aquiring it.  E.g., if a spinlock was used to
     * control access to forks in a "dining philosopher" scenario,
     * starvation might occur.
     *
     * A spinlock may also cause priority inversion, i.e., a lower
     * priority thread preempting a high priority thread from acquiring
     * the lock.
     *
     * A spinlock is useful for protecting very brief accesses to
     * shared data, i.e., when the amount of time that the lock will
     * be held is predictably constant in all circumstances, e.g.,
     * reading or writing a single piece of data.  A mutex should be
     * used if the time is unpredictable, e.g., if it is necessary
     * to call system calls within the critical section.
     *
     * \note On Linux, the spinlock is currently implemented using a mutex.
     * \note On VxWorks, the spinlock is currently implemented using a mutex.
     */

    class SpinLock {
    public:
      /**
       * Constructor: Initializes the spinlock instance.
       *
       * \post The spinlock is unlocked.
       *
       * \throw std::string Operating system error creating the spinlock object.
       */

      SpinLock ()
        throw (std::string);

      /**
       * Destructor.
       *
       * \pre The spinlock shall be unlocked.
       */

      ~SpinLock ()
        throw ();

      /**
       * Acquires the spinlock.
       *
       * If the spinlock is currently locked, spin until it becomes
       * available.
       *
       * \pre The spinlock shall not be held by the current thread.
       * \post The spinlock is held by the current thread.
       * \throw std::string Operating system error.
       */

      void lock ()
        throw (std::string);

      /**
       * Attempts to acquire the lock without spinning.
       *
       * \return true if the lock was successfully acquired without
       *         spinning. false if the lock is already locked.
       *
       * \throw std::string Operating system error.
       */

      bool trylock ()
        throw (std::string);

      /**
       * Releases the spinlock.
       *
       * \pre The spinlock shall be held by the current thread.
       * \post The spinlock is released.
       *
       * \throw std::string Operating system error.
       */

      void unlock ()
        throw (std::string);

    private:
      CPI::OS::uint64_t m_osOpaque[5];

    private:
      /**
       * Not implemented.
       */

      SpinLock (const SpinLock &);

      /**
       * Not implemented.
       */

      SpinLock & operator= (const SpinLock &);
    };

  }
}

#endif
