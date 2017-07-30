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

#ifndef OCPI_LIBRARY_API_H
#define OCPI_LIBRARY_API_H
#include <string>

namespace OCPI {
  namespace API {
    // How to express how a worker will be connected in the application
    // it is defined here since it is relevant to connections that are hardwired
    // in artifacts.
    struct Connection {
      const char *port, *otherWorker, *otherPort;
    };
    class LibraryManager {
      // The function that allows a control application to set the library
      // path independent of the environment.  It will supercede any
      // environment setting.
      static void setPath(const char*);
      static std::string getPath(); // return a copy so it can change...
    };
  }
}
#endif
