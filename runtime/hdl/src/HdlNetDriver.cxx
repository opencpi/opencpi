/*
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
// linux: (mac has this as default) sudo ifconfig lo multicast
// mac: sudo route -v add -net -ifscope lo0 224.0.0.0 127.0.0.1 240.0.0.0 
// linux: sudo route add -net 224.0.0.0 netmask 240.0.0.0 dev lo

#include <assert.h>
#include <arpa/inet.h>
#include <set>
#include "OcpiOsMisc.h"
#include "HdlNetDriver.h"

namespace OCPI {
  namespace HDL {
    namespace Net {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
      namespace OE = OCPI::OS::Ether;


      unsigned typeLength[] = {
	sizeof(EtherControlNop), sizeof(EtherControlWrite),
	sizeof(EtherControlRead), sizeof(EtherControlResponse)
      };
      Device::
      Device(Driver &driver, OE::Interface &ifc, std::string &name, OS::Ether::Address &devAddr,
	     bool discovery, const char *data_proto, unsigned delayms, uint64_t ep_size,
	     uint64_t controlOffset, uint64_t dataOffset, std::string &error)
	: OCPI::HDL::Device(name, data_proto),
	  m_socket(NULL), m_devAddr(devAddr), m_discovery(discovery), m_delayms(delayms) {
	// We need to get a socket to talk to this device.
	// If we are at the ethernet level AND we don't a driver,
	// we must share the socket for all devices on the same interface
	if (devAddr.isEther() && !OE::haveDriver())
	  m_socket = driver.findSocket(ifc, discovery, error);
	else
	  m_socket = new OE::Socket(ifc, discovery ? ocpi_discovery : ocpi_master, &devAddr, 0, error);
	if (error.length())
	  return;
	EtherControlPacket *ecp =  (EtherControlPacket *)(m_request.payload);
	ecp->header.tag = 0;
	OU::formatString(m_endpointSpecific, "%s:%s", data_proto, name.c_str());
	m_endpointSize = ep_size;
	cAccess().setAccess(NULL, this, OCPI_UTRUNCATE(RegisterOffset, controlOffset));
	dAccess().setAccess(NULL, this, OCPI_UTRUNCATE(RegisterOffset, dataOffset));
	init(error);
      }
      Device::
      ~Device() {
	if (!m_devAddr.isEther() || OE::haveDriver())
	  delete m_socket;
      }
      // Networks only push.
      uint32_t Device::
      dmaOptions(ezxml_t /*icImplXml*/, ezxml_t /*icInstXml*/, bool isProvider) {
	return 1 << (isProvider ? OCPI::RDT::ActiveFlowControl : OCPI::RDT::ActiveMessage);
      }
      void Device::
      request(EtherControlMessageType type, RegisterOffset offset,
	      size_t bytes, OS::Ether::Packet &recvFrame, uint32_t *status,
	      size_t extra, unsigned delayms) {
	if (m_isFailed)
	  throw OU::Error("HDL::Net::Device::request after previous failure");
	EtherControlHeader &ech_out =  *(EtherControlHeader *)(m_request.payload);
	ocpiDebug("Net::Driver request: delay %u m_delay %u tag %u",
		  delayms, m_delayms, ech_out.tag);
	ech_out.pad = 0;
	ech_out.tag++;
	if (!delayms)
	  delayms = m_delayms;
	ech_out.typeEtc =
	  OCCP_ETHER_TYPE_ETC(type,
			      (~(-1 << bytes) << (offset & 3)) & 0xf,
			      m_discovery ? 1 : 0, extra ? 1 : 0);
	EtherControlResponse response = OK;
	if (status)
	  *status = 0;
	if (!delayms)
	  delayms = DELAYMS;
	for (unsigned n = 0;
	     n < RETRIES &&
	       m_socket->send(m_request, ntohs(ech_out.length)+2, m_devAddr, 0, NULL, m_error); n++) {
	  size_t length;
	  OS::Ether::Address addr;
	  uint64_t ns = delayms * (uint64_t)1000000;
	  OS::Timer timer((uint32_t)(ns / 1000000), (uint32_t)(ns % 1000000));
	  // FIXME: use shared receive socket
	  ocpiDebug("Sent request type %u tag %u offset %u delay %u",
		    OCCP_ETHER_MESSAGE_TYPE(ech_out.typeEtc), ech_out.tag,
		    ntohl(((EtherControlRead *)m_request.payload)->address), delayms);
	  while (m_socket->receive(recvFrame, length, delayms, addr, m_error)) {
	    EtherControlHeader &ech_in =  *(EtherControlHeader *)(recvFrame.payload);
	    if (length < (unsigned)(ntohs(ech_in.length)) + 2)
	      ocpiBad("Ethernet control packet too short: got %zu, while expecting at least %u",
		      length, ntohs(ech_in.length) + 2);
	    ocpiDebug("response received from %s %x %x tag %u",
		      addr.pretty(), ech_in.length, ech_in.typeEtc, ech_in.tag);
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
	    ocpiInfo("Timeout after sending network control packet to: %s",
		     m_devAddr.pretty());
	}
	if (response == OK)
	  response = ETHER_TIMEOUT;
	if (response != OK) {
	  if (status)
	    *status =
	      response == WORKER_TIMEOUT ? OCCP_STATUS_READ_TIMEOUT :
	      response == WORKER_BUSY ? OCCP_STATUS_READ_FAIL :
	      response == ERROR ? OCCP_STATUS_READ_ERROR :
	      OCCP_STATUS_ACCESS_ERROR;
	  else {
	    m_isFailed = true;
	    throw OU::Error("HDL network %s error: %s",
			    extra ? "command" : (type == OCCP_READ ? "read" :
						 (type == OCCP_WRITE ? "write" : "nop")),
			    response == WORKER_TIMEOUT ? "worker timeout" :
			    response == WORKER_BUSY ? "worker busy" :
			    response == ERROR ? "worker error" :
			    "ethernet timeout - no valid response");
	  }
	}
      }

      // Shared "get" that returns value, and *status if status != NULL
      uint32_t Device::
      get(RegisterOffset offset, size_t bytes, uint32_t *status) {
	ocpiDebug("Accessor read for offset 0x%zx of %zu bytes", offset, bytes);
	EtherControlRead &ecr =  *(EtherControlRead *)(m_request.payload);
	ecr.address = htonl((offset & 0xffffff) & ~3);
	ecr.header.length = htons((short)(sizeof(ecr)-2));
	OS::Ether::Packet recvFrame;
	request(OCCP_READ, offset, bytes, recvFrame, status);
	uint32_t data = ntohl(((EtherControlReadResponse *)(recvFrame.payload))->data);
	uint32_t r = bytes == 4 ? data : (data >> ((offset&3) * 8)) & ~(UINT32_MAX << (bytes*8));
	ocpiDebug("Accessor read received 0x%x from offset %zx tag %u return %x",
		  data, offset, ecr.header.tag, r);
	return r;
      }
      void Device::
      set(RegisterOffset offset, size_t bytes, uint32_t data, uint32_t *status) {
	ocpiDebug("Accessor write for offset 0x%zx of %zu bytes", offset, bytes);
	EtherControlWrite &ecw =  *(EtherControlWrite *)(m_request.payload);
	ecw.address = htonl((offset & 0xffffff) & ~3);
	ecw.data = htonl(data << ((offset & 3) * 8));
	ecw.header.length = htons((short)(sizeof(ecw)-2));
	OS::Ether::Packet recvFrame;
	request(OCCP_WRITE, offset, bytes, recvFrame, status);
      }
      void Device::
      command(const char *cmd, size_t bytes, char *response, size_t rlen, unsigned delayms) {
	EtherControlHeader &eh_out =  *(EtherControlHeader *)(m_request.payload);
	eh_out.length = htons((short)(sizeof(eh_out)-2 + bytes));
	if (cmd && bytes)
	  memcpy((void*)(&eh_out+1), cmd, bytes);
	OS::Ether::Packet recvFrame;
	request(OCCP_NOP, 0, 0, recvFrame, NULL, bytes, delayms);
	EtherControlHeader &eh_in =  *(EtherControlHeader *)(recvFrame.payload);
	if (response) {
	  size_t length = ntohs(eh_in.length) - (sizeof(eh_in) - 2);
	  memcpy(response, (void*)(&eh_in+1), length > rlen ? rlen : length);
	}
      }
      uint64_t Device::
      get64(RegisterOffset offset, uint32_t *status) {
	union {
	  uint64_t u64;
	  uint32_t u32[sizeof(uint64_t) / sizeof(uint32_t)];
	} u;
	u.u32[0] = get(offset, sizeof(uint32_t), status);
	if (!status || !*status)
	  u.u32[1] = get(offset + sizeof(uint32_t), sizeof(uint32_t), status);
	return u.u64;
      }
      void Device::
      getBytes(RegisterOffset offset, uint8_t *buf, size_t length, size_t elementBytes,
	       uint32_t *status, bool string) {
	while (length) {
	  size_t bytes = sizeof(uint32_t) - (offset & 3); // bytes in word
	  if (bytes > length)
	    bytes = length;
	  if (bytes > elementBytes)
	    bytes = elementBytes;
	  uint32_t val = get(offset, bytes, status);
	  if (status && *status)
	    return;
	  uint8_t *data = (uint8_t*)&val + (offset & 3);
	  memcpy(buf, data, bytes);
	  if (string && strnlen((char *)data, bytes) < bytes)
	    break;
	  length -= bytes;
	  buf += bytes;
	  offset += bytes;
	}
      }
      void Device::
      set64(RegisterOffset offset, uint64_t val, uint32_t *status) {
	set(offset, sizeof(uint32_t), (uint32_t)val, status);
	if (!status || !*status)
	  set(offset + sizeof(uint32_t), sizeof(uint32_t), (uint32_t)(val >> 32), status);
      }
      void Device::
      setBytes(RegisterOffset offset, const uint8_t *buf, size_t length, size_t elementBytes,
	       uint32_t *status)  {
	while (length) {
	  size_t bytes = sizeof(uint32_t) - (offset & 3); // bytes in word
	  if (bytes > length)
	    bytes = length;
	  if (bytes > elementBytes)
	    bytes = elementBytes;
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
      static void
      initNop(EtherControlNop &nop) {
	memset(&nop, 0, sizeof(nop));
	nop.header.etherTypeOverlay = 0; // for valgrind
        nop.header.tag = 0;
	nop.header.length = htons((short)(sizeof(nop)-2));
	nop.header.pad = 0;
	nop.header.typeEtc = OCCP_ETHER_TYPE_ETC(OCCP_NOP, 0xf, 1, 0);
	nop.mbx80 = 0x80;
	nop.mbz0 = 0;
        nop.mbz1 = 1;
	nop.maxCoalesced = 0;
      }
      static bool
      checkNopResponse(EtherControlNopResponse &response, std::string &error) {
	if (response.header.length == htons((short)(sizeof(response)-2)) &&
	    response.header.typeEtc == OCCP_ETHER_TYPE_ETC(OCCP_RESPONSE, OK, 1, 0) &&
	    response.mbx40 == 0x40 &&
	    response.mbz0 == 0)
	  return true;
	ocpiBad("Bad network discovery response:");
	for (unsigned i = 0; i < sizeof(response); i++)
	  ocpiBad("Response byte %u: 0x%x", i, ((uint8_t*)&response)[i+2]);
	error = "Bad ethernet discovery response";
	return false;
      }
      Driver::
      ~Driver() {
	for (SocketsIter si = m_sockets.begin(); si != m_sockets.end(); si = m_sockets.begin()) {
	  delete (*si).second;
	  m_sockets.erase(si);
	}
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
      // Try a discovery (send and receive) on a socket.
      bool Driver::
      trySocket(OE::Interface &ifc, OE::Socket &s, OE::Address &addr, bool discovery,
		const char **exclude, Macs *argMacs, Device **dev, std::string &error) {
	// keep track of different discovery source addresses discovered when we broadcast.
	ocpiDebug("Trying socket on interface %s to address %s",
		  ifc.name.c_str(), addr.pretty());
	std::set<OE::Address,OE::Address::Compare> addrs;
	OE::Packet sendFrame;
	initNop(*(EtherControlNop *)(sendFrame.payload));
	const size_t recvLength = sizeof(EtherControlNopResponse);

	unsigned count = 0;
	// FIXME:  We need to be able to probe one while others are running?
	for (unsigned n = 0; error.empty() && n < RETRIES; n++) {
	  if (!s.send(sendFrame, sizeof(EtherControlNop), addr, 0, &ifc, error))
	    break;
	  OE::Packet recvFrame;
	  OE::Address devAddr;
	  size_t length;
	  OS::Timer timer(0, DELAYMS * 1000000);
	  while (s.receive(recvFrame, length, DELAYMS, devAddr, error)) {
	    if (exclude)
	      for (const char **ap = exclude; *ap; ap++)
		if (!strcmp(*ap, devAddr.pretty())) {
		  ocpiInfo("Net device %s specifically excluded/ignored", *ap);
		  continue;
		}
	    EtherControlNopResponse &ecnr = *(EtherControlNopResponse *)(recvFrame.payload);
	    if (length > recvLength)
	      ocpiDebug("receive truncation for interface '%s': %zu > %zu",
			ifc.name.c_str(), length, recvLength);
	    if (length < recvLength)
	      OS::setError(error, "probe return was short:  length was %d when %d was expected",
			   length, recvLength);
	    else if (addr.isBroadcast() && addrs.find(devAddr) != addrs.end()) {
	      ocpiDebug("Received redundant ethernet discovery response from %s",
			devAddr.pretty());
	      continue;
	    } else if (checkNopResponse(ecnr, error)) {
	      if (addr.isBroadcast())
		addrs.insert(devAddr);
	      else if (devAddr != addr) {
		ocpiInfo("Received ethernet discovery response from wrong address");
		continue;
	      }
	      OS::Ether::Address mac((const unsigned char *)ecnr.mac);
	      // We found one or THE one.
	      if (addr.isBroadcast()) {
		std::string server;
		OU::format(server, "%s/%u", mac.pretty(), ecnr.pid);
		Macs &macs = *argMacs;
		MacsIter mi = macs.find(server);
		if (mi == macs.end()) {
		  ocpiInfo("Discovered MAC %s from address %s for the first time",
			   server.c_str(), devAddr.pretty());
		  macs.insert(MacInsert(server, MacPair(devAddr, &ifc)));
		} else {
		  ocpiInfo("Discovered server %s from address %s after seeing it before (from %s)",
			   server.c_str(), devAddr.pretty(), mi->second.first.pretty());
		  if (devAddr.isLoopback()) {
		    ocpiInfo("New address %s is loopback, so deleting previous one %s",
			     devAddr.pretty(), mi->second.first.pretty());
		    macs.erase(mi);
		    macs.insert(MacInsert(server, MacPair(devAddr, &ifc)));
		  }
		}
	      } else {
		// We were probing a single address
		Device *d;
		if ((d = createDevice(ifc, devAddr, discovery, error))) {
		  assert(dev); // should be set if not broadcasting
		  *dev = d;
		  return 1;
		} else
		  ocpiInfo("Net device discovery from %s had device creation error: %s",
			   devAddr.pretty(), error.c_str());
	      }
	    }
	    if (!timer.expired())
	      OS::sleep(1);
	  }
	}
	if (error.size())
	  ocpiInfo("error on interface '%s' when probing for %s: %s",
		   ifc.name.c_str(), addr.pretty(), error.c_str());
	else if (!count && !addr.isBroadcast())
	  ocpiInfo("no network probe response on '%s' from '%s' after %u attempts %ums apart",
		   ifc.name.c_str(), addr.pretty(), RETRIES, DELAYMS);
	return count;
      }

      // Try to reach the target on the given interface.
      // If devAddr == NULL, use broadcast on both ethernet and udp
      unsigned Driver::
      tryIface(OE::Interface &ifc, OE::Address &devAddr, const char **exclude,
	       Device **dev,   // optional output arg to return the found device when mac != NULL
	       bool discovery, // is this about discovery? (broadcast *OR* specific probing)
	       Macs *macs,
	       std::string &error) {
	error.clear();
	unsigned count = 0;
	OE::Socket *s;
	if (devAddr.isEther()) {
	  if ((s = findSocket(ifc, discovery, error)))
	    return trySocket(ifc, *s, devAddr, discovery, exclude, macs, dev, error); 
	  // not "ocpiBad" due to needing sudo for bare sockets without a driver
	  ocpiDebug("Could not open socket on interface '%s' to reach device at '%s: %s",
		    ifc.name.c_str(), devAddr.pretty(), error.c_str());
	} else {
	  OE::Socket s(ifc, discovery ? ocpi_discovery : ocpi_master, &devAddr, 0, error);
	  //	  OE::Interface i("udp", error);
	  //	  if (error.length())
	  //	    ocpiInfo("Could not open udp interface for discovery: %s", error.c_str());
	  //	  else 
	  if (error.empty()) {
	    count = trySocket(ifc, s, devAddr, discovery, exclude, macs, dev, error);
	    if (error.length())
	      ocpiInfo("Error in discovery for udp interface: %s", error.c_str());
	  } else
	    ocpiInfo("Could not open socket for udp discovery: %s", error.c_str());
	}
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
	if (addr.hasError())
	  error = "Invalid network address in network device name";
	else {
	  // If we're doing udp, (and thus have no specific interface) we force the use of the udp
	  // pseudo-interface since there is no broadcasting etc., so no need of per-interface
	  // issues.
	  if (!slash && !addr.isEther())
	    iName = "udp";
	  OE::IfScanner ifs(error);
	  if (error.empty()) {
	    OE::Interface eif;
	    while (ifs.getNext(eif, error, iName.size() ? iName.c_str() : NULL))
	      if (eif.up && eif.connected) {
		Device *dev;
		if (tryIface(eif, addr, NULL, &dev, discovery, NULL, error) == 1)
		  return dev;
		else
		  break;
	      }
	    if (error.empty())
	      OU::formatString(error, "HDL ether platform %s not found", name);
	  }
	}
	return NULL;
      }

      unsigned Driver::
      search(const OU::PValue *props, const char **exclude, bool discoveryOnly, bool udp,
	     std::string &error) {
	if (getenv("OCPI_SUPPRESS_HDL_NETWORK_DISCOVERY"))
	  return 0;
	unsigned count = 0;
	OE::IfScanner ifs(error);
	if (error.size())
	  return 0;
	const char *ifName = NULL;
	OU::findString(props, "interface", ifName);
	OE::Interface eif;
	Macs macs;
	while (ifs.getNext(eif, error, ifName)) {
	  if (eif.name == "udp") // the udp pseudo interface is not used for discovery
	    continue;
	  ocpiDebug("NetDriver: Considering interface \"%s\", addr 0x%x",
		    eif.name.c_str(), eif.ipAddr.addrInAddr());
	  if (eif.up && eif.connected && (!udp || eif.ipAddr.addrInAddr())) {
	    OE::Address bcast(udp);
	    ocpiDebug("Sending to broadcast/multicast: %s udp %u", bcast.pretty(), udp);
	    count += tryIface(eif, bcast, exclude, NULL, discoveryOnly, &macs, error);
	    if (error.size()) {
	      ocpiDebug("Error during network discovery on '%s': %s",
		       eif.name.c_str(), error.c_str());
	      error.clear();
	    }
	  } else
	    ocpiDebug("Interface '%s' is %s and %s",
		     eif.name.c_str(), eif.up ? "up" : "down",
		     eif.connected ? "connected" : "not connected");
	  if (ifName)
	    break;
	}
	if (error.size())
	  ocpiInfo("Error during network discovery on '%s': %s",
		   eif.name.c_str(), error.c_str());
	for (MacsIter mi = macs.begin(); mi != macs.end(); mi++) {
	  ocpiInfo("Processing discovery for %s from network address %s",
		   mi->first.c_str(), mi->second.first.pretty());
	  Device *d;
	  if ((d = createDevice(*mi->second.second, mi->second.first, discoveryOnly, error)))
	    if (found(*d, error))
	      delete d;
	    else
	      count++;
	  else
	    ocpiInfo("error creating device for %s (MAC %s): %s", mi->second.first.pretty(),
		     mi->first.c_str(), error.c_str());
	}
	return count;
      }
    } // namespace Net
  } // namespace HDL
} // namespace OCPI
