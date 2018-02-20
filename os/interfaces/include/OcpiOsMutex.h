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

// -*- c++ -*-

#ifndef OCPIOSMUTEX_H__
#define OCPIOSMUTEX_H__

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
 */

#include <OcpiOsDataTypes.h>
#include <string>

namespace OCPI {
  namespace OS {

    /**
     * \brief A class for mutual exclusion between threads.
     *
     * Allows for synchronization between threads within the same process,
     * e.g., to protect access to shared data. A single instance is shared
     * between the threads that want to synchronize.
     */

    class Mutex {
    public:
      /**
       * Constructor: Initializes the mutex instance.
       *
       * \param[in] recursive Controls whether the mutex is recursive or not.
       *                 A recursive mutex can be locked multiple times
       *                 by the same thread. With a non-recursive mutex,
       *                 an attempt to lock a mutex that is already locked
       *                 by the current thread causes a deadlock.
       *
       * \post The mutex is unlocked.
       * \throw std::string Operating system error creating the mutex object.
       */

      Mutex (bool recursive = false)
        throw (std::string);

      /**
       * Destructor.
       *
       * \pre The mutex shall be unlocked.
       */

      ~Mutex ()
        throw ();

      /**
       * \brief Acquires the mutex.
       *
       * If the mutex is locked by a different thread, block until it
       * becomes unlocked, then proceed as below.
       *
       * If the mutex is locked by the current thread, and if the mutex
       * is recursive, increment an internal counter and return.
       *
       * If the mutex is not locked, mark it as locked by the current
       * thread, and return.
       *
       * \pre If not recursive, the mutex shall not be locked by the
       *      current thread.
       * \post The mutex is locked by the current thread.
       * \throw std::string Operating system error.
       */

      void lock ()
        throw (std::string);

      /**
       * \brief Attempts to acquire the mutex without blocking.
       *
       * If the mutex is not locked, or if the mutex is recursive and
       * locked by the current thread, behave as lock(), and return
       * true. Otherwise, do not acquire the lock and returns false.
       *
       * \return true if the mutex was successfully acquired without
       *         blocking, false if the mutex could not be acquired.
       * \throw std::string Operating system error.
       */

      bool trylock ()
        throw (std::string);

      /**
       * \brief Release a currently-held lock.
       *
       * If the lock is held by the current thread, and the mutex is
       * not recursive, release the lock.
       *
       * If the lock is held by the current thread, and the mutex is
       * recursive, decrement the lock counter. If zero, release the
       * lock.
       *
       * \pre The mutex is locked by the current thread.
       * \post If not recursive, or if recursive and the lock counter
       * is decreased to zero, the lock is released. 
       * \throw std::string Operating system error.
       */

      void unlock (bool okIfUnlocked = false)
        throw (std::string);

    private:
      unsigned m_locked;
      OCPI::OS::uint64_t m_osOpaque[8];

    private:
      /**
       * Not implemented.
       */

      Mutex (const Mutex &);

      /**
       * Not implemented.
       */

      Mutex & operator= (const Mutex &);
    };

  }
}

#endif
