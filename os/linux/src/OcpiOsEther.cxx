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

// Ethernet support implementation for unix

#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include "ocpi-config.h"
#ifdef OCPI_OS_macos
#include <arpa/inet.h>
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
#include <sys/socket.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>
#endif
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include "OcpiOsAssert.h"
#include "OcpiOsSizeCheck.h"
#include "OcpiOsMisc.h"
#include "OcpiOsEther.h"

namespace OCPI {
  namespace OS {
    namespace Ether {
      Address::
      Address(bool isUdp, uint16_t port) {
	m_broadcast = true;
	m_isEther = !isUdp;
	if (isUdp) {
	  m_udp.port = port;
	  m_udp.addr = inet_addr(c_multicastGroup);
	} else
	m_addr64 = s_broadcast.m_addr64;
	m_pretty[0] = 0;
      }
      void Address::
      set(const void *x) {
	m_isEther = true;
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
      void Address::
      set(uint16_t port, uint32_t addr_in) {
	m_isEther = false;
	m_error = false;
	m_addr64 = 0;
	m_pretty[0] = 0;
	m_broadcast = addr_in == INADDR_BROADCAST;
	m_udp.port = port;
	m_udp.addr = addr_in;
      }

      bool Address::setString(const char *m) {
	m_error = false;
	m_pretty[0] = 0;
	m_addr64 = 0;
	if (strchr(m, '.')) {
	  m_isEther = false;
	  struct in_addr x;
	  char addr_tmp[3+1+3+1+3+1+3+1+5+1];
	  if (strlen(m) >= sizeof(addr_tmp))
	    return m_error = true;
	  strcpy(addr_tmp, m);
	  char *cp = strrchr(addr_tmp, ':');
	  if (cp)
	    *cp = 0;
	  if (!inet_aton(addr_tmp, &x))
	    return m_error = true;
	  if (cp)
	    m_udp.port = (uint16_t)atoi(cp+1);
	  else
	    m_udp.port = c_udpPort;
	  m_udp.addr = x.s_addr;
	  m_broadcast = x.s_addr == INADDR_BROADCAST;
	} else {
	  m_isEther = true;
	  uint8_t mac[s_size];
	  if (sscanf(m, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
		     &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
	    m_broadcast = false;
	    return m_error = true;
	  } else
	    set(mac);
	}
	return false;
      }

      const char *Address::prettyInAddr() {
	struct in_addr x = {m_udp.addr};
	return inet_ntoa(x);
      }
      const char *Address::pretty() {
	if (!m_pretty[0]) {
	  if (m_isEther)
	    snprintf(m_pretty, sizeof(m_pretty),
		     "%02x:%02x:%02x:%02x:%02x:%02x",
		     m_addr[0], m_addr[1], m_addr[2], m_addr[3], m_addr[4], m_addr[5]);
	  else {
	    struct in_addr x = {m_udp.addr};
	    snprintf(m_pretty, sizeof(m_pretty), "%s:%u",
		     x.s_addr == INADDR_ANY ? "<ANY>" : inet_ntoa(x), m_udp.port);
	  }
	}
	return m_pretty;
      }
      bool Address::isLoopback() const {
	return !m_isEther && m_udp.addr == ntohl(INADDR_LOOPBACK);
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
      // Pick the "primary" address
      in_addr_t
      myAddress() {
	static char name[256];
	static in_addr_t addr;
	if (!addr) {
	  struct hostent * hent;
	  if (gethostname(name, sizeof(name)) != 0 ||
	      !(hent = ::gethostbyname (name)) ||
	      !hent->h_name || !*hent->h_addr_list)
	    throw std::string ("gethostbyname() failed");
	  struct in_addr in;
	  for (char **ap = hent->h_addr_list; *ap; ap++) {
	    in = *(in_addr*)*ap;
	    if (in.s_addr != INADDR_LOOPBACK)
	      break;
	  }
	  ocpiDebug("My own address is %s", inet_ntoa(in));
	  addr = in.s_addr;
	}
	return addr;
      }
      Socket::
      Socket(Interface &i, ocpi_role_t role, Address *remote, uint16_t endpoint, std::string &error)
	: m_ifIndex(i.index), m_ifAddr(i.addr), m_brdAddr(i.brdAddr),
	  //	  m_ipAddr(i.ipAddr),
	  m_type(role == ocpi_data ? OCDP_ETHER_TYPE : OCCP_ETHER_MTYPE),
	  m_fd(-1), m_timeout(0), m_role(role)
      {
	ocpiDebug("Socket for if '%s'(%u) role %u addr %s port %u",
		  i.name.c_str(), i.index, role, remote ? remote->pretty() : "none", endpoint);
	//	ocpiDebug("setting ethertype socket option on type 0x%x", m_type);
	if (i.addr.isEther() && (!remote || remote->isEther())) {
	  if (haveDriver()) {
	    if ((m_fd = socket(PF_OPENCPI, SOCK_DGRAM, 0)) < 0) {
	      OS::setError(error, "opening raw socket");
	      return;
	    }
	    union {
	      struct sockaddr_ocpi sa;
	      struct sockaddr      sock; // here to suppress valgrind error
	    } s;
	    memset(&s, 0, sizeof(s));
	    s.sa.ocpi_family = PF_OPENCPI;      // supposedly not used for bind
	    s.sa.ocpi_role = (uint8_t)role;
	    s.sa.ocpi_ifindex = (uint8_t)i.index;
	    if (remote)
	      memcpy(s.sa.ocpi_remote, remote->addr(), sizeof(s.sa.ocpi_remote));
	    if (role == ocpi_data)
	      s.sa.ocpi_endpoint = endpoint;
	    if (bind(m_fd, &s.sock, sizeof(s.sa))) {
	      OS::setError(error, "binding ethertype socket");
	      return;
	    }
	  } else {
#ifdef OCPI_OS_macos
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
	    memset(&ndd, 0, sizeof(ndd));
	    ndd.type = NDRV_DEMUXTYPE_ETHERTYPE;
	    ndd.length = sizeof(ndd.data.ether_type);
	    ndd.data.ether_type = htons(m_type);

	    struct ndrv_protocol_desc npd;
	    memset(&npd, 0, sizeof(npd));
	    npd.version = NDRV_PROTOCOL_DESC_VERS;
	    npd.protocol_family = m_type; // some random number???
	    npd.demux_count = 1;
	    npd.demux_list = &ndd;

	    if (setsockopt(m_fd, SOL_NDRVPROTO, NDRV_SETDMXSPEC, (caddr_t)&npd, sizeof(npd)) != 0) {
	      OS::setError(error, "setting ethertype socket option on type 0x%x", m_type);
	      return;
	    }
#else
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
#endif
	    ocpiDebug("Successfully opened ether socket on '%s' for ethertype 0x%x bound to %s",
		      i.name.c_str(), m_type, i.addr.pretty());
	  }
	} else {
	  // Generic UDP setup, with no fixed ports
	  if ((m_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
	    OS::setError(error, "opening udp socket");
	    return;
	  }
	  struct sockaddr_in sin;
	  memset(&sin, 0, sizeof(sin));
	  int val = 1;
	  do { // break on error
	    if ((role == ocpi_slave && ::setsockopt(m_fd, SOL_SOCKET,
#ifdef OCPI_OS_macos
						    SO_REUSEPORT
#else
						    SO_REUSEADDR
#endif
						    , &val, sizeof(val)))) {
	      OS::setError(error, "setting udp socket options");
	      break;
	    }
#ifdef OCPI_OS_macos
	    sin.sin_len = sizeof(sin);
#endif
	    sin.sin_family = AF_INET;
	    sin.sin_port = role == ocpi_slave ? htons(endpoint ? endpoint : c_udpPort) : (uint16_t)0;
	    sin.sin_addr.s_addr = i.ipAddr.addrInAddr();
	    if (::bind(m_fd, (struct sockaddr*)&sin, sizeof(sin))) {
	      OS::setError(error, "binding udp socket for role %u", role);
	      break;
	    }
	    ocpiDebug("Successfully bound UDP socket on '%s' to %s %s",
		      i.name.c_str(), i.addr.pretty(), i.ipAddr.pretty());
	    switch (role) {
	    case ocpi_slave:
	      if (::setsockopt(m_fd, IPPROTO_IP, IP_PKTINFO, &val, sizeof(val))) {
		OS::setError(error, "enabling interface information");
		break;
	      }
	      {
		// Enable the slave to receive multicast
		struct ip_mreq mreq;
		bzero(&mreq, sizeof(mreq));
		mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.1");
		mreq.imr_interface.s_addr = INADDR_ANY;
		std::string
		  ia(inet_ntoa(mreq.imr_interface)),
		  na(inet_ntoa(mreq.imr_multiaddr));
		ocpiDebug("Adding multicast iface %s group %s", ia.c_str(), na.c_str());
		if (::setsockopt(m_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq))) {
		  OS::setError(error, "adding multicast membership");
		  break;
		}
		ia = inet_ntoa(mreq.imr_interface);
		na = inet_ntoa(mreq.imr_multiaddr);
		ocpiDebug("Slave has iface %s group %s", ia.c_str(), na.c_str());
	      }
	      break;
	    case ocpi_discovery:
	    case ocpi_master:
	      if (::setsockopt(m_fd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)))
		OS::setError(error, "enabling broadcast option");
	      else {
		struct in_addr ifaddr;
		ifaddr.s_addr = i.ipAddr.addrInAddr();
		if (::setsockopt(m_fd, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr,
				 sizeof(ifaddr)))
		  OS::setError(error, "setting the multicast interface %s(%u) to ip %s (%s)",
			       i.name.c_str(), i.index, i.ipAddr.pretty(), i.addr.pretty());
		else
		  ocpiDebug("Set the multicast interface %s(%u) to ip %s (%s) %x/%x",
			    i.name.c_str(), i.index, i.ipAddr.pretty(), i.addr.pretty(),
			    ifaddr.s_addr, INADDR_LOOPBACK);
	      }
	      break;
	    default:;
	    }
	    if (!error.empty())
	      break;
	    socklen_t alen = sizeof(sin);
	    if (::getsockname(m_fd, (struct sockaddr *)&sin, &alen)) {
	      OS::setError(error, "getting udp address");
	      break;
	    }
	    // We use m_type for port for UDP
	    m_type = ntohs(sin.sin_port);
	    m_ifAddr.set(m_type, sin.sin_addr.s_addr);
	    ocpiDebug("Successfully opened udp socket on '%s' for port %u bound to %s role %u",
		      i.name.c_str(), m_type, m_ifAddr.pretty(), role);
	    return;
	  } while (0);
	  ::close(m_fd);
	}
      }
      Socket::
      ~Socket() {
	ocpiDebug("Closing OsEther Socket fd %d", m_fd);
	if (m_fd >= 0)
	  ::close(m_fd);
      }

      bool Socket::
      receive(Packet &packet, size_t &payLoadLength, unsigned timeoutms, Address &addr,
	      std::string &error, unsigned *indexp) {
	size_t offset;
	bool b = receive((uint8_t *)&packet, offset, payLoadLength, timeoutms, addr, error, indexp);
	ocpiAssert(offset == offsetof(Packet, payload));
	return b;
      }
      bool Socket::
      receive(uint8_t *buffer, size_t &offset, size_t &payLoadLength, unsigned timeoutms,
	      Address &addr, std::string &error, unsigned *indexp) {
	if (timeoutms != m_timeout) {
	  struct timeval tv;
	  tv.tv_sec = timeoutms/1000;
	  tv.tv_usec = (timeoutms % 1000) * 1000;
	  ocpiDebug("[Socket::receive (ether)] Setting socket timeout to %u ms", timeoutms);
	  if (setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
	    setError(error, "setting receive timeout on socket");
	    return false;
	  }
	  m_timeout = timeoutms;
	}
	union {
	  struct sockaddr      saddr;
	  struct sockaddr_in   in;
	  struct sockaddr_ocpi ocpi;
#ifdef OCPI_OS_macos
	  struct sockaddr_dl   dl;
#else
	  struct sockaddr_ll   ll;
#endif
	} sa;
	socklen_t alen;  // must be set exactly, not larger
	int flags = 0;

	offset = offsetof(Packet, payload);
	Packet &packet(*(Packet *)buffer);
	void *payload = (void *)buffer;
	if (m_ifAddr.isEther()) {
#ifdef OCPI_OS_macos
	  alen = sizeof(sa.dl);
#else
	  flags = MSG_TRUNC;
	  if (haveDriver()) {
	    payload = packet.payload;
	    alen = sizeof(sa.ocpi);
	  } else
	    alen = sizeof(sa.ll);
#endif
	} else {
	  payload = packet.payload;
	  alen = sizeof(sa.in);
	}
	struct iovec iov;
	iov.iov_base = payload;
	iov.iov_len = sizeof(Packet);
	struct {
	  struct cmsghdr hdr;
	  struct in_pktinfo info;
	} cmsg;
	struct msghdr mh;
	mh.msg_name = &sa.saddr;
	mh.msg_namelen = alen;
	mh.msg_iovlen = 1;
	mh.msg_iov = &iov;
	mh.msg_control = &cmsg;
	mh.msg_controllen = (socklen_t)sizeof(cmsg);
	ssize_t rlen;
	do { // loop to filter out packets that arent used
	  if ((rlen = recvmsg(m_fd, &mh, flags)) < 0 && errno == EINTR)
	    continue;
	  if (rlen <= 0)
	    break;
	  // figure out the address to loop and skip packets to myself
	  if (m_ifAddr.isEther())
	    addr.set(haveDriver() ? sa.ocpi.ocpi_remote : packet.source);
	  else
	    addr.set(ntohs(sa.in.sin_port), sa.in.sin_addr.s_addr);
	  if (addr.addr64() != m_ifAddr.addr64())
	    break;
	  ocpiDebug("Received packet from myself\n");
	} while (1);

	if (rlen < 0 && errno == EWOULDBLOCK)
	  return false; // timeout
	if ((rlen <= 0) || (mh.msg_flags & MSG_TRUNC)) {
	  setError(error, "receiving %zd packet bytes failed%s", rlen,
		   mh.msg_flags & MSG_TRUNC ? ": truncated" : "");
	  return false;
	}
	if (mh.msg_namelen < alen) {
	  ocpiDebug("received sockaddr len is %d, should be %u", mh.msg_namelen, alen);
	  setError(error, "received sockaddr len is %d, should be %u", alen, sizeof(sa));
	  return false;
	}
	// Figure out actual payload length and ifindex
	unsigned int ifindex = 0;
	if (m_ifAddr.isEther()) {
	  Type type = ntohs(((Header *)&packet)->type);
	  if (m_type != type) {
	    setError(error, "Ethertype mismatch: ours is 0x%x, packet's is 0x%x",
		     m_type, type);
	    return false;
	  }
	  if (haveDriver()) {
	    ocpiDebug("driver packet fam %u role %u index %d",
		      sa.ocpi.ocpi_family, sa.ocpi.ocpi_role, sa.ocpi.ocpi_ifindex);
	    payLoadLength = (unsigned)rlen;
	    ifindex = sa.ocpi.ocpi_ifindex;
	  } else {
	    payLoadLength = (unsigned)(rlen - (sizeof(Header) - sizeof(uint16_t)));
#ifdef OCPI_OS_linux
	    ocpiDebug("fam %u prot %u index %d hatype %u ptype %u len %u off %zu",
		      sa.ll.sll_family, sa.ll.sll_protocol, sa.ll.sll_ifindex, sa.ll.sll_hatype,
		      sa.ll.sll_pkttype, sa.ll.sll_halen, offsetof(struct sockaddr_ll, sll_addr[0]));
	    ifindex = sa.ll.sll_ifindex;
#else
	    ocpiDebug("fam %u index %u type %u len %u off %zu",
		      sa.dl.sdl_family, sa.dl.sdl_index, sa.dl.sdl_type, sa.dl.sdl_len,
		      offsetof(struct sockaddr_dl, sdl_data[0]));
	    ifindex = sa.dl.sdl_index;
#endif
	  }
	} else {
	  payLoadLength = (unsigned)rlen;
	  ocpiDebug("udp: fam %u port %u addr %s",
		    sa.in.sin_family, ntohs(sa.in.sin_port), inet_ntoa(sa.in.sin_addr));
	  // iterate through all the control headers
	  for (struct cmsghdr *cmsg_tmp = CMSG_FIRSTHDR(&mh); cmsg_tmp != NULL; cmsg_tmp = CMSG_NXTHDR(&mh, cmsg_tmp))
	    if ((cmsg_tmp->cmsg_level == IPPROTO_IP) && (cmsg_tmp->cmsg_type == IP_PKTINFO)) {
	      ifindex = ((struct in_pktinfo *)CMSG_DATA(cmsg_tmp))->ipi_ifindex;
	      break;
	    }
	}
	if (indexp) {
	  if (ifindex)
	    *indexp = ifindex;
	  else {
	    setError(error, "Cannot determine interface index");
	    return false;
	  }
	}
	ocpiDebug("Received packet length %zu address: sizeof %zu alen %u family %u index %u", 
		  rlen, sizeof(sa), mh.msg_namelen, ((struct sockaddr *)&sa)->sa_family, ifindex);
	return true;
      }

      bool Socket::
      send(Packet &packet, size_t payLoadLength, Address &addr, unsigned timeoutms,
	   Interface *ifc, std::string &error) {
	if (payLoadLength > sizeof(Packet) - (sizeof(Header) - sizeof(uint16_t))) {
	  setError(error, "sending packet that is too long: %u, p %u h %u",
		   payLoadLength, sizeof(Packet), sizeof(Header));
	  return false;
	}
	OS::IOVec iov[2];
	iov[0].iov_base = packet.payload;
	iov[0].iov_len = payLoadLength;
	return send(iov, 1, addr, timeoutms, ifc, error);
      }
      bool Socket::
      send(IOVec *iov, unsigned iovlen, Address &addr, unsigned /*timeoutms*/,
	   Interface *ifc, std::string &error) {
	ocpiDebug("Sending to ifs %s addr %s bcast %u",
		  ifc ? ifc->name.c_str() : "none", addr.pretty(), addr.isBroadcast());
	IOVec myiov[10];
	ocpiAssert(iovlen < 10);
	union {
#ifdef OCPI_OS_macos
	  struct sockaddr_dl dl;
#else
	  struct sockaddr_ll ll;
#endif
	  struct sockaddr_ocpi ocpi;
	  struct sockaddr_in in;
	} sa;
	memset(&sa, 0, sizeof(sa));
	struct msghdr msg;
        msg.msg_name = (void*)&sa;
	msg.msg_control = 0;
	msg.msg_controllen = 0;
	msg.msg_flags = 0; // FIXME: checkfor MSG_TRUNC
	char cbuf[CMSG_SPACE(sizeof(struct in_pktinfo))]; // keep in scope in case it is used.

	// FIXME: see if we really need this in raw sockets at all, since it is redundant 
	if (m_ifAddr.isEther() && addr.isEther()) {
	  if (haveDriver()) {
	    sa.ocpi.ocpi_family = PF_OPENCPI;
	    sa.ocpi.ocpi_role = m_role;
	    sa.ocpi.ocpi_ifindex = (uint8_t)m_ifIndex;
	    memcpy(sa.ocpi.ocpi_remote, addr.addr(), Address::s_size);
	    msg.msg_namelen = (socklen_t)sizeof(sa.ocpi);
	  } else {
#ifdef OCPI_OS_macos
	    sa.dl.sdl_len = sizeof(sa);
	    sa.dl.sdl_family = AF_LINK;
	    sa.dl.sdl_index = 0;
	    sa.dl.sdl_type = IFT_ETHER;
	    sa.dl.sdl_nlen = 0;
	    sa.dl.sdl_alen = 6;
	    sa.dl.sdl_slen = 0;
	    memcpy(sa.dl.sdl_data, addr.addr(), Address::s_size);
	    msg.msg_namelen = (socklen_t)sizeof(sa.dl);
#else
	    sa.ll.sll_family = PF_PACKET;
	    memcpy(sa.ll.sll_addr, addr.addr(), Address::s_size);
	    sa.ll.sll_halen = Address::s_size;
	    sa.ll.sll_ifindex = m_ifIndex;
	    msg.msg_namelen = (socklen_t)sizeof(sa.ll);
#endif
	    Header header;
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
	} else {
#ifdef OCPI_OS_macos
	  sa.in.sin_len = sizeof(sa.in);
#endif
	  msg.msg_namelen = (socklen_t)sizeof(sa.in);
	  sa.in.sin_family = AF_INET;
	  if (addr.isBroadcast()) {
	    if (!ifc) {
	      error = "No interface specified for UDP broadcast";
	      return false;
	    }
#if 1
	    sa.in.sin_addr.s_addr = addr.addrInAddr();
	    ocpiDebug("addr is %x, %s", sa.in.sin_addr.s_addr, inet_ntoa(sa.in.sin_addr));
#else
	    sa.in.sin_addr.s_addr = ifc->brdAddr.addrInAddr();
#endif
	    sa.in.sin_port = htons(addr.addrPort() ? addr.addrPort() : c_udpPort);
	    // For broadcast, we specify the interface, otherwise IP routing does the right thing.
	    memset(cbuf, 0, sizeof(cbuf));
	    msg.msg_control = cbuf;
	    msg.msg_controllen = (socklen_t)sizeof(cbuf);
	    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	    //msg.msg_control = NULL;
	    //	    msg.msg_controllen = 0;
	    cmsg->cmsg_level = IPPROTO_IP;
	    cmsg->cmsg_type = IP_PKTINFO;
	    cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
	    struct in_pktinfo *pip = (struct in_pktinfo *)CMSG_DATA(cmsg);
	    pip->ipi_ifindex = ifc->index;
	  } else {
	    sa.in.sin_port = htons(addr.addrPort());
	    sa.in.sin_addr.s_addr = addr.addrInAddr();
	  }
	}
	size_t len = 0;
	for (IOVec *i = iov; i < &iov[iovlen]; i++)
	  len += i->iov_len;
        msg.msg_iov = (iovec*)iov;
	msg.msg_iovlen = iovlen;
	ssize_t rlen = sendmsg(m_fd, &msg, 0);
	ocpiDebug("Send packet length %zd, to %s/%s via %u(%s), port %u returned %zd errno %u (%s) fd %u",
		  len, inet_ntoa(sa.in.sin_addr), addr.pretty(), ifc ? ifc->index : 0,
		  m_ifAddr.pretty(),
		  ntohs(sa.in.sin_port), rlen, errno, strerror(errno), m_fd);
	if (rlen != (ssize_t)len) {
	  setError(error, "sendto of %u bytes failed, returning %d", len, rlen);
	  return false;
	}
	return true;
      }
      // status of interface scanning
      struct Opaque {
#ifdef OCPI_OS_macos
	char *buffer, *end;
	struct if_msghdr *ifm;
#else
#define NETIFDIR "/sys/class/net"
        typedef std::vector<std::pair<unsigned int, std::string> > ifnames_t;
        ifnames_t *ifnames;
#endif
      };

      bool IfScanner::findIpAddr(const char *interface, std::string &ipAddr, std::string &error) {
	IfScanner ifs(error);
	Interface eif;
	if (error.empty())
	  while (ifs.getNext(eif, error))
	    if (eif.connected && eif.up && !eif.loopback && eif.addr.isEther() &&
		eif.ipAddr.addrInAddr()) {
	      if (interface && strcasecmp(eif.name.c_str(), interface))
		  continue;
	      ipAddr = eif.ipAddr.prettyInAddr();
	      ocpiInfo("Choosing our IP address, %s, using interface %s with MAC %s",
		       ipAddr.c_str(), eif.name.c_str(), eif.addr.pretty());
	      break;
	    }
	return !error.empty();
      }
      IfScanner::IfScanner(std::string &err)
	: m_init(false), m_index(0) {
	ocpiAssert((compileTimeSizeCheck<sizeof (m_opaque), sizeof (Opaque)> ()));
	memset(m_opaque, 0, sizeof(m_opaque));
#ifdef OCPI_OS_macos
#else
        Opaque *o = (Opaque *)m_opaque;
        o->ifnames = new Opaque::ifnames_t();
#endif
	err.clear();
      }
      IfScanner::
      ~IfScanner() {
        Opaque *o = (Opaque *)m_opaque;
#ifdef OCPI_OS_macos
	delete [] o->buffer;
#else
        delete o->ifnames;
#endif
      }

      void IfScanner::
      reset() {
	m_index = 0;
#ifdef OCPI_OS_macos
        Opaque &o = *(Opaque *)m_opaque;
	o.ifm = (struct if_msghdr *)o.buffer;
#endif
      }

      // Delayed initialization - done when we get to the real interfaces
      // (after the udp one).
      // return true if error set
      bool IfScanner::
      init(std::string &err) {
	Opaque &o = *(Opaque *)m_opaque;
	err.clear();
#ifdef OCPI_OS_macos
	o.buffer = NULL;
	// We need this loop because things can change out from under us.
	for (unsigned n = 0; err.empty() && n < 10; n++) {
	  int mib[6] = { CTL_NET, PF_ROUTE, 0, 0, NET_RT_IFLIST, 0};
	  size_t space;
	  if (sysctl(mib, 6, NULL, &space, NULL, 0) < 0)
	    setError(err, "can't sysctl to get buffer size for interfaces");
	  else if (space) {
	    o.buffer = new char[space];
	    o.end = o.buffer + space;
	    if (sysctl(mib, 6, o.buffer, &space, NULL, 0) < 0) {
	      if (errno == ENOMEM && n < 10) {
		ocpiBad("Routing table grew, retrying\n");
		delete o.buffer;
		o.buffer = NULL;
		usleep(10000);
	      } else
		setError(err, "can't sysctl to retrieve interface info");
	    } else
	      break;
	  }
	}
	if (err.empty()) {
	  if (o.buffer)
	    o.ifm = (struct if_msghdr *)o.buffer;
	  else
	    err = "sysctl failed after 10 retries";
	}
#else
        // Put together a list of possible interface names.
        struct if_nameindex *if_ni, *i;
        Opaque::ifnames_t &ifnames = *o.ifnames; // Alias
        if_ni = if_nameindex();
        if (if_ni == NULL) {
            setError(err, "failed call to if_nameindex");
        } else {
          for (i = if_ni; !(i->if_index == 0 && i->if_name == NULL); ++i) {
            ifnames.push_back(std::make_pair(i->if_index, i->if_name));
            ocpiDebug("if_nameindex scan: %u = %s (%zu total)", i->if_index, i->if_name, ifnames.size());
          }
          if_freenameindex(if_ni);
          std::sort(ifnames.begin(), ifnames.end());
        }
#endif
	m_init = true;
	return !err.empty();
      }

#ifdef OCPI_OS_linux
      // Return true if we got a value from the named file (e.g. /sys/class/net/XXX/carrier)
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
	if (sresult)
          *sresult = buf;
	return true;
      }
#endif

#ifdef OCPI_OS_macos
/*
 * Systems missing SA_SIZE(). Taken from FreeBSD net/route.h:1.63
 */
#ifndef SA_SIZE
#define SA_SIZE(sa)						\
    (  (!(sa) || ((struct sockaddr *)(sa))->sa_len == 0) ?	\
	sizeof(long)		:				\
	1 + ( (((struct sockaddr *)(sa))->sa_len - 1) | (sizeof(long) - 1) ) )
#endif
      static void
      doNewAddr(struct if_msghdr *ifm, char *end, Interface &i) {
	struct ifa_msghdr *ifa = (struct ifa_msghdr *)ifm;
	char *extra = (char *)(ifa + 1);
	// We have an address record for the "found" interface
	for (unsigned n = 0; n < RTAX_MAX && extra < end; n++)
	  if (ifm->ifm_addrs & (1 << n)) {
	    struct sockaddr *sa = (struct sockaddr *)extra;
	    extra += SA_SIZE(sa);
	    ocpiDebug("%u: NEWADDR n %u len %u af %u",
		      ifm->ifm_index, n, sa->sa_len, sa->sa_family);
	    if (sa->sa_family == AF_INET) {
	      struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	      ocpiDebug("IP Addr is %u %s", ntohs(sin->sin_port), inet_ntoa(sin->sin_addr));
	      switch (n) {
	      case RTAX_IFA:
		i.ipAddr.set(ntohs(sin->sin_port), sin->sin_addr.s_addr);
		break;
	      case RTAX_BRD:
		i.brdAddr.set(ntohs(sin->sin_port), sin->sin_addr.s_addr);
		break;
	      default:
		;
	      }
	    }
	  }
      }
      static bool
      doIfInfo(struct if_msghdr *ifm, const char *end, const char *only, Interface &i) {
	char *extra = (char *)(ifm + 1);
	ocpiDebug("iface %u: flags 0x%04x addrs 0x%x type 0x%x",
		  ifm->ifm_index, ifm->ifm_flags, ifm->ifm_addrs, ifm->ifm_data.ifi_type);
	for (unsigned n = 0; n < RTAX_MAX && extra < end; n++)
	  if (ifm->ifm_addrs & (1 << n)) {
	    struct sockaddr *sa = (struct sockaddr *)extra;
	    extra += sa->sa_len;
	    if (n == RTAX_IFP) {
	      struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;
	      ocpiDebug("sockaddr_dl: addr %2u alen %2u %2u %2u %2u", n,
			sdl->sdl_len, sdl->sdl_nlen, sdl->sdl_alen, sdl->sdl_slen);
	      ocpiAssert(sdl->sdl_nlen);
	      ocpiDebug("iface sock type %u if type %u ifphys %u name: '%.*s' only: %s",
			sdl->sdl_type, ifm->ifm_data.ifi_type, ifm->ifm_data.ifi_physical, sdl->sdl_nlen,
			sdl->sdl_data, only ? only : "\"\"");
	      if ((ifm->ifm_data.ifi_type == IFT_ETHER ||
		   ifm->ifm_data.ifi_type == IFT_LOOP ||
		   ifm->ifm_data.ifi_type == IFT_BRIDGE) &&
		  (!only || 	    
		   (sdl->sdl_nlen == strlen(only) &&
		    !strncmp(sdl->sdl_data, only, sdl->sdl_nlen)))) {
		i.name.assign(sdl->sdl_data, sdl->sdl_nlen);
		i.index = ifm->ifm_index;
		i.up = (ifm->ifm_flags & IFF_UP) != 0;
		if (ifm->ifm_data.ifi_type == IFT_LOOP) {
		  i.connected = true;
		  ocpiDebug("loopback: %s, up: %d connected: %d", i.addr.pretty(), i.up, i.connected);
		  i.addr.set(0, 0);
		  i.loopback = true;
		  return true;
		}
		struct ifmediareq ifmr;
		memset(&ifmr, 0, sizeof(ifmr));
		strncpy(ifmr.ifm_name, sdl->sdl_data, sdl->sdl_nlen);
		ifmr.ifm_name[sdl->sdl_nlen] = 0;
		// PF_INET/SOCK_DGRAM since it works without root privileges.
		int s = socket(PF_INET, SOCK_DGRAM, 0);
		ocpiAssert(s);
		ocpiCheck(ioctl(s, SIOCGIFMEDIA, &ifmr) == 0);
		::close(s);
		ocpiDebug("IFMEDIA: 0x%x:", ifmr.ifm_status);
		if (sdl->sdl_alen && sdl->sdl_type == IFT_ETHER &&
		    sdl->sdl_alen == Address::s_size) {
		  i.connected =
		    (ifmr.ifm_status & (IFM_AVALID|IFM_ACTIVE)) == (IFM_AVALID|IFM_ACTIVE);
		  i.addr.set(sdl->sdl_data + sdl->sdl_nlen);
		  i.loopback = false;
		  ocpiDebug("ether: %s, up: %d connected: %d", i.addr.pretty(), i.up, i.connected);
		  return true;
		}
	      }
	    }
	  }
	return false;
      }

#endif // MacOS Only

      bool IfScanner::
      getNext(Interface &i, std::string &err, const char *only) {
	err.clear();
	i.init();
	Opaque &o = *(Opaque *)m_opaque;
	if (m_index == 0) {
	  if (only && (!strcmp(only, "udp") || !strcmp(only, "udplocal"))) {
	    // The udp "pseudo-interface", either public or local
	    i.addr.set(0, ntohl(!strcmp(only, "udp") ? INADDR_ANY : INADDR_LOOPBACK));
	    i.index = 0;
	    i.name = "udp";
	    i.up = true;
	    i.connected = true;
	    return true;
	  }
	  if (!m_init && init(err))
	    return false;
	}
#ifdef OCPI_OS_macos
	ocpiAssert(o.buffer);
	// Loop through all messages until we have a good one.
	bool found = false;
	for (struct if_msghdr *ifm = o.ifm; (char *)ifm < o.end; ifm = o.ifm) {
	  if (ifm->ifm_type == RTM_IFINFO && found)
	    break;
	  // We are now consuming this message
	  char *end = (char *)ifm + ifm->ifm_msglen;
	  o.ifm = (struct if_msghdr *)end;
	  switch (ifm->ifm_type) {
	  default: ;
	  case RTM_NEWADDR:
	    if (!found)
	      continue; // skip it if we skipped the interface itself
	    ocpiDebug("RTM_NEWADDR: %u: len: %u vers: %u flags: 0x%x",
		      ifm->ifm_index, ifm->ifm_msglen, ifm->ifm_version, ifm->ifm_flags);
	    doNewAddr(ifm, end, i);
	    break;
	  case RTM_IFINFO:
	    if (found)
	      break;
	    found = doIfInfo(ifm, end, only, i);
	  } // switch on message types
	} // loop over sysctl ifm messages
	if (!found && only)
	  err = "the requested interface was not found";
	return found;
#else
        Opaque::ifnames_t &ifnames = *o.ifnames; // Alias
	while (m_index < ifnames.size()) {
          const size_t v_index = m_index++;
          if (!only or ifnames[v_index].second == only) {
	    std::string s(NETIFDIR), addr;
	    s += '/';
	    s += ifnames[v_index].second;
	    long nval, carrier, flags;
	    if (getValue(s, "type", &nval) && (nval == ARPHRD_ETHER || nval == ARPHRD_LOOPBACK) &&
		getValue(s, "address", NULL, &addr) &&
		getValue(s, "carrier", &carrier) &&
		getValue(s, "flags", &flags)) {
	      if (nval == ARPHRD_ETHER) {
		i.addr.setString(addr.c_str());
	      } else {
		i.addr.set(0,0); // loop back is not an ether interface
		i.loopback = true;
	      }
	      if (!i.addr.hasError()) {
		int fd = socket(PF_INET, SOCK_DGRAM, 0);
		if (fd < 0) {
		  err = "can't open socket for interface addresses";
		  return false;
		}
		struct ifreq ifr;
		strncpy(ifr.ifr_name, ifnames[v_index].second.c_str(), IFNAMSIZ);
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
		  i.ipAddr.set(0, ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);
		  if (ioctl(fd, SIOCGIFBRDADDR, &ifr) == 0) {
		    i.brdAddr.set(0, ((struct sockaddr_in *)&ifr.ifr_broadaddr)->sin_addr.s_addr);
		    i.name = ifnames[v_index].second;
		    i.index = ifnames[v_index].first;
		    i.up = (flags & IFF_UP) != 0;
		    i.connected = carrier != 0;
		    ocpiDebug("found ether interface '%s' which is %s, %s, at address %s",
			      i.name.c_str(), i.up ? "up" : "down",
			      i.connected ? "connected" : "disconnected", i.addr.pretty());
		    ::close(fd);
		    return true;
		  }
		} else {
		  ocpiDebug("ioctl(%d, SIOCGIFADDR) call failed for '%s'", fd, ifr.ifr_name);
		}
		::close(fd);
	      } else {
		ocpiDebug("Unknown parsing error for '%s'", ifnames[v_index].second.c_str());
	      }
	    } else {
	      ocpiDebug("Invalid type or failed parsing parameter file(s) for '%s'", s.c_str());
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
      Interface::Interface() {
	init();
      }
      void Interface::init() {
	index = 0;
	name.clear();
	addr.set(0);
	ipAddr.set(0);
	brdAddr.set(0);
	up = connected = loopback = false;
      }
      Interface::Interface(const char *name_in, std::string &error) {
	IfScanner ifs(error);
	while (error.empty() && ifs.getNext(*this, error, name_in))
	  if (up && connected)
	    return;
	error = "No interfaces found that are up and connected";
      }
    }
  }
}
