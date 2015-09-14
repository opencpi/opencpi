/*
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <cstdlib>
#include <string.h>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>
#include <signal.h>
#include <unistd.h>

#include "OcpiContainerApi.h"
#include "OcpiApplication.h"
#include "OcpiLibraryManager.h"
#include "DtDriver.h"
#include "ContainerManager.h"
#include "OcpiOsDebug.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "RemoteDriver.h"
#define OCPI_OPTIONS_HELP \
  "Usage is: ocpirun <options>... [<application-xml-file>]\n" \

//          name      abbrev  type    value description
//         name      abbr type    value description
#define OCPI_OPTIONS \
  CMD_OPTION(dump,       d, Bool,   0, "dump properties after execution")\
  CMD_OPTION(verbose,    v, Bool,   0, "be verbose in describing what is happening")\
  CMD_OPTION(hex,        x, Bool,   0, "print numeric property values in hex, not decimal")\
  CMD_OPTION_S(selection,s, String, 0, "<instance-name>=<expression>\n" \
                                       "provide selection expression for worker instance")\
  CMD_OPTION_S(model,    m, String, 0, "[<instance-name>]=<model>\n" \
	                               "set model (rcc, hdl, ocl, etc.) for worker instance")\
  CMD_OPTION_S(property, p, String, 0, "[<instance-name>]=<property>=<value>\n"\
	                               "set a property value of a worker instance") \
  CMD_OPTION_S(container,c, String, 0, "[<instance-name>]=<container-name>]\n" \
	                               "assign instance to a specific container (name or number from -C)") \
  CMD_OPTION_S(platform, P, String, 0, "[<instance-name>]=<platform-name>\n"\
	                               "assign instance to any container of this platform type (see output from -C)") \
  CMD_OPTION_S(worker,   w, String, 0, "<instance-name>=<implementation-name>\n" \
	                               "choose a particular worker name") \
  CMD_OPTION(processors, n, ULong,  0, "Number of RCC containers to create") \
  CMD_OPTION_S(file,     f, String, 0, "<external-name>=<file-name>\n" \
	                               "connect external port to a specific file") \
  CMD_OPTION_S(device,   D, String, 0, "<external-name>=[container/][slot/]<device-name>\n" \
	                               "connect external port to a specific device") \
  CMD_OPTION_S(url,      u, String, 0, "<external-name>=<URL>\n" \
	                               "connect external port to a URL")\
  CMD_OPTION(loglevel,   l, ULong,  0, "<log-level>\n" \
	                               "set log level during execution, overriding OCPI_LOG_LEVEL")\
  CMD_OPTION(seconds,    t, ULong,  0, "<seconds>\n" \
	                               "specify seconds of runtime") \
  CMD_OPTION(list,       C, Bool,   0, "show available containers") \
  CMD_OPTION(servers,    S, String, 0, "comma-separated list of servers to explicitly contact (no UDP discovery)") \
  CMD_OPTION(remote,     R, Bool,   0, "discover/include/use remote containers") \
  CMD_OPTION(exclude,    X, String, 0, "comma-separated list of containers to exclude from usage") \
  CMD_OPTION_S(transport,T, String, 0, "<instance-name>=<port-name>=<transport-name>\n" \
	                               "set transport of connection at a port\n" \
	                               "if no port name, then the single output port") \
  CMD_OPTION(artifacts,  A, String, 0, "Comma-separated list of targets to print artifacts in path on stdout") \
  CMD_OPTION(deployment, ,  String, 0, "XML file to read deployment from, avoid automatic deployment") \
  CMD_OPTION(deploy_out, ,  String, 0, "XML file to write deployment to") \
  CMD_OPTION(no_execute, ,  Bool,   0, "Suppress execution, just determin deployment") \
  CMD_OPTION(path,       ,  String, 0, "Search path for executable artifacts, overriding OCPI_LIBRARY_PATH environment") \
  CMD_OPTION(libraries,  ,  String, 0, "Search path for source libraries, implying to search for possible source workers")\
  CMD_OPTION(build,      ,  String, 0, "Build any source workers deployed")\
  CMD_OPTION(sim_dir,    ,  String, "simtest", "Directory in which to run simulations")\
  CMD_OPTION(simulator,  ,  String, 0, "Run this simulator for this execution")\
  CMD_OPTION(art_lib_path,L,String, 0, "Specify/override OCPI_LIBRARY_PATH") \
  /**/
#include "CmdOption.h"
#include "RemoteServer.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OL = OCPI::Library;
namespace OR = OCPI::Remote;
namespace OE = OCPI::Util::EzXml;

static void addParam(const char *name, const char *value, std::vector<OA::PValue> &params) {
  params.push_back(OA::PVString(name, value));
}

static void addParams(const char *name, const char **ap, std::vector<OA::PValue> &params) {
  while (ap && *ap)
    addParam(name, *ap++, params);
}

static const char *doServer(const char *server, void *) {
  static std::string error;
  return OR::useServer(server, options.verbose(), NULL, error) ? error.c_str() : NULL;
}
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
  OL::Manager::printArtifacts(caps);
  return NULL;
}

#if 0
static const char *arg(const char **&ap) {
  if (ap[0][2])
    return &ap[0][2];
  if (!ap[1])
    throw OU::Error("Missing argument to the -%c option", ap[0][1]);
  return *++ap;
}
#endif
static int mymain(const char **ap) {
  std::vector<OA::PValue> params;

  if (options.art_lib_path()) {
    std::string env("OCPI_LIBRARY_PATH=");
    env += options.art_lib_path();
    putenv(strdup(env.c_str()));
  }
  signal(SIGPIPE, SIG_IGN);
  if (options.loglevel())
    OCPI::OS::logSetLevel(options.loglevel());
  if (!*ap && !options.list() && !options.artifacts())
    return options.usage();
  if (options.verbose())
    params.push_back(OA::PVBool("verbose", true));
  if (options.hex())
    params.push_back(OA::PVBool("hex", true));
  size_t n;
  addParams("worker", options.worker(n), params);
  addParams("selection", options.selection(n), params);
  addParams("model", options.model(n), params);
  addParams("property", options.property(n), params);
  addParams("container", options.container(n), params);
  addParams("platform", options.platform(n), params);
  addParams("file", options.file(n), params);
  addParams("device", options.device(n), params);
  addParams("url", options.url(n), params);
  addParams("transport", options.transport(n), params);
  if (options.deployment())
    addParam("deployment", options.deployment(), params);
  if (params.size())
    params.push_back(OA::PVEnd);
  
  std::string file;  // the file that the application XML came from
  ezxml_t xml = NULL;
  if (*ap) {
    if (options.artifacts()) {
      fprintf(stderr, "Error: can't request artifact dump (-A) and specify an xml file (%s)\n",
	      *ap);
      return 1;
    }
    file =*ap;
    if (!OS::FileSystem::exists(file)) {
      file += ".xml";
      if (!OS::FileSystem::exists(file)) {
	fprintf(stderr, "Error: file %s (or %s.xml) does not exist\n", *ap, *ap);
	return 1;
      }
    }
    const char *err;
    if ((err = OE::ezxml_parse_file(file.c_str(), xml))) {
      fprintf(stderr, "Error parsing XML file %s: %s\n", file.c_str(), err);
      return 1;
    }
    if (!strcasecmp(ezxml_name(xml), "deployment")) {
      file.clear();
      OE::getOptionalString(xml, file, "application");
      ezxml_free(xml); // we only used it to grab the app attribute
      if (file.empty()) {
	fprintf(stderr, "Input file, \"%s\" is a deployment file with no application attribute",
		*ap);
	return 1;
      }
      if (!OS::FileSystem::exists(file)) {
	file += ".xml";
	if (!OS::FileSystem::exists(file)) {
	  fprintf(stderr, "Error: application file %s (or %s) does not exist\n", file.c_str(),
		  file.c_str());
	  return 1;
	}
      }
      if ((err = OE::ezxml_parse_file(file.c_str(), xml))) {
	fprintf(stderr, "Error parsing XML file %s: %s\n", file.c_str(), err);
	return 1;
      }
    } else if (strcasecmp(ezxml_name(xml), "application")) {
      fprintf(stderr, "Error: file \"%s\" is a \"%s\" XML file.\n", file.c_str(),
	      ezxml_name(xml));
      return 1;
    }
  } else if (options.artifacts()) {
    // FIXME: no way to suppress all discovery EXCEPT one manager...
    OCPI::Container::Manager::getSingleton().suppressDiscovery();
    DataTransfer::XferFactoryManager::getSingleton().suppressDiscovery();
    const char *err;
    if ((err = OU::parseList(options.artifacts(), doTarget))) {
      fprintf(stderr, "Error processing artifact target list (\"%s\"): %s\n",
	      options.artifacts(), err);
      return 1;
    }
    return 0;
  }
  if (options.deployment())
    OCPI::Library::Manager::getSingleton().suppressDiscovery();

  if (!options.remote())
    OR::g_suppressRemoteDiscovery = true;
  if (options.servers()) {
    const char *err;
    if ((err = OU::parseList(options.servers(), doServer))) {
      fprintf(stderr, "Error processing server list (\"%s\"): %s\n",
	      options.servers(), err);
      return 1;
    }
  }
  OA::Container *c;
  if (options.processors())
    for (unsigned n = 1; n < options.processors(); n++) {
      std::string name;
      OU::formatString(name, "rcc%d", n);
      OA::ContainerManager::find("rcc", name.c_str());
    }
  if (options.list()) {
    (void)OA::ContainerManager::get(0); // force config
    printf("Available containers:\n"
	   " #  Model Platform    OS     OS Version  Name\n");
    for (unsigned n = 0; (c = OA::ContainerManager::get(n)); n++)
      printf("%2u  %-5s %-11s %-6s %-11s %s\n",
	     n,  c->model().c_str(), c->platform().c_str(), c->os().c_str(),
	     c->osVersion().c_str(), c->name().c_str());
    fflush(stdout);
  } else if (options.verbose()) {
    for (unsigned n = 0; (c = OA::ContainerManager::get(n)); n++)
      fprintf(stderr, "%s%s [model: %s os: %s platform: %s]", n ? ", " : "Available containers are: ",
	      c->name().c_str(), c->model().c_str(), c->os().c_str(), c->platform().c_str());
    fprintf(stderr, "\n");
  }
    
  if (!xml)
    return 0;
  std::string name;
  OU::baseName(file.c_str(), name);
  
  OA::ApplicationX app(xml, name.c_str(), params.size() ? &params[0] : NULL);
  if (options.verbose())
    fprintf(stderr,
	    "Application XML parsed and deployments (containers and implementations) chosen\n");
  if (options.deploy_out()) {
    std::string dfile;
    if (*options.deploy_out())
      dfile = options.deploy_out();
    else {
      OU::baseName(file.c_str(), dfile);
      dfile += "-deploy.xml";
    }
    app.dumpDeployment(file.c_str(), dfile);
  }
  if (options.no_execute())
    return 0;
  app.initialize();
  if (options.verbose())
    fprintf(stderr,
	    "Application established: containers, workers, connections all created\n"
	    "Communication with the application established\n");
  if (options.dump()) {
    std::string name, value;
    bool isParameter;
    if (options.verbose())
      fprintf(stderr, "Dump of all initial property values:\n");
    for (unsigned n = 0; app.getProperty(n, name, value, options.hex(), &isParameter); n++)
      fprintf(stderr, "Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
	      isParameter ? " (parameter)" : "");
  }
  app.start();
  if (options.verbose())
    fprintf(stderr, "Application started/running\n");
  if (options.seconds()) {
    if (options.verbose())
      fprintf(stderr, "Waiting %u seconds for application to complete\n", options.seconds());
    sleep(options.seconds());
    if (options.verbose())
      fprintf(stderr, "After %u seconds, stopping application...\n", options.seconds());
    app.stop();
  } else {
    if (options.verbose())
      fprintf(stderr, "Waiting for application to be finished (no timeout)\n");
    app.wait();
    if (options.verbose())
      fprintf(stderr, "Application finished\n");
  }
  // In case the application specifically defines things to do that aren't in the destructor
  app.finish();
  if (options.dump()) {
    std::string name, value;
    bool isParameter;
    if (options.verbose())
      fprintf(stderr, "Dump of all final property values:\n");
    for (unsigned n = 0; app.getProperty(n, name, value, options.hex(), &isParameter); n++)
      if (!isParameter)
	fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
  }
  return 0;
}

int
main(int /*argc*/, const char **argv) {
  return options.main(argv, mymain);
}
