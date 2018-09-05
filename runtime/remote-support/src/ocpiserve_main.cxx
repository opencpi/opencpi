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
#include "ContainerManager.h"
#include "RemoteServer.h"
#include "OcpiServer.h"

namespace OU =  OCPI::Util;
namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OS = OCPI::OS;
namespace OT = OCPI::RDT;
namespace OR = OCPI::Remote;


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
  CMD_OPTION(list,       C,    Bool,    0,   "Show available containers") \
  CMD_OPTION(only_platforms,,  Bool,    0,   "modifies the list command to show only platforms")\

// FIXME: local-only like ocpihdl simulate?
#include "CmdOption.h"

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
  // We can't throw here to we just exit...
  options.exitbad("interrupted");
}

namespace {
class RemoteContainerServer : public OU::Server {
  OCPI::Library::Library &m_library;
  bool m_remove;
  std::string m_directory;
  std::vector<bool> m_needsBridging; // per container, does it need bridging to sockets
public:
  RemoteContainerServer(const char *addrFile) :
    OU::Server(options.verbose(), options.discoverable(), options.loopback(),
		 options.onlyloopback(), options.port(), "container", addrFile, options.error()),
    m_library(*OCPI::Library::Library::s_firstLibrary),
    m_remove(options.remove()) {
    if (!options.error().empty())
      return;
    if (options.verbose()) {
      fprintf(stderr,
	      "Artifacts stored/cached in the directory \"%s\", which will be %s on exit.\n",
	      m_library.libName().c_str(), m_remove ? "removed" : "retained");
      fprintf(stderr, "Containers offered to clients are:\n");
    }
    m_needsBridging.resize(OC::Manager::s_nContainers, true); // initially false
    OA::Container *ac;
    for (unsigned n = 0; (ac = OA::ContainerManager::get(n)); n++) {
      OC::Container &c = *static_cast<OC::Container *>(ac);
      if (options.verbose())
	fprintf(stderr, "  %2d: %s, model: %s, os: %s, osVersion: %s, platform: %s\n",
		n, c.name().c_str(), c.model().c_str(), c.os().c_str(),
		c.osVersion().c_str(), c.platform().c_str());
      OU::formatAdd(discoveryInfo(), "%s|%s|%s|%s|%s|%s|%c|", c.name().c_str(),
		    c.model().c_str(), c.os().c_str(), c.osVersion().c_str(),
		    c.arch().c_str(), c.platform().c_str(), c.dynamic() ? '1' : '0');
      for (unsigned nn = 0;  nn < c.transports().size(); nn++) {
	const OC::Transport &t = c.transports()[nn];
	OU::formatAdd(discoveryInfo(), "%s,%s,%u,%u,0x%x,0x%x|",
		      t.transport.c_str(), t.id.c_str()[0] ? t.id.c_str() : " ", t.roleIn,
		      t.roleOut, t.optionsIn, t.optionsOut);
	if (t.transport == "ocpi-socket-rdma")
	  m_needsBridging[n] = false;
      }
      if (m_needsBridging[n])
	OU::formatAdd(discoveryInfo(), "%s,%s,%u,%u,0x%x,0x%x|",
		      "ocpi-socket-rdma", " ", OT::ActiveFlowControl, OT::ActiveMessage,
		      (1 << OT::ActiveFlowControl), (1 << OT::ActiveMessage));
      discoveryInfo() += "\n";
    }
  }
  // callback from derived server class
protected:
  OU::Client *newClient(OS::ServerSocket &server, std::string &error) {
    return new OR::Server(m_library, server, discoveryInfo(), m_needsBridging, error);
  }
};
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
  if (options.list()) {
    OA::ContainerManager::list(options.only_platforms());
    return 0;
  }
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
  RemoteContainerServer server(addrFile);
  if (options.error().length() || server.run(options.error()))
    options.bad("Container server error");
  if (options.remove())
    rmdir();
  return 0;
}
