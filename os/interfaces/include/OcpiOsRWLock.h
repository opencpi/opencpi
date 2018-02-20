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

#ifndef OCPIOSRWLOCK_H__
#define OCPIOSRWLOCK_H__

/**
 * \file
 *
 * \brief A class for mutual exclusion between threads for read/write access.
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

#include <OcpiOsDataTypes.h>
#include <string>

namespace OCPI {
  namespace OS {

    /**
     * \brief A class for mutual exclusion between threads for read/write access.
     *
     * RWLock implements a read/write lock that distinguishes read and
     * write locks.  Multiple read locks can be acquired concurrently,
     * while the write lock is exclusive.
     *
     * \note Locks are not recursive.
     */

    class RWLock {
    public:
      /**
       * Constructor: Initializes the RWLock instance.
       *
       * \post The lock is unlocked.
       * \throw std::string Operating system error creating the mutex object.
       */

      RWLock ()
        throw (std::string);

      /**
       * Destructor.
       *
       * \pre The lock shall be unlocked.
       */

      ~RWLock ()
        throw ();

      /**
       * Acquires a read lock.
       *
       * Blocks until a read lock is available (i.e., no thread holds
       * the write lock), then acquires a read lock.
       *
       * \post The current thread holds a read lock.  No thread holds the
       * write lock.
       * \throw std::string Operating system error.
       */

      void rdLock ()
        throw (std::string);

      /**
       * Attempts to acquire a read lock without blocking.
       *
       * \return true if a read lock was successfully acquired without
       *         blocking. false if a read lock could not be acquired.
       *
       * \throw std::string Operating system error.
       */

      bool rdTrylock ()
        throw (std::string);

      /**
       * Releases a currently-held read lock.
       *
       * \pre The current thread holds a read lock.
       * \post The lock is unlocked.
       *
       * \throw std::string Operating system error.
       */

      void rdUnlock ()
        throw (std::string);

      /**
       * Acquires a write lock.
       *
       * Blocks until the write lock is available (i.e., no thread holds
       * a read or write lock), then acquires the write lock.
       *
       * \throw std::string Operating system error.
       *
       * \post The current thread holds the write lock.  No thread holds
       * the write lock.
       */

      void wrLock ()
        throw (std::string);

      /**
       * Attempts to acquire the write lock without blocking.
       *
       * \return  true if the write lock was successfully acquired without
       *          blocking. false if the write lock could not be acquired.
       *
       * \throw std::string Operating system error.
       */

      bool wrTrylock ()
        throw (std::string);

      /**
       * Releases a currently-held write lock.
       *
       * \throw std::string Operating system error.
       *
       * \pre The current thread holds the write lock.
       * \post The lock is unlocked.
       */

      void wrUnlock ()
        throw (std::string);

    private:
      OCPI::OS::uint64_t m_osOpaque[25];

    private:
      /**
       * Not implemented.
       */

      RWLock (const RWLock &);

      /**
       * Not implemented.
       */

      RWLock & operator= (const RWLock &);
    };

  }
}

#endif
