
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

#ifndef PCIDRIVER_H
#define PCIDRIVER_H
#include <stdint.h>
#ifdef __cplusplus
#include <string>
#include "OcpiPValue.h"
#include "HdlAccess.h"


namespace OCPI {
  namespace HDL {
    namespace PCI {
#endif
#define OCFRP0_PCI_VENDOR 0x10ee
#define OCFRP0_PCI_DEVICE 0x4243
#define OCFRP0_PCI_CLASS 0x05
#define OCFRP0_PCI_SUBCLASS 0x00
#ifdef __cplusplus
      struct Bar {
	uint64_t address;
	uint64_t size;
	bool io, prefetch;
	unsigned addressSize;
      };
      class Driver {
      protected:
	virtual ~Driver();
	virtual bool
	found(const char *name, Access &cAccess, Access &dAccess,
	      std::string &endpoint, std::string &error) = 0;
      public:
	static int s_pciMemFd;
	unsigned
	search(const OCPI::Util::PValue *props, const char **exclude, std::string &error);
	bool
	open(const char *pciName, std::string &name, HDL::Access &cAccess, HDL::Access &dAccess,
	     std::string &endpoint, std::string &err);
	     
      };
    }
  }
}
#endif
#endif
