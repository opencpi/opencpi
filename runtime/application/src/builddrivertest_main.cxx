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

// This is a small test application to attempt to load all the drivers.
// It is not to be released to the end user.

#include <dlfcn.h>
#include <unistd.h>
#include <iostream>

#include "OcpiContainerApi.h"
#include "XferDriver.h"
#include "ContainerManager.h"

// Test driver loading
bool doTestDrivers() {
  bool failure(false);
  const std::string DriverList(DRIVERLIST);
  assert(NULL != dlopen(".libs/libocpi_ofed_stub.so", RTLD_NOW | RTLD_GLOBAL)); // for the ofed stuff
  size_t pos(0), lastpos(0);
  // The list will always have an extra space at the end thanks to the Makefile.
  while (std::string::npos != (pos = DriverList.find(' ', lastpos))) {
    const std::string Driver(".libs/" + DriverList.substr(lastpos, pos - lastpos) + ".so");
    std::cout << "Loading Driver '" << Driver << "': ";
    const void *handle = dlopen(Driver.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
      std::cout << "Pass" << std::endl;
    } else {
      std::cout << "FAIL: " << dlerror() << std::endl;
      failure = true;
    }
    lastpos = pos + 1;
  }
  return failure;
}

int main(int /* argc */, const char ** /* argv */) {
  // This allows us to ensure things get linked in (discussion in AV-2655):
  OCPI::Container::Manager::getSingleton().suppressDiscovery();

  // Go to the proper directory (the intermediate build one).
  // Not very robust, but not production code either.
  // I had some stuff parsing realpath("/proc/self/exe") but then got super lazy
  // and just had Makefile tell us.
  const int cd = chdir(CDTO);
  assert(0 == cd);

  return doTestDrivers();
}
