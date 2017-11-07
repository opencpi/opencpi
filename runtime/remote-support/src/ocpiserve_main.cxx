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

#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include "OcpiContainerApi.h"
#include "OcpiComponentLibrary.h"
#include "Container.h"
#include "OcpiServer.h"

namespace OU =  OCPI::Util;


#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiserve [options]\n" \
  "This command acts as a server allowing other systems to use this system's containers.\n" \
  "Some option must be provided, e.g. -v, or -d, or -p.\n"
#define OCPI_OPTIONS \
  CMD_OPTION(directory,  D,    String, "artifacts", "The directory used for caching artifact/executable files") \
  CMD_OPTION(verbose,    v,    Bool,    0,   "Provide verbose output during operation") \
  CMD_OPTION(loglevel,   l,    UChar,   "0", "The logging level to be used during operation") \
  CMD_OPTION(processors, n,    UShort,  "1", "The number of software (rcc) containers to create") \
  CMD_OPTION(remove,     r,    Bool,    0,   "Remove cached artifacts after execution or control-C") \
  CMD_OPTION(port,       p,    UShort,  0,   "Explicit TCP port for server; default is dynamic") \
  CMD_OPTION(discoverable, d,  Bool,    0,   "Make this server discoverable, via UDP multicast") \
  CMD_OPTION(addresses , a,    String,  0,   "Write server's TCP addresses to this file, one per line") \
  CMD_OPTION(loopback  , L,    Bool,    0,   "Allow discovery on the local/loopback subnet") \
  CMD_OPTION(onlyloopback, O,  Bool,    0,   "Allow discovery ONLY on local/loopback subnet") \

// FIXME: local-only like ocpihdl simulate?
#include "CmdOption.h"

namespace OC = OCPI::Container;
namespace OA = OCPI::API;

static void
rmdir() {
  if (options.verbose())
    fprintf(stderr, "Removing the artifacts directory and contents in \"%s\".\n",
	    options.directory());
  std::string cmd, err;
  OU::format(cmd, "rm -r -f %s", options.directory());
  int rc = system(cmd.c_str());
  switch(rc) {
  case 127:
    OU::format(err, "Couldn't start execution of command: %s", cmd.c_str());
    break;
  case -1:
    OU::format(err, "System error (%s, errno %d) while executing command: %s",
	       strerror(errno), errno, cmd.c_str());
    break;
  default:
    OU::format(err, "Error return %u while executing command: %s", rc, cmd.c_str());
    break;
  case 0:
    ocpiInfo("Successfully removed artifact directory: \"%s\"", options.directory());
  }
}
static void
sigint(int /* signal */) {
  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);
  if (options.remove())
    rmdir();
  options.bad("interrupted");
}

static int mymain(const char **) {
  // Cause the normal library discovery process to only use the one directory
  // that will also act as a cache.
  // The directory is not created until it is needed.
  OCPI::OS::logSetLevel(options.loglevel());
  setenv("OCPI_LIBRARY_PATH", options.directory(), 1);
  // Catch signals in order to delete the artifact cache
  // Catch SIGINT all the time to make valgrind happe
  ocpiCheck(signal(SIGINT, sigint) != SIG_ERR);
  if (options.remove())
    ocpiCheck(signal(SIGTERM, sigint) != SIG_ERR);
  OCPI::Driver::ManagerManager::configure();
  assert(OCPI::Library::Library::s_firstLibrary);
  for (unsigned n = 1; n < options.processors(); n++) {
    std::string name;
    OU::formatString(name, "rcc%d", n);
    OA::ContainerManager::find("rcc", name.c_str());
  }
  const char *addrFile = options.addresses();
  if (!addrFile)
    addrFile = getenv("OCPI_SERVER_ADDRESSES_FILE");
  if (!addrFile)
    addrFile = getenv("OCPI_SERVER_ADDRESS_FILE"); // deprecated
  if (options.verbose())
    fprintf(stderr, "Discovery options:  discoverable: %u, loopback: %u, onlyloopback: %u\n",
	    options.discoverable(), options.loopback(), options.onlyloopback());
  OCPI::Application::Server server(options.verbose(), options.discoverable(),
				   options.loopback(), options.onlyloopback(),
				   *OCPI::Library::Library::s_firstLibrary,
				   options.port(), options.remove(), addrFile,
				   options.error());
  if (options.error().length() || server.run(options.error()))
    options.bad("Container server error");
  if (options.remove())
    rmdir();
  return 0;
}
