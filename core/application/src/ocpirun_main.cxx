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
#include <unistd.h>
#include "OcpiOsDebug.h"
#include "OcpiOsFileSystem.h"
#include "OcpiContainerApi.h"
#include "OcpiApplicationApi.h"
#include "OcpiUtilMisc.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;

static std::string error;

static void
usage(const char *name) {
  fprintf(stderr,
	  "Usage is: %s <options>... <application-xml-file>]\n"
	  "  Options: (values are either directly after the letter or in the next argument)\n"
	  "    -d           # dump properties after execution\n"
	  "    -v           # be verbose in describing what is happening\n"
	  "    -x           # print numeric property values in hex, not decimal\n"
	  "    -s <instance-name>=<expression>\n"
	  "                 # provide selection expression for worker instance\n"
	  "    -m [<instance-name>]=<model>\n"
	  "                 # set model (rcc, hdl, ocl, etc.) for worker instance\n"
	  "    -p <instance-name>=<property>=<value>\n"
	  "                 # set a property value of a worker instance\n"
	  "    -c [<instance-name>]=<container-name>]\n"
	  "                 # assign instance to a specific container (name or number from -C)\n"
	  "    -P [<instance-name>]=<platform-name>\n"
	  "                 # assign instance to any container of this platform type (see output from -C)\n"
	  "    -w <instance-name>=<implementation-name>\n"
	  "                 # choose a particular worker name.\n"
	  "    -n <processor-count>]\n"
	  "                 # set processor allocation policy for application\n"
	  "    -f [<external-name>]=<file-name>\n"
	  "                 # connect external port to a specific file\n"
	  "    -D [<external-name>]=[//container/][slot/]<device-name>]\n"
	  "                 # connect external port to a specific device\n"
	  "    -u [<external-name>]=<URL>]\n"
	  "                 # connect external port to a URL\n"
	  "    -l <log-level>\n"
	  "                 # set log level during execution\n"
	  "    -t <seconds>\n"
	  "                 # specify seconds of runtime\n"
	  "    -C           # show available containers\n",
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
  bool verbose = false, dump = false, containers = false, hex = false;
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
      case 'i':
	addParam("worker", ap);
	break;
      case 'v':
	verbose = true;
	params.push_back(OA::PVBool("verbose", true));
	break;
      case 'x':
	hex = true;
	params.push_back(OA::PVBool("hex", true));
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
      case 'P':
	addParam("platform", ap);
	break;
      case 'f':
	addParam("file", ap);
	break;
      case 'D':
	addParam("device", ap);
	break;
      case 'u':
	addParam("url", ap);
	break;
      case 'n':
	nProcs = atoi(ap[0][2] ? &ap[0][2] : *++ap);
	break;
      case 'l':
	OCPI::OS::logSetLevel(atoi(ap[0][2] ? &ap[0][2] : *++ap));
	break;
      case 'C':
	containers = true;
	break;
      default:
	usage(argv0);
      }
    if (!*ap && !containers)
      usage(argv0);
    std::string file;
    if (*ap) {
      file = *ap;
      if (!OS::FileSystem::exists(file)) {
	file += ".xml";
	if (!OS::FileSystem::exists(file)) {
	  fprintf(stderr, "Error: file %s (or %s.xml) does not exist\n", *ap, *ap);
	  return 1;
	}
      }
    }
    if (params.size())
      params.push_back(OA::PVEnd);
    OA::Container *c;
    if (nProcs)
      for (unsigned n = 1; n < nProcs; n++) {
	std::string name;
	OU::formatString(name, "rcc%d", n);
	OA::ContainerManager::find("rcc", name.c_str());
      }
    if (containers) {
      (void)OA::ContainerManager::get(0); // force config
      printf("Available containers:\n"
	     " #  Model  Platform     OS         Name\n");
      for (unsigned n = 0; (c = OA::ContainerManager::get(n)); n++)
	printf("%2u  %-6s %-12s %-10s %s\n",
	       n,  c->model().c_str(), c->platform().c_str(), c->os().c_str(), c->name().c_str());
      fflush(stdout);
    } else if (verbose) {
      for (unsigned n = 0; (c = OA::ContainerManager::get(n)); n++)
	fprintf(stderr, "%s%s [model: %s os: %s platform: %s]", n ? ", " : "Available containers are: ",
		c->name().c_str(), c->model().c_str(), c->os().c_str(), c->platform().c_str());
      fprintf(stderr, "\n");
    }
    
    if (file.empty())
      return 0;
    //OA::Application app(file.c_str(), params.size() ? params.data() : NULL);
    // Some STL vector implementations don't have the "data" memberfunction...
    OA::Application app(file.c_str(), params.size() ? &params[0] : NULL);
    if (verbose)
      fprintf(stderr, "Application XML parsed and deployments (containers and implementations) chosen\n");
    app.initialize();
    if (verbose)
      fprintf(stderr,
	      "Application established: containers, workers, connections all created\n"
	      "Communication with the application established\n");
    if (dump) {
      std::string name, value;
      bool isParameter;
      if (verbose)
	fprintf(stderr, "Dump of all initial property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter); n++)
	fprintf(stderr, "Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
		isParameter ? " (parameter)" : "");
    }
    app.start();
    if (verbose)
      fprintf(stderr, "Application started/running\n");
    if (seconds) {
      if (verbose)
	fprintf(stderr, "Waiting %u seconds for application to complete\n", seconds);
      sleep(seconds);
      if (verbose)
	fprintf(stderr, "After %u seconds, stopping application...\n", seconds);
      app.stop();
    } else {
      if (verbose)
	fprintf(stderr, "Waiting for application to be finished (no timeout)\n");
      app.wait();
      if (verbose)
	fprintf(stderr, "Application finished\n");
    }
    // In case the application specifically defines things to do that aren't in the destructor
    app.finish();
    if (dump) {
      std::string name, value;
      bool isParameter;
      if (verbose)
	fprintf(stderr, "Dump of all final property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter); n++)
	if (!isParameter)
	  fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
    }
    return 0;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return 1;
  } catch (...) {
    fprintf(stderr, "Unexpected/unknown exception thrown\n");
    return 1;
  }
  return 1;
}
