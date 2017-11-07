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

namespace OCPI {
  namespace Remote {
    // These function are called without knowing whether the driver is available.
    // Thus they are not in the driver.
    extern bool g_enableRemoteDiscovery;
    extern bool (*g_probeServer)(const char *server, bool verbose, const char **exclude,
				 bool discovery, std::string &error);
    bool useServer(const char *server, bool verbose, const char **exclude, std::string &error);
  }
}
