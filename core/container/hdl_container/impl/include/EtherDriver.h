
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
namespace OCPI {
  namespace HDL {
    namespace Ether {
#define OCCP_ETHER_MTYPE 0xf040
#define OCCP_ETHER_STYPE 0xf040
      typedef struct {
	uint16_t etherTypeOverlay;  // this is to align the protocol in ethernet frames.
	uint16_t length; // big endian
	uint16_t pad;
	uint8_t  typeEtc;
	uint8_t  tag;
      } EtherControlHeader;
      typedef struct {
	EtherControlHeader header;
	uint8_t mbx80;
	uint8_t mbz0;
	uint8_t mbz1;
	uint8_t maxCoalesced;
      } EtherControlNop;
      typedef struct {
	EtherControlHeader header;
	uint32_t address, data;
      } EtherControlWrite;
      typedef struct {
	EtherControlHeader header;
	uint32_t address;
      } EtherControlRead;
      typedef struct {
	EtherControlHeader header;
	uint8_t mbx40;
	uint8_t mbz0;
	uint8_t mbz1;
	uint8_t maxCoalesced;
      } EtherControlNopResponse;
      typedef struct {
	EtherControlHeader header;
	uint32_t data; // read and nop
      } EtherControlReadResponse;
      typedef struct {
	EtherControlHeader header;
      } EtherControlWriteResponse;
      typedef union {
	EtherControlHeader header;
	EtherControlNop nop;
	EtherControlWrite write;
	EtherControlRead read;
	EtherControlNopResponse nopResponse;
	EtherControlWriteResponse writeResponse;
	EtherControlReadResponse readResponse;
      } EtherControlPacket;
      typedef enum {
	NOP,
	WRITE,
	READ,
	RESPONSE,
	TYPE_LIMIT
      } EtherControlMessageType;
      typedef enum {
	OK,
	WORKER_TIMEOUT,
	ERROR,
	ETHER_TIMEOUT,
	RESPONSE_LIMIT
      } EtherControlResponse;
#define OCCP_ETHER_MESSAGE_TYPE(t_and_be) ((OCPI::HDL::Ether::EtherControlMessageType)((t_and_be) >> 4))
#define OCCP_ETHER_BYTE_ENABLES(t_and_be) ((t_and_be) & 0xf)
#define OCCP_ETHER_RESPONSE(t_and_be) ((OCPI::HDL::Ether::EtherControlResponse)((t_and_be) & 0xf))
#define OCCP_ETHER_TYPE_ETC(type, be) (((type) << 4) | ((be) & 0xf))
      const unsigned RETRIES = 10;
      const unsigned DELAYMS = 100;
      const unsigned MAX_INTERFACES = 10;

      class Driver {
	OCPI::OS::Ether::Socket *m_socket; // a pointer since we only open it on demand
	const char **m_exclude; // during discovery
	typedef std::map<const std::string, OCPI::OS::Ether::Socket *> Sockets;
	typedef Sockets::const_iterator SocketsIter;
	typedef std::pair<const std::string, OCPI::OS::Ether::Socket*> SocketPair;
	Sockets m_sockets;
	OCPI::OS::Ether::Socket *
	findSocket(OCPI::OS::Ether::Interface &ifc, std::string &error);
	unsigned 
	tryIface(OCPI::OS::Ether::Interface &ifc, OCPI::OS::Ether::Address &mac, const char **exclude,
		 std::string &name, Access &cAccess, Access &dAccess, std::string &endpoint,
		 std::string &error);
	virtual bool
	found(const char *name, Access &cAccess, Access &dAccess,
	      std::string &endpoint, std::string &error) = 0;
      protected:
	virtual ~Driver();
      public:
	unsigned
	search(const OCPI::Util::PValue *props, const char **exclude, std::string &error);
	bool
	probe(const char *which, std::string &error);
	bool
	open(const char *etherName, std::string &name, HDL::Access &cAccess, HDL::Access &dAccess,
	     std::string &endpoint, std::string &err);
      };

    }
  }
}
#endif
