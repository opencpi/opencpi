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

#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <string>
#include <cerrno>
#include <climits>
#include <cstdlib> // strtoull
#include "OcpiUtilMisc.h"
#include "OcpiUtilPci.h"

namespace OCPI {
namespace Util {

static const char *
getPciValue(const char *dev, const char *value, char *buf, unsigned len) {
  int fd;
  ssize_t n;
  // get bars
  n = snprintf(buf, len, "%s/%s/%s", OCPI_HDL_SYS_PCI_DIR, dev, value);
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
getPciNumber(const char *dev, const char *value, char *buf, unsigned len, unsigned long *np) {
  const char *err = getPciValue(dev, value, buf, len);
  if (err)
    return err;
  errno = 0;
  unsigned long long ull = strtoull(buf, NULL, 0);
  if (ull == ULLONG_MAX && errno == ERANGE)
    return "unexpected attribute value for PCI Device";
  *np = OCPI_UTRUNCATE(unsigned long, ull);
  return 0;
}
// See if this looks like an appropriate PCI entry: return true if OK otherwise set error
bool
probePci(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
	 unsigned theSubClass, bool verbose, Bar *bars, unsigned &nbars, std::string &error) {
  unsigned long domain, bus, deviceN, function, vendor, device, classword, subvendor, subdevice;
  const char *err = 0;
  char buf[512], rbuf[512];
  if (sscanf(name, "%lx:%lx:%lx.%ld", &domain, &bus, &deviceN, &function) != 4)
    err = "bad PCI /sys device name";
  else if ((err = getPciValue(name, "resource", rbuf, sizeof(rbuf))) ||
	   (err = getPciNumber(name, "vendor", buf, sizeof(buf), &vendor)) ||
	   (err = getPciNumber(name, "class", buf, sizeof(buf), &classword)) ||
	   (err = getPciNumber(name, "device", buf, sizeof(buf), &device)) ||
	   (err = getPciNumber(name, "subsystem_vendor", buf, sizeof(buf), &subvendor)) ||
	   (err = getPciNumber(name, "subsystem_device", buf, sizeof(buf), &subdevice)) ||
	   (err = getPciValue(name, "config", buf, sizeof(buf))))
    err = "PCI device attributes not accessible";
  else {
    unsigned long pciClass = classword >> 16, pciSubClass = (classword >> 8) & 0xff;
    if (verbose)
      printf("dom %lu bus %lu devN %lu func %lu vendor 0x%lx "
	     "device 0x%lx class 0x%lx subclass 0x%lx subvendor %lx subdevice %lx\n",
	     domain, bus, deviceN, function, vendor, device, pciClass, pciSubClass,
	     subvendor, subdevice);
    if (vendor == theVendor && (theDevice == UINT_MAX || device == theDevice) &&
	pciClass == theClass && pciSubClass == theSubClass) {
      // device words match, look at BARs in the "resource" file (rbuf)
      char *r = rbuf;
      Bar *bar = bars;
      for (unsigned i = 0; i < nbars; i++) {
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
	    else {
	      err = "Invalid address space indication";
	      break;
	    }
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
      if (!err) {
	nbars = OCPI_UTRUNCATE(unsigned, bar - bars);
	return true;
      }
    }
  }
  if (err)
    error = err;
  return false;
}
} // Util
} // OCPI
