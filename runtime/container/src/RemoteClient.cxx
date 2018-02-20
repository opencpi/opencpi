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

#include "OcpiUtilException.h"
#include "OcpiUtilMisc.h"
#include "RemoteClient.h"

// This "remote" code is not part of the remote driver library, but supports the ACI
// whether or not the remote driver is loaded.
namespace OU = OCPI::Util;
namespace OR = OCPI::Remote;
namespace OCPI {
  // When the remote container driver is loaded it needs to see this.
  namespace Remote {
    bool g_enableRemoteDiscovery = false;
    bool (*g_probeServer)(const char *server, bool verbose, const char **exclude, bool discovery,
			  std::string &error) = NULL;
    bool
    useServer(const char *server, bool verbose, const char **exclude, std::string &error) {
      if (g_probeServer)
	return (*g_probeServer)(server, verbose, exclude, false, error);
      OU::format(error,
		 "the remote container driver is not loaded; server \"%s\" cannot be accessed",
		 server);
      return true;
    }
  }
  namespace API {
    // ACI functions for using servers
    void useServer(const char *server, bool verbose) {
      std::string error;
      if (OR::useServer(server, verbose, NULL, error))
	throw OU::Error("error trying to use remote server \"%s\": %s", server, error.c_str());
    }
    void enableServerDiscovery() {
      if (OR::g_probeServer)
	OR::g_enableRemoteDiscovery = true;
      else
	throw OU::Error("remote container driver is not loaded so remote discovery cannot be "
			"enabled");
    }
    bool isServerSupportAvailable() {
      return OR::g_probeServer != NULL;
    }
  }
}
