// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

// -*- c++ -*-

#ifndef CPIOSWIN32SOCKET_H__
#define CPIOSWIN32SOCKET_H__

/*
 * Abstract:
 *
 *     Windows implementation of socket-related functionality.
 *     (Internal to CPI OS. Not exported.)
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>

namespace CPI {
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
       * Windows implementations of CPI::OS::getHostname(), CPI::OS::getFQDN()
       * and CPI::OS::isLocalhost(), respectively.
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
