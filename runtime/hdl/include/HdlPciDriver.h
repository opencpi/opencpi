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
// !!!!! These definitions are also in core/hdl/primitives/platform/platform_pkg.vhd
#define OCPI_HDL_PCI_ANY (~0)              // PCI_ANY_ID on linux
#define OCPI_HDL_PCI_OLD_VENDOR_ID 0x10ee  // Xilinx
#define OCPI_HDL_PCI_OLD_DEVICE_ID 0x4243  // Where did this come from?  Shep?
#define OCPI_HDL_PCI_OLD_CLASS     0x05    // PCI_BASE_CLASS_MEMORY on linux
#define OCPI_HDL_PCI_OLD_SUBCLASS  0x00    // PCI_CLASS_MEMOR_RAM on linux
#define OCPI_HDL_PCI_VENDOR_ID     0x1df7  // opencpi.org
#define OCPI_HDL_PCI_DEVICE_ID     OCPI_HDL_PCI_ANY
#define OCPI_HDL_PCI_CLASS         0x0B    // PCI_BASE_CLASS_PROCESSOR on linux
#define OCPI_HDL_PCI_SUBCLASS      0x40    // PCI_CLASS_PROCESSOR_CO on linux
#define OCPI_HDL_SYS_PCI_DIR "/sys/bus/pci/devices"
#ifdef __cplusplus
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
