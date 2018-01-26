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

#ifndef UTILPCI_H
#define UTILPCI_H
#include <inttypes.h>
#include <string>

#define OCPI_HDL_SYS_PCI_DIR "/sys/bus/pci/devices"

namespace OCPI {
  namespace Util {
    struct Bar {
      uint64_t address;
      uint64_t size;
      bool io, prefetch;
      unsigned addressSize;
    };
    const unsigned MAXBARS = 6;

    // Utility functions, for use even when driver class is not used.
    // vendor, device, class, subclass can be UINTMAX meaning wildcard...
    bool
    probePci(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
	     unsigned theSubClass, bool probe, Bar *bars, unsigned &nbars, std::string &error);
  }
}
#endif
