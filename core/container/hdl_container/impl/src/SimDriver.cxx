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

#include <errno.h> 
#include <signal.h>
#include <dirent.h>
#include <arpa/inet.h>
#include "OcpiOsFileSystem.h"
#include "OcpiOsMisc.h"
#include "OcpiOsTimer.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "HdlAccess.h"
#include "HdlOCCP.h"
#include "EtherDefs.h"
#include "SimDriver.h"

namespace OCPI {
  namespace HDL {
    namespace Sim {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
      using namespace OCPI::HDL::Ether;
      
      static ssize_t myread(int fd, void *argBuf, size_t n) {
	uint8_t *buf = (uint8_t*)argBuf;
	ssize_t soFar = 0;
	do {
	  ssize_t r = read(fd, buf, n);
	  if (r <= 0)
	    return r;
	  soFar += r;
	  buf += r;
	  n -= r;
	} while (n);
	return soFar;
      }
      class Device
	: public OCPI::HDL::Device,
	  public OCPI::HDL::Accessor {
	friend class Driver;
	bool m_discovery;
	std::string m_directory;
	EtherControlPacket m_request;
	int m_toSim, m_fromSim;
      protected:
	Device(std::string &name, std::string &dir, bool discovery, std::string &error)
	  : OCPI::HDL::Device(name),
	    m_discovery(discovery), m_directory(dir), m_toSim(-1), m_fromSim(-1) {
	  m_request.header.tag = 0;
	  const char *cp = name.c_str();
	  if (!strncasecmp("Sim:", cp, 4))
	    cp += 4;
	  std::string
	    toSim(m_directory + "/external"),
	    fromSim(m_directory + "/response");
	  if ((m_toSim = open(toSim.c_str(), O_WRONLY | O_NONBLOCK)) < 0 ||
	      fcntl(m_toSim, F_SETFL, 0) != 0) {
	    OU::formatString(error, "Can't open or set up named pipe to simulator: %s", toSim.c_str());
	    return;
	  }
	  if ((m_fromSim = open(fromSim.c_str(), O_RDONLY | O_NONBLOCK)) < 0 ||
	      fcntl(m_fromSim, F_SETFL, 0) != 0) {
	    OU::formatString(error, "Can't open or set up named pipe from simulator: %s", fromSim.c_str());
	    return;
	  }
	  ocpiDebug("Sim container %s opened both named pipes", name.c_str());
	  //OU::formatString(m_endpointSpecific, "ocpi-ether-rdma:%s", cp);
	  m_endpointSize = sizeof(OccpSpace);
	  cAccess().setAccess(NULL, this, m_endpointSize - sizeof(OccpSpace));
	  // dAccess().setAccess(NULL, this, 0);
	}
      public:
	~Device() {
	}
	void request(EtherControlMessageType type, RegisterOffset offset,
		     unsigned bytes, EtherControlPacket &responsePacket, uint32_t *status) {
	  m_request.header.pad = 0;
	  m_request.header.tag++;
	  m_request.header.typeEtc =
	    OCCP_ETHER_TYPE_ETC(type,
				(~(-1 << bytes) << (offset & 3)) & 0xf,
				m_discovery ? 1 : 0);
	  EtherControlResponse response = OK;
	  if (status)
	    *status = 0;
	  if (write(m_toSim, (void*)&m_request.header.length, typeLength[type]-2) != typeLength[type]-2)
	    throw OU::Error("Can't write %u bytes of control request to named pipe", typeLength[type]-2);
	  ocpiDebug("Request sent type %u tag %u offset %u",
		    OCCP_ETHER_MESSAGE_TYPE(m_request.header.typeEtc), m_request.header.tag,
		    ntohl(((EtherControlRead *)&m_request)->address));
	  ssize_t r = myread(m_fromSim, (uint8_t*)&responsePacket.header+2,
			     sizeof(responsePacket.header)-2);
	  if (r != sizeof(responsePacket.header)-2)
	    throw OU::Error("Can't read header of control request to named pipe: %zd %d",
			    r, errno);
	  ocpiDebug("response received from sim %x %x %x",
		    responsePacket.header.length, responsePacket.header.typeEtc,
		    responsePacket.header.tag);
	  if (OCCP_ETHER_MESSAGE_TYPE(responsePacket.header.typeEtc) != OCCP_RESPONSE)
	    throw OU::Error("Control packet from sim not a response, ignored: typeEtc 0x%x",
			    responsePacket.header.typeEtc);
	  if (responsePacket.header.tag != m_request.header.tag)
	    throw OU::Error("Control packet from sim has extraneous tag %u, expecting %u, ignored",
			    responsePacket.header.tag, m_request.header.tag);
	  if (type != OCCP_WRITE &&
	      (r = myread(m_fromSim, (void*)(&responsePacket.header + 1), 4)) != 4)
	    throw OU::Error("Can't read response data (4 bytes) of control request to named pipe %zd %d",
			    r, errno);
	  if ((response = OCCP_ETHER_RESPONSE(responsePacket.header.typeEtc)) == OK)
	    return;
	  ocpiInfo("Control packet from sim got non-OK response: %u", response);
	  if (status)
	    *status =
	      response == WORKER_TIMEOUT ? OCCP_STATUS_READ_TIMEOUT :
	      response == ERROR ? OCCP_STATUS_READ_ERROR :
	      OCCP_STATUS_ACCESS_ERROR;
	  else
	    throw OU::Error("HDL Sim Control %s error: %s",
			    type == OCCP_READ ? "read" : "write",
			    response == WORKER_TIMEOUT ? "worker timeout" :
			    response == ERROR ? "worker error" :
			    "ethernet timeout - no valid response");
	}

	// Shared "get" that returns value, and *status if status != NULL
	uint32_t get(RegisterOffset offset, unsigned bytes, uint32_t *status) {
	  EtherControlRead &ecr =  m_request.read;
	  ecr.address = htonl((offset & 0xffffff) & ~3);
	  ecr.header.length = htons(sizeof(ecr)-2);
	  EtherControlPacket responsePacket;
	  request(OCCP_READ, offset, bytes, responsePacket, status);
	  uint32_t data = ntohl(responsePacket.readResponse.data);
	  ocpiDebug("Accessor read received 0x%x from offset %x tag %u", data, offset, ecr.header.tag);
	  return data;
	}
	void
	set(RegisterOffset offset, unsigned bytes, uint32_t data, uint32_t *status) {
	  EtherControlWrite &ecw =  m_request.write;
	  ecw.address = htonl((offset & 0xffffff) & ~3);
	  ecw.data = htonl(data);
	  ecw.header.length = htons(sizeof(ecw)-2);
	  EtherControlPacket responsePacket;
	  request(OCCP_WRITE, offset, bytes, responsePacket, status);
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
      Driver::
      ~Driver() {
      }

      OCPI::HDL::Device *Driver::
      open(const char *name, bool discovery, std::string &error) {
	std::string myName;
	if (!strncasecmp("sim:", name, 4)) {
	  myName = name;
	  name += 4;
	} else {
	  myName = "sim:";
	  myName += name;
	}
	ocpiDebug("Attemping to open sim device \"%s\"", myName.c_str());
	pid_t pid = atoi(name);
	if (kill(pid, 0)) {
	  OU::formatString(error, "Found simulator sim:%s, but its process is gone or privileged",
		    name);
	  return NULL;
	}
	std::string dir;
	OU::formatString(dir, "%s/%s/%s.%s", TMPDIR, SIMDIR, SIMPREF, name);
	bool isDir;
	if (!OS::FileSystem::exists(dir, &isDir) || !isDir) {
	  OU::formatString(error, "Directory for sim:%s, which is \"%s\", doesn't exist or isn't a directory",
			   name, dir.c_str());
	  return NULL;
	}
	return new Device(myName, dir, discovery, error);
      }
      unsigned Driver::
      search(const OU::PValue */*params*/, const char **exclude, std::string &error) {
	unsigned count = 0;
	std::string simDir;
	OU::formatString(simDir, "%s/%s", TMPDIR, SIMDIR);
	DIR *d = opendir(simDir.c_str());
	if (!d) {
	  ocpiDebug("Couldn't open simulator directory \"%s\" so search finds nothing",
		    simDir.c_str());
	  return 0;
	} else {
	  for (struct dirent *ent; error.empty() && (ent = readdir(d)) != NULL;)
	    if (ent->d_name[0] != '.') {
	      unsigned len = strlen(SIMPREF);
	      if (strncmp(ent->d_name, SIMPREF, len) ||
		  ent->d_name[len] != '.') {
		ocpiDebug("Found unexpected file \"%s\" in simulator directory \"%s\"",
			  ent->d_name, simDir.c_str());
		continue;
	      }
	      const char *pidStr = ent->d_name + len + 1;
	      // Opening implies canonicalizing the name, which is needed for excludes
	      OCPI::HDL::Device *dev = open(pidStr, true, error);
	      if (error.empty()) {
		if (exclude)
		  for (const char **ap = exclude; *ap; ap++)
		    if (!strcmp(*ap, dev->name().c_str()))
		      goto skipit; // continue(2);
		if (found(*dev, error)) {
		  count++;
		  continue;
		}
	      }
	    skipit:
	      if (error.size())
		ocpiInfo("Error opening sim device: %s", error.c_str());
	      if (dev)
		delete dev;
	    }
	  closedir(d); // FIXME: try/catch?
	}
	return count;
      }
    } // namespace Sim
  } // namespace HDL
} // namespace OCPI
