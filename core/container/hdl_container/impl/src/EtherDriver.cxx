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

#include <arpa/inet.h>
#include <set>
#include "OcpiOsMisc.h"
#include "OcpiUtilMisc.h"
#include "HdlAccess.h"
#include "HdlOCCP.h"
#include "EtherDriver.h"

namespace OCPI {
  namespace HDL {
    namespace Ether {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
      namespace OE = OCPI::OS::Ether;


      class EtherAccessor : public Accessor {
	uint8_t m_tag;
	OS::Ether::Socket *m_ownSocket;
	OS::Ether::Socket &m_socket;
	OS::Ether::Address m_addr; // not a reference, a copy
	OS::Ether::Packet m_request;
	std::string m_error;

      public:
	// Create a new socket based on interface.  Error indicated in "error"
	EtherAccessor(OS::Ether::Interface &ifc, OS::Ether::Address &addr, std::string &error)
	  : m_ownSocket(new OS::Ether::Socket(ifc, error, OCCP_ETHER_STYPE, OCCP_ETHER_MTYPE)),
	    m_socket(*m_ownSocket), m_addr(addr)
	{
	  EtherControlPacket *ecp =  (EtherControlPacket *)(m_request.payload-2);
	  ecp->header.tag = 0;
	}
	// Use provided socket.  Error indicated in "error"
	EtherAccessor(OS::Ether::Socket &socket, OS::Ether::Address &addr)
	  : m_ownSocket(NULL), m_socket(socket), m_addr(addr)
	{
	  EtherControlPacket *ecp =  (EtherControlPacket *)(m_request.payload-2);
	  ecp->header.tag = 0;
	}
	~EtherAccessor() { delete m_ownSocket; }
	inline OS::Ether::Address &addr() { return m_addr; }
	void request(EtherControlMessageType type, RegisterOffset offset,
		     unsigned bytes, OS::Ether::Packet &recvFrame, uint32_t *status) {
	  EtherControlHeader &ech_out =  *(EtherControlHeader *)(m_request.payload-2);
	  ech_out.tag++;
	  ech_out.typeEtc = OCCP_ETHER_TYPE_ETC(type, (~(-1 << bytes) << (offset & 3)) & 0xf);
	  EtherControlResponse response = OK;
	  if (status)
	    *status = 0;
	  for (unsigned n = 0;
	       n < RETRIES &&
		 m_socket.send(m_request, sizeof(EtherControlPacket)-2, m_addr, 0, m_error); n++) {
	    unsigned length;
	    // FIXME: use shared receive socket
	    if (m_socket.receive(recvFrame, length, DELAYMS, m_error)) {
	      EtherControlHeader &ech_in =  *(EtherControlHeader *)(recvFrame.payload-2);
	      if (OCCP_ETHER_MESSAGE_TYPE(ech_in.typeEtc) != RESPONSE)
		ocpiBad("Ethernet control packet from %s not a response, ignored",
			OS::Ether::Address(recvFrame.header.source).pretty());
	      else if (ech_in.tag != ech_out.tag)
		ocpiInfo("Ethernet control packet from %s has extraneous tag %u, expecting %u, ignored",
			 OS::Ether::Address(recvFrame.header.source).pretty(),
			 ech_in.tag, ech_out.tag);
	      else if (OCCP_ETHER_RESPONSE(ech_in.typeEtc) == OK)
		return;
	      else {
		response = OCCP_ETHER_RESPONSE(ech_in.typeEtc);
		ocpiInfo("Ethernet control packet from %s got non-OK response: %u",
			 OS::Ether::Address(recvFrame.header.source).pretty(), response);
		break;
	      }
	    } else if (m_error.size())
	      ocpiBad("Ethernet Control Response receive error: %s",
		      m_error.c_str());
	    else
	      ocpiInfo("Timeout after sending ethernet control packet to: %s",
		       m_addr.pretty());
	  }
	  if (response == OK)
	    response = ETHER_TIMEOUT;
	  if (response != OK)
	    if (status)
	      *status =
		response == WORKER_TIMEOUT ? OCCP_STATUS_READ_TIMEOUT :
		response == ERROR ? OCCP_STATUS_READ_ERROR :
		OCCP_STATUS_ACCESS_ERROR;
	    else
	      throw OU::Error("HDL Ethernet Property %s error: %s",
			      type == READ ? "read" : "write",
			      response == WORKER_TIMEOUT ? "worker timeout" :
			      response == ERROR ? "worker error" :
			      "ethernet timeout - no valid response");
	}

	// Shared "get" that returns value, and *status if status != NULL
	uint32_t get(RegisterOffset offset, unsigned bytes, uint32_t *status) {
	  EtherControlRead &ecr =  *(EtherControlRead *)(m_request.payload-2);
	  ecr.address = htonl(offset & ~3);
	  ecr.header.length = htons(sizeof(ecr)-2);
	  OS::Ether::Packet recvFrame;
	  request(READ, offset, bytes, recvFrame, status);
	  uint32_t data = ntohl(((EtherControlReadResponse *)(recvFrame.payload-2))->data);
	  ocpiDebug("Accessor read received 0x%x from offset %d", data, offset);
	  return data;
	}
	void
	set(RegisterOffset offset, unsigned bytes, uint32_t data, uint32_t *status) {
	  EtherControlWrite &ecw =  *(EtherControlWrite *)(m_request.payload-2);
	  ecw.address = htonl(offset & ~3);
	  ecw.data = htonl(data);
	  ecw.header.length = htons(sizeof(ecw)-2);
	  OS::Ether::Packet recvFrame;
	  request(WRITE, offset, bytes, recvFrame, status);
	}
      public:
	uint64_t get64(RegisterOffset offset, uint32_t *status) {
	  union {
	    uint64_t u64;
	    uint32_t u32[sizeof(uint64_t) / sizeof(uint32_t)];
	  } u;
	  u.u32[0] = get(offset, sizeof(uint32_t), status);
	  if (!status || !*status)
	    u.u32[1] = get(offset + sizeof(uint32_t), sizeof(uint32_t), status);
	  return u.u64;
	}
	uint32_t get32(RegisterOffset offset, uint32_t *status) {
	  return get(offset, sizeof(uint32_t), status);
	}
	uint16_t get16(RegisterOffset offset, uint32_t *status) {
	  return (uint16_t)get(offset, sizeof(uint16_t), status);
	}
	uint8_t get8(RegisterOffset offset, uint32_t *status) {
	  return (uint8_t)get(offset, sizeof(uint8_t), status);
	}
	void getBytes(RegisterOffset offset, uint8_t *buf, unsigned length, uint32_t *status) {
	  while (length) {
	    unsigned bytes = sizeof(uint32_t) - (offset & 3); // bytes in word
	    if (bytes > length)
	      bytes = length;
	    uint32_t val = get(offset, bytes, status);
	    if (status && *status)
	      return;
	    memcpy(buf, (uint8_t*)&val + (offset & 3), bytes);
	    length -= bytes;
	    buf += bytes;
	    offset += bytes;
	  }
	}
	void set64(RegisterOffset offset, uint64_t val, uint32_t *status) {
	  set(offset, sizeof(uint32_t), (uint32_t)val, status);
	  if (!status || !*status)
	    set(offset + sizeof(uint32_t), sizeof(uint32_t), (uint32_t)(val >> 32), status);
	}
	void set32(RegisterOffset offset, uint32_t val, uint32_t *status) {
	  set(offset, sizeof(uint32_t), val, status);
	}
	void set16(RegisterOffset offset, uint16_t val, uint32_t *status) {
	  set(offset, sizeof(uint16_t), val << ((offset & 3) * 8), status);
	}
	void set8(RegisterOffset offset, uint8_t val, uint32_t *status) {
	  set(offset, sizeof(uint8_t), val << ((offset & 3) * 8), status);
	}
	void setBytes(RegisterOffset offset, const uint8_t *buf, unsigned length, uint32_t *status)  {
	  while (length) {
	    unsigned bytes = sizeof(uint32_t) - (offset & 3); // bytes in word
	    if (bytes > length)
	      bytes = length;
	    uint32_t data;
	    memcpy((uint8_t*)&data + (offset & 3), buf, bytes);
	    set(offset, bytes, data, status);
	    if (status && *status)
	      return;
	    length -= bytes;
	    buf += bytes;
	    offset += bytes;
	  }
	}
      };
      static void
      initNop(EtherControlNop &nop) {
	nop.header.length = htons(sizeof(nop)-2);
	nop.header.typeEtc = OCCP_ETHER_TYPE_ETC(NOP, 0xf);
	nop.mbx80 = 0x80;
	nop.mbz0 = 0;
	nop.mbz1 = 0;
	nop.maxCoalesced = 1;
      }
      static bool
      checkNopResponse(EtherControlNopResponse &response, std::string &error) {
	if (response.header.length == htons(sizeof(response)-2) &&
	    response.header.typeEtc == OCCP_ETHER_TYPE_ETC(RESPONSE, OK) &&
	    response.mbx40 == 0x40 &&
	    response.mbz0 == 0 &&
	    response.mbz1 == 0 &&
	    response.maxCoalesced == 1)
	  return true;
	ocpiBad("Bad ethernet discovery response:");
	for (unsigned i = 0; i < sizeof(response)-2; i++)
	  ocpiBad("Response byte %u: 0x%x", i, ((uint8_t*)&response)[i+2]);
	error = "Bad ethernet discovery response";
	return false;
      }
      Driver::
      ~Driver() {
      }
      OE::Socket *Driver::
      findSocket(OE::Interface &ifc, std::string &error) {
	SocketsIter it = m_sockets.find(ifc.name);
	OE::Socket *s;
	if (it == m_sockets.end()) {
	  s = new OE::Socket(ifc, error, OCCP_ETHER_MTYPE, OCCP_ETHER_STYPE);
	  if (error.size()) {
	    delete s;
	    return NULL;
	  }
	  m_sockets[ifc.name] = s;
	} else
	  s = it->second;
	return s;
      }
      // Try to reach the target on the given interface.
      // mac may be broadcast or specific
      unsigned Driver::
      tryIface(OE::Interface &ifc, OE::Address &mac, const char **exclude,
	       std::string &name, Access &cAccess, Access &dAccess, std::string &endpoint,
	       std::string &error) {
	OE::Socket *s = findSocket(ifc, error);
	if (!s)
	  return 0;
	// keep track of different macs discovered when we broadcast.
	std::set<OE::Address,OE::Address::Compare> macs;

	OE::Packet sendFrame;
	initNop(*(EtherControlNop *)(sendFrame.payload-2));
	const unsigned recvLength = sizeof(EtherControlNopResponse)-2;
	unsigned count = 0;
      
	// FIXME:  We need to be able to probe one while others are running?
	for (unsigned n = 0; error.empty() && n < RETRIES; n++) {
	  if (!s->send(sendFrame, sizeof(EtherControlNop)-2, mac, 0, error))
	    break;
	  OE::Packet recvFrame;
	  unsigned length;

	  if (s->receive(recvFrame, length, DELAYMS, error)) {
	    OE::Address devMac(recvFrame.header.source);
	    if (exclude)
	      for (const char **ap = exclude; *ap; ap++)
		if (!strcmp(*ap, devMac.pretty())) {
		  ocpiInfo("Ether device %s specifically excluded/ignored", *ap);
		  goto skipit; // continue(2);
		}
	    if (length > recvLength)
	      ocpiDebug("receive truncation for interface '%s': %d > %u",
			ifc.name.c_str(), length, recvLength);
	    if (length < recvLength)
	      OS::setError(error, "probe return was short:  length was %d when %d was expected",
			   length, recvLength);
	    else if (checkNopResponse(*(EtherControlNopResponse *)(recvFrame.payload-2), error)) {
	      if (mac.broadcast()) {
		if (macs.find(devMac) != macs.end()) {
		  ocpiDebug("Received redundant ethernet discovery response");
		  continue;
		}
		macs.insert(devMac);
	      } else if (mac != devMac) {
		ocpiInfo("Received ethernet discovery response from wrong address");
		continue;
	      }
	      cAccess.setAccess(NULL, new EtherAccessor(*s, devMac));
	      name = "Ether:" + ifc.name + "/" + devMac.pretty();
	      if (!mac.broadcast())
		return 1;
	      std::string endpoint;
	      if (found(name.c_str(), cAccess, dAccess, endpoint, error))
		count++;
	    }
	  }
	skipit:;
	}
	if (error.size())
	  ocpiInfo("error on interface '%s' when probing for %s: %s",
		   ifc.name.c_str(), mac.pretty(), error.c_str());
	else if (!count && !mac.broadcast())
	  ocpiInfo("no ethernet probe response on '%s' from '%s' after %u attempts %ums apart",
		   ifc.name.c_str(), mac.pretty(), RETRIES, DELAYMS);
	return count;
      }
      bool Driver::
      probe(const char *which, std::string &error) {
	std::string name;
	Access cAccess, dAccess;
	std::string endpoint;
	if (open(which, name, cAccess, dAccess, endpoint, error))
	  return found(name.c_str(), cAccess, dAccess, endpoint, error);
	return false;
      }

      // This entry point is for utilities, etc., not for creation.
      bool Driver::
      open(const char *name, std::string &cName, HDL::Access &cAccess,
	   HDL::Access &dAccess, std::string &endpoint, std::string &error) {
	const char *slash = strchr(name, '/');
	std::string iName;
	if (slash) {
	  iName.assign(name,  slash - name);
	  name = slash + 1;
	}
	OE::Address addr(name);
	if (addr.error())
	  error = "Invalid ethernet address in ethernet device name";
	else {
	  OE::IfScanner ifs(error);
	  if (error.empty()) {
	    OE::Interface eif;
	    while (ifs.getNext(eif, error, iName.size() ? iName.c_str() : NULL))
	      if (eif.up && eif.connected)
		if (tryIface(eif, addr, NULL, cName, cAccess, dAccess, endpoint, error) == 1)
		  return true;
		else
		  break;
	    if (error.empty())
	      OU::formatString(error, "HDL ether platform %s not found", name);
	  }
	}
	return false;
      }

      unsigned Driver::
      search(const OU::PValue *props, const char **exclude, std::string &error) {
	unsigned count = 0;
	OE::IfScanner ifs(error);
	if (error.size())
	  return 0;
	const char *ifName = NULL;
	OU::findString(props, "interface", ifName);
	OE::Interface eif;
	while (ifs.getNext(eif, error, ifName)) {
	  if (eif.up && eif.connected) {
	    Access cAccess, dAccess;
	    std::string name, endpoint;
	    count += tryIface(eif, OE::Address::s_broadcast, exclude,
			      name, cAccess, dAccess, endpoint, error);
	    if (error.size()) {
	      ocpiInfo("Error during ethernet discovery on '%s': %s",
		       eif.name.c_str(), error.c_str());
	      error.clear();
	    }
	  } else
	    ocpiInfo("Interface '%s' is %s and %s",
		     eif.name.c_str(), eif.up ? "up" : "down",
		     eif.connected ? "connected" : "not connected");
	  if (ifName)
	    break;
	}
	if (error.size())
	  ocpiInfo("Error during ethernet discovery on '%s': %s",
		   eif.name.c_str(), error.c_str());
	return count;
      }
    } // namespace Ether
  } // namespace HDL
} // namespace OCPI
