
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

// This file implements a variation on mutexes that are inherited by
// the class that needs protection, and where the inheritance is usually
// virtual so that the mutex is only destroyed AFTER all other
// member objects and non-virtually inherited classes are detroyed.

#ifndef OCPIUTILSELFMUTEX_H__
#define OCPIUTILSELFMUTEX_H__

#include "OcpiOsMutex.h"

namespace OCPI {
  namespace Util {
    // The class to inherit
    class SelfMutex : protected OCPI::OS::Mutex {
      friend class SelfAutoMutex;
    protected:
      SelfMutex() : OCPI::OS::Mutex(true) {}
      ~SelfMutex() { unlock(true); }
    };
    // The class used with automatic storage.
    class SelfAutoMutex {
      SelfMutex *m_mutex;
    public:
      // The argument is expected to be "this"
      SelfAutoMutex(SelfMutex *m) : m_mutex(m) {
	m->lock();
      }
      ~SelfAutoMutex() { m_mutex->unlock(); }
    };
  }
}

#endif
// Local Variables:
// mode:c++
// End:
