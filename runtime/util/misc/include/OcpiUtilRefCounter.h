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

#ifndef OCPIUTILREFCOUNTER_H__
#define OCPIUTILREFCOUNTER_H__

/**
 * \file
 * \brief Utility base class used for reference counting an object.
 *
 * Revision History:
 *
 *     08/10/2005 - John Miller
 *                  Initial version.
 */

namespace OCPI {
  namespace Util {
    namespace Misc {

      /**
       * \brief Utility base class used for reference counting an object.
       *
       * \note The implementation is not thread safe.
       */

      class RefCounter {
      public:
        RefCounter();
        virtual ~RefCounter();
        int incRef();
        int decRef();
      private:
        int refCount;
        
      };

    }
  }
}

#endif
