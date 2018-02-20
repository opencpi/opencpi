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

#ifndef OCPIOSWIN32ERROR_H__
#define OCPIOSWIN32ERROR_H__

#include <string>

/*
 * Abstract:
 *
 *     Functions related to Windows error codes and messages.
 *     (Internal to OCPI OS. Not exported.)
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

namespace OCPI {
  namespace OS {
    namespace Win32 {

      /*
       * Returns the message associated with an error code, i.e., one that
       * was returned from GetLastError ().
       */

      std::string getErrorMessage (unsigned long errorCode)
        throw ();

    }
  }
}

#endif
