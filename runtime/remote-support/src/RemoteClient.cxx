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
#include "OcpiDriverManager.h"
#include "OcpiUtilMisc.h"
#include "RemoteDriver.h"
#include "RemoteClient.h"

// This "remote" code is not part of the remote driver library, but supports the ACI
// whether or not the remote driver is loaded.
namespace OU = OCPI::Util;
namespace OR = OCPI::Remote;
namespace OD = OCPI::Driver;
namespace OCPI {
  // When the remote container driver is loaded it needs to see this.
  namespace Remote {
    bool
    probeRemote(OR::Driver *&driver, const char *server, bool verbose, const char **exclude, std::string &err) {
      if (!driver) {
	OD::Driver *d = OD::ManagerManager::findDriver("container", "remote");
	if (!d)
	  return OU::eformat(err, "remote container driver not loaded per system.xml file");
	driver = static_cast<OR::Driver *>(d);
      }
      return driver->probeServer(server, verbose, exclude, NULL, false, err);
    }
    void
    probeServer(OR::Driver *&driver, const char *server, bool verbose) {
      std::string error;
      if (OR::probeRemote(driver, server, verbose, NULL, error))
	throw OU::Error("error trying to use remote server \"%s\": %s", server, error.c_str());
    }
  }
  namespace API {
    // ACI functions for using servers
    void useServer(const char *server, bool verbose) {
      OR::Driver *driver = NULL;
      OR::probeServer(driver, server, verbose);
    }
    // Use servers found in the environment or in the params supplied or in the arg supplied
    void useServers(const char *server, const PValue *params, bool verbose) {
      OR::Driver *driver = NULL;
      if (server)
	OR::probeServer(driver, server, verbose);
      if ((server = getenv("OCPI_SERVER_ADDRESS")))
	OR::probeServer(driver, server, verbose);
      if ((server = getenv("OCPI_SERVER_ADDRESSES")))
	for (OU::TokenIter li(server); li.token(); li.next())
	  OR::probeServer(driver, li.token(), verbose);
      if ((server = getenv("OCPI_SERVER_ADDRESS_FILE"))) {
	std::string addrs;
	const char *err = OU::file2String(addrs, server, ' ');
	if (err)
	  throw OU::Error("The file indicated by the OCPI_SERVER_ADDRESS_FILE environment "
			  "variable, \"%s\", cannot be opened: %s", server, err);
	for (OU::TokenIter li(addrs); li.token(); li.next())
	  OR::probeServer(driver, li.token(), verbose);
      }
      for (const OU::PValue *p = params; p && p->name; ++p)
	if (!strcasecmp(p->name, "server")) {
	  if (p->type != OCPI_String)
	    throw OU::Error("Value of \"server\" parameter is not a string");
	  OR::probeServer(driver, p->vString, verbose);
	}
    }
    void enableServerDiscovery() {
      OD::Driver *driver = OD::ManagerManager::findDriver("container", "remote");
      if (driver)
	driver->enableDiscovery();
      else
	throw OU::Error("remote container driver is not loaded so remote discovery cannot be "
			"enabled");
    }
    bool isServerSupportAvailable() {
      return OD::ManagerManager::findDriver("container", "remote") != NULL;
    }
  }
}
