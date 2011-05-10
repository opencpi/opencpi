
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

// -*- c++ -*-

#ifndef OCPIUTILAUTOMUTEX_H__
#define OCPIUTILAUTOMUTEX_H__

/**
 * \file
 * \brief Auto-release an OCPI::OS::Mutex.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 */

#include <OcpiOsMutex.h>
#include <string>

namespace OCPI {
  namespace Util {

    /**
     * \brief Auto-release an OCPI::OS::Mutex.
     *
     * Keeps track of the locked state of a OCPI::OS::Mutex object.  If
     * the lock is held at the time of destruction, the lock is released.
     * This helps releasing the locked mutex in all code paths, e.g.,
     * when an exception occurs.
     */

    class AutoMutex {
    public:
      /**
       * Constructor.
       *
       * \param[in] mutex The mutex object to manage.
       * \param[in] locked  If true, calls lock().
       *
       * \note Throughout the life time of this object, this thread
       * shall not manipulate \a mutex directly.
       */

      AutoMutex(OCPI::OS::Mutex & mutex, bool locked = true)
        throw (std::string);

      /**
       * Destructor.
       *
       * Releases the mutex lock if it is currently held.
       */

      ~AutoMutex ()
        throw ();

      void lock ()
        throw (std::string);

      bool trylock ()
        throw (std::string);

      void unlock ()
        throw (std::string);

    private:
      bool m_locked;
      OCPI::OS::Mutex & m_mutex;
    };

  }
}

#endif
