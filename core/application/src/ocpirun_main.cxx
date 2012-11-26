/* TODO:
 * container creation - possibly with maxprocessors
 * sync options with doc
 * perhaps add file I/O pre-existing ports?
 * API for log level
 */
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>
#include "OcpiContainerApi.h"
#include "OcpiApplicationApi.h"

namespace OA = OCPI::API;

static std::string error;

static void
usage(const char *name) {
  fprintf(stderr,
	  "Usage is: %s <options>... <application-xml-file>]\n"
	  "  Options: (values are either directly after the letter or in the next argument)\n"
	  "    -d           # dump properties after execution\n"
	  "    -v           # be verbose in describing what is happening\n"
	  "    -s <instance-name>=<expression>\n"
	  "                 # provide selection expression for worker instance\n"
	  "    -m <instance-name>=<model>\n"
	  "                 # set model (rcc, hdl, ocl, etc.) for worker instance\n"
	  "    -p <instance-name>:<property>=<value>\n"
	  "                 # set a property value of a worker instance\n"
	  "    -a <policy-name>[=<processor-count>]\n"
	  "                 # set processor allocation policy for application\n"
	  "    -c <instance-name>=<container-name>]\n"
	  "                 # assign instance to a specific container\n",
	  name);
  exit(1);
}

int
main(int /*argc*/, char **argv) {
  bool verbose = false, dump = false;
  const char *argv0 = strrchr(argv[0], '/');
  if (argv0)
    argv0++;
  else
    argv0 = argv[0];
  if (!argv[1])
    usage(argv0);
  char **ap;
  try {
    for (ap = &argv[1]; *ap && ap[0][0] == '-'; ap++)
      switch (ap[0][1]) {
      case 'v':
	verbose = true;
	break;
      case 'd':
	dump = true;
	break;
      default:
	usage(argv0);
      }
    if (!*ap)
      usage(argv0);
    OA::Application app(*ap);
    if (verbose)
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    if (verbose)
      fprintf(stderr,
	      "Application established: containers, workers, connections all created\n"
	      "Communication with the application established\n");
    app.start();
    if (verbose)
      fprintf(stderr, "Application started/running\n");
    app.wait();
    if (verbose)
      fprintf(stderr, "Application finished\n");
    if (dump) {
      std::string name, value;
      if (verbose)
	fprintf(stderr, "Dump of all final property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value); n++)
	fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
    }
    return 0;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  } catch (...) {
    fprintf(stderr, "Unexpected/unknown exception thrown\n");
  }
  return 1;
}
