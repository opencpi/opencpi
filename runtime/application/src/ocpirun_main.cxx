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
#include "XferManager.h"
#include "ContainerManager.h"
#include "OcpiOsDebug.h"
#include "OcpiOsFileSystem.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "RemoteClient.h"
#define OCPI_OPTIONS_HELP \
  "Usage is: ocpirun <options>... [<application-xml-file>]\n" \

//          name      abbrev  type    value description
//         name      abbr type    value description
#define OCPI_OPTIONS \
  CMD_OPTION(dump,       d, Bool,   0, "dump properties after execution")\
  CMD_OPTION(verbose,    v, Bool,   0, "be verbose in describing what is happening")\
  CMD_OPTION(hex,        x, Bool,   0, "print numeric property values in hex, not decimal")\
  CMD_OPTION(hidden,     h, Bool,   0, "print hidden property values") \
  CMD_OPTION_S(selection,s, String, 0, "<instance-name>=<expression>\n" \
                                       "provide selection expression for worker instance")\
  CMD_OPTION_S(model,    m, String, 0, "[<instance-name>]=<model>\n" \
	                               "set model (rcc, hdl, ocl, etc.) for worker instance")\
  CMD_OPTION_S(property, p, String, 0, "[<instance-name>]=<property>=<value>\n"\
	                               "set a property value of a worker instance") \
  CMD_OPTION_S(container,c, String, 0, "[<instance-name>]=<container-name>]\n" \
	                               "assign instance to container (name or number from -C)") \
  CMD_OPTION_S(platform, P, String, 0, "[<instance-name>]=<platform-name>\n"\
	                               "assign instance to any container of this platform type") \
  CMD_OPTION_S(scale,    N, String, 0, "<instance-name>=<crew-size>\n" \
	                               "set scale factor for scalable worker") \
  CMD_OPTION_S(worker,   w, String, 0, "<instance-name>=<implementation-name>\n" \
	                               "choose a particular worker name") \
  CMD_OPTION(processors, n, ULong,  0, "Number of RCC containers to create") \
  CMD_OPTION_S(file,     f, String, 0, "<external-name>=<file-name>\n" \
	                               "connect external port to a specific file") \
  CMD_OPTION_S(device,   D, String, 0, "<external-name>=[container/][slot/]<device-name>\n" \
	                               "connect external port to a specific device") \
  CMD_OPTION_S(url,      u, String, 0, "<external-name>=<URL>\n" \
	                               "connect external port to a URL")\
  CMD_OPTION(log_level,  l, ULong,  0, "<log-level>\n" \
	                               "set log level, overriding OCPI_LOG_LEVEL")\
  CMD_OPTION(duration,   t, Long,   0, "<seconds>\n" \
	                               "duration (seconds) to run the application if not done\n" \
                                       "first; exit status is zero in eithercases")\
  CMD_OPTION(timeout,    O, ULong,  0, "<seconds>\n"			\
	                               "time limit (seconds) to run the application; if not\n" \
                                       "done/finished before that time; an error occurs\n")\
  CMD_OPTION(list,       C, Bool,   0, "show available containers\n" \
	                               "can be used with no xml file argument - no execution") \
  CMD_OPTION_S(server,   S, String, 0, "a server to explicitly contact, without UDP discovery") \
  CMD_OPTION(remote,     R, Bool,   0, "discover/include/use remote containers") \
  CMD_OPTION_S(exclude,  X, String, 0, "a container to exclude from usage") \
  CMD_OPTION_S(transport,T, String, 0, "<instance-name>=<port-name>=<transport-name>\n" \
                                       "set transport of connection at a port") \
  CMD_OPTION_S(transfer_role,,String,0, "<instance-name>=<port-name>=<transfer-role>\n" \
                                        "set transfer role at a port") \
  CMD_OPTION_S(buffer_count,B,String,0, "<instance-name>=<port-name>=<buffercount>\n" \
                                        "set buffercount at a port") \
  CMD_OPTION_S(buffer_size, Z, String,0, "<instance-name>=<port-name>=<buffersize>\n" \
                                         "set buffer size at a port") \
  CMD_OPTION_S(target,   r, String, 0, "a target when printing artifacts/specs in path") \
  CMD_OPTION(list_artifacts,, Bool, 0, "print artifacts in path, for specified targets") \
  CMD_OPTION(list_specs,,     Bool, 0, "print specs in path, for specified targets") \
  CMD_OPTION(uncached,   U, Bool,   0, "dump cached properties uncached, ignoring cache") \
  CMD_OPTION(deployment, ,  String, 0, "XML file to read deployment from, to avoid automatic\n" \
	                               "deployment process") \
  CMD_OPTION(deploy_out, ,  String, 0, "XML file to write deployment to") \
  CMD_OPTION(no_execute, ,  Bool,   0, "Suppress execution, just determine deployment") \
  CMD_OPTION(library_path,, String, 0, "Search path for executable artifacts, overriding\n" \
	                               "the OCPI_LIBRARY_PATH environment variable") \
  CMD_OPTION(sim_dir,    ,  String, "simulations", "Directory in which simulations are run\n")\
  CMD_OPTION(dump_platforms,M,Bool, 0, "dump platform and device worker properties") \
  CMD_OPTION(sim_ticks,  ,  ULong,  0, "simulator clock cycles to allow") \
  CMD_OPTION(artifacts,  A, String, 0, "deprecated: comma-separated targets for artifacts") \
  CMD_OPTION(specs,      G, String, 0, "deprecated: comma-separated targets for specs") \
  CMD_OPTION(only_platforms,, Bool, 0, "modifies the list command to show only platforms")\
  CMD_OPTION(dump_file,   , String, 0, "dump properties in raw parsable format to this file") \
  CMD_OPTION(component,   , Bool,   0, "first non-option argument is a component name,\n" \
	                               "not an application XML file") \
  CMD_OPTION(seconds,     , Long,   0, "<seconds> -- legacy, use \"duration\" now\n") \
  CMD_OPTION(version,     , Bool,   0, "print the OpenCPI release version") \
  /**/

//  CMD_OPTION_S(simulator, H,String, 0, "Create a container with this HDL simulator")
//  CMD_OPTION(libraries,  ,  String, 0, "Search path for source libraries, implying to search for possible source workers")
//  CMD_OPTION(build,      ,  String, 0, "Build any source workers deployed")

#include "CmdOption.h"

namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OL = OCPI::Library;
namespace OC = OCPI::Container;
namespace OR = OCPI::Remote;
namespace OE = OCPI::Util::EzXml;

static void addParams(const char *name, const char **ap, OU::PValueList &params) {
  const char *err;
  while (ap && *ap)
    if ((err = params.add(name, *ap++)))
      throw OU::Error("Parameter error: %s", err);
}

static void doTarget(const char *a_target, bool specs) {
  static std::string error;
  OL::Capabilities caps;
  std::string s;
  const char *target = a_target, *slash = strrchr(a_target, '/');
  if (slash) {
    s.assign(a_target, slash - a_target);
    target = s.c_str();
    caps.m_dynamic = slash[1] == 'd';
  }
  const char *dash = strchr(target, '-');
  if (dash) {
    const char *dash1 = strchr(dash+1, '-');
    if (dash1) {
      caps.m_os.assign(target, dash - target);
      target = ++dash;
      dash = strchr(target, '-');
      if (dash) {
	caps.m_osVersion.assign(target, dash - target);
	target = ++dash;
      }
      caps.m_arch = target;
    } else {
      caps.m_osVersion.assign(target, dash - target);
      caps.m_arch = ++dash;
    }
  } else
    caps.m_platform = target;
  OL::Manager::printArtifacts(caps, specs);
}

// Return true on error
static bool setup(const char *arg, ezxml_t &xml, std::string &file,
		  OU::PValueList &params, std::string &error) {
  const char *e = NULL;
  if (arg) {
    if (options.artifacts() || options.list_artifacts() || options.specs() ||
	options.list_specs())
      return
	OU::eformat(error,
		    "can't request printing artifacts or specs and also specify an xml file (%s)",
		    arg);
    if (options.component()) {
      static char *copy;
      asprintf(&copy,
	       "<Application><instance component='%s' externals='1'/></application>", arg);
      xml = ezxml_parse_str(copy, strlen(copy));
    } else {
      file = arg;
      bool isDir;
      if (!OS::FileSystem::exists(file, &isDir) || isDir) {
	file += ".xml";
	if (!OS::FileSystem::exists(file))
	  return OU::eformat(error, "file %s (or %s.xml) does not exist", arg, arg);
      }
      if ((e = OE::ezxml_parse_file(file.c_str(), xml)))
	return OU::eformat(error, "parsing XML file %s: %s", file.c_str(), e);
    }
    if (!strcasecmp(ezxml_name(xml), "deployment")) {
      // We were given a deployment file as the command arg for the app file, so act like there
      // is a deployment file option if there isn't already
      if (!options.deployment()) {
	OL::Manager::getSingleton().suppressDiscovery();
	params.addString("deployment", file.c_str());
      }
      file.clear();
      OE::getOptionalString(xml, file, "application");
      if (file.empty())
	return
	  OU::eformat(error,
		      "Input file \"%s\" is a deployment file with no application attribute",
		      arg);
      if (!OS::FileSystem::exists(file)) {
	file += ".xml";
	if (!OS::FileSystem::exists(file))
	  return OU::eformat(error, "application file %s (or %s) does not exist", file.c_str(),
			     file.c_str());
      }
      if ((e = OE::ezxml_parse_file(file.c_str(), xml)))
	return OU::eformat(error, "parsing XML file %s: %s", file.c_str(), e);
    } else if (strcasecmp(ezxml_name(xml), "application"))
      return OU::eformat(error, "file \"%s\" is a \"%s\" XML file.", file.c_str(),
			 ezxml_name(xml));
  } else if (options.artifacts() || options.list_artifacts() || options.specs() ||
	     options.list_specs()) {
    OCPI::Driver::ManagerManager::suppressDiscovery();
    OL::getManager().enableDiscovery();
    const char **targets;
    // ========= start backwards compatibility
    struct Here {
      std::list<std::string> stargets;
      std::vector<const char *> ptargets;
      static const char *addTarget(const char *target, void *a_arg) {
	Here &me = *(Here *)a_arg;
	me.stargets.push_back(target);
	me.ptargets.push_back(me.stargets.back().c_str());
	return NULL;
      }
    } here;
    if (options.artifacts() || options.specs()) {
      if ((options.artifacts() &&
	   (e = OU::parseList(options.artifacts(), Here::addTarget, &here))) ||
	  (options.specs() &&
	   (e = OU::parseList(options.specs(), Here::addTarget, &here))))
	return OU::eformat(error, "processing artifact target list (\"%s%s\"): %s",
			   options.artifacts(), options.specs(), e);
      here.ptargets.push_back(NULL);
      targets = &here.ptargets[0];
    } else
    // ========= end backwards compatibility
      targets = options.target();
    for (const char **tp = targets; *tp; ++tp)
      doTarget(*tp, options.specs() || options.list_specs());
    return false;
  } else if (options.list()) { // no xml here
    OCPI::Driver::ManagerManager::suppressDiscovery();
    OC::getManager().enableDiscovery();
  }
  if (options.deployment())
    OL::Manager::getSingleton().suppressDiscovery();
  if (options.remote()) {
    (void)OA::ContainerManager::get(0); // force config/discovery
    OA::enableServerDiscovery();
  }
  //  OA::Container *c;
  if (options.processors())
    for (unsigned n = 1; n < options.processors(); n++) {
      std::string name;
      OU::formatString(name, "rcc%d", n);
      OA::ContainerManager::find("rcc", name.c_str());
    }
  if (options.list()) {
    (void)OA::ContainerManager::get(0); // force config/discovery
    OA::useServers(NULL, params, options.verbose());
    OA::ContainerManager::list(options.only_platforms());
  }
  return false;
}

static int mymain(const char **ap) {
  OU::PValueList params;

  if (options.version()) {
    printf("OpenCPI version is " OCPI_PACKAGE_VERSION "\n");
    return 0;
  }
  if (options.library_path()) {
    std::string env("OCPI_LIBRARY_PATH=");
    env += options.library_path();
    putenv(strdup(env.c_str()));
  }
  signal(SIGPIPE, SIG_IGN);
  if (options.log_level())
    OCPI::OS::logSetLevel(options.log_level());
  if (!*ap && !options.list() && !options.artifacts())
    return options.usage();
  if (options.verbose())
    params.addBool("verbose", true);
  if (options.uncached())
    params.addBool("uncached", true);
  if (options.hex())
    params.addBool("hex", true);
  if (options.hidden())
    params.addBool("hidden", true);
  if (options.dump())
    params.addBool("dump", true);
  if (options.dump_file())
    params.addString("dumpFile", options.dump_file());
  if (options.dump_platforms())
    params.addBool("dumpPlatforms", true);
  if (options.sim_dir())
    params.addString("simDir", options.sim_dir());
  if (options.sim_ticks())
    params.addULong("simTicks", options.sim_ticks());
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
  addParams("transferRole", options.transfer_role(n), params);
  addParams("portBufferCount", options.buffer_count(n), params);
  addParams("portBufferSize", options.buffer_size(n), params);
  addParams("scale", options.scale(n), params);
  addParams("server", options.server(n), params);
  if (options.deployment())
    params.addString("deployment", options.deployment());
  std::string file;  // the file that the application XML came from
  ezxml_t xml = NULL;
  std::string error;
  if (setup(*ap, xml, file, params, error))
    throw OU::Error("Error: %s", error.c_str());
  if (xml) try {
      std::string name;
      OU::baseName(file.c_str(), name);

      OA::ApplicationX app(xml, name.c_str(), params);
      if (options.verbose())
	fprintf(stderr,
		"Application XML parsed and deployments (containers and artifacts) chosen\n");
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
      if (!options.no_execute()) {
	app.initialize();
	app.start();

	unsigned timeout =
	  options.timeout() ? options.timeout() :
	  options.duration() < 0 ? -options.duration() : // legacy negative
	  options.duration() ? options.duration() :
	  options.seconds() < 0 ? -options.seconds() :
	  options.seconds();
	app.wait(timeout * 1000000, options.timeout() != 0);
	app.stop(); // make sure all workers are stopped after time duration or done
	// In case application specifically defines things to do that aren't in the destructor
	app.finish();
      }
  } catch (...) {
    ezxml_free(xml);
    throw;
  }
  ezxml_free(xml);
  return 0;
}
