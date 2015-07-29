
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
