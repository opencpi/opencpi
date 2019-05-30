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
#include "dlfcn.h"
#include "ocpi-config.h"
#include "OcpiUtilCppMacros.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "ContainerManager.h"

namespace OU = OCPI::Util;

#define OCPI_OPTIONS_HELP "This program loads all drivers. Supply at least one dummy argument.\n"
#include "CmdOption.h" // for convenient main program and exception handling

static int
mymain(const char **) {
  std::string path, list, name;
  // FIXME: add the "stubs" indicator to driver-list to avoid special casing ofed/ocl
  OU::format(path, "%s/%s/lib", OU::getCDK().c_str(), OCPI_CPP_STRINGIFY(OCPI_PLATFORM));
  name = (path + "/driver-list").c_str();
  const char *err;
  if ((err = (OU::file2String(list, name.c_str()))))
    throw OU::Error("Failed to open driver-list file %s: %s\n", name.c_str(), err);
  OCPI::Container::Manager::getSingleton().suppressDiscovery();
  for (OU::TokenIter ti(list.c_str()); ti.token(); ti.next()) {
    OU::format(name, "%s/libocpi_%s%s%s", path.c_str(), ti.token(),
               OCPI_DYNAMIC ? "" : "_s",
               OCPI_CPP_STRINGIFY(OCPI_DYNAMIC_SUFFIX));
    ocpiBad("Trying to load driver %s from %s", ti.token(), name.c_str());
    if (!dlopen(name.c_str(),
                strcmp(ti.token(), "ofed") && strcmp(ti.token(), "ocl") ?
                RTLD_NOW : RTLD_LAZY))
      throw OU::Error("Failed to open driver: %s", dlerror());
  }
  ocpiBad("All drivers succesfully loaded");
  return 0;
}
