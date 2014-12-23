
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
	virtual Net::Device &createDevice(OS::Ether::Interface &ifc, OS::Ether::Address &addr,
				     bool discovery, std::string &error);
      };
    }
  }
}
#endif
