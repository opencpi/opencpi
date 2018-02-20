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

#ifndef OCPIOSPOSIXSOCKET_H__
#define OCPIOSPOSIXSOCKET_H__

#include <string>

namespace OCPI {
  namespace OS {
    namespace Posix {
      /*
       * We use this mutex to protect calls into the netdb functions,
       * i.e., gethostbyname() and gethostbyaddr(), which are not
       * reentrant.
       */

      void netDbLock ()
        throw (std::string);

      void netDbUnlock ()
        throw (std::string);

      std::string getHostname ()
        throw (std::string);

      std::string getFQDN ()
        throw (std::string);

      std::string getIPAddress ()
        throw (std::string);

      bool isLocalhost (const std::string &)
        throw (std::string);

    }
  }
}

#endif
