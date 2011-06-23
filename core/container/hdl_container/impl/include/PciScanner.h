
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
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

#ifndef PCISCANNER_H
#define PCISCANNER_H
#include <stdint.h>
namespace OCPI {
  namespace PCI {
    struct Bar {
      uint64_t address;
      uint64_t size;
      bool io, prefetch;
      unsigned addressSize;
    };
    class Driver {
    protected:
      virtual ~Driver();
    public:
      virtual bool found(const char *name, Bar *bars, unsigned nBars) = 0;
    };
    // See if the device with the given name is a good one.
    // nbars on input is size of Bars array.
    // nbars on output is number of good bars found
    // Return an error string or null on success
    bool
      probe(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
            unsigned theSubClass, Bar *bars, unsigned &nbars, const char *&err);
    // Search for devices of this vendor/device/class/subclass
    // call the "found" function for each such device,
    // set the output arg "count" to the number of devices found,
    // return an error string if something went wrong.
    const char *
      search(const char **exclude,
             unsigned vendor, unsigned device, unsigned pciClass, unsigned subClass,
                       Driver &driver, unsigned &count);
  }
}
#endif
