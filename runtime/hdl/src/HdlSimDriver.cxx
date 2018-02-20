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

#include <inttypes.h>
#include <unistd.h>
#include <assert.h>
#include "OcpiOsFileSystem.h"
#include "OcpiOsSocket.h"
#include "HdlSdp.h"
#include "HdlSimDriver.h"

namespace OCPI {
  namespace HDL {
    namespace Sim {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
      namespace OE = OCPI::OS::Ether;
      namespace OH = OCPI::HDL;
      
      class Device
	: public OCPI::HDL::Net::Device {
	friend class Driver;
	OS::Socket m_sdpSocket;
	bool m_sdpConnected;
      protected:
	Device(Driver &driver, OS::Ether::Interface &ifc, std::string &a_name,
	       OE::Address a_addr, bool discovery, const OU::PValue *params, std::string &error)
	  : Net::Device(driver, ifc, a_name, a_addr, discovery, "ocpi-udp-rdma", 10000,
			OH::SDP::Header::max_addressable_bytes * OH::SDP::Header::max_nodes,
			0, OH::SDP::Header::max_addressable_bytes, params, error),
	    m_sdpConnected(false) {
#if 0
	  // Send the "flush all state - I am a new master" command.
	  if (error.empty() && !discovery)
	    command("F", 2, NULL, 0, 5000);
#endif
	  init(error);
	}
      public:
	~Device() {
	}
      private:
	// Convenience for setting the m_isAlive and throwing OU::Error
	void throwit(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
	  m_isAlive = false;
	  va_list ap;
	  va_start(ap, fmt);
	  throw OU::Error(ap, fmt);
	}
	void
	doConnect(const char *portString) {
	  uint16_t port = OCPI_UTRUNCATE(uint16_t, atoi(portString));
	  std::string l_addr = this->addr().pretty(); // this will have a colon and port...
	  l_addr.resize(l_addr.find_first_of(':'));
	  m_sdpSocket.connect(l_addr, port);
	  m_sdpConnected = true;
	}
      public:
	// After loading or after deciding that the loaded bitstream is ok, this is called.
	void
	connect() {
	  char err[1000];
	  err[0] = 0;
	  command("C", 2, err, sizeof(err), 5000);
	  if (*err != 'O')
	    throwit("Error contacting/running simulator: %s", err + 1);
	  doConnect(err + 1);
	}

	// Load a bitstream via jtag
	bool load(const char *a_name, std::string &error) {
	  char err[1000];
	  err[0] = 0;
	  std::string cmd(a_name);
	  bool isDir;
	  uint64_t length;
	  if (!OS::FileSystem::exists(cmd, &isDir, &length) || isDir)
	    return OU::eformat(error,
			       "New sim executable file '%s' is non-existent or a directory",
			       a_name);
	  std::string cwd_str = OS::FileSystem::cwd();
	  OU::format(cmd, "L%" PRIu64 " %s %s", length, a_name, cwd_str.c_str());
	  command(cmd.c_str(), cmd.length()+1, err, sizeof(err), 5000);
	  ocpiDebug("load response: '%s'", err);
	  err[sizeof(err)-1] = 0;
	  switch (*err) {
	  case 'E': // we have an error
	    return OU::eformat(error, "Loading new executable %s failed: %s", a_name, err + 1);
	  case 'O': // OK its loaded and running - presumbably since it was cached?
	    ocpiInfo("Successfully loaded new sim executable: %s", a_name);
	    break;
	  case 'X': // The file needs to be transferred
	    {
	      uint16_t port = OCPI_UTRUNCATE(uint16_t, atoi(err + 1));
	      std::string l_addr = this->addr().pretty(); // this will have a colon and port...
	      l_addr.resize(l_addr.find_first_of(':'));
	      
	      OS::Socket wskt;
	      wskt.connect(l_addr, port);
	      wskt.linger(true); //  wait on close for far side ack of all data
	      int rfd = -1;
	      if ((rfd = open(a_name, O_RDONLY)) < 0)
		OU::format(error, "Can't open executable: %s", a_name);
	      else {
		char buf[64*1024];
		ssize_t n;
		while ((n = read(rfd, buf, sizeof(buf))) > 0) {
		  while (n) {
		    unsigned nn = (unsigned)wskt.send(buf, n);
		    // no errors - it throws
		    n -= nn;
		  }
		}
		if (n < 0)
		  OU::format(error, "Error reading executable file: %s", a_name);
		else {
		  wskt.shutdown(true);
		  char c;
		  // Wait for the other end to shutdown its writing side to give us the EOF
		  wskt.recv(&c, 1, 0);
		}
		close(rfd);
	      }
	      wskt.close();
	      if (error.empty()) {
		cmd = "S";
		command(cmd.c_str(), cmd.length()+1, err, sizeof(err), 5000);
		if (*err == 'O') {
		  ocpiInfo("Successfully started new sim executable: %s", a_name);
		  doConnect(err + 1);
		} else
		  OU::format(error, "Running new executable %s failed: %s", a_name, err + 1);
	      }
	    }
	  }
	  return error.empty() ? init(error) : false;
	}
	void
	sdpRequest(bool read, uint64_t a_address, size_t length, uint8_t *data,
		   uint32_t *status) {
	  RegisterOffset address = OCPI_UTRUNCATE(RegisterOffset, a_address);
	  if (m_sdpConnected) {
	    if (m_isFailed)
	      throw OU::Error("HDL::Sim::Device::request after previous failure");
	    OH::SDP::Header h(read, address, length);
	    std::string error;
	    if (h.doRequest(m_sdpSocket, data, error)) {
	      m_isFailed = true;
	      throw OU::Error("HDL Sim SDP error: %s", error.c_str());
	    }
	    uint32_t result = 0;
	    if (status)
	      *status = result;
	    else if (result) {
	      m_isFailed = true;
	      throw OU::Error("HDL Sim SDP control error");
	    }
	  } else
	    // Call back to the UDP control path for emulated accesses
	    switch (length) {
	    case 8:
	      {
		uint32_t *u32 = (uint32_t*)data;
		if (read) {
		  *u32++ = HDL::Net::Device::get(address, 4, status);
		  *u32 = HDL::Net::Device::get(address+4, 4, status);
		} else {
		  HDL::Net::Device::set(address, 4, *u32++, status);
		  HDL::Net::Device::set(address+4, 4, *u32, status);
		}
	      }
	      break;
	    case 4:
	      if (read)
		*(uint32_t*)data = HDL::Net::Device::get(address, 4, status);
	      else
		HDL::Net::Device::set(address, 4, *(uint32_t*)data, status);
	      break;
	    case 2:
	      if (read)
		*(uint16_t*)data =
		  OCPI_UTRUNCATE(uint16_t, HDL::Net::Device::get(address, 2, status));
	      else
		HDL::Net::Device::set(address, 2, *(uint16_t*)data, status);
	      break;
	    case 1:
	      if (read)
		*data = OCPI_UTRUNCATE(uint8_t, HDL::Net::Device::get(address, 1, status));
	      else
		HDL::Net::Device::set(address, 1, *data, status);
	      break;
	    }
	}
	uint32_t
	get(RegisterOffset offset, size_t bytes, uint32_t *status) {
	  ocpiDebug("SDP Accessor read for offset 0x%zx of %zu bytes", offset, bytes);
	  uint32_t data;
	  sdpRequest(true, offset, bytes, (uint8_t*)&data, status);
	  ocpiDebug("SDP Accessor read received 0x%x from offset %zx", data, offset);
	  return data;
	}
	void 
	set(RegisterOffset offset, size_t bytes, uint32_t data, uint32_t *status) {
	  ocpiDebug("SDP Accessor write %x for offset 0x%zx of %zu bytes", data, offset, bytes);
	  sdpRequest(false, offset, bytes, (uint8_t *)&data, status);
	  ocpiDebug("SDP Accessor write from offset %zx complete", offset);
	}
	uint64_t
	get64(RegisterOffset offset, uint32_t *status) {
	  ocpiDebug("SDP Accessor read64 for offset 0x%zx", offset);
	  uint64_t data;
	  sdpRequest(true, offset, sizeof(uint64_t), (uint8_t*)&data, status);
	  ocpiDebug("SDP Accessor read received 0x%" PRIx64 " from offset %zx", data, offset);
	  return data;
	}
	void
	getBytes(RegisterOffset offset, uint8_t *buf, size_t length, size_t elementBytes,
		 uint32_t *status, bool string) {
	  ocpiDebug("Accessor read %zu bytes for offset 0x%zx", length, offset);
	  for (size_t bytes; length; length -= bytes, buf += bytes, offset += bytes) {
	    bytes = offset & 7 || length < 8 ? (offset & 3 || length < 4 ?
						(offset & 1 || length < 2 ? 1 : 2) : 4) : 8;
	    if (bytes > elementBytes)
	      bytes = elementBytes;
	    if (offset & (elementBytes - 1))
	      bytes = elementBytes - (offset & (elementBytes - 1));
	    sdpRequest(true, offset, bytes, buf, status);
	    if (string && strnlen((char *)buf, bytes) < bytes)
	      break;
	  }
	  ocpiDebug("Accessor read bytes complete");
	}
	void
	set64(RegisterOffset offset, uint64_t val, uint32_t *status) {
	  ocpiDebug("SDP Accessor write64 for offset 0x%zx", offset);
	  assert(!(offset & 7));
	  sdpRequest(false, offset, sizeof(uint64_t), (uint8_t*)&val, status);
	  ocpiDebug("SDP Accessor write64 from offset %zx complete", offset);
	}
	void
	setBytes(RegisterOffset offset, const uint8_t *buf, size_t length,
		 size_t elementBytes, uint32_t *status) {
	  ocpiDebug("SDP Accessor write %zu bytes to offset 0x%zx", length, offset);
	  for (size_t bytes; length; length -= bytes, buf += bytes, offset += bytes) {
	    bytes = offset & 7 || length < 8 ? (offset & 3 || length < 4 ?
						(offset & 1 || length < 2 ? 1 : 2) : 4) : 8;
	    if (bytes > elementBytes)
	      bytes = elementBytes;
	    if (offset & (elementBytes - 1))
	      bytes = elementBytes - (offset & (elementBytes - 1));
	    sdpRequest(false, offset, bytes, (uint8_t *)buf, status);
	  }
	  ocpiDebug("SDP Accessor write to offset %zx complete", offset);
	}
	bool
	unload(std::string &error) {
	  return OU::eformat(error, "Can't unload bitstreams for simulated devices yet");
	}
      };

      Driver::
      ~Driver() {
      }
      Net::Device *Driver::
      createDevice(OS::Ether::Interface &ifc, OS::Ether::Address &addr, bool discovery,
		   const OU::PValue *params, std::string &error) {
	std::string name("sim:");
	name += addr.pretty();
	Device *d = new Device(*this, ifc, name, addr, discovery, params, error);
	if (error.empty())
	  return d;
	delete d;
	return NULL;
      }
    } // namespace Sim
  } // namespace HDL
} // namespace OCPI
