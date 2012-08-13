
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
#include <limits.h>
#include "PciDriver.h"

typedef int bool;
const bool false = 0;
const bool true = 1;
typedef struct {
  uint64_t address;
  uint64_t size;
  bool io, prefetch;
  unsigned addressSize;
} Bar;
#define PCIDIR "/sys/bus/pci/devices"
static const char *
getPciValue(const char *dev, char *value, char *buf, unsigned len)
{
  int n, fd;
  // get bars
  n = snprintf(buf, len, "%s/%s/%s", PCIDIR, dev, value);
  if (n <= 0 || (unsigned)n >= len)
    return "buffer violation";
  fd = open(buf, O_RDONLY);
  if (fd < 0)
    return "can't open descriptor file for PCI device";
  n = read(fd, buf, len-1);
  close(fd);
  if (n <= 0 && (unsigned)n >= len-1)
    return "can't read descriptor file for PCI device";
  buf[n] = 0;
  return 0;
}
static const char*
getPciNumber(const char *dev, char *value, char *buf, unsigned len, unsigned long long *np)
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
// Get PCI info from device name
#define MAXBARS 6
const char *
getPci(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
       unsigned theSubClass, bool verbose, Bar *bars, unsigned *nbars) {
  const char *err = 0;
  char buf[1024], rbuf[1024];
  unsigned long long domain, bus, deviceN, function, vendor, device, classword;
  unsigned pciClass, pciSubClass;
  char *r = rbuf;
  Bar *bar = &bars[0];
  *nbars = 0;
  if (sscanf(name, "%llx:%llx:%llx.%lld", &domain, &bus, &deviceN, &function) != 4)
    return "Invalid PCI device name";
  if ((err = getPciValue(name, "resource", rbuf, sizeof(rbuf))) ||
      (err = getPciNumber(name, "vendor", buf, sizeof(buf), &vendor)) ||
      (err = getPciNumber(name, "class", buf, sizeof(buf), &classword)) ||
      (err = getPciNumber(name, "device", buf, sizeof(buf), &device)) ||
      (err = getPciValue(name, "config", buf, sizeof(buf))))
    return "PCI device attributes not accessible";
  pciClass = classword >> 16;
  pciSubClass = (classword >> 8) & 0xff;
  if (verbose) {
    printf("dom %lld bus %lld devN %lld func %lld vendor 0x%llx "
	   "device 0x%llx class 0x%x subclass 0x%x\n",
	   domain, bus, deviceN, function, vendor, device, pciClass, pciSubClass);
    printf("resource = '%s'\n", rbuf);
  }
  if (vendor != theVendor || device != theDevice ||
      pciClass != theClass || pciSubClass != theSubClass)
    return NULL;
  for (unsigned i = 0; !err && i < 6; i++) {
    unsigned long long bottom, top, flags;
    if (!r || sscanf(r, "%llx %llx %llx\n", &bottom, &top, &flags) != 3)
      return "Bad PCI resource/bar file contents";
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
	  return "Invalid address space indication";
      }			  
      if (verbose)
	printf("   BAR %d: 0x%llx to 0x%llx (%llu%s %db %s %s)\n",
	       i, bottom, top, 
	       (unsigned long long)
	       (bar->size >= 1024 ? bar->size/1024 : bar->size),
	       bar->size >= 1024 ? "K" : "",
	       bar->addressSize, bar->io ? "io":"mem", bar->prefetch ? "pref" : "npf");
      bar++;
    } // end of good bar
  } // end of bar loop
  *nbars = bar - bars;
  return NULL;
}

const char *
getOpenCPI(char *pci, Bar *bars, unsigned *nbars, bool verbose) {
  return getPci(pci, OCPI_HDL_PCI_VENDOR_ID, OCPI_HDL_PCI_DEVICE_ID, OCPI_HDL_PCI_CLASS, OCPI_HDL_PCI_SUBCLASS,
		verbose, bars, nbars);
}
