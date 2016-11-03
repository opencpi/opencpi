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

/*
 * Generic net driver, used for ethernet, udp, and sim
 */

#ifndef HDLNETDRIVER_H
#define HDLNETDRIVER_H

#include <string>
#include <map>

#include "OcpiOsEther.h"
#include "OcpiPValue.h"
#include "HdlDevice.h"
#include "HdlNetDefs.h"

namespace OCPI {
  namespace HDL {
    namespace Net {
      const unsigned RETRIES = 3;
      const unsigned DELAYMS = 500;
      const unsigned MAX_INTERFACES = 10;

      class Device;
      class Driver {
	friend class Device;
	// discovery socket. pointer since we only open it on demand
	// OCPI::OS::Ether::Socket *m_socket;
	//	const char **m_exclude;            // during discovery
	// A mapping from interface name to sockets per interface, during discovery
	typedef std::map<const std::string, OCPI::OS::Ether::Socket *> Sockets;
	typedef Sockets::iterator SocketsIter;
	typedef std::pair<std::string, OCPI::OS::Ether::Socket*> SocketPair;
	typedef std::pair<OCPI::OS::Ether::Address,OCPI::OS::Ether::Interface *> MacPair;
	// The string keys for a server are a string of mac-address/pid
	typedef std::pair<std::string, MacPair> MacInsert;
	typedef std::map<std::string, MacPair> Macs;
	typedef Macs::iterator MacsIter;
	Sockets m_sockets;
	bool trySocket(OCPI::OS::Ether::Interface &ifc, OCPI::OS::Ether::Socket &s,
		       OCPI::OS::Ether::Address &addr, bool discovery, const char **exclude,
		       Macs *macs, Device **dev, std::string &error);
	// Try to find one or more devices on this interface
	// mac is NULL for broadcast
	// discovery is false only if mac is true
	unsigned 
	tryIface(OCPI::OS::Ether::Interface &ifc, OCPI::OS::Ether::Address &devAddr,
		 const char **exclude, Device **dev, bool discovery, Macs *macs,
		 std::string &error);
      protected:
	virtual ~Driver();
	// device constructor
	virtual Device *createDevice(OS::Ether::Interface &ifc, OS::Ether::Address &addr,
				     bool discovery, std::string &error) = 0;
      public:
	// Find the discovery socket for this interface
	OCPI::OS::Ether::Socket *
	findSocket(OCPI::OS::Ether::Interface &ifc, bool discovery, std::string &error);
	unsigned
	search(const OCPI::Util::PValue *props, const char **exclude, bool discoveryOnly,
	       bool verbose, bool udp, std::string &error);
	OCPI::HDL::Device *
	open(const char *etherName, bool discovery, std::string &err);
	// Callback when found
	virtual bool found(OCPI::HDL::Device &dev, const char **excludes, bool discoveryOnly,
			   bool verbose, std::string &error) = 0;
      };
      class Device
	: public OCPI::HDL::Device,
	  public OCPI::HDL::Accessor {
	friend class Driver;
	OS::Ether::Socket *m_socket;
	OS::Ether::Address m_devAddr;
	OS::Ether::Packet m_request;
	std::string m_error;
	bool m_discovery;
	unsigned m_delayms;
      protected:
	Device(Driver &driver, OCPI::OS::Ether::Interface &ifc, std::string &name,
	       OCPI::OS::Ether::Address &devAddr, bool discovery, const char *data_proto,
	       unsigned delayms,  uint64_t ep_size, uint64_t controlOffset, uint64_t dataOffset,
	       std::string &);
      public:
	virtual ~Device();
	// Load a bitstream via jtag
	//	virtual void load(const char *) = 0;
	inline OS::Ether::Address &addr() { return m_devAddr; }
	void setAddr(OS::Ether::Address &addr);
	uint32_t dmaOptions(ezxml_t icImplXml, ezxml_t icInstXml, bool isProvider);
      protected:
	// Tell me which socket to use (not to own)
	//	inline void setSocket(OCPI::OS::Ether::Socket &socket) { m_socket = &socket; }
	void request(EtherControlMessageType type, RegisterOffset offset,
		     size_t bytes, OCPI::OS::Ether::Packet &recvFrame, uint32_t *status,
		     size_t extra = 0, unsigned delayms = 0);
	// Shared "get" that returns value, and *status if status != NULL
	uint32_t get(RegisterOffset offset, size_t bytes, uint32_t *status);
	void set(RegisterOffset offset, size_t bytes, uint32_t data, uint32_t *status);
	void command(const char *cmd, size_t bytes, char *response, size_t rlen, unsigned delay);
      public:
	uint64_t get64(RegisterOffset offset, uint32_t *status);
#if 0
	inline uint32_t get32(RegisterOffset offset, uint32_t *status) {
	  return get(offset, sizeof(uint32_t), status);
	}
	inline uint16_t get16(RegisterOffset offset, uint32_t *status) {
	  return (uint16_t)(get(offset, sizeof(uint16_t), status) >> ((offset&3)*8));
	}
	inline uint8_t get8(RegisterOffset offset, uint32_t *status) {
	  return (uint8_t)(get(offset, sizeof(uint8_t), status) >> ((offset&3)*8));
	}
#endif
	void getBytes(RegisterOffset offset, uint8_t *buf, size_t length, size_t elementBytes,
		      uint32_t *status, bool string);
	void set64(RegisterOffset offset, uint64_t val, uint32_t *status);
#if 0
	inline void set32(RegisterOffset offset, uint32_t val, uint32_t *status) {
	  set(offset, sizeof(uint32_t), val, status);
	}
	inline void set16(RegisterOffset offset, uint16_t val, uint32_t *status) {
	  set(offset, sizeof(uint16_t), val << ((offset & 3) * 8), status);
	}
	inline void set8(RegisterOffset offset, uint8_t val, uint32_t *status) {
	  set(offset, sizeof(uint8_t), val << ((offset & 3) * 8), status);
	}
#endif
	void setBytes(RegisterOffset offset, const uint8_t *buf, size_t length,
		      size_t elementBytes, uint32_t *status);
      };
    }
  }
}
#endif
