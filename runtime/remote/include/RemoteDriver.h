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

#ifndef _OCPI_REMOTE_DRIVER_H_
#define _OCPI_REMOTE_DRIVER_H_
namespace OCPI {
  namespace Remote {
    // These are common - they are initialized to zero, but accessible by driver users
    // even when the driver is not loaded.  They are declared in the driver because
    // the driver cannot depend on the non-driver/server code or headers.
    extern bool g_suppressRemoteDiscovery; // not extern so it is common
    extern bool (*g_probeServer)(const char *server, bool verbose, const char **exclude,
			  std::string &error);
  }
}
#endif
