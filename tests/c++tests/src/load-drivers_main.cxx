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
#include "ocpi-config.h"
#include "OcpiOsLoadableModule.h"
#include "OcpiUtilCppMacros.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "ContainerManager.h"

namespace OU = OCPI::Util;

#define OCPI_OPTIONS_HELP "This program loads all drivers. Supply at least one dummary argument"
#include "CmdOption.h" // for convenient main program and exception handling

static int
mymain(const char **) {
  OCPI::Container::Manager::getSingleton().suppressDiscovery();
  std::string path, list, name;
  OU::format(path, "%s/%s/lib", OU::getCDK().c_str(), OCPI_CPP_STRINGIFY(OCPI_PLATFORM));
  name = (path + "/driver-list").c_str();
  const char *err;
  if ((err = (OU::file2String(list, name.c_str()))))
    throw OU::Error("Failed to open driver-list file %s: %s\n", name.c_str(), err);
  for (OU::TokenIter ti(list.c_str()); ti.token(); ti.next())
    if (strcmp(ti.token(), "ofed") && strcmp(ti.token(), "ocl")) { // need stubs for these
      OU::format(name, "%s/libocpi_%s%s%s", path.c_str(), ti.token(),
		 OCPI_DYNAMIC ? "" : "_s",
		 OCPI_CPP_STRINGIFY(OCPI_DYNAMIC_SUFFIX));
      ocpiBad("Trying to load driver %s from %s", ti.token(), name.c_str());
      new OCPI::OS::LoadableModule(name);
    }
  return 0;
}
