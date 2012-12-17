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
#include <vector>
#include <signal.h>
#include "OcpiOsDebug.h"
#include "OcpiContainerApi.h"
#include "OcpiApplicationApi.h"
#include "OcpiUtilMisc.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;

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
	  "    -p <instance-name>=<property>=<value>\n"
	  "                 # set a property value of a worker instance\n"
	  "    -n <processor-count>]\n"
	  "                 # set processor allocation policy for application\n"
	  "    -c <instance-name>=<container-name>]\n"
	  "                 # assign instance to a specific container\n"
	  "    -l <log-level>\n"
	  "                 # set log level during execution\n"
	  "    -t <seconds>\n"
	  "                 # specify seconds of runtime\n",
	  name);
  exit(1);
}

static std::vector<OA::PValue> params;
static void addParam(const char *name, const char **&ap) {
  const char *value = ap[0][2] ? &ap[0][2] : *++ap;
  params.push_back(OA::PVString(name, value));
}

int
main(int /*argc*/, const char **argv) {
  bool verbose = false, dump = false;
  unsigned seconds = 0, nProcs = 0;
  const char *argv0 = strrchr(argv[0], '/');
  if (argv0)
    argv0++;
  else
    argv0 = argv[0];
  if (!argv[1])
    usage(argv0);
  const char **ap;
  signal(SIGPIPE, SIG_IGN);
  try {
    for (ap = &argv[1]; *ap && ap[0][0] == '-'; ap++)
      switch (ap[0][1]) {
      case 'v':
	verbose = true;
	break;
      case 't':
	seconds = atoi(ap[0][2] ? &ap[0][2] : *++ap);
	break;
      case 'd':
	dump = true;
	break;
      case 's':
	addParam("selection", ap);
	break;
      case 'm':
	addParam("model", ap);
	break;
      case 'p':
	addParam("property", ap);
	break;
      case 'c':
	addParam("container", ap);
	break;
      case 'n':
	nProcs = atoi(ap[0][2] ? &ap[0][2] : *++ap);
	break;
      case 'l':
	OCPI::OS::logSetLevel(atoi(ap[0][2] ? &ap[0][2] : *++ap));
	break;
      default:
	usage(argv0);
      }
    if (!*ap)
      usage(argv0);
    if (params.size())
      params.push_back(OA::PVEnd);
    OA::Container *c;
    if (nProcs)
      for (unsigned n = 1; n < nProcs; n++) {
	std::string name;
	OU::formatString(name, "rcc%d", n);
	OA::ContainerManager::find("rcc", name.c_str());
      }
    if (verbose) {
      for (unsigned n = 0; (c = OA::ContainerManager::get(n)); n++)
	fprintf(stderr, "%s[%d]: %s", n ? ", " : "Available containers are: ", n, c->name().c_str());
      fprintf(stderr, "\n");
    }
    OA::Application app(*ap, params.size() ? params.data() : NULL);
    if (verbose)
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    if (verbose)
      fprintf(stderr,
	      "Application established: containers, workers, connections all created\n"
	      "Communication with the application established\n");
    if (dump) {
      std::string name, value;
      if (verbose)
	fprintf(stderr, "Dump of all initial property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value); n++)
	fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
    }
    app.start();
    if (verbose)
      fprintf(stderr, "Application started/running\n");
    if (seconds) {
      if (verbose)
	fprintf(stderr, "Waiting %u seconds for application to complete\n", seconds);
      sleep(seconds);
      if (verbose)
	fprintf(stderr, "After %u seconds...\n", seconds);
    } else {
      app.wait();
      if (verbose)
	fprintf(stderr, "Application finished\n");
    }
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
