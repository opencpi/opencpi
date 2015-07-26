#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include "OcpiContainerApi.h"
#include "OcpiComponentLibrary.h"
#include "Container.h"
#include "RemoteDriver.h"
#include "OcpiServer.h"

namespace OU =  OCPI::Util;


#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiserve [options]\n" \
  "This command acts as a server allowing other systems to use this system's containers.\n"
#define OCPI_OPTIONS \
  CMD_OPTION(directory,  D,    String, "artifacts", "The directory used to capture artifact/executable files") \
  CMD_OPTION(verbose,    v,    Bool,   "false", "Provide verbose output during operation") \
  CMD_OPTION(loglevel,   l,    UChar,  "0",     "The logging level to be used during operation") \
  CMD_OPTION(processors, n,    UShort,  "1",     "The number of software (rcc) containers to create") \
  CMD_OPTION(remove,     r,    Bool,   "false", "Remove artifacts") \
  CMD_OPTION(port,       p,    UShort, 0, "Explicit TCP port for server") \
  CMD_OPTION(discoverable, d,  Bool,   "true", "Make server discoverable.") \

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
  OCPI::OS::logSetLevel(options.loglevel());
  setenv("OCPI_LIBRARY_PATH", options.directory(), 1);
  for (unsigned n = 1; n < options.processors(); n++) {
    std::string name;
    OU::formatString(name, "rcc%d", n);
    OA::ContainerManager::find("rcc", name.c_str());
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
