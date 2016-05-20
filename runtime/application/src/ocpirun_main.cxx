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

#include "OcpiContainerApi.h"
#include "OcpiApplicationApi.h"

#include "OcpiLibraryManager.h"
#include "DtDriver.h"
#include "ContainerManager.h"
#include "OcpiOsDebug.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilMisc.h"
#include "RemoteDriver.h"
#include "RemoteServer.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OL = OCPI::Library;
namespace OR = OCPI::Remote;

static std::string error;

static void
usage(const char *name) {
  fprintf(stderr,
	  "Usage is: %s <options> ... [<application-xml-file>]\n"
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
	  "                 # specify (absolute) seconds of runtime\n"
	  "                 # use negative values for \"up to\" timeout\n"
	  "    -C           # show available containers\n"
	  "    -S           # list of servers to explicitly contact (no UDP discovery)\n"
	  "    -R           # discover/include/use remote containers\n"
	  "    -X           # list of containers to exclude from usage\n"
	  "    -T <instance-name>=<port-name>=<transport-name>\n"
	  "                 # set transport of connection at a port\n"
	  "                 # if no port name, then the single output port\n",
	  name);
  exit(1);
}

static std::vector<OA::PValue> params;
static void addParam(const char *name, const char **&ap) {
  const char *value = ap[0][2] ? &ap[0][2] : *++ap;
  params.push_back(OA::PVString(name, value));
}

static bool verbose = false;
static const char *doServer(const char *server, void *) {
  static std::string error;
  return OR::useServer(server, verbose, NULL, error) ? error.c_str() : NULL;
}
static bool specs;
static const char *doTarget(const char *target, void *) {
  static std::string error;
  OL::Capabilities caps;
  const char *dash = strchr(target, '-');
  if (dash) {
    caps.m_os.assign(target, dash - target);
    target = ++dash;
    dash = strchr(target, '-');
    if (dash) {
      caps.m_osVersion.assign(target, dash - target);
      target = ++dash;
    }
    caps.m_platform = target;
  } else
    caps.m_platform = target;
  OL::Manager::printArtifacts(caps, specs);
  return NULL;
}

static const char *arg(const char **&ap) {
  if (ap[0][2])
    return &ap[0][2];
  if (!ap[1])
    throw OU::Error("Missing argument to the -%c option", ap[0][1]);
  return *++ap;
}

int
main(int /*argc*/, const char **argv) {
  bool dump = false, containers = false, hex = false, remote = false, uncached = false;
  int seconds = 0;
  unsigned nProcs = 0;
  const char *servers = NULL, *artifacts = NULL;
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
      case 'w':
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
	seconds = atoi(arg(ap));
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
      case 'T':
	addParam("transport", ap);
	break;
      case 'n':
	nProcs = atoi(arg(ap));
	break;
      case 'l':
	OCPI::OS::logSetLevel(atoi(arg(ap)));
	break;
      case 'C':
	containers = true;
	break;
      case 'G':
	specs = true;
        // Intentional fall-thru
      case 'A':
	artifacts = arg(ap);
	break;
      case 'S':
	servers = arg(ap);
	break;
      case 'R':
	remote = true;
	break;
      case 'U':
	uncached = true;
	break;
      default:
	usage(argv0);
      }
    if (!*ap && !containers && !artifacts)
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
    if (artifacts) {
      if (*ap) {
	  fprintf(stderr,
		  "Error: can't request artifact dump (-A) and specify an xml file (%s)\n",
		  *ap);
	  return 1;
      }
      // FIXME: no way to suppress all discovery EXCEPT one manager...
      OCPI::Container::Manager::getSingleton().suppressDiscovery();
      DataTransfer::XferFactoryManager::getSingleton().suppressDiscovery();
      const char *err;
      if ((err = OU::parseList(artifacts, doTarget))) {
	fprintf(stderr, "Error processing artifact target list (\"%s\"): %s\n",
		artifacts, err);
	return 1;
      }
    }

    if (!remote)
      OR::g_suppressRemoteDiscovery = true;
    if (servers) {
      const char *err;
      if ((err = OU::parseList(servers, doServer))) {
	fprintf(stderr, "Error processing server list (\"%s\"): %s\n",
		servers, err);
	return 1;
      }
    }
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
	     " #  Model Platform    OS     OS Version  Name\n");
      for (unsigned n = 0; (c = OA::ContainerManager::get(n)); n++)
	printf("%2u  %-5s %-11s %-6s %-11s %s\n",
	       n,  c->model().c_str(), c->platform().c_str(), c->os().c_str(),
	       c->osVersion().c_str(), c->name().c_str());
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
      bool isParameter, isCached;
      if (verbose)
	fprintf(stderr, "Dump of all initial property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter, &isCached,
					   uncached); n++)
	fprintf(stderr, "Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
		isParameter ? " (parameter)" : (isCached ? " (cached)" : ""));
    }
    app.start();
    if (verbose)
      fprintf(stderr, "Application started/running\n");
    if (seconds) {
      int remaining = -seconds;
      if (verbose)
	fprintf(stderr, "Waiting %s%u seconds for application to complete\n",
		(seconds < 0) ? "up to " : "", abs(seconds));
      if (seconds < 0) { // "Negative" time is "up to"
        bool cont = true;
        while (remaining-- && cont)
          cont = app.wait(1E6);
      } else // Given a positive time
        sleep(seconds);
      if (verbose && (remaining <= 0)) // remaining would be negative if given positive seconds
	fprintf(stderr, "After %d seconds, stopping application...\n", abs(seconds));
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
      bool isParameter, isCached;
      if (verbose)
	fprintf(stderr, "Dump of all final property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter, &isCached,
					   uncached); n++)
	if (!isParameter && !isCached)
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
