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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <string>
#include "zlib.h"
#ifdef OCPI_OS_macos
#define mmap64 mmap
#endif

#include "OcpiUtilMisc.h"
#include "BusDriver.h"
#include "HdlOCCP.h"
namespace OCPI {
  namespace HDL {
    namespace Bus {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;

      class Device
	: public OCPI::HDL::Device {
	uint32_t m_paddr;
	uint8_t  *m_vaddr;
	friend class Driver;
	Device(std::string &name, int fd, uint32_t paddr, bool forLoad, std::string &err)
	  : OCPI::HDL::Device(name, "ocpi-pci-pio"), m_paddr(paddr), m_vaddr(NULL) {
	  const char *cp = name.c_str();
	  if (!strncasecmp("BUS:", cp, 4))
	    cp += 4;
	  OU::formatString(m_endpointSpecific,
			   "ocpi-pci-pio:%u.0x%" PRIx32 ".0x%" PRIx32 ".0x%" PRIx32,
			   0, paddr, 0, 0);
	  if ((m_vaddr = (uint8_t*)mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
			      fd, OCPI_STRUNCATE(off_t, m_paddr))) == (uint8_t*)-1) {
	    m_vaddr = NULL;
	    err = "can't mmap /dev/mem for control space";
	    return;
	  }	  
	  cAccess().setAccess(m_vaddr, NULL, OCPI_UTRUNCATE(RegisterOffset, 0));
	  init(err);
	  if (forLoad)
	    err.clear();
	}
	~Device() {
	  if (m_vaddr)
	    munmap(m_vaddr, sizeof(OccpSpace));
	}
	// Scan the buffer and identify the start of the sync pattern
	static uint8_t *findsync(uint8_t *buf, size_t len) {
	  static uint8_t startup[] = {
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff, 
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff, 
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff, 
	    0xff, 0xff, 0xff, 0xff,
	    0xff, 0xff, 0xff, 0xff, 
	    0x00, 0x00, 0x00, 0xbb,
	    0x11, 0x22, 0x00, 0x44,
	    0xff, 0xff, 0xff, 0xff, 
	    0xff, 0xff, 0xff, 0xff, 
	    0xaa, 0x99, 0x55, 0x66};
	  uint8_t *p8 = startup;
	  for (uint8_t *u8 = buf; u8 < buf + len; u8++)
	    if (*u8 == *p8++) {
	      if (p8 >= startup+sizeof(startup))
		return u8 + 1 - sizeof(startup);
	    } else {
	      p8 = startup;
	      if (*u8 == *p8) p8++;
	    }
	  return 0;
	}

	// Load a bitstream
	void load(const char *fileName) {
	  struct Xld { // struct allocated on the stack for easy cleanup
	    int xfd, bfd;
	    gzFile gz;
	    uint8_t buf[8*1024];
	    int zerror;
	    size_t len;
	    int n;
	    void cleanup() { // used for constructor catch cleanup and in destructor
	      if (xfd >= 0) ::close(xfd);
	      if (bfd >= 0) ::close(bfd);
	      if (gz) gzclose(gz);
	    }
	    Xld(const char *fileName) : xfd(-1), bfd(-1), gz(NULL) {
	      try {
		// Open the device LAST since just opening it will do bad things
		if ((bfd = ::open(fileName, O_RDONLY)) < 0)
		  throw OU::Error("Can't open bitstream file '%s' for reading: %s(%d)",
				  fileName, strerror(errno), errno);
		if ((gz = ::gzdopen(bfd, "rb")) == NULL)
		  throw OU::Error("Can't open compressed bitstream file '%s' for : %s(%u)",
				  fileName, strerror(errno), errno);
		bfd = -1; // gzclose closes the fd
		// Read up to the sync pattern before byte swapping
		if ((n = ::gzread(gz, buf, sizeof(buf))) <= 0)
		  throw OU::Error("Error reading initial bitstream buffer: %s(%u/%d)",
				  gzerror(gz, &zerror), errno, n);
		uint8_t *p8 = findsync(buf, sizeof(buf));
		if (!p8)
		  throw OU::Error("Can't find sync pattern in compressed bit file");
		len = buf + sizeof(buf) - p8;
		if (p8 != buf)
		  memcpy(buf, p8, len);
		// We've done as much as we can before opening the device, which
		// does bad things...
		if ((xfd = ::open("/dev/xdevcfg", O_RDWR)) < 0)
		  throw OU::Error("Can't open /dev/xdevcfg for bitstream loading: %s(%d)",
				  strerror(errno), errno);
	      } catch (...) {
		cleanup();
		throw;
	      }
	    }
	    ~Xld() {
	      cleanup();
	    }
	    int gzread(uint8_t *&argBuf) {
	      if ((n = ::gzread(gz, buf + len, (unsigned)(sizeof(buf) - len))) < 0)
		throw OU::Error("Error reading compressed bitstream: %s(%u/%d)",
				gzerror(gz, &zerror), errno, n);
	      n += len;
	      len = 0;
	      argBuf = buf; 
	      return n;
	    }
	    uint32_t readConfigReg(unsigned /*reg*/) {
	      // write the "read config register" frame.
	      // read back the value
	      return 0;
	    }
	  };
	  Xld xld(fileName);
	  do {
	    uint8_t *buf;
	    int n = xld.gzread(buf);
	    if (n & 3)
	      throw OU::Error("Bitstream data in is '%s' not a multiple of 4 bytes",
			      fileName);
	    if (n == 0)
	      break;
	    uint32_t *p32 = (uint32_t*)buf;
	    for (unsigned nn = n; nn; nn -= 4, p32++)
	      *p32 = OU::swap32(*p32);
	    if (write(xld.xfd, buf, n) <= 0)
	      throw OU::Error("Error writing to /dev/xdevcfg for bitstream loading: %s(%u/%d)",
			      strerror(errno), errno, n);
	  } while(1);
	  ::close(xld.xfd);
	  xld.xfd = -1;
#if 0
	  // We have written all the data from the file to the device.
	  // Now we can retrieve status registers
	  const uint32_t
	    c_opencpi = 0xdd9fda7a,
	    c_statmask = 0x12345678,
	    c_statvalue = 0x12345678;
	  uint32_t
	    stat = xld.readConfigReg(7),
	    axss = xld.readConfigReg(9);
	  // Check stat for good value
	  if ((stat & c_statmask) != c_statvalue)
	    throw OU::Error("After loading, the configuration status register was 0x%x,"
			    " but should be 0x%x (masked with 0x%x)",
			    stat, c_statvalue, c_statmask);
	  // Check axss for our special value 0xdd9fda7a
	  if (axss != c_opencpi)
	    throw OU::Error("After loading, the USR_ACCESS code was not correct: 0x%x - should be 0x%x",
			    axss, c_opencpi);
#endif
	}
      };

      Driver::
      Driver()
	: m_memFd(-1) {
      }
      Driver::
      ~Driver() {
	if (m_memFd >= 0)
	  ::close(m_memFd);
      }

      unsigned Driver::
      search(const OU::PValue */*params*/, const char **exclude, bool /*discoveryOnly*/,
	     std::string &error) {
	// Opening implies canonicalizing the name, which is needed for excludes
	OCPI::HDL::Device *dev = open("0", false, error);
	if (dev) {
	  if (exclude)
	    for (const char **ap = exclude; *ap; ap++)
	      if (!strcmp(*ap, dev->name().c_str()))
		goto skipit;
	  if (!found(*dev, error))
	    return 1;
      skipit:
	  delete dev;
	}
	return 0;
      }
      
      OCPI::HDL::Device *Driver::
      open(const char *busName, bool forLoad, std::string &error) {
	std::string name("BUS:");
	name += busName;

#ifdef OCPI_PLATFORM_arm
	if (m_memFd < 0 && (m_memFd = ::open("/dev/mem", O_RDWR|O_SYNC)) < 0)
	  error = "Can't open /dev/mem, forgot to load the driver? sudo?";
	else {
	  Device *dev = new Device(name, m_memFd, 0x40000000, forLoad, error);
	  if (error.empty())
	    return dev;
	  delete dev;
	}
	ocpiBad("When searching for BUS device '%s': %s", busName, error.c_str());
#else
	(void)forLoad; (void)error;
#endif
	return NULL;
      }

      void *Driver::
      map(uint32_t size, uint64_t &phys, std::string &error) {
	// FIXME: mutex
	(void)size;(void)phys;
	if (m_memFd == -1) {
	  m_memFd = ::open("/dev/mem", O_RDWR | O_SYNC);
	  if (m_memFd < 0) {
	    OU::formatString(error, "Can't open memory /dev/mem for DMA memory.  Forgot sudo or missing driver");
	    return NULL;
	  }
	}
	return NULL;
      }
    } // namespace BUS
  } // namespace HDL
} // namespace OCPI
