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
#include <fcntl.h>
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
#include <dirent.h>
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

      bool haveDriver() {
#ifdef OCPI_OS_linux
	// FIXME: mutex
	static bool init = false, haveit;
	if (!init) {
	  int dfd = open(OCPI_DRIVER_MEM, O_RDONLY);
	  if (dfd >= 0) {
	    haveit = true;
	    close(dfd);
	  } else
	    haveit = false;
	  init = true;
	}	  
	return haveit;
#else
	return false;
#endif
      }
      Socket::
      Socket(Interface &i, ocpi_role_t role, Address *remote, uint16_t endpoint, std::string &error)
	: m_ifIndex(i.index), m_ifAddr(i.addr),
	  m_type(role == ocpi_data ? OCDP_ETHER_TYPE : OCCP_ETHER_MTYPE),
	  m_fd(-1), m_timeout(0), m_role(role), m_endpoint(endpoint)
      {
	ocpiDebug("setting ethertype socket option on type 0x%x", m_type);
#ifdef OCPI_OS_darwin
	(void)remote;
	if ((m_fd = socket(PF_NDRV, SOCK_RAW, 0)) < 0) {
	  OS::setError(error, "opening raw socket");
	  return;
	}
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
	ndd.data.ether_type = htons(m_type);

	struct ndrv_protocol_desc npd;
	npd.version = NDRV_PROTOCOL_DESC_VERS;
	npd.protocol_family = m_type; // some random number???
	npd.demux_count = 1;
	npd.demux_list = &ndd;

	if (setsockopt(m_fd, SOL_NDRVPROTO, NDRV_SETDMXSPEC, (caddr_t)&npd, sizeof(npd)) != 0) {
	  OS::setError(error, "setting ethertype socket option on type 0x%x", m_type);
	  return;
	}
#else
	if (haveDriver()) {
	  if ((m_fd = socket(PF_OPENCPI, SOCK_DGRAM, 0)) < 0) {
	    OS::setError(error, "opening raw socket");
	    return;
	  }
	  struct sockaddr_ocpi sa;
	  memset(&sa, 0, sizeof(sa));
	  sa.ocpi_family = PF_OPENCPI;      // supposedly not used for bind
	  sa.ocpi_role = role;
	  sa.ocpi_ifindex = i.index;
	  if (remote)
	    memcpy(sa.ocpi_remote, remote, sizeof(sa.ocpi_remote));
	  if (role == ocpi_data)
	    sa.ocpi_endpoint = endpoint;
	  if (bind(m_fd, (const struct sockaddr *)&sa, sizeof(sa))) {
	    OS::setError(error, "binding ethertype socket");
	    return;
	  }
	} else {
	  // is this type needed here or just in bind?
	  if ((m_fd = socket(PF_PACKET, SOCK_RAW, htons(m_type))) < 0) {
	    OS::setError(error, "opening raw socket");
	    return;
	  }
	  struct sockaddr_ll sa;
	  sa.sll_family = PF_PACKET;      // supposedly not used for bind
	  sa.sll_protocol = htons(m_type);
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
	}
#endif
	ocpiDebug("Successfully opened raw socket on '%s' for ethertype 0x%x bound to %s",
		  i.name.c_str(), m_type, i.addr.pretty());
      }
      Socket::
      ~Socket() {
	if (m_fd >= 0)
	  close(m_fd);
      }

      bool Socket::receive(Packet &packet, unsigned &payLoadLength, unsigned timeoutms, Address &addr,
			   std::string &error) {
	unsigned offset;
	bool b = receive((uint8_t *)&packet, offset, payLoadLength, timeoutms, addr, error);
	ocpiAssert(offset == offsetof(Packet, payload));
	return b;
      }
      bool Socket::receive(uint8_t *buffer, unsigned &offset, unsigned &payLoadLength, unsigned timeoutms,
			   Address &addr, std::string &error) {
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
	socklen_t alen;
	offset = offsetof(Packet, payload);
	Packet &packet(*(Packet *)buffer);
	void *payload = (void *)buffer;
#ifdef OCPI_OS_darwin
	struct sockaddr_dl sa;
	const int flags = 0;
	alen = sizeof(sa);
#else
	union {
	  struct sockaddr_ll ll;
	  struct sockaddr_ocpi ocpi;
	} sa;
	const int flags = MSG_TRUNC;
	if (haveDriver()) {
	  payload = packet.payload;
	  alen = sizeof(sa.ocpi);
	} else
	  alen = sizeof(sa.ll);
#endif
	do { // loop to filter our junk packets
	  socklen_t before = alen;
	  ssize_t rlen = recvfrom(m_fd, payload, sizeof(Packet), flags, (struct sockaddr*)&sa, &alen);
	  if (rlen < 0) {
	    if (errno != EAGAIN && errno != EINTR)
	      setError(error, "receiving packet bytes failed");
	    return false;
	  } else
	    ocpiDebug("Received packet length %zu address: sizeof %zu alen %u family %u", 
		      rlen, sizeof(sa), alen,
		      ((struct sockaddr *)&sa)->sa_family);
	  if (alen > before) {
	    ocpiDebug("received sockaddr len is %d, should be %u", alen, before);
	    setError(error, "received sockaddr len is %d, should be %u", alen, sizeof(sa));
	    return false;
	  }
	  payLoadLength = rlen - (sizeof(Header) - sizeof(uint16_t));
	  uint8_t *source = packet.source;
#ifdef OCPI_OS_linux
	  if (haveDriver()) {
	    ocpiDebug("fam %u role %u index %d",
		      sa.ocpi.ocpi_family, sa.ocpi.ocpi_role, sa.ocpi.ocpi_ifindex);
	    payLoadLength = rlen;
	    source = sa.ocpi.ocpi_remote;
	  } else
	    ocpiDebug("fam %u prot %u index %d hatype %u ptype %u len %u off %zu",
		      sa.ll.sll_family, sa.ll.sll_protocol, sa.ll.sll_ifindex, sa.ll.sll_hatype,
		      sa.ll.sll_pkttype, sa.ll.sll_halen, offsetof(struct sockaddr_ll, sll_addr[0]));
#endif
	  if (!memcmp(source, m_ifAddr.addr(), Address::s_size)) {
	    ocpiDebug("Received packet from myself\n");
	    continue;
	  }
	  Type type = ((Header *)&packet)->type;
	  if (m_type != ntohs(type)) {
	    setError(error, "Ethertype mismatch: ours is 0x%x, packet's is0x%x",
		     m_type, ntohs(type));
	    return false;
	  }
#ifdef OCPI_DEBUG
	  Address from(source);
	  Address to(packet.destination);
	  ocpiDebug("Received ether packet from: %s to %s type 0x%x payload len %d",
		    from.pretty(), to.pretty(), ntohs(type), payLoadLength);
#endif
	  addr.set(source);
	  return true;
	} while (1);
      }

      bool Socket::
      send(Packet &packet, unsigned payLoadLength, Address &addr, unsigned timeoutms,
	   std::string &error) {
	if (payLoadLength > sizeof(Packet) - (sizeof(Header) - sizeof(uint16_t))) {
	  setError(error, "sending packet that is too long: %u, p %u h %u",
		   payLoadLength, sizeof(Packet), sizeof(Header));
	  return false;
	}
	OS::IOVec iov[2];
	iov[0].iov_base = packet.payload;
	iov[0].iov_len = payLoadLength;
	return send(iov, 1, addr, timeoutms, error);
      }
      bool Socket::
      send(IOVec *iov, unsigned iovlen, Address &addr, unsigned /*timeoutms*/, std::string &error) {
	IOVec myiov[10];
	bool prepend = true;
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
	memcpy(sa.sdl_data, addr.addr(), Address::s_size);
	ocpiAssert(iovlen < 10);
#else
	union {
	  struct sockaddr_ll ll;
	  struct sockaddr_ocpi ocpi;
	} sa;
	memset(&sa, 0, sizeof(sa));
	if (haveDriver()) {
	  sa.ocpi.ocpi_family = PF_PACKET;
	  sa.ocpi.ocpi_role = m_role;
	  sa.ocpi.ocpi_ifindex = m_ifIndex;
	  memcpy(sa.ocpi.ocpi_remote, addr.addr(), Address::s_size);
	  prepend = false;
	} else {
	  ocpiAssert(iovlen < 10);
	  sa.ll.sll_family = PF_PACKET;
	  memcpy(sa.ll.sll_addr, addr.addr(), Address::s_size);
	  sa.ll.sll_halen = Address::s_size;
	  sa.ll.sll_ifindex = m_ifIndex;
	}
#endif
	Header header;
	if (prepend) {
	  myiov[0].iov_base = &header;
	  myiov[0].iov_len = sizeof(header);
	  if (iov[0].iov_len > 2) {
	    memcpy(&myiov[1], iov, iovlen * sizeof(*iov));
	    myiov[1].iov_base = (uint8_t *)myiov[1].iov_base + 2;
	    myiov[1].iov_len -= 2;
	  } else {
	    iov++;
	    iovlen--;
	    memcpy(&myiov[1], iov, iovlen * sizeof(*iov));
	  }
	  iov = myiov;
	  iovlen++;
	  memcpy(header.source, m_ifAddr.addr(), sizeof(header.source));
	  memcpy(header.destination, addr.addr(), sizeof(header.destination));
	  header.type = htons(m_type);
	}
	size_t len = 0;
	for (IOVec *i = iov; i < &iov[iovlen]; i++)
	  len += i->iov_len;
	struct msghdr msg;
        msg.msg_name = (void*)&sa;
	msg.msg_namelen = sizeof(sa);
        msg.msg_iov = (iovec*)iov;
	msg.msg_iovlen = iovlen;
	msg.msg_control = 0;
	msg.msg_controllen = 0;
	msg.msg_flags = 0; // checkfor MSG_TRUNC
	
	ssize_t rlen = sendmsg(m_fd, &msg, 0);
	ocpiDebug("Send ether length %zd, to %s, returned %zd", len,
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
#define NETIFDIR "/sys/class/net"
	DIR *dfd;
	long start;
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
	if ((o->dfd = opendir(NETIFDIR)) == NULL)
	  setError(err, "failed opening interface scanning socket (via %s", NETIFDIR);
	if ((o->start = telldir(o->dfd)) < 0) {
	  closedir(o->dfd);
	  o->dfd = NULL;
	  setError(err, "telldir failure on %s directory");
	}
	o->index = 0;
#endif
      }
      IfScanner::
      ~IfScanner() {
	Opaque *o = (Opaque *)m_opaque;
#ifdef OCPI_OS_darwin
	delete o->buffer;
#else
	if (o->dfd >= 0)
	  closedir(o->dfd);
#endif
      }
#ifdef OCPI_OS_linux
      // Return true if we got a valu
      static bool
      getValue(std::string &file, const char *name, long *nresult, std::string *sresult = NULL) {
	size_t s = file.size();
	file += '/';
	file += name;
	int fd = open(file.c_str(), O_RDONLY);
	file.resize(s);
	if (fd < 0)
	  return false;
	char buf[100];
	ssize_t n = read(fd, buf, sizeof(buf));
	close(fd);
	if (n <= 0 || n >= (ssize_t)sizeof(buf))
	  return false;
	while (n > 0 && buf[n-1] == '\n')
	  n--;
	buf[n] = 0;
	if (nresult) {
	  char *end;
	  *nresult = strtol(buf, &end, 0);
	  return *end == 0 || *end == '\n';
	}
	*sresult = buf;
	return true;
      }
#endif

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
	// Somewhat ugly and unscalable.  We can use the driver if needed.
	while (++o->index < 10) {
	  seekdir(o->dfd, o->start);
	  struct dirent ent, *entp;
	  while (readdir_r(o->dfd, &ent, &entp) == 0 && entp)
	    if (entp->d_name[0] != '.' && (!only || !strcmp(only, entp->d_name))) {
	      std::string s(NETIFDIR), addr;
	      s += '/';
	      s += entp->d_name;
	      long nval, carrier, index;

	      if (getValue(s, "type", &nval) && nval == ARPHRD_ETHER &&
		  getValue(s, "address", NULL, &addr) &&
		  getValue(s, "ifindex", &index) && (only || index == o->index) &&
		  getValue(s, "carrier", &carrier) &&
		  getValue(s, "flags", &nval)) {
		i.addr.setString(addr.c_str());
		if (!i.addr.error()) {
		  i.index = index;
		  i.name = entp->d_name;
		  i.up = (nval & IFF_UP) != 0;
		  i.connected = carrier != 0;
		  ocpiDebug("found ether interface '%s' which is %s, %s, at address %s",
			    entp->d_name, i.up ? "up" : "down",
			    i.connected ? "connected" : "disconnected", i.addr.pretty());
		  return true;
		}
	      }
	    }
	}
	return false;
#if 0 // old sudo-required way
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
#endif
      }
      Interface::Interface(){}
      Interface::Interface(const char *name, std::string &error) {
	IfScanner ifs(error);
	while (error.empty() && ifs.getNext(*this, error, name))
	  if (up && connected)
	    return;
	error = "No interfaces found that are up and connected";
      }
    }
  }
}
