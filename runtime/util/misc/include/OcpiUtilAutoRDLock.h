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

#ifndef OCPIUTILAUTORDLOCK_H__
#define OCPIUTILAUTORDLOCK_H__

/**
 * \file
 * \brief Auto-release a OCPI::OS::RWLock read lock.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiOsRWLock.h>
#include <string>

namespace OCPI {
  namespace Util {

    /**
     * \brief Auto-release a OCPI::OS::RWLock read lock.
     *
     * Keeps track of the read-locked state of a OCPI::OS::RWLock object.  If
     * the lock is held at the time of destruction, the read lock is released.
     * This helps releasing the lock in all code paths, e.g., when an
     * exception occurs.
     */

    class AutoRDLock {
    public:
      /**
       * Constructor.
       *
       * \param[in] rwlock The lock object to manage.
       * \param[in] locked If true, calls lock().
       *
       * \note Throughout the life time of this object, this thread
       * shall not manipulate \a mutex directly.
       */

      AutoRDLock (OCPI::OS::RWLock & rwlock, bool locked = true)
        throw (std::string);

      /**
       * Destructor.
       *
       * Releases the read lock if it is currently held.
       */

      ~AutoRDLock ()
        throw ();

      void lock ()
        throw (std::string);

      bool trylock ()
        throw (std::string);

      void unlock ()
        throw (std::string);

    private:
      bool m_locked;
      OCPI::OS::RWLock & m_rwlock;
    };

  }
}

#endif
