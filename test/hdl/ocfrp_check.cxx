
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <time.h>
#include "OCCP.h"

namespace OCPI {
  namespace RPL {

    struct Bar {
      uint64_t address;
      uint64_t size;
      bool io, prefetch;
      unsigned addressSize;
    };
    typedef void Found(Bar *bars, unsigned nBars, bool verbose);
#define PCIDIR "/sys/bus/pci/devices"
    static const char *
    getPciValue(char *dev, char *value, char *buf, unsigned len)
    {
      int n, fd;
      // get bars
      n = snprintf(buf, len, "%s/%s/%s", PCIDIR, dev, value);
      if (n <= 0 || (unsigned)n >= len)
	return "buffer violation";
      fd = open(buf, O_RDONLY);
      if (fd < 0)
	return "can't open descriptor file for PCI device";
      n = read(fd, buf, len);
      close(fd);
      if (n <= 0 && (unsigned)n >= len)
	return "can't read descriptor file for PCI device";
      buf[n] = 0;
      return 0;
    }
    static const char*
    getPciNumber(char *dev, char *value, char *buf, unsigned len, unsigned long long *np)
    {
      const char *err = getPciValue(dev, value, buf, len);
      if (err)
	return err;
      errno = 0;
      unsigned long long ull = strtoull(buf, NULL, 0);
      if (ull == ULLONG_MAX && errno == ERANGE)
	return "unexpected attribute value for PCI Device";
      *np = ull;
      return 0;
    }
    const char *
    search(const char **exclude,
	   unsigned theVendor, unsigned theDevice, unsigned theClass, unsigned theSubClass,
	   Found &found, bool verbose) {
      DIR *pcid = opendir(PCIDIR);
      if (!pcid) 
	return "can't open the " PCIDIR " directory";
      struct dirent *ent;
      const char *err = 0;
      while (!err && (ent = readdir(pcid)) != NULL) {
	char buf[512], rbuf[512];
	unsigned long long domain, bus, deviceN, function, vendor, device, classword;
	if (ent->d_name[0] == '.')
	  continue;
	if (sscanf(ent->d_name, "%llx:%llx:%llx.%lld", &domain, &bus, &deviceN, &function) != 4)
	  err = "bad PCI /sys device name";
	else if ((err = getPciValue(ent->d_name, "resource", rbuf, sizeof(rbuf))) ||
		 (err = getPciNumber(ent->d_name, "vendor", buf, sizeof(buf), &vendor)) ||
		 (err = getPciNumber(ent->d_name, "class", buf, sizeof(buf), &classword)) ||
		 (err = getPciNumber(ent->d_name, "device", buf, sizeof(buf), &device)) ||
		 (err = getPciValue(ent->d_name, "config", buf, sizeof(buf))))
	  err = "PCI device attributes not accessible";
	else {
	  unsigned pciClass = classword >> 16, pciSubClass = (classword >> 8) & 0xff;
	  if (verbose)
	    printf("dom %lld bus %lld devN %lld func %lld vendor 0x%llx "
		   "device 0x%llx class 0x%x subclass 0x%x\n",
		   domain, bus, deviceN, function, vendor, device, pciClass, pciSubClass);
	  if (vendor == theVendor && device == theDevice &&
	      pciClass == theClass && pciSubClass == theSubClass) {
	    // device words match, look at BARs
	    char *r = rbuf;
	    Bar bars[6], *bar = &bars[0];
	    for (unsigned i = 0; !err && i < 6; i++) {
	      unsigned long long bottom, top, flags;
	      if (!r || sscanf(r, "%llx %llx %llx\n", &bottom, &top, &flags) != 3) {
		err = "bad resource/bar file contents";
		break;
	      }
	      r = strchr(r, '\n');
	      if (r)
		r++;
	      if (bottom && bottom != top) {
		// Got a bar
		bar->size = top - bottom + 1;
		bar->io = flags & 1;
		bar->address = bottom;
		if (bar->io) {
		  bar->prefetch = false;
		  bar->addressSize = 32;
		} else {
		  bar->prefetch = (flags & 8) != 0;
		  if ((flags & 0x6) == 0)
		    bar->addressSize = 32;
		  else if ((flags & 0x6) == 4)
		    bar->addressSize = 64;
		  else
		    err = "Invalid address space indication";
		}			  
		if (verbose)
		  printf("   BAR %d: 0x%llux to 0x%llux (%lu%s %db %s %s)\n",
			 i, bottom, top, 
			 bar->size >= 1024 ? bar->size/1024 : bar->size, bar->size >= 1024 ? "K" : "",
			 bar->addressSize, bar->io ? "io":"mem", bar->prefetch ? "pref" : "npf");
		bar++;
	      } // end of good bar
	    } // end of bar loop
	    if (!err)
	      found(bars, bar - bars, verbose);
	  } // end of matching device
	} // end of good dir entry
      } // end of processing dir entry
      return 0;
    } // end of search


    int pciMemFd = -1;
    bool foundIt = false;

    void found(Bar *bars, unsigned nbars, bool verbose) {
      const char *err = 0;
      if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
	  bars[0].addressSize != 32 || bars[0].size < sizeof(OccpSpace))
	err = "OCFRP found but bars are misconfigured\n";
      else {
	void *bar0 = 0, *bar1 = 0;
	// PCI config info looks good.  Now check the OCCP signature.
	if (pciMemFd < 0 && (pciMemFd = open("/dev/mem", O_RDWR)) < 0)
	  err = "Can't open /dev/mem, probably need to run as root/su";
	else if ((bar0 = mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
			      pciMemFd, bars[0].address)) == (void*)-1)
	  err = "can't mmap /dev/mem for bar0";
	else if ((bar1 = mmap(NULL, bars[1].size, PROT_READ|PROT_WRITE, MAP_SHARED,
			      pciMemFd, bars[1].address)) == (void*)-1)
	  err = "can't mmap /dev/mem for bar1";
	else {
	  volatile OccpSpace *occp = (OccpSpace *)bar0;
	  if (occp->admin.magic1 != OCCP_MAGIC1 || occp->admin.magic2 != OCCP_MAGIC2) {
	    err = "Magic numbers do not match in region/bar 0";
	    if (verbose)
	      printf("PCI Device matches OCFRP vendor/device, but not OCCP signature: "
		     "magic1: 0x%x (sb 0x%x), magic2: 0x%x (sb 0x%x)",
		     occp->admin.magic1, OCCP_MAGIC1, occp->admin.magic2,
		     OCCP_MAGIC2);
	  } else {
	    char tbuf[30];
	    time_t bd = occp->admin.birthday;
	    printf("Found an OpenOCPI reference platform with bitstream birthday: %s", ctime_r(&bd, tbuf));
	    foundIt = true;
	  }
	}
	if (err) {
	  if (bar0)
	    munmap(bar0, sizeof(OccpSpace));
	  if (bar1)
	    munmap(bar1, bars[1].size);
	}
      }
      if (err)
	fprintf(stderr, "Error during probe for OCFRP: %s\n", err);
    }

  }
}
int
main(int argc, char **argv)
{
  const char *err = OCPI::RPL::search(0, OCFRP0_VENDOR, OCFRP0_DEVICE,
				     OCFRP0_CLASS, OCFRP0_SUBCLASS, OCPI::RPL::found,
				     argv[1] && strcmp(argv[1], "-v") == 0);
  if (err) {
    fprintf(stderr, "PCI Scanner Error: %s\n", err);
    return 1;
  } else if (!OCPI::RPL::foundIt) {
    fprintf(stderr, "Did not find an OpenOCPI FPGA reference platform.\n");
    return 1;
  }
  return 0;

}
