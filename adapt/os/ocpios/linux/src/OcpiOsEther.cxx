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

// Ethernet support implementation for unix

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <stdio.h>
#ifdef OCPI_OS_darwin
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <net/route.h>
#include <net/ndrv.h>
#endif
#ifdef OCPI_OS_linux
#include <arpa/inet.h>
#include <linux/if_arp.h>
#endif

#include "OcpiOsAssert.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiOsMisc.h"
#include "OcpiOsEther.h"

namespace OCPI {
  namespace OS {
    namespace Ether {
      void Address::
      set(const void *x) {
	m_error = false;
	m_addr64 = 0;
	if (x)
	  memcpy(m_addr, x, sizeof(m_addr));
	m_pretty[0] = 0;
	m_broadcast = true;
	for (unsigned n = 0; n < sizeof(m_addr); n++)
	  if (m_addr[n] != 0xff) {
	    m_broadcast = false;
	    break;
	  }
      }

      void Address::setString(const char *m) {
	uint8_t mac[s_size];
	if (sscanf(m, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
		   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
	  m_error = true;
	  m_broadcast = true;
	} else
	  set(mac);
      }

      const char *Address::pretty() {
	if (!m_pretty[0])
	  snprintf(m_pretty, sizeof(m_pretty),
		   "%02x:%02x:%02x:%02x:%02x:%02x",
		   m_addr[0], m_addr[1], m_addr[2], m_addr[3], m_addr[4], m_addr[5]);
	return m_pretty;
      }

      Address Address::
      s_broadcast("ff:ff:ff:ff:ff:ff");

      Socket::
      Socket(Interface &i, std::string &error, Type sType, Type rType)
	: m_ifIndex(i.index), m_ifAddr(i.addr), m_sType(sType), m_rType(rType ? rType : sType),
#ifdef OCPI_OS_darwin
	  m_fd(socket(PF_NDRV, SOCK_RAW, 0)), 
#else
	  m_fd(socket(PF_PACKET, SOCK_RAW, htons(m_rType))), // is this type needed here or just in bind?
#endif
	  m_timeout(0)
      {
	if (m_fd < 0) {
	  OS::setError(error, "opening raw socket");
	  return;
	}
	ocpiDebug("setting ethertype socket option on type 0x%x", m_rType);
#ifdef OCPI_OS_darwin
	struct sockaddr_ndrv sa;
	sa.snd_len = sizeof(sa);
	sa.snd_family = PF_NDRV;
	strncpy((char *)sa.snd_name, i.name.c_str(), sizeof(sa.snd_name));
	sa.snd_name[sizeof(sa.snd_name)-1] = 0;
	if (bind(m_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
	  OS::setError(error, "binding raw socket");
	  return;
	}
	struct ndrv_demux_desc ndd;
	ndd.type = NDRV_DEMUXTYPE_ETHERTYPE;
	ndd.length = sizeof(ndd.data.ether_type);
	ndd.data.ether_type = htons(m_rType);

	struct ndrv_protocol_desc npd;
	npd.version = NDRV_PROTOCOL_DESC_VERS;
	npd.protocol_family = m_rType; // some random number???
	npd.demux_count = 1;
	npd.demux_list = &ndd;

	if (setsockopt(m_fd, SOL_NDRVPROTO, NDRV_SETDMXSPEC, (caddr_t)&npd, sizeof(npd)) != 0) {
	  OS::setError(error, "setting ethertype socket option on type 0x%x", m_rType);
	  return;
	}
#else
	struct sockaddr_ll sa;
	sa.sll_family = PF_PACKET;      // supposedly not used for bind
	sa.sll_protocol = htons(m_rType);
	sa.sll_ifindex = i.index;
	sa.sll_hatype = ARPHRD_ETHER;   // supposedly not used for bind
	sa.sll_pkttype = PACKET_HOST;   // supposedly not used for bind
	sa.sll_halen = sizeof(Address); // supposedly not used for bind
	memcpy(sa.sll_addr,             // supposedly not used for bind
	       i.addr.addr(),
	       sizeof(sa.sll_addr));
	if (bind(m_fd, (const struct sockaddr *)&sa, sizeof(sa))) {
	  OS::setError(error, "binding ethertype socket");
	  return;
	}
#endif
	ocpiDebug("Successfully opened raw socket on '%s' for ethertype 0x%x/0x%x bound to %s",
		  i.name.c_str(), m_rType, m_sType, i.addr.pretty());
      }
      Socket::
      ~Socket() {
	if (m_fd >= 0)
	  close(m_fd);
      }

      bool Socket::receive(Packet &packet, unsigned &payLoadLength, unsigned timeoutms, std::string &error) {
	if (timeoutms != m_timeout) {
	  struct timeval tv;
	  tv.tv_sec = timeoutms/1000;
	  tv.tv_usec = (timeoutms % 1000) * 1000;
	  ocpiDebug("Setting socket timeout to %u ms", timeoutms);
	  if (setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
	    setError(error, "setting receive timeout on socket");
	    return false;
	  }
	  m_timeout = timeoutms;
	}
#ifdef OCPI_OS_darwin
	struct sockaddr_dl sa;
	const int flags = 0;
#else
	struct sockaddr_ll sa;
	const int flags = MSG_TRUNC;
#endif
	socklen_t alen = sizeof(sa);
	ssize_t rlen = recvfrom(m_fd, &packet, sizeof(Packet), flags,
				(struct sockaddr*)&sa, &alen);
	if (rlen < 0) {
	  if (errno != EAGAIN)
	    setError(error, "receiving packet bytes failed");
	  return false;
	} else
	  ocpiDebug("Received packet length %d address: sizeof %u alen %u family %u", 
		    rlen, sizeof(sa), alen,
		    ((struct sockaddr *)&sa)->sa_family);
	if (alen > sizeof(sa)) {
	  ocpiDebug("received sockaddr len is %d, should be %u", alen, sizeof(sa));
#ifdef OCPI_OS_linux
	  ocpiDebug("fam %u prot %u index %d hatype %u ptype %u len %u off %u",
		    sa.sll_family, sa.sll_protocol, sa.sll_ifindex, sa.sll_hatype,
		    sa.sll_pkttype, sa.sll_halen, offsetof(struct sockaddr_ll, sll_addr[0]));		    
#endif
	  setError(error, "received sockaddr len is %d, should be %u", alen, sizeof(sa));
	  return false;
	}
	if (m_rType != ntohs(packet.header.type)) {
	  setError(error, "Ethertype mismatch: ours is 0x%x, packet's is0x%x",
		   m_rType, ntohs(packet.header.type));
	  return false;
	}
	payLoadLength = rlen - sizeof(Header);
#ifdef OCPI_DEBUG
	Address from(packet.header.source);
	Address to(packet.header.destination);
#endif
	ocpiDebug("Received ether packet from: %s to %s type 0x%x payload len %d",
		  from.pretty(), to.pretty(), ntohs(packet.header.type), payLoadLength);
	return true;
      }

      bool Socket::
      send(Packet &packet, unsigned payLoadLength, Address &addr, unsigned /*timeoutms*/,
	   std::string &error) {
	if (payLoadLength > sizeof(Packet) - sizeof(Header)) {
	  error = "sending packet that is too long";
	  return false;
	}
	packet.header.type = htons(m_sType);
	memcpy(packet.header.source, m_ifAddr.addr(), sizeof(packet.header.source));
	memcpy(packet.header.destination, addr.addr(),
	       sizeof(packet.header.destination));

	// FIXME: see if we really need this in raw sockets at all, since it is redundant 
#ifdef OCPI_OS_darwin
	struct sockaddr_dl sa;
	sa.sdl_len = sizeof(sa);
	sa.sdl_family = AF_LINK;
	sa.sdl_index = 0;
	sa.sdl_type = IFT_ETHER;
	sa.sdl_nlen = 0;
	sa.sdl_alen = 6;
	sa.sdl_slen = 0;
	memcpy(sa.sdl_data, packet.header.destination, Address::s_size);
#else
	struct sockaddr_ll sa;
	memset(&sa, 0, sizeof(sa)); // man page says initialize to zero except:
	sa.sll_family = PF_PACKET;
	memcpy(sa.sll_addr, packet.header.destination, Address::s_size);
	sa.sll_halen = Address::s_size;
	sa.sll_ifindex = m_ifIndex;
#endif
	size_t len = sizeof(Header) + payLoadLength;
	if (len < 60)
	  len = 60;
	ssize_t rlen = sendto(m_fd, &packet, len, 0, (struct sockaddr*)&sa, sizeof(sa));
	ocpiDebug("Send ether length %d, to %s, returned %d", len,
		  addr.pretty(), rlen);
	if (rlen != (ssize_t)len) {
	  setError(error, "sendto of %u bytes failed, returning %d", len, rlen);
	  return false;
	}
	return true;
      }
      struct Opaque {
#ifdef OCPI_OS_darwin
	char *buffer, *end;
	struct if_msghdr *ifm;
#else
	int sfd;
#endif
	int index;
      };

      IfScanner::IfScanner(std::string &err) {
	ocpiAssert((compileTimeSizeCheck<sizeof (m_opaque), sizeof (Opaque)> ()));
	Opaque *o = (Opaque *)m_opaque;
	o->index = -1; // this indicates there are no interfaces
	err.clear();
#ifdef OCPI_OS_darwin
	o->buffer = NULL;
	// We need this loop because things can change out from under us.
	for (unsigned n = 0; err.empty() && n < 10; n++) {
	  int mib[6] = { CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0};
	  size_t space;
	  if (sysctl(mib, 6, NULL, &space, NULL, 0) < 0)
	    setError(err, "can't sysctl to get buffer size for interfaces");
	  else if (space) {
	    o->buffer = new char[space];
	    o->end = o->buffer + space;
	    if (sysctl(mib, 6, o->buffer, &space, NULL, 0) < 0) {
	      if (errno == ENOMEM && n < 10) {
		ocpiBad("Routing table grew, retrying\n");
		delete o->buffer;
		o->buffer = NULL;
		usleep(10000);
	      } else
		setError(err, "can't sysctl to retrieve interface info");
	    } else {
	      o->index = 0; // indicate that there are interfaces to look at
	      break;
	    }
	  }
	}
	if (err.empty())
	  if (o->buffer) {
	    o->index = 0;
	    o->ifm = (struct if_msghdr *)o->buffer;
	  } else
	    err = "sysctl failed after 10 retries";
#else
	if ((o->sfd = socket(PF_PACKET, SOCK_RAW, 0)) < 0)
	  setError(err, "failed opening interface scanning socket");
	o->index = 0;
#endif
      }
      IfScanner::
      ~IfScanner() {
	Opaque *o = (Opaque *)m_opaque;
#ifdef OCPI_OS_darwin
	delete o->buffer;
#else
	if (o->sfd >= 0)
	  close(o->sfd);
#endif
      }
      bool IfScanner::
      getNext(Interface &i, std::string &err, const char *only) {
	err.clear();
	Opaque *o = (Opaque *)m_opaque;
#ifdef OCPI_OS_darwin
	if (o->index == -1)
	  return false;
	ocpiAssert(o->buffer);
	// Loop through all messages until we have a good one.
	bool found = false;
	for (struct if_msghdr *ifm = o->ifm; (char *)ifm < o->end && !found; ifm = o->ifm) {
	  o->ifm = (struct if_msghdr *)((char *)ifm + ifm->ifm_msglen);
	  if (ifm->ifm_type == RTM_IFINFO) {
	    ocpiDebug("iface %u: flags 0x%04x addrs 0x%x type 0x%x",
		      ifm->ifm_index, ifm->ifm_flags, ifm->ifm_addrs, ifm->ifm_data.ifi_type);
	    char *extra = (char *)ifm + sizeof(*ifm);
	    for (unsigned n = 0; n < RTAX_MAX && extra < (char *)o->ifm; n++)
	      if (ifm->ifm_addrs & (1 << n)) {
		struct sockaddr_dl *sdl = (struct sockaddr_dl *)extra;
		ocpiDebug("sockaddr_dl: addr %2u alen %2u %2u %2u %2u", n,
			  sdl->sdl_len, sdl->sdl_nlen, sdl->sdl_alen, sdl->sdl_slen);
		extra += sdl->sdl_len;
		if (n == RTAX_IFP) {
		  ocpiAssert(sdl->sdl_nlen);
		  ocpiDebug("iface type %u name: '%.*s'", sdl->sdl_type, sdl->sdl_nlen, sdl->sdl_data);
		  if (ifm->ifm_data.ifi_type == IFT_ETHER &&
		      (!only || 	    
		       (sdl->sdl_nlen == strlen(only) &&
			!strncmp(sdl->sdl_data, only, sdl->sdl_nlen)))) {
		    // PF_INET/SOCK_DGRAM since it works without root privileges.
		    int s = socket(PF_INET, SOCK_DGRAM, 0);
		    ocpiAssert(s);
		    struct ifmediareq ifmr;
		    memset(&ifmr, 0, sizeof(ifmr));
		    strncpy(ifmr.ifm_name, sdl->sdl_data, sdl->sdl_nlen);
		    ifmr.ifm_name[sdl->sdl_nlen] = 0;
		    ocpiCheck(ioctl(s, SIOCGIFMEDIA, &ifmr) == 0);
		    ocpiDebug("IFMEDIA: 0x%x:", ifmr.ifm_status);
		    if (sdl->sdl_alen && sdl->sdl_type == IFT_ETHER &&
			sdl->sdl_alen == Address::s_size) {
		      i.connected =
			(ifmr.ifm_status & (IFM_AVALID|IFM_ACTIVE)) == (IFM_AVALID|IFM_ACTIVE);
		      i.name.assign(sdl->sdl_data, sdl->sdl_nlen);
		      i.addr.set(sdl->sdl_data + sdl->sdl_nlen);
		      i.index = ifm->ifm_index;
		      i.up = (ifm->ifm_flags & IFF_UP) != 0;
		      ocpiDebug("ether: %s, up: %d connected: %d", i.addr.pretty(), i.up, i.connected);
		      found = true;
		    }
		  }
		}
	      }
	  } // if RTM_IFINFO message
	}
	if (!found && only)
	  err = "the requested interface was not found";
	return found;
#else
	struct ifreq ifr;
	do {
	  i.index = ifr.ifr_ifindex = ++o->index;
	  if (ioctl(o->sfd, SIOCGIFNAME, &ifr) < 0) {
	    if (errno == ENODEV)
	      continue;
	    setError(err, "retrieving interface name for index %u", o->index);
	    break;
	  }
	  i.name = ifr.ifr_name;
	  if (ioctl(o->sfd, SIOCGIFFLAGS, &ifr) < 0) {
	    setError(err, "retrieving interface flags for index %u", o->index);
	    break;
	  }
	  i.up = (ifr.ifr_flags & IFF_UP) != 0;
	  i.connected = (ifr.ifr_flags & IFF_RUNNING) != 0;
	  if (ioctl(o->sfd, SIOCGIFHWADDR, &ifr) < 0) {
	    setError(err, "retrieving interface address for index %u", o->index);
	    break;
	  }
	  ocpiDebug("found interface '%s' which is %s (family %u)",
		    ifr.ifr_name, i.up ? "up" : "down", ifr.ifr_hwaddr.sa_family);
	  if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
	    i.addr.set((uint8_t *)ifr.ifr_hwaddr.sa_data);
	    ocpiDebug("found ethernet '%s' address %s", ifr.ifr_name, i.addr.pretty());
	    if (!only || i.name == only)
	      return true;
	  }
	} while(i.index < 10);
	return false;
#endif
      }
      Interface::Interface(){}
      Interface::Interface(const char *name, std::string &error) {
	IfScanner ifs(error);
	while (error.empty() && ifs.getNext(*this, error, name))
	  if (up || connected)
	    return;
	error = "No interfaces found that are up and connected";
      }
    }
  }
}
