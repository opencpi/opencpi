
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

#ifndef HDLPCIDRIVER_H
#define HDLPCIDRIVER_H
#ifndef __KERNEL__
#include <stdint.h>
#endif
#ifdef __cplusplus
#include <string>
#include "OcpiPValue.h"
#include "HdlDevice.h"


namespace OCPI {
  namespace HDL {
    namespace PCI {
#endif
      //#define OCPI_HDL_PCI_DIR "/dev/ocpi/pci"
#define OCPI_HDL_PCI_VENDOR_ID 0x10ee
#define OCPI_HDL_PCI_DEVICE_ID 0x4243
#define OCPI_HDL_PCI_CLASS 0x05    // PCI_BASE_CLASS_MEMORY on linux
#define OCPI_HDL_PCI_SUBCLASS 0x00 // PCI_CLASS_MEMOR_RAM & 0xff on linux
#define OCPI_HDL_SYS_PCI_DIR "/sys/bus/pci/devices"
#ifdef __cplusplus
      struct Bar {
	uint64_t address;
	uint64_t size;
	bool io, prefetch;
	unsigned addressSize;
      };
      const unsigned MAXBARS = 6;
      // Utility functions, for use even when driver class is not used.
      bool
      probePci(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
	       unsigned theSubClass, bool probe, Bar *bars, unsigned &nbars, std::string &error);

      class Driver {
	int m_pciMemFd;
	bool m_useDriver;
      protected:
	Driver();
	virtual ~Driver();
      public:
	// Grab some DMA memory, return a virtual mapping to it, and set base to the physical place
	//	void *
	//	map(uint32_t size, uint64_t &base, std::string &error);
	unsigned
	  search(const OCPI::Util::PValue *props, const char **exclude, bool discoveryOnly,
		 std::string &error);
	OCPI::HDL::Device *
        open(const char *pciName, const OCPI::Util::PValue *params, std::string &err);
	// Callback when found
	virtual bool found(OCPI::HDL::Device &dev, const char **excludes, bool discoveryOnly,
			   std::string &error) = 0;
      };
    }
  }
}
#endif
#endif
