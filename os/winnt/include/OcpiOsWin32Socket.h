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

#ifndef OCPIOSWIN32SOCKET_H__
#define OCPIOSWIN32SOCKET_H__

/*
 * Abstract:
 *
 *     Windows implementation of socket-related functionality.
 *     (Internal to OCPI OS. Not exported.)
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>

namespace OCPI {
  namespace OS {
    namespace Win32 {

      /*
       * Initializes the Windows Socket library. May be called multiple
       * times.
       */

      void winSockInit ()
        throw (std::string);

      /*
       * Uninitializes the Windows Socket library. Must be called as
       * often as winSockInit().
       */

      void winSockFini ()
        throw (std::string);

      /*
       * Windows implementations of OCPI::OS::getHostname(), OCPI::OS::getFQDN()
       * and OCPI::OS::isLocalhost(), respectively.
       */

      std::string getHostname ()
        throw (std::string);

      std::string getFQDN ()
        throw (std::string);

      std::string getIPAddress ()
        throw (std::string);

      bool isLocalhost (const std::string &)
        throw (std::string);

      /*
       * Returns the message associated with an error code that is
       * returned from any Windows Socket function.
       */

      std::string getWinSockErrorMessage (int errorCode)
        throw ();

    }
  }
}

#endif
