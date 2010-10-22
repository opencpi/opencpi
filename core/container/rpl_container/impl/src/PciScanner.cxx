
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
#include "PciScanner.h"
// This should be in OCPIOS.
// This is the linux version (kernel 2.6+).
namespace OCPI {
  namespace PCI {
#define PCIDIR "/sys/bus/pci/devices"
    static const char *
    getPciValue(const char *dev, const char *value, char *buf, unsigned len)
    {
      int n, fd;
      // get bars
      n = snprintf(buf, len, "%s/%s/%s", PCIDIR, dev, value);
      if (n <= 0 || (unsigned)n >= len)
        return "buffer violation";
      fd = open(buf, O_RDONLY);
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
    // return true if it is a candidate
    bool
    probe(const char *name, unsigned theVendor, unsigned theDevice, unsigned theClass,
          unsigned theSubClass, Bar *bars, unsigned &nbars, const char *&err) {
      unsigned long domain, bus, deviceN, function, vendor, device, classword;
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
      return false;
    }
    const char *
    search(const char **exclude,
           unsigned theVendor, unsigned theDevice, unsigned theClass, unsigned theSubClass,
           Found found, unsigned &count) {
      DIR *pcid = opendir(PCIDIR);
      if (!pcid)
        return "can't open the " PCIDIR " directory";
      struct dirent *ent;
      const char *err = 0;
      while (!err && (ent = readdir(pcid)) != NULL) {
        if (ent->d_name[0] == '.')
          continue;
        if (exclude) {
          const char **ap;
          for (ap = exclude; *ap; ap++)
            if (!strcmp(*ap, ent->d_name))
              break;
          if (*ap)
            continue;
        }
        Bar bars[6];
        unsigned nbars = 6;
        if (probe(ent->d_name, theVendor, theDevice, theClass, theSubClass, bars, nbars,
                  err) &&
            found(ent->d_name, bars, nbars))
          count++;
      } // end of processing dir entry
      return err;
    } // end of search
  } // namespace PCI
} // namespace OCPI
