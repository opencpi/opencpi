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

#include "RemoteDriver.h"
namespace OU =  OCPI::Util;
namespace OD =  OCPI::Driver;

#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiremote [options] <command>\n" \
  "  Commands are:\n" \
  "    search    - find available discoverable devices\n" \
  "    probe     - test whether a server is available\n" \

//           name      abbrev  type    value    description
#define OCPI_OPTIONS \
  CMD_OPTION(  verbose,   v,   Bool,   "false", "Provide verbose output during operation") \
  CMD_OPTION(  loglevel,  l,   UChar,  "0",     "Set logging level used during operation") \
  CMD_OPTION(  interface, i,   String, NULL,    "Restrict discovery to this network interface") \

#include "CmdOption.h"

namespace OA = OCPI::API;
namespace OR = OCPI::Remote;

static int mymain(const char **ap) {
  OCPI::OS::logSetLevel(options.loglevel());
  OCPI::Driver::ManagerManager::suppressDiscovery();
  if (!*ap)
    return 0;
  // Now we explicitly load the remote container driver
  std::string err;
  OD::Driver *d = OD::ManagerManager::loadDriver("container", "remote", err);
  if (!d)
    options.bad("Error during loading of remote container driver: %s", err.c_str());
  OR::Driver &driver = *static_cast<OR::Driver*>(d);
  if (!strcasecmp(*ap, "search")) {
    OA::PVarray vals(5);
    unsigned n = 0;
    vals[n++] = OA::PVBool("printOnly", true);
    if (options.verbose())
      vals[n++] = OA::PVBool("verbose", true);
    if (options.interface())
      vals[n++] = OA::PVString("interface", options.interface());
    vals[n++] = OA::PVEnd;
    OA::enableServerDiscovery();
    driver.search(vals, NULL, true);
  } else if(!strcasecmp(*ap, "probe")) {
    if (!*++ap)
      options.bad("Missing server name argument to probe");
    if (driver.probeServer(*ap, options.verbose(), NULL, NULL, true,
						       err))
      options.bad("Error during probe for \"%s\": %s", *ap, err.c_str());
  }
  return 0;
}
