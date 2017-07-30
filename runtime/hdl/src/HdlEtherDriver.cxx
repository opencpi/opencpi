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

#include "HdlEtherDriver.h"

namespace OCPI {
  namespace HDL {
    namespace Ether {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
      namespace OE = OCPI::OS::Ether;

      class Device
	: public OCPI::HDL::Net::Device {
	friend class Driver;
      protected:
	Device(Driver &driver, OS::Ether::Interface &ifc, std::string &a_name,
	       OE::Address &a_addr, bool discovery, const OU::PValue *params, std::string &error)
	  : Net::Device(driver, ifc, a_name, a_addr, discovery, "ocpi-ether-rdma", 0,
			(uint64_t)1 << 32, ((uint64_t)1 << 32) - sizeof(OccpSpace), 0, params,
			error) {
	}
      public:
	~Device() {
	}
	// Load a bitstream via jtag
	bool load(const char *, std::string &error) {
	  return OU::eformat(error, "Can't load bitstreams for ethernet devices yet");
	}
	bool unload(std::string &error) {
	  return OU::eformat(error, "Can't unload bitstreams for ethernet devices yet");
	}
      };
      Driver::
      ~Driver() {
      }
      Net::Device *Driver::
      createDevice(OS::Ether::Interface &ifc, OS::Ether::Address &addr, bool discovery,
		   const OU::PValue *params, std::string &error) {
	std::string name("Ether:" + ifc.name + "/" + addr.pretty());
	Device *d = new Device(*this, ifc, name, addr, discovery, params, error);
	if (error.empty())
	  return d;
	delete d;
	return NULL;
      }
    } // namespace Ether
  } // namespace HDL
} // namespace OCPI
