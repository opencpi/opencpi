
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
#include <uuid/uuid.h>
// FIXME: integrate this into our UUID utility properly
#ifndef _UUID_STRING_T
#define _UUID_STRING_T
typedef char uuid_string_t[50]; // darwin has 37 - lousy unsafe interface
#endif
#include "HdlOCCP.h"
#include "getPci.h" // this is actual code

static const char *found(const char *name,Bar *bars, unsigned nBars, bool verbose);
// consider a specific PCI device return error if something
// bad happened.
const char *
doDevice(const char *name,
	 unsigned theVendor, unsigned theDevice, unsigned theClass, unsigned theSubClass,
	 bool verbose) {
  const char *err;
  Bar bars[6];
  unsigned nbars;
  if ((err = getPci(name, theVendor,theDevice,theClass, theSubClass, verbose,bars, &nbars)))
    return err;
  return nbars ? found(name, bars, nbars, verbose) : NULL;
}
const char *
search(const char **exclude,
       unsigned theVendor, unsigned theDevice, unsigned theClass, unsigned theSubClass,
       bool verbose) {
  const char *err = NULL;
  struct dirent *ent;
  DIR *pcid = opendir(PCIDIR);
  if (!pcid) 
    return "Can't open the " PCIDIR " directory";
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
found(const char *name, Bar *bars, unsigned nbars, bool verbose) {
  const char *err = 0;
  void *bar0 = 0, *bar1 = 0;
  volatile OccpSpace *occp;

  if (nbars != 2 || bars[0].io || bars[0].prefetch || bars[1].io || bars[1].prefetch ||
      bars[0].addressSize != 32 || bars[0].size < sizeof(OccpSpace))
    return "OCFRP found but bars are misconfigured\n";
  // PCI config info looks good.  Now check the OCCP signature.
  if (pciMemFd < 0 && (pciMemFd = open("/dev/mem", O_RDWR)) < 0)
    return "Can't open /dev/mem, probably need to run as root/sudo";
  do { // break on error to recover mappings
    if ((bar0 = mmap(NULL, sizeof(OccpSpace), PROT_READ|PROT_WRITE, MAP_SHARED,
		     pciMemFd, bars[0].address)) == (void*)-1) {
      err = "can't mmap /dev/mem for bar0";
      break;
    }
    if ((bar1 = mmap(NULL, bars[1].size, PROT_READ|PROT_WRITE, MAP_SHARED,
		     pciMemFd, bars[1].address)) == (void*)-1) {
      err = "can't mmap /dev/mem for bar1";
      break;
    }
    occp = (OccpSpace *)bar0;
    if (occp->admin.magic1 != OCCP_MAGIC1 || occp->admin.magic2 != OCCP_MAGIC2) {
      if (verbose)
	printf("PCI Device matches OCFRP vendor/device, but not OCCP signature: "
	       "magic1: 0x%x (sb 0x%x), magic2: 0x%x (sb 0x%x)",
	       occp->admin.magic1, OCCP_MAGIC1, occp->admin.magic2,
	       OCCP_MAGIC2);
      err = "Magic numbers do not match in region/bar 0";
      break;
    }
    {
      char tbuf[30], tbuf1[30];
      HdlUUID myUUID;
      char platform[sizeof(myUUID.platform)+1];
      char device[sizeof(myUUID.device)+1];
      char load[sizeof(myUUID.load)+1];
      uuid_string_t textUUID;
      time_t bsvbd, bsbd;
      // Capture the UUID info that tells us about the platform
      for (unsigned n = 0; n < sizeof(HdlUUID); n++)
	((uint8_t*)&myUUID)[sizeof(HdlUUID) - 1 - n] = ((volatile uint8_t *)&occp->admin.uuid)[n];
      strncpy(platform, myUUID.platform, sizeof(myUUID.platform));
      platform[sizeof(myUUID.platform)] = '\0';
      strncpy(device, myUUID.device, sizeof(myUUID.device));
      device[sizeof(myUUID.device)] = '\0';
      strncpy(load, myUUID.load, sizeof(myUUID.load));
      load[sizeof(myUUID.load)] = '\0';
      uuid_unparse_lower(myUUID.uuid, textUUID);
      bsvbd = occp->admin.birthday;
      bsbd = myUUID.birthday;
      ctime_r(&bsvbd, tbuf);
      tbuf[strlen(tbuf)-1] = 0;
      ctime_r(&bsbd, tbuf1);
      tbuf1[strlen(tbuf1)-1] = 0;
#if 1
      printf("OpenCPI FPGA at PCI %s: BSV date %s, bitstream date %s, "
	     "platform \"%s\", device \"%s\", UUID %s, loadParam \"%s\"\n",
	     name, tbuf, tbuf1, platform, device, textUUID, load);
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
    munmap(bar0, sizeof(OccpSpace));
  if (bar1)
    munmap(bar1, bars[1].size);
  return err;
}

int
main(int argc, char **argv)
{
  bool verbose = false;
  if (argv[1] && !strcmp(argv[1], "-v")) {
    verbose = true;
    argv++;
  }
  const char *err;
  if (argv[1]) {
    if ((err = doDevice(argv[1], OCFRP0_VENDOR, OCFRP0_DEVICE, OCFRP0_CLASS, OCFRP0_SUBCLASS,
			verbose))) {
      fprintf(stderr, "Couldn't find \"%s\": %s\n", argv[1], err);
      return 1;
    } else if (!nFound) {
      fprintf(stderr, "Device specified is not recognized as an OpenCPI FPGA platform\n");
      return 1;
    }
  } else {
    if ((err = search(0, OCFRP0_VENDOR, OCFRP0_DEVICE, OCFRP0_CLASS, OCFRP0_SUBCLASS,
		      verbose))) {
      fprintf(stderr, "PCI Scanner Error: %s\n", err);
      return 1;
    } else if (!nFound) {
      fprintf(stderr, "Did not find any OpenOCPI FPGA platforms.\n");
      return 1;
    }
  }
  return 0;
}

