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
#include "OcpiOsTimer.h"
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


      static unsigned typeLength[] = {
	sizeof(EtherControlNop), sizeof(EtherControlWrite),
	sizeof(EtherControlRead), sizeof(EtherControlResponse)
      };
      class Device
	: public OCPI::HDL::Device,
	  public OCPI::HDL::Accessor {
	friend class Driver;
	OS::Ether::Socket *m_socket; // if !haveDriver(), shared with others...
	OS::Ether::Address m_addr;
	OS::Ether::Packet m_request;
	std::string m_error;
	bool m_discovery;
	OS::Ether::Address m_ifcAddr;
      protected:
	Device(Driver &driver, std::string &name, OS::Ether::Interface &ifc,
	       OS::Ether::Address &addr, bool discovery, std::string &error)
	  : OCPI::HDL::Device(name, "ocpi-ether-rdma"),
	    m_socket(NULL), m_addr(addr), m_discovery(discovery), m_ifcAddr(ifc.addr) {
	  if (OE::haveDriver()) {
	    m_socket = new OS::Ether::Socket(ifc, discovery ? ocpi_discovery : ocpi_master, &addr, 0, error);
	    if (error.size()) {
	      delete m_socket;
	      return;
	    }
	  } else if ((m_socket = driver.findSocket(ifc, discovery, error)) == NULL)
	    return;
	  EtherControlPacket *ecp =  (EtherControlPacket *)(m_request.payload);
	  ecp->header.tag = 0;
	  const char *cp = name.c_str();
	  if (!strncasecmp("Ether:", cp, 6))
	    cp += 6;
	  OU::formatString(m_endpointSpecific, "ocpi-ether-rdma:%s", cp);
	  m_endpointSize = ((uint64_t)1) << 32;
	  cAccess().setAccess(NULL, this, m_endpointSize - sizeof(OccpSpace));
	  dAccess().setAccess(NULL, this, 0);
	}
      public:
	~Device() {
	  if (OE::haveDriver())
	    delete m_socket;
	}
	inline OS::Ether::Address &addr() { return m_addr; }
	void request(EtherControlMessageType type, RegisterOffset offset,
		     unsigned bytes, OS::Ether::Packet &recvFrame, uint32_t *status) {
	  EtherControlHeader &ech_out =  *(EtherControlHeader *)(m_request.payload);
	  ech_out.tag++;
	  ech_out.typeEtc =
	    OCCP_ETHER_TYPE_ETC(type,
				(~(-1 << bytes) << (offset & 3)) & 0xf,
				m_discovery ? 1 : 0);
	  EtherControlResponse response = OK;
	  if (status)
	    *status = 0;
	  for (unsigned n = 0;
	       n < RETRIES &&
		 m_socket->send(m_request, typeLength[type], m_addr, 0, m_error); n++) {
	    unsigned length;
	    OS::Ether::Address addr;
	    OS::Timer timer(0, DELAYMS * 1000000);
	    // FIXME: use shared receive socket
	    ocpiDebug("Request type %u tag %u offset %u",
		      OCCP_ETHER_MESSAGE_TYPE(ech_out.typeEtc), ech_out.tag,
		      ntohl(((EtherControlRead *)m_request.payload)->address));
	    while (m_socket->receive(recvFrame, length, DELAYMS, addr, m_error)) {
	      if (addr == m_ifcAddr)
		continue;
	      EtherControlHeader &ech_in =  *(EtherControlHeader *)(recvFrame.payload);
	      ocpiDebug("response received from %s (we are %s) %x %x %x",
			addr.pretty(), m_ifcAddr.pretty(), ech_in.length, ech_in.typeEtc, ech_in.tag);
	      if (OCCP_ETHER_MESSAGE_TYPE(ech_in.typeEtc) != OCCP_RESPONSE)
		ocpiBad("Ethernet control packet from %s not a response, ignored: typeEtc 0x%x",
			addr.pretty(), ech_in.typeEtc);
	      else if (ech_in.tag != ech_out.tag)
		ocpiInfo("Ethernet control packet from %s has extraneous tag %u, expecting %u, ignored",
			 addr.pretty(),
			 ech_in.tag, ech_out.tag);
	      else if ((response = OCCP_ETHER_RESPONSE(ech_in.typeEtc)) == OK)
		return;
	      else {
		ocpiInfo("Ethernet control packet from %s got non-OK response: %u",
			 addr.pretty(), response);
		break;
	      }
	      if (!timer.expired())
		OS::sleep(2);
	    }
	    if (m_error.size())
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
			      type == OCCP_READ ? "read" : "write",
			      response == WORKER_TIMEOUT ? "worker timeout" :
			      response == ERROR ? "worker error" :
			      "ethernet timeout - no valid response");
	}

	// Shared "get" that returns value, and *status if status != NULL
	uint32_t get(RegisterOffset offset, unsigned bytes, uint32_t *status) {
	  EtherControlRead &ecr =  *(EtherControlRead *)(m_request.payload);
	  ecr.address = htonl((offset & 0xffffff) & ~3);
	  ecr.header.length = htons(sizeof(ecr)-2);
	  OS::Ether::Packet recvFrame;
	  request(OCCP_READ, offset, bytes, recvFrame, status);
	  uint32_t data = ntohl(((EtherControlReadResponse *)(recvFrame.payload))->data);
	  ocpiDebug("Accessor read received 0x%x from offset %x tag %u", data, offset, ecr.header.tag);
	  return data;
	}
	void
	set(RegisterOffset offset, unsigned bytes, uint32_t data, uint32_t *status) {
	  EtherControlWrite &ecw =  *(EtherControlWrite *)(m_request.payload);
	  ecw.address = htonl((offset & 0xffffff) & ~3);
	  ecw.data = htonl(data);
	  ecw.header.length = htons(sizeof(ecw)-2);
	  OS::Ether::Packet recvFrame;
	  request(OCCP_WRITE, offset, bytes, recvFrame, status);
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
	nop.header.typeEtc = OCCP_ETHER_TYPE_ETC(OCCP_NOP, 0xf, 1);
	nop.mbx80 = 0x80;
	nop.mbz0 = 0;
	nop.mbz1 = 0;
	nop.maxCoalesced = 1;
      }
      static bool
      checkNopResponse(EtherControlNopResponse &response, std::string &error) {
	if (response.header.length == htons(sizeof(response)-2) &&
	    response.header.typeEtc == OCCP_ETHER_TYPE_ETC(OCCP_RESPONSE, OK, 1) &&
	    response.mbx40 == 0x40 &&
	    response.mbz0 == 0 &&
	    response.mbz1 == 0 &&
	    response.maxCoalesced == 1)
	  return true;
	ocpiBad("Bad ethernet discovery response:");
	for (unsigned i = 0; i < sizeof(response); i++)
	  ocpiBad("Response byte %u: 0x%x", i, ((uint8_t*)&response)[i+2]);
	error = "Bad ethernet discovery response";
	return false;
      }
      Driver::
      ~Driver() {
      }
      OE::Socket *Driver::
      findSocket(OE::Interface &ifc, bool discovery, std::string &error) {
	SocketsIter it = m_sockets.find(ifc.name);
	OE::Socket *s;
	if (it == m_sockets.end()) {
	  s = new OE::Socket(ifc, discovery ? ocpi_discovery : ocpi_master, NULL, 0, error);
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
      // If mac == NULL, use broadcast
      unsigned Driver::
      tryIface(OE::Interface &ifc, OE::Address *mac, const char **exclude,
	       Device **dev,   // optional output arg to return the found device when mac != NULL
	       bool discovery, // is this about discovery? (broadcast OR specific probing)
	       std::string &error) {
	// Get the discovery socket for this interface
	OE::Socket *s = findSocket(ifc, discovery, error);
	if (!s)
	  return 0;
	// keep track of different macs discovered when we broadcast.
	std::set<OE::Address,OE::Address::Compare> macs;

	OE::Packet sendFrame;
	initNop(*(EtherControlNop *)(sendFrame.payload));
	const unsigned recvLength = sizeof(EtherControlNopResponse);
	unsigned count = 0;
	OE::Address &to = mac ? *mac : OE::Address::s_broadcast;
      
	// FIXME:  We need to be able to probe one while others are running?
	for (unsigned n = 0; error.empty() && n < RETRIES; n++) {
	  if (!s->send(sendFrame, sizeof(EtherControlNop), to, 0, error))
	    break;
	  OE::Packet recvFrame;
	  OE::Address devMac;
	  unsigned length;

	  OS::Timer timer(0, DELAYMS * 1000000);
	  while (s->receive(recvFrame, length, DELAYMS, devMac, error)) {
	    if (devMac == ifc.addr)
	      continue;
	    if (exclude)
	      for (const char **ap = exclude; *ap; ap++)
		if (!strcmp(*ap, devMac.pretty())) {
		  ocpiInfo("Ether device %s specifically excluded/ignored", *ap);
		  continue;
		}
	    if (length > recvLength)
	      ocpiDebug("receive truncation for interface '%s': %d > %u",
			ifc.name.c_str(), length, recvLength);
	    if (length < recvLength)
	      OS::setError(error, "probe return was short:  length was %d when %d was expected",
			   length, recvLength);
	    else if (!mac && macs.find(devMac) != macs.end()) {
	      ocpiDebug("Received redundant ethernet discovery response");
	      continue;
	    } else if (checkNopResponse(*(EtherControlNopResponse *)(recvFrame.payload), error)) {
	      if (!mac)
		macs.insert(devMac);
	      else if (devMac != to) {
		ocpiInfo("Received ethernet discovery response from wrong address");
		continue;
	      }
	      // We found one or THE one.
	      std::string name("Ether:" + ifc.name + "/" + devMac.pretty());
	      Device *d = new Device(*this, name, ifc, devMac, discovery, error);
	      if (mac) {
		*dev = d;
		return 1;
	      } else if (found(*d, error))
		count++;
	    }
	    if (!timer.expired())
	      OS::sleep(2);
	  }
	}
	if (error.size())
	  ocpiInfo("error on interface '%s' when probing for %s: %s",
		   ifc.name.c_str(), to.pretty(), error.c_str());
	else if (!count && mac)
	  ocpiInfo("no ethernet probe response on '%s' from '%s' after %u attempts %ums apart",
		   ifc.name.c_str(), to.pretty(), RETRIES, DELAYMS);
	return count;
      }

      OCPI::HDL::Device *Driver::
      open(const char *name, bool discovery, std::string &error) {
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
	      if (eif.up && eif.connected) {
		Device *dev;
		if (tryIface(eif, &addr, NULL, &dev, discovery, error) == 1)
		  return dev;
		else
		  break;
	      }
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
	    count += tryIface(eif, NULL, exclude, NULL, true, error);
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
