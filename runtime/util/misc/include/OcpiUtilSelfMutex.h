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

// This file implements a variation on mutexes that are inherited by
// the class that needs protection, and where the inheritance is usually
// virtual so that the mutex is only destroyed AFTER all other
// member objects and non-virtually inherited classes are detroyed.

#ifndef OCPIUTILSELFMUTEX_H__
#define OCPIUTILSELFMUTEX_H__

#include "OcpiOsMutex.h"

namespace OCPI {
  namespace Util {
    // The class to inherit that has a mutex
    class SelfMutex : public OCPI::OS::Mutex {
      friend class SelfAutoMutex;
    protected:
      SelfMutex() : OCPI::OS::Mutex(true) {}
      ~SelfMutex() { unlock(true); }
      //    public:
      //      operator OCPI::OS::Mutex &() { return *this; }
    };
    // The class to inherit, that references a mutex provided upon construction
    class SelfRefMutex {
      friend class SelfAutoMutex;
      OCPI::OS::Mutex &m_mutex;
    protected:
      SelfRefMutex(OCPI::OS::Mutex &m) : m_mutex(m) {}
      operator OCPI::OS::Mutex &() { return m_mutex; }
    };
    // The class used with automatic storage.
    class SelfAutoMutex {
      // this is a pointer and not a reference just to simplify usage
      // I.e. you construct with "this" rather than "*this".
      OCPI::OS::Mutex &m_mutex;
    public:
      // The argument is expected to be "this"
      SelfAutoMutex(SelfRefMutex *srm) : m_mutex(*srm) {
	m_mutex.lock();
      }
      SelfAutoMutex(SelfMutex *m) : m_mutex(*m) {
	m_mutex.lock();
      }
      ~SelfAutoMutex() { m_mutex.unlock(); }
    };
  }
}

#endif
// Local Variables:
// mode:c++
// End:
