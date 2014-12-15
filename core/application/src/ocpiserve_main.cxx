#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include "OcpiContainerApi.h"
#include "OcpiComponentLibrary.h"
#include "RemoteDriver.h"

#include "OcpiServer.h"

#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiserve [options]\n" \
  "This command acts as a server allowing other systems to use this system's containers.\n"
#define OCPI_OPTIONS \
  CMD_OPTION(directory,  D,    String, "artifacts", "The directory used to capture artifact/executable files") \
  CMD_OPTION(verbose,    v,    Bool,   "false", "Provide verbose output during operation") \
  CMD_OPTION(log,        l,    UChar,  "0",     "The logging level to be used during operation") \
  CMD_OPTION(processors, n,    UChar,  "1",     "The number of software (rcc) containers to create") \
  CMD_OPTION(containers, C,    Bool,   "false", "List containers")  \
  CMD_OPTION(serve,      s,    Bool,   "false", "Serve containers") \
  CMD_OPTION(remove,     r,    Bool,   "false", "Remove artifacts") \
  CMD_OPTION(port,       p,    UShort, 0, "Explicit TCP port for server") \
  CMD_OPTION(discoverable, d,  Bool,   "true", "Make server discoverable") \

// Others: socket number to use, whether to be discoverable, whether to be loopback/local only?

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
  rmdir();
  options.bad("interrupted");
}

static int mymain(const char **) {
  // Cause the normal library discovery process to only use the one directory
  // that will also act as a cache.
  // The directory is not created until it is needed.
  setenv("OCPI_LIBRARY_PATH", options.directory(), 1);
  for (unsigned n = 1; n < options.processors(); n++) {
    std::string name;
    OU::formatString(name, "rcc%d", n);
    OA::ContainerManager::find("rcc", name.c_str());
  }
  if (options.containers()) {
    OA::Container *ac;
    for (unsigned n = 0; (ac = OA::ContainerManager::get(n)); n++) {
      OC::Container &c = *static_cast<OC::Container *>(ac);
      fprintf(stderr,
	      "Container: \"%s\", model \"%s\", os \"%s\", version \"%s\", platform \"%s\"\n",
	      c.name().c_str(), c.model().c_str(), c.os().c_str(), c.osVersion().c_str(),
	      c.platform().c_str());
    }
    return 0;
  }
  // Catch signals in order to delete the artifact cache
  if (options.remove()) {
    ocpiCheck(signal(SIGINT, sigint) != SIG_ERR);
    ocpiCheck(signal(SIGTERM, sigint) != SIG_ERR);
  }
  OCPI::Remote::g_suppressRemoteDiscovery = true;
  OCPI::Driver::ManagerManager::configure();
  assert(OCPI::Library::Library::s_firstLibrary);
  OCPI::Application::Server server(options.verbose(), options.discoverable(),
				   *OCPI::Library::Library::s_firstLibrary,
				   options.port(), options.remove(), options.error());
  if (options.error().length() || server.run(options.error()))
    options.bad("Container server error");
  if (options.remove())
    rmdir();
  return 0;
}

int
main(int /*argc*/, const char **argv) {
  return options.main(argv, mymain);
}
