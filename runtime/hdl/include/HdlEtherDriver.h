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

/*
  Generic Ethernet L2 "discovery" etc.
  We define L2 Ethernet discovery as sending a broadcast packet with a defined ethertype and payload,
  and receiving one or more responses back of the same ethertype, and with a payload.
  This the "found" callback is the "name" (including mac address) and response payload.
  Thus this module knows about ethernet, but not about the details of discovery payloads.
*/

#ifndef HDLETHERDRIVER_H
#define HDLETHERDRIVER_H
#include "HdlNetDriver.h"
namespace OCPI {
  namespace HDL {
    namespace Ether {
      class Device;
      class Driver : public OCPI::HDL::Net::Driver {
	friend class Device;
      protected:
	virtual ~Driver();
	// constructor
	virtual Net::Device *createDevice(OS::Ether::Interface &ifc, OS::Ether::Address &addr,
					  bool discovery, const OCPI::Util::PValue *params,
					  std::string &error);
      };
    }
  }
}
#endif
