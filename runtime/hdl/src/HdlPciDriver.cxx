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
#include <dirent.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include "ocpi-config.h"
#ifdef OCPI_OS_macos
#define mmap64 mmap
#endif
#include "OcpiUtilPci.h"

#include "fasttime.h"

#include "KernelDriver.h"
#include "OcpiOsFileSystem.h"
#include "HdlPciDriver.h"

// This should be in OCPIOS.
// This is the linux version (kernel 2.6+).
namespace OS = OCPI::OS;
namespace OU = OCPI::Util;
namespace OCPI {
  namespace HDL {
    namespace PCI {

      class Device
	: public OCPI::HDL::Device {
	void *m_bar0, *m_bar1;
	uint32_t m_bar0size, m_bar1size;
	int m_fd;
	friend class Driver;
	Device(std::string &a_name, int fd, ocpi_pci_t &pci, void *bar0, void *bar1,
	       const OU::PValue *params, std::string &err)
	  : OCPI::HDL::Device(a_name, "ocpi-dma-pio", params), m_bar0(bar0), m_bar1(bar1),
	    m_bar0size(pci.size0), m_bar1size(pci.size1), m_fd(fd) {
	  uint64_t endpointPaddr, controlOffset, bufferOffset, holeStartOffset, holeEndOffset;
	  if (pci.bar0 < pci.bar1) {
	    endpointPaddr = pci.bar0;
	    m_endpointSize = pci.bar1 + pci.size1 - endpointPaddr;
	    controlOffset = 0;
	    bufferOffset = pci.bar1 - pci.bar0;
	    holeStartOffset = pci.size0;
	    holeEndOffset = pci.bar1 - pci.bar0;
	  } else {
	    endpointPaddr = pci.bar1;
	    m_endpointSize = pci.bar0 + pci.size0 - endpointPaddr;
	    controlOffset = pci.bar0 - pci.bar1;
	    bufferOffset = 0;
	    holeStartOffset = pci.size1;
	    holeEndOffset = pci.bar0 - pci.bar1;
	  }
	  OU::formatString(m_endpointSpecific,
			   "ocpi-dma-pio:0x%" PRIx64 ".0x%" PRIx64 ".0x%" PRIx64,
			   endpointPaddr, holeStartOffset, holeEndOffset);
	  cAccess().setAccess((uint8_t*)bar0, NULL, OCPI_UTRUNCATE(RegisterOffset, controlOffset));
	  dAccess().setAccess((uint8_t*)bar1, NULL, OCPI_UTRUNCATE(RegisterOffset, bufferOffset));
	  init(err);
	}
	~Device() {
	  if (m_bar0)
	    munmap(m_bar0, m_bar0size);
	  if (m_bar1)
	    munmap(m_bar1, m_bar1size);
	  if (m_fd >= 0)
	    ::close(m_fd);
	}
	uint32_t
	dmaOptions(ezxml_t icImplXml, ezxml_t /*icInstXml*/, bool isProvider) {
	  const char *icname = ezxml_cattr(icImplXml, "name");
	  if (icname && !strncasecmp(icname, "sdp", 3))
	    return isProvider ?
	      (1 << OCPI::RDT::ActiveFlowControl) | (1 << OCPI::RDT::ActiveMessage) |
	      (1 << OCPI::RDT::FlagIsMeta) :
	      1 << OCPI::RDT::ActiveMessage; // fix this for peer-to-peer
	  return 1 << OCPI::RDT::ActiveMessage;
	  // return (1 << OCPI::RDT::ActiveFlowControl) | (1 << OCPI::RDT::ActiveMessage);
	}
	static int compu32(const void *a, const void *b) { return *(int32_t*)a - *(int32_t*)b; }
    // Set up the FPGAs clock, assuming it has no GPS.
    // This does two things:
    // 1. calculate the delay when the CPU reads the 64 bit time register.
    //   Ie. the time interval between the actual sampling of the 64 bit clock ON the FPGA and when
    //   the "load" instruction actually returns that value.
    // 2. set the FPGA time value to something close to our system time.
    static inline uint64_t ticks2ns(uint64_t ticks) {
      return (ticks * 1000000000ull + (1ull << 31)) / (1ull << 32);
    }
    static inline int64_t dticks2ns(int64_t ticks) {
      return (ticks * 1000000000ll + (1ll << 31))/ (1ll << 32);
    }
    static inline uint64_t ns2ticks(unsigned long sec, unsigned long nsec) {
      return ((uint64_t)sec << 32ull) + ((nsec + 500000000ull) * (1ull<<32)) /1000000000ull;
    }
    static inline uint64_t now() {
#if 1
      static int x;
      if (!x) {
	fasttime_init();
	x = 1;
      }
      struct timespec tv;
      fasttime_gettime(&tv);
      return ns2ticks(tv.tv_sec, tv.tv_nsec);
#else
#ifdef __APPLE__
      struct timeval tv;
      gettimeofday(&tv, NULL);
      return ns2ticks(tv.tv_sec, tv.tv_usec * 1000);
#else
      struct timespec ts;
      clock_gettime (CLOCK_REALTIME, &ts);
      return ns2ticks(ts.tv_sec, ts.tv_nsec);
#endif
#endif
    }
	void initTime() {
	  unsigned n;
	  uint32_t delta[100];
	  uint32_t sum = 0;
	  Access *ts = timeServer();
  
	  if (!ts)
	    return;
	  // Take a hundred samples of round trip time, and sort
	  for (n = 0; n < 100; n++) {
	    // Read the FPGA's time, and set its delta register
	    ts->set64RegisterOffset(sizeof(uint64_t), ts->get64RegisterOffset(0));
	    // occp->admin.timeDelta = occp->admin.time;
	    // Read the (incorrect endian) delta register
	    delta[n] = (int32_t)swap32(ts->get64RegisterOffset(sizeof(uint64_t)));
	  }
	  qsort(delta, 100, sizeof(uint32_t), compu32);
  
	  // Ignore the slowest 10%
	  for (n = 0; n < 90; n++)
	    sum += delta[n];
	  sum = (sum + 45) / 90;
	  m_timeCorrection = sum;
	  // we have average delay
	  ocpiInfo("For %s: round trip times (ns) deltas: min %"
		   PRIu64 " max %" PRIu64 " avg of best 90: %" PRIu64 "\n",
		   name().c_str(), ticks2ns(delta[0]), ticks2ns(delta[99]), ticks2ns(sum));
	  sum /= 2; // assume half a round trip for writes
	  uint64_t nw1 = now();
	  uint64_t nw1a = nw1 + sum;
	  uint64_t nw1as = swap32(nw1a);
	  ts->set64RegisterOffset(0, nw1as);
	  // set32Register(admin.scratch20, OccpSpace, nw1as>>32);
	  // set32Register(admin.scratch24, OccpSpace, (nw1as&0xffffffff));
	  //uint64_t nw1b = 0; // get64Register(admin.time, OccpSpace);
	  //      uint64_t nw1bs = swap32(nw1b);
	  uint64_t nw2 = 0; // now();
	  uint64_t nw2s = swap32(nw2);
	  ts->set64RegisterOffset(sizeof(uint64_t), nw1as);
	  uint64_t nw1b = ts->get64RegisterOffset(0);
	  uint64_t nw1bs = swap32(nw1b);
	  int64_t dt = ts->get64RegisterOffset(sizeof(uint64_t));
	  ocpiDebug("Now delta is: %" PRIi64 "ns "
		    "(dt 0x%" PRIx64 " dtsw 0x%" PRIx64 " nw1 0x%" PRIx64 " nw1a 0x%" PRIx64 " nw1as 0x%" PRIx64
		    " nw1b 0x%" PRIx64 " nw1bs 0x%" PRIx64 " nw2 0x%" PRIx64 " nw2s 0x%" PRIx64 " t 0x%lx)",
		    dticks2ns(swap32(dt)), dt, swap32(dt), nw1, nw1a, nw1as, 
		    nw1b, nw1bs, nw2, nw2s, time(0));
#ifndef __APPLE__
	  {
	    struct timespec tspec;
	    clock_getres(CLOCK_REALTIME, &tspec);
	    ocpiInfo("res: %ld", tspec.tv_nsec);
	  }
#endif
	}
	// Load a bitstream via jtag
	bool load(const char *fileName, std::string &error) {
	  error.clear();
	  // FIXME: there should be a utility to run a script in this way
	  char *command;
	  int aslen =
	    asprintf(&command,
		     "%s/scripts/loadBitStream \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
		     OU::getCdk().c_str(), fileName, name().c_str(), m_platform.c_str(),
		     m_part.c_str(), m_esn.c_str(), m_position.c_str());
          ocpiAssert(aslen > 0);
	  ocpiInfo("Executing command to load bit stream for device %s: \"%s\"\n",
		   fileName, command);
	  int rc = system(command);
	  switch (rc) {
	  case 127:
	    error = "Couldn't execute bitstream loading command.  Bad OCPI_CDK_DIR environment variable?";
	    break;
	  case -1:
	    OU::format(error,
		       "Unknown system error (errno %d) while executing bitstream loading command: %s",
		       errno, command);
	    break;
	  case 0:
	    ocpiInfo("Successfully loaded bitstream file: \"%s\" on HDL device \"%s\"\n",
		     fileName, name().c_str());
	    break;
	  default:
	    OU::format(error, "Bitstream loading error (%s%s: %d) loading \"%s\" on HDL device"
		       " \"%s\" with command: %s",
		       WIFEXITED(rc) ? "exit code" : "signal ",
		       WIFEXITED(rc) ? "" : strsignal(WTERMSIG(rc)),
		       WIFEXITED(rc) ? WEXITSTATUS(rc) : WTERMSIG(rc),
		       fileName, name().c_str(), command);
	  }
	  return error.empty() ? init(error) : true;
	}
	bool
	unload(std::string &error) {
	  return OU::eformat(error, "Can't unload bitstreams for PCI/JTAG devices yet");
	}
      };

      Driver::
      Driver()
	: m_pciMemFd(-1), m_useDriver(OS::FileSystem::exists(OCPI_DRIVER_PCI)) {
      }
      Driver::
      ~Driver() {
	if (m_pciMemFd >= 0) {
	  ::close(m_pciMemFd);
	  m_pciMemFd = -1;
	}
      }
      unsigned Driver::
      search(const OU::PValue *params, const char **excludes, bool discoveryOnly,
	     std::string &error) {
	ocpiInfo("Searching for local PCI-based HDL devices.");
	error.clear();
	unsigned count = 0;
	const char *dir = m_useDriver ? OCPI_DRIVER_PCI : OCPI_HDL_SYS_PCI_DIR;
	DIR *pcid = opendir(dir);
	if (pcid) {
	  for (struct dirent *ent; (ent = readdir(pcid)) != NULL; )
	    if (ent->d_name[0] != '.') {
	      std::string err;
	      OCPI::HDL::Device *dev = open(ent->d_name, params, err);
	      if (dev && !found(*dev, excludes, discoveryOnly, err))
		count++;
	      if (error.empty())
		error = err;
	    }
	  closedir(pcid);
	} else if (!m_useDriver)
#ifndef OCPI_OS_macos
	  OU::format(error, "can't open the %s directory for PCI search", dir);
#else
	;
#endif
	return count;
      }
      
      OCPI::HDL::Device *Driver::
      open(const char *pciName, const OU::PValue *params, std::string &error) {
	const char *cp;
	for (cp = pciName; *cp && isdigit(*cp); cp++)
	  ;
	std::string name("PCI:");
	if (*cp)
	  name += pciName;
	else
	  OU::formatAdd(name, "0000:%02d:00.0", atoi(pciName));

	void *bar0 = 0, *bar1 = 0;
	ocpi_pci_t pci;
	int fd = -1;
	if (m_useDriver) {
	  std::string devName(OCPI_DRIVER_PCI);
	  devName += '/';
	  devName += name.c_str() + 4;
	  fd = ::open(devName.c_str(), O_RDWR | O_SYNC);
	  if (fd < 0)
	    OU::formatString(error, "Can't open device %s", devName.c_str());
	  else if (ioctl(fd, OCPI_CMD_PCI, &pci))
	    OU::formatString(error, "Can't get pci info for device %s", devName.c_str());
	  else if ((bar0 = mmap64(NULL, pci.size0, PROT_READ|PROT_WRITE, MAP_SHARED,
				fd, 0)) == (void*)-1)
	    OU::formatString(error, "Can't mmap %s for bar0", devName.c_str());
	  else if ((bar1 = mmap64(NULL, pci.size1, PROT_READ|PROT_WRITE, MAP_SHARED,
				fd, pci.size0)) == (void*)-1)
	    OU::formatString(error, "can't mmap %s for bar1", devName.c_str());
	  // So fd, bar0, bar1, and pci are good here if error.empty()
	} else {
	  OU::Bar bars[OU::MAXBARS];
	  unsigned nbars = OU::MAXBARS;
	  const char *cname = name.c_str()+4;
	  std::string dummy;
	  
	  // First look for our registered ID, then fall back to legacy
	  // If it doesn't match, look for the old values
	  if (OU::probePci(cname, OCPI_HDL_PCI_VENDOR_ID, UINT_MAX, OCPI_HDL_PCI_CLASS,
			   OCPI_HDL_PCI_SUBCLASS, false, bars, nbars, error) ||
	      OU::probePci(cname, OCPI_HDL_PCI_OLD_VENDOR_ID, OCPI_HDL_PCI_OLD_DEVICE_ID,
			   OCPI_HDL_PCI_OLD_CLASS, OCPI_HDL_PCI_OLD_SUBCLASS, false, bars, nbars,
			   dummy))
	    if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
		bars[0].addressSize != 32 || bars[0].size != sizeof(OccpSpace))
	      error =
		"Found PCI device w/ good vendor/device/class, but bars are wrong; "
		"skipping it; use lspci";
	    else {
	      pci.bar0 = bars[0].address;
	      pci.bar1 = bars[1].address;
	      pci.size0 = OCPI_UTRUNCATE(ocpi_size_t,bars[0].size);
	      pci.size1 = OCPI_UTRUNCATE(ocpi_size_t,bars[1].size);
	      if (m_pciMemFd < 0 && (m_pciMemFd = ::open("/dev/mem", O_RDWR|O_SYNC)) < 0)
		error = "Can't open /dev/mem, forgot to load the driver? sudo?";
	      else if ((bar0 = mmap64(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
				    m_pciMemFd, OCPI_STRUNCATE(off_t, bars[0].address))) == (void*)-1)
		error = "can't mmap /dev/mem for bar0";
	      else if ((bar1 = mmap64(NULL, OCPI_UTRUNCATE(size_t, bars[1].size), PROT_READ|PROT_WRITE, MAP_SHARED,
				    m_pciMemFd, OCPI_STRUNCATE(off_t, bars[1].address))) == (void*)-1)
		error = "can't mmap /dev/mem for bar1";
	    }
	  else
	    return NULL; // not really an error
	}
	if (error.empty()) {
	  Device *dev = new Device(name, fd, pci, bar0, bar1, params, error);
	  if (error.empty())
	    return dev; // we have passed the fd into the device.
	  delete dev;
	}
	if (bar0)
	  munmap(bar0, pci.size0);
	if (bar1)
	  munmap(bar1, pci.size1);
	if (fd >= 0)
	  ::close(fd);
	ocpiBad("When searching for PCI device '%s': %s", pciName, error.c_str());
	return NULL;
      }
#if 0
      void *Driver::
      map(uint32_t size, uint64_t &phys, std::string &error) {
	// FIXME: mutex
	if (m_pciMemFd == -1) { // This can only happen if there is no driver to open
	  m_pciMemFd = ::open("/dev/mem", O_RDWR | O_SYNC);
	  if (m_pciMemFd < 0) {
	    OU::formatString(error, "Can't open memory /dev/mem for DMA memory.  Forgot sudo or missing driver");
	    return NULL;
	  }
	}
	ocpi_request_t request;
	request.how_cached = ocpi_uncached;
	request.needed = size;
	if (m_useDriver) {
	  if (ioctl(m_pciMemFd, OCPI_CMD_REQUEST, &request)) {
	    OU::formatString(error, "Can't allocate memory size %u for DMA memory", size);
	    return NULL;
	  }
	} else {
	  static uint64_t base = 0, top;
	  static unsigned pagesize = getpagesize();
	  
	  if (!base) {
	    const char *dma = getenv("OCPI_DMA_MEMORY");
	    if (!dma) {
	      error = "No OCPI_DMA_MEMORY environment setup";
	      return NULL;
	    }
	    unsigned sizeM;
	    if (sscanf(dma, "%uM$0x%" SCNx64, &sizeM, &base) != 2) {
	      error = "Bad format for OCPI_DMA_MEMORY environment setup";
	      return NULL;
	    }
	    ocpiDebug("DMA Memory:  %uM at 0x%" PRIx64 "\n", sizeM, base);
	    top = base + sizeM * 1024llu * 1024llu;
	    if (base & (pagesize-1)) {
	      base += pagesize - 1;
	      base &= ~(pagesize - 1);
	      top &= ~(pagesize - 1);
	      ocpiDebug("DMA Memory is NOT page aligned.  Now %" PRIu64 "u at 0x%" PRIx64 "\n",
			top - base, base);
	    }
	  }
	  request.actual = (size + pagesize - 1) & ~(pagesize - 1);
	  if (top - base < request.actual) {
	    OU::formatString(error, "Insufficient memory for request of %u", size);
	    return NULL;
	  }
	  request.address = base;
	  base += request.actual;
	}
	void *vaddr =  mmap64(NULL, request.actual, PROT_READ|PROT_WRITE, MAP_SHARED,
			    m_pciMemFd, OCPI_STRUNCATE(off_t, request.address));
	if (vaddr == (void *)-1) {
	  error = "DMA mmap failure"; // FIXME: a leak of physical memory in this case
	  return NULL;
	}
	phys = request.address;
	return vaddr;
      }
#endif
    } // namespace PCI
  } // namespace HDL
} // namespace OCPI
