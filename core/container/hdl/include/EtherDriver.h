
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

#ifndef ETHERDRIVER_H
#define ETHERDRIVER_H
#include <map>
#include "OcpiOsEther.h"
#include "OcpiPValue.h"
#include "HdlAccess.h"
#include "EtherDefs.h"
namespace OCPI {
  namespace HDL {
    namespace Ether {
      const unsigned RETRIES = 10;
      const unsigned DELAYMS = 100;
      const unsigned MAX_INTERFACES = 10;

      class Device;
      class Driver {
	friend class Device;
	OCPI::OS::Ether::Socket *m_socket; // discovery socket. pointer since we only open it on demand
	const char **m_exclude;            // during discovery
	// A mapping from interface name to sockets per interface, during discovery
	typedef std::map<const std::string, OCPI::OS::Ether::Socket *> Sockets;
	typedef Sockets::const_iterator SocketsIter;
	typedef std::pair<const std::string, OCPI::OS::Ether::Socket*> SocketPair;
	Sockets m_sockets;
	// Find the discovery socket for this interface
	OCPI::OS::Ether::Socket *
	findSocket(OCPI::OS::Ether::Interface &ifc, bool discovery, std::string &error);
	// Try to find one or more devices on this interface
	// mac is NULL for broadcast
	// discovery is false only if mac is true
	unsigned 
	tryIface(OCPI::OS::Ether::Interface &ifc, OCPI::OS::Ether::Address *mac, const char **exclude,
		 Device **dev, bool discovery, std::string &error);
      protected:
	virtual ~Driver();
      public:
	unsigned
	search(const OCPI::Util::PValue *props, const char **exclude, std::string &error);
	OCPI::HDL::Device *
	open(const char *etherName, bool discovery, std::string &err);
	// callback when found
	virtual bool found(OCPI::HDL::Device &dev, std::string &error) = 0;
      };

    }
  }
}
#endif
