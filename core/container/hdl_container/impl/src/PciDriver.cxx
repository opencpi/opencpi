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
#include <string>
#include "OcpiUtilMisc.h"
#include "HdlOCCP.h"
#include "PciDriver.h"
// This should be in OCPIOS.
// This is the linux version (kernel 2.6+).
namespace OCPI {
  namespace HDL {
    namespace PCI {
      namespace OS = OCPI::OS;
      namespace OU = OCPI::Util;
#define PCIDIR "/sys/bus/pci/devices"
      int Driver::s_pciMemFd = -1;


      static const char *
      getPciValue(const char *dev, const char *value, char *buf, unsigned len)
      {
	int n, fd;
	// get bars
	n = snprintf(buf, len, "%s/%s/%s", PCIDIR, dev, value);
	if (n <= 0 || (unsigned)n >= len)
	  return "buffer violation";
	fd = ::open(buf, O_RDONLY);
	if (fd < 0)
	  return "can't open descriptor file for PCI device";
	n = read(fd, buf, len - 1); // leave room for the null char
	close(fd);
	if (n <= 0)
	  return "can't read descriptor file for PCI device";
	buf[n] = 0;
	return 0;
      }
      static const char*
      getPciNumber(const char *dev, const char *value, char *buf, unsigned len, unsigned long *np)
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
      // See if this looks like an appropriate PCI entry
      static bool
      probe(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
	    unsigned theSubClass, Bar *bars, unsigned &nbars, std::string &error) {
	unsigned long domain, bus, deviceN, function, vendor, device, classword;
	const char *err = 0;
	char buf[512], rbuf[512];
	if (sscanf(name, "%lx:%lx:%lx.%ld", &domain, &bus, &deviceN, &function) != 4)
	  err = "bad PCI /sys device name";
	else if ((err = getPciValue(name, "resource", rbuf, sizeof(rbuf))) ||
		 (err = getPciNumber(name, "vendor", buf, sizeof(buf), &vendor)) ||
		 (err = getPciNumber(name, "class", buf, sizeof(buf), &classword)) ||
		 (err = getPciNumber(name, "device", buf, sizeof(buf), &device)) ||
		 (err = getPciValue(name, "config", buf, sizeof(buf))))
	  err = "PCI device attributes not accessible";
	else {
	  unsigned pciClass = classword >> 16, pciSubClass = (classword >> 8) & 0xff;
#if 0
	  printf("dom %ld bus %ld devN %ld func %ld vendor 0x%lx "
		 "device 0x%lx class 0x%x subclass 0x%x\n",
		 domain, bus, deviceN, function, vendor, device, pciClass, pciSubClass);
#endif
	  if (vendor == theVendor && device == theDevice &&
	      pciClass == theClass && pciSubClass == theSubClass) {
	    // device words match, look at BARs in the "resource" file (rbuf)
	    char *r = rbuf;
	    Bar *bar = bars;
	    for (unsigned i = 0; !err && i < nbars; i++) {
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
#if 0
		printf("   BAR %d: 0x%llx to 0x%llx (%lu%s %db %s %s)\n",
		       i, bottom, top,
		       bar->size >= 1024 ? bar->size/1024 : bar->size,
		       bar->size >= 1024 ? "K" : "",
		       bar->addressSize, bar->io ? "io":"mem",
		       bar->prefetch ? "pref" : "npf");
#endif
		bar++;
	      } // end of good bar
	    } // end of bar loop
	    if (!err) {
	      nbars = bar - bars;
	      return true;
	    }
	  }
	}
	if (err)
	  error = err;
	return false;
      }
      unsigned Driver::
      search(const OU::PValue */*params*/, const char **exclude, std::string &error) {
	unsigned count = 0;
	DIR *pcid = opendir(PCIDIR);
	if (!pcid)
	  error = "can't open the " PCIDIR " directory for PCI search";
	else {
	  for (struct dirent *ent; error.empty() && (ent = readdir(pcid)) != NULL;)
	    if (ent->d_name[0] != '.') {
	      Access cAccess, dAccess;
	      std::string name, endpoint;
	      // Opening implies canonicalizing the name, which is needed for excludes
	      if (open(ent->d_name, name, cAccess, dAccess, endpoint, error)) {
		if (exclude)
		  for (const char **ap = exclude; *ap; ap++)
		    if (!strcmp(*ap, name.c_str()))
		      goto skipit; // continue(2);
		if (found(name.c_str(), cAccess, dAccess, endpoint, error))
		  count++;
	      }
	    skipit:
	      ;
	    }
	  closedir(pcid); // FIXME: try/catch?
	}
	return count;
      }
      bool Driver::
      open(const char *pciName, std::string &name, HDL::Access &cAccess, HDL::Access &dAccess,
	   std::string &endpoint, std::string &error) {
	const char *cp;
	for (cp = pciName; *cp && isdigit(*cp); cp++)
	  ;
	name = "PCI:";
	if (*cp)
	  name += pciName;
	else
	  OU::formatStringAdd(name, "0000:%02d:00.0", atoi(pciName));
	Bar bars[6];
	unsigned nbars = 6;
	if (probe(name.c_str()+4, OCFRP0_PCI_VENDOR, OCFRP0_PCI_DEVICE, OCFRP0_PCI_CLASS, OCFRP0_PCI_SUBCLASS,
		  bars, nbars, error))
	  if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
	      bars[0].addressSize != 32 || bars[0].size != sizeof(OccpSpace))
	    error = "Found PCI device w/ good vendor/device/class, but bars are wrong; skipping it; use lspci";
	  else {
	    void *bar0 = 0, *bar1 = 0;
	    // PCI config info looks good.  Now check the OCCP signature.
	    if (s_pciMemFd < 0 && (s_pciMemFd = ::open("/dev/mem", O_RDWR|O_SYNC)) < 0)
	      error = "Can't open /dev/mem";
	    else if ((bar0 = mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
				  s_pciMemFd, bars[0].address)) == (void*)-1)
	      error = "can't mmap /dev/mem for bar0";
	    else if ((bar1 = mmap(NULL, bars[1].size, PROT_READ|PROT_WRITE, MAP_SHARED,
				  s_pciMemFd, bars[1].address)) == (void*)-1)
	      error = "can't mmap /dev/mem for bar1";
	    else {
	      uint64_t endpointPaddr, endpointSize, controlOffset, bufferOffset;
	      if (bars[0].address < bars[1].address) {
		endpointPaddr = bars[0].address;
		endpointSize = bars[1].address + bars[1].size - endpointPaddr;
		controlOffset = 0;
		bufferOffset = bars[1].address - bars[0].address;
	      } else {
		endpointPaddr = bars[1].address;
		endpointSize = bars[0].address + sizeof(OccpSpace) - endpointPaddr;
		controlOffset = bars[0].address - bars[1].address;
		bufferOffset = 0;
	      }
	      const char *busId = "0";
	      OU::formatString(endpoint,
			       "ocpi-pci-pio:%s.0x%llx:%lld.3.20", busId,
			       (long long unsigned)endpointPaddr,
			       (long long unsigned)endpointSize);
	      cAccess.setAccess((uint8_t*)bar0, NULL, controlOffset);
	      dAccess.setAccess((uint8_t*)bar1, NULL, bufferOffset);
	      return true;
	    }
	  }
	if (error.size())
	  ocpiBad("When searching for PCI device '%s': %s", pciName, error.c_str());
	return false;
      }
      Driver::
      ~Driver() {
	if (s_pciMemFd >= 0) {
	  close(s_pciMemFd);
	  s_pciMemFd = -1;
	}
      }
    } // namespace PCI
  } // namespace HDL
} // namespace OCPI
