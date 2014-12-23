
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


#define _POSIX_SOURCE // to force ctime_r to be defined on linux FIXME
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/mman.h>
#include "OcpiUuid.h"
#include "HdlOCCP.h"
#include "HdlPciDriver.h"

namespace OP = OCPI::HDL::PCI;
namespace OH = OCPI::HDL;

static const char *found(const char *name, OP::Bar *bars, unsigned nBars, bool verbose);

// consider a specific PCI device return error if something
// bad happened.
const char *
doDevice(const char *name,
	 unsigned theVendor, unsigned theDevice, unsigned theClass, unsigned theSubClass,
	 bool verbose) {
  OP::Bar bars[OP::MAXBARS];
  unsigned nbars = OP::MAXBARS;
  static std::string error;
  if (!OP::probePci(name, theVendor, theDevice, theClass, theSubClass, verbose, bars, nbars, error))
    return error.empty() ? NULL : error.c_str();
  return nbars ? found(name, bars, nbars, verbose) : NULL;
}

const char *
search(const char **/*exclude*/,
       unsigned theVendor, unsigned theDevice, unsigned theClass, unsigned theSubClass,
       bool verbose) {
  const char *err = NULL;
  struct dirent *ent;
  DIR *pcid = opendir(OCPI_HDL_SYS_PCI_DIR);
  if (!pcid) 
    return "Can't open the " OCPI_HDL_SYS_PCI_DIR " directory";
  while (!err && (ent = readdir(pcid)) != NULL)
    if (ent->d_name[0] != '.')
      err = doDevice(ent->d_name, theVendor, theDevice, theClass, theSubClass, verbose);
  closedir(pcid);
  return err;
}

int pciMemFd = -1;
unsigned nFound = 0;

// found a correctly looking PCI device
static const char*
found(const char *name, OP::Bar *bars, unsigned nbars, bool verbose) {
  const char *err = 0;
  void *bar0 = 0, *bar1 = 0;
  volatile OH::OccpSpace *occp;

  if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
      bars[0].addressSize != 32 || bars[0].size < sizeof(OH::OccpSpace))
    return "OCFRP found but bars are misconfigured\n";
  // PCI config info looks good.  Now check the OCCP signature.
  if (pciMemFd < 0 && (pciMemFd = open("/dev/mem", O_RDWR)) < 0)
    return "Can't open /dev/mem, probably need to run as root/sudo";
  do { // break on error to recover mappings
    if ((bar0 = mmap(NULL, sizeof(OH::OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
		     pciMemFd, OCPI_UTRUNCATE(off_t, bars[0].address))) == (void*)-1) {
      err = "can't mmap /dev/mem for bar0";
      break;
    }
    if ((bar1 = mmap(NULL,OCPI_UTRUNCATE(size_t, bars[1].size), PROT_READ|PROT_WRITE, MAP_SHARED,
		     pciMemFd, OCPI_UTRUNCATE(off_t, bars[1].address))) == (void*)-1) {
      err = "can't mmap /dev/mem for bar1";
      break;
    }
    occp = (OH::OccpSpace *)bar0;
    if (occp->admin.magic != OCCP_MAGIC) {
      if (verbose)
	printf("PCI Device matches OCFRP vendor/device, but not OCCP signature: "
	       "magic: 0x%" PRIx64 " (sb 0x%" PRIx64 ")\n",
	       occp->admin.magic, OCCP_MAGIC);
      err = "Magic numbers do not match in region/bar 0";
      break;
    }
    {
      volatile OH::OccpWorkerRegisters *w = &occp->worker[0].control;
      uint32_t bits = occp->admin.config;
      volatile uint8_t *uuid;
      // new platform worker: worker zero connected
      if (bits != (uint32_t)-1 && (bits & 1)) {
	uint32_t control = w->control;
	if (!(control & OCCP_WORKER_CONTROL_ENABLE))
	  w->control = control | OCCP_WORKER_CONTROL_ENABLE;
	uuid = occp->config[0];
      } else
	uuid = (uint8_t *)&occp->admin.uuid;
      char tbuf[30], tbuf1[30];
      OH::HdlUUID myUUID;
      char platform[sizeof(myUUID.platform)+1];
      char device[sizeof(myUUID.device)+1];
      char load[sizeof(myUUID.load)+1];
      OCPI::Util::UuidString textUUID;
      time_t bsvbd, bsbd;
      // Capture the UUID info that tells us about the platform
      for (unsigned n = 0; n < sizeof(OH::HdlUUID); n++)
	((uint8_t*)&myUUID)[n] = uuid[(n & ~3) + (3 - (n&3))];
      strncpy(platform, myUUID.platform, sizeof(myUUID.platform));
      platform[sizeof(myUUID.platform)] = '\0';
      strncpy(device, myUUID.device, sizeof(myUUID.device));
      device[sizeof(myUUID.device)] = '\0';
      strncpy(load, myUUID.load, sizeof(myUUID.load));
      load[sizeof(myUUID.load)] = '\0';
      OCPI::Util::uuid2string(myUUID.uuid, textUUID);
      bsvbd = occp->admin.birthday;
      bsbd = myUUID.birthday;
      ctime_r(&bsvbd, tbuf);
      tbuf[strlen(tbuf)-1] = 0;
      ctime_r(&bsbd, tbuf1);
      tbuf1[strlen(tbuf1)-1] = 0;
#if 1
      printf("OpenCPI FPGA at PCI %s: bitstream date %s, "
	     //	     "platform \"%s\", device \"%s\", UUID %s, loadParam \"%s\"\n",
	     // name, tbuf, tbuf1, platform, device, textUUID, load);
	     "platform \"%s\", part \"%s\", UUID %s\n",
	     name, tbuf1, platform, device, textUUID);
#else
      printf("OpenCPI FPGA at PCI %s\n", name);
      for (unsigned n = 0; n < sizeof(myUUID); n++) {
	int c = ((char *)&myUUID)[n] & 0xff;
	printf("%2u: %2x (%c)\n", n, c, isprint(c) ? c : '?');
      }
#endif
      
    }
    nFound++;
  } while (0);
  if (bar0)
    munmap(bar0, sizeof(OH::OccpSpace));
  if (bar1)
    munmap(bar1, OCPI_UTRUNCATE(size_t, bars[1].size));
  return err;
}

int
main(int /*argc*/, char **argv)
{
  bool verbose = false;
  if (argv[1] && !strcmp(argv[1], "-v")) {
    verbose = true;
    argv++;
  }
  const char *err;
  int rv = 0;
  if (argv[1]) {
    if ((err = doDevice(argv[1], OCPI_HDL_PCI_VENDOR_ID, OCPI_HDL_PCI_DEVICE_ID, OCPI_HDL_PCI_CLASS, OCPI_HDL_PCI_SUBCLASS,
			verbose))) {
      fprintf(stderr, "Error: Couldn't find \"%s\": %s\n", argv[1], err);
      rv = 1;
    } else if (!nFound) {
      fprintf(stderr, "Error: Device specified is not recognized as an OpenCPI FPGA platform\n");
      rv = 1;
    }
  } else {
    if ((err = search(0, OCPI_HDL_PCI_VENDOR_ID, OCPI_HDL_PCI_DEVICE_ID, OCPI_HDL_PCI_CLASS, OCPI_HDL_PCI_SUBCLASS,
		      verbose))) {
      fprintf(stderr, "Error: PCI Scanner Error: %s\n", err);
      rv = 1;
    } else if (!nFound) {
      fprintf(stderr, "Did not find any OpenOCPI FPGA platforms.\n");
      rv = 1;
    }
  }
  {
    const char *dmaEnv;
    unsigned dmaMeg;
    unsigned long long dmaBase;
    int fd;
    volatile uint8_t *cpuBase;

    if (!(dmaEnv = getenv("OCPI_DMA_MEMORY"))) {
      if (verbose)
	fprintf(stderr,
		"Warning: You must set the OCPI_DMA_MEMORY environment variable or load the driver before using any OpenCPI FPGA platform\n"
		"         Use \"sudo -E\" to allow this program to have access to environmant variables\n");
    } else if (sscanf(dmaEnv, "%uM$0x%llx", &dmaMeg, (unsigned long long *) &dmaBase) != 2) {
      fprintf(stderr, "Error: The OCPI_DMA_MEMORY environment variable is not formatted correctly\n");
      rv = 1;
    } else if ((fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0 ||
	       (cpuBase = (uint8_t*)mmap(NULL, OCPI_UTRUNCATE(size_t, (uint64_t)dmaMeg * 1024 * 1024),
					 PROT_READ|PROT_WRITE, MAP_SHARED, fd,
					 OCPI_UTRUNCATE(off_t, dmaBase))) == MAP_FAILED) {
      fprintf(stderr, "Error: Can't map to local DMA memory defined in OCPI_DMA_MEMORY using /dev/mem (%s/%d)\n",
	      strerror(errno), errno);
      rv = 1;
    } else {
      uint8_t save = *cpuBase;

      if ((*cpuBase = 1, *cpuBase != 1) || (*cpuBase = 2, *cpuBase != 2)) {
	*cpuBase = save; // do this before any sys calls etc.
	fprintf(stderr, "Error: Can't write to start of local DMA memory defined in OCPI_DMA_MEMORY using /dev/mem\n");
	rv = 1;
      } else {
	*cpuBase = save; // on the wild chance that we shouldn't actually be touching this?
	cpuBase += (unsigned long long)dmaMeg * 1024ull * 1024ull - 1ull;
	save = *cpuBase;
	if ((*cpuBase = 1, *cpuBase != 1) || (*cpuBase = 2, *cpuBase != 2)) {
	  *cpuBase = save; // do this before any sys calls etc.
	  fprintf(stderr, "Error: Can't write to end of local DMA memory defined in OCPI_DMA_MEMORY using /dev/mem\n");
	  rv = 1;
	} else
	  *cpuBase = save; // on the wild chance that we shouldn't actually be touching this?
      }
    }
  }
  return rv;
}

