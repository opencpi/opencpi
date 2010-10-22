
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

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
int main(int argc, char **argv) {
  int fd;
  uint32_t val;
  off_t off;
  assert(argc == 3);
  errno = 0;
  off = strtoul(argv[1], NULL, 16);
  assert(errno == 0);
  assert((fd = open("/dev/mem", O_RDWR)) != -1);
  if (argv[2][0] == 'm') {
    uint32_t *p;
    printf("Accessing, via pointer indirection:");
    assert ((p = mmap(NULL, sizeof(val), PROT_READ, MAP_SHARED, fd, off)) != (uint32_t *)-1);
    val = *p;
  } else {
    printf("Accessing, via seek/read:");
    assert(lseek(fd, off, SEEK_SET) != -1);
    assert(read(fd, &val, sizeof(val)) == sizeof(val));
  }
  printf("value: 0x%x\n", val);
  return 0;
}
