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
#include "cl.h"
#include "cl_ext.h"
#include "ocpi-config.h"
#include "OcpiOsDebugApi.h"
#include "OcpiOsLoadableModule.h"
#include "OcpiDriverManager.h"

namespace OS = OCPI::OS;

#define OCPI_OPTIONS_HELP \
  "Usage syntax is: ocpiocltest test\n" \
  "Exit status is 0 if OpenCL appears to be available for one or more targets\n" \

//           name      abbrev  type    value description
#define OCPI_OPTIONS \
  CMD_OPTION(verbose,    v,    Bool,   "false", "Provide verbose output during operation") \
  CMD_OPTION(loglevel,   l,    UChar,  "0",     "Set logging level used during operation") \
  CMD_OPTION(library,    L,    String,  NULL,   "Set name of OpenCL library to test") \

#include "CmdOption.h"

const char *defaultLib =
// FIXME - these should be based on autoconf
#ifdef OCPI_OS_macos
  "/System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL"
#else
 "/lib64/libOpenCL.so.1"
#endif
  ;

static int
mymain(const char **/*ap*/) {
  if (options.loglevel())
    OS::logSetLevel(options.loglevel());
  if (options.verbose() && !OCPI::OS::logWillLog(8))
    OS::logSetLevel(8);
  OCPI::Driver::ManagerManager::suppressDiscovery();
  const char *env = getenv("OCPI_HAVE_OPENCL");
  if (!env || env[0] != '1') {
    if (options.verbose())
      ocpiLog(8, "OCL test failed since OCPI_HAVE_OPENCL not set to 1");
    return 1;
  }
  env = getenv("OCPI_OPENCL_LIB");
  const char *lib = env && env[0] ? env : options.library() ? options.library() : defaultLib;

  try {
    OS::LoadableModule lm(lib, true);
    if (options.verbose() || OCPI::OS::logWillLog(8))
      ocpiLog(8, "Successfully loaded OpenCL library: \"%s\" with OCPI_OPENCL_LIB=%s",
	      lib, env && env[0] ? env : "<none>");
    void *p;
    try {
      p = lm.getSymbol ("clGetPlatformIDs");
    } catch (const std::string &s) {
      if (options.verbose() || OCPI::OS::logWillLog(8))
	ocpiLog(8, "Failed to find symbol in OpenCL library: %s", s.c_str());
      return 1;
    } catch (...) {
      if (options.verbose() || OCPI::OS::logWillLog(8))
	ocpiLog(8, "Failed to find symbol in OpenCL library \"%s\": unknown exception", lib);
      return 1;
    }
    cl_uint np;
    cl_int rc = (*(decltype(clGetPlatformIDs) *)p)(0, 0, &np);
    if (rc || np == 0) {
      if (options.verbose() || OCPI::OS::logWillLog(8)) {
	if (rc && rc != CL_PLATFORM_NOT_FOUND_KHR)
	  ocpiLog(8, "OpenCL clGetPlatformIDs query returned error: %d", rc);
	else
	  ocpiLog(8, "OpenCL clGetPlatformIDs query returned no available platforms");
      }
      return 1;
    } else if (options.verbose() || OCPI::OS::logWillLog(8))
      ocpiLog(8, "Found %zu OpenCL platform%s", (size_t)np, np == 1 ? "" : "s");
  } catch (const std::string &s) {
    if (options.verbose() || OCPI::OS::logWillLog(8))
      ocpiLog(8, "Failed to load OpenCL library: %s", s.c_str());
    return 1;
  } catch (...) {
    if (options.verbose() || OCPI::OS::logWillLog(8))
      options.bad("Missing/invalid OpenCL support library, tried  \"%s\"; unknown exception",
		  lib);
    return 1;
  }
  return 0;
}
