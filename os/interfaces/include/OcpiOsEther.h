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

// -*- c++ -*-

#ifndef OCPIOSETHER_H__
#define OCPIOSETHER_H__

#include <inttypes.h>
#include <memory.h>
#include <stdio.h>
#include <string>

#include "OcpiOsIovec.h"
#include "KernelDriver.h"
// Ethernet support

namespace OCPI {
  namespace OS {
    namespace Ether {
      const uint16_t c_udpPort = 18077;
      const char c_multicastGroup[] = "224.0.0.1";
      typedef uint16_t Type;
      // Note this value includes the ethertype bytes
      const unsigned MaxPacketSize = (1518 - 4 - 12);
      // This is worth inlining since addresses are processed at runtime.
      extern bool haveDriver();
      class Address {
      public:
	static const unsigned s_size = 6;
      private:
	bool m_isEther;
	union {
	  uint8_t m_addr[s_size];
	  struct {
	    uint32_t addr;
	    uint16_t port;
	  } m_udp;
	  uint64_t m_addr64;
	};
	char m_pretty[s_size*3+1];
	bool m_error, m_broadcast;
      public:
        inline bool isEther() const { return m_isEther; }
        bool isLoopback() const;
        bool setString(const char *x); // return true on error
        void set(const void *x);
        void set(uint16_t, uint32_t);
        void set64(uint64_t a) {
          set(NULL); // initialize m_pretty, etc
          m_addr64 = a;
        }
        bool empty() { return m_addr64 == 0; }
        Address(bool isUdp, uint16_t port = 0);

	inline explicit Address(const unsigned char *m = NULL) {
	  set(m);
	}
	inline explicit Address(const char *m) {
	  setString(m);
	}
	Address(const Address &addr_in) {
	  m_isEther = addr_in.m_isEther;
	  set64(addr_in.addr64());
	  m_broadcast = addr_in.m_broadcast;
	  m_pretty[0] = 0;
	}
	inline const uint8_t *addr() const { return m_addr; }
	inline uint64_t addr64() const { return m_addr64; }
	inline uint16_t addrPort() const { return m_udp.port; }
	inline uint32_t addrInAddr() const { return m_udp.addr; }
	const char *pretty();
	const char *prettyInAddr();
	inline bool hasError() const { return m_error; }
	inline bool isBroadcast() const { return m_broadcast; }
	inline bool operator==(const Address &rhs) {
	  return memcmp(addr(), rhs.addr(), s_size) == 0;
	}
	inline bool operator!=(const Address &rhs) {
	  return memcmp(addr(), rhs.addr(), s_size) != 0;
	}
	static Address s_broadcast;
	class Compare {
	public:
	  bool operator() (const Address &a, const Address &b) const {
	    return memcmp(a.addr(), b.addr(), s_size) < 0;
	  }
	};
      };
      struct Interface;
      struct Header {
	uint8_t destination[Address::s_size];
	uint8_t source[Address::s_size];
	Type type;
      };
      struct Packet {
	uint8_t destination[Address::s_size];
	uint8_t source[Address::s_size];
	uint8_t payload[MaxPacketSize];
      };
      // A socket that sends and receives L2 layer packets.
      // Note that receives are done into internal buffers and it is assumed that
      // the packet buffer can be reused on the next receive call.
      // This socket is for communicating to a simple adress,
      // but since L2 sockets are only bound to an ethertype, we receive
      // from everyone.  Hence even though there is one of these sockets per destination,
      // there may only be one underlying socket, depending on the OS/implementation.
      class Socket {
	unsigned m_ifIndex;
	Address m_ifAddr, m_brdAddr;
	//m_ipAddr;
	Type m_type;
	int m_fd;
	unsigned m_timeout;
	ocpi_role_t m_role;
	//	uint16_t m_endpoint;
      public:
	Socket(Interface &, ocpi_role_t role, Address *remote, uint16_t endpoint, std::string &error);
	~Socket();
	inline Address &ifAddr() { return m_ifAddr; }
	inline unsigned ifIndex() { return m_ifIndex; }
	//	inline Address &ipAddr() { return m_ipAddr; }
	// These return false if error or timeout
	// Receive directly into packet buffer.
	bool receive(Packet &, size_t &payLoadLength, unsigned timeoutms,
		     Address &addr, std::string &error, unsigned *indexp = NULL);
	bool receive(uint8_t *buf, size_t &offset, size_t &length, unsigned timeoutms,
		     Address &addr, std::string &error, unsigned *indexp = NULL);
	// Send, and fill out the addressing first (hence packet not const).
	// If no "addr", then use address in socket.
	// If an ifc is supplied, send on that ifc.
	// Return false if error or timeout
	bool send(Packet &, size_t payloadLength, Address &addr, unsigned timeoutms,
		  Interface *, std::string &error);
	bool send(IOVec *, unsigned vecLen, Address &addr, unsigned timeoutms,
		  Interface *, std::string &error);
	inline int fd() const { return m_fd; }
      };
      // The interface object has no dependencies on the scanner.
      // It also does not have to persist after sockets are created from it.
      struct Interface {
	Interface();
	Interface(const char *name, std::string &error);
	void init();
	unsigned index;
	std::string name;
	Address addr, ipAddr, brdAddr;
	bool up, connected, loopback;
      };
      class IfScanner {
	uint64_t m_opaque[4];
	bool     m_init;
	size_t   m_index;
      public:
	static bool
	findIpAddr(const char *interface, std::string &ipAddr, std::string &error);
	// Initialize the scanner
	// err is set if there is an error
	IfScanner(std::string &err);
	~IfScanner();
	// Get the basic information about the current interface, without "opening" it for I/O
	bool getNext(Interface &i, std::string &err, const char *only = NULL);
	void reset();
      private:
	bool init(std::string &err);
      };
    }
  }
}

#endif
