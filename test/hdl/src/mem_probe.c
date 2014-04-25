
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


/*
  Mercury Federal Systems, Inc.
  James E/P Saari, Systems Engineer, Reston, Virginia

  simple memory peek/poke program

  This test provides one-word access to /dev/mem

  Parameters:

        offset       - the offset of the buffer in /dev/mem
        nbytes       - the size of the buffer
        update-value - if this parameter is provided, a write
                       operation is done with this value to
                       the offset address

  NOTE: this program references /dev/mem. This program must
  be run as root or have appropriate permissions on these files.

   7 May 2008 - jsaari@mc.com - update docs
  27 Jan 2008 - jsaari@mc.com - original code
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifdef XIL_CACHE
#include "xil_cache.h"
#endif

#define MAPIT
uint64_t
atoi_any(char *arg)
{

#if 1
  return strtoull(arg, 0, 0);
#else
  int value ;
  if(strncmp(arg,"0x",2))
    sscanf(arg,"%u",&value) ;
  else
    sscanf(arg,"0x%ux",&value) ;

  return value ;
#endif
}



int
main(int argc, char *argv[])
{
  if(argc<3)
    {
      fprintf(stderr,"%s <offset> <nbytes> (update-value)\n",
        argv[0]) ;
      return 1 ;
    }
  {
    #define PAGE 0x10000

    char
      *offset_arg = argv[1],
      *nbytes_arg = argv[2];
    int fd ;
    uint64_t offset = atoi_any(offset_arg) ;
    int nbytes = atoi_any(nbytes_arg) ;
    int mmoffset = offset&(PAGE-1) ;
    int mmbase = offset&(~(PAGE-1)) ;
#ifdef MAPIT
    char *update_value_arg = argv[3] ;
    int update_value = (argc>3)?atoi_any(update_value_arg):0 ;
    int update = (argc>3) ;
    void *ptr ;
#endif

    printf("offset=%08llx nbytes=%d mmoffset=%08x mmbase=%08x\n",
	   (long long)offset, nbytes, mmoffset, mmbase) ;

    if((fd = open("/dev/mem", O_RDWR|O_SYNC))==-1)
      {
        perror("/dev/mem") ;
        return 1 ;
      }

#ifdef MAPIT
    if((ptr = mmap(NULL, nbytes+mmoffset, PROT_READ|PROT_WRITE, MAP_SHARED,
                   fd, mmbase))==MAP_FAILED)
      {
        fprintf(stderr,"mmap(..., size=%d, offset=0x%08x,...), error=%s\n",
                nbytes+mmoffset, mmbase, strerror(errno)) ;
        return 1 ;
      }

#ifdef XIL_CACHE
    Xil_DCacheInvalidate();
#endif
    ptr += mmoffset ;

    if(update)
      {
        * (int *) ptr = update_value ;
	nbytes=4 ;
      }
#endif

      {
       int i, j ;
       for(i=0; i<nbytes; i+=16)
         {
           fprintf(stdout,"%8llx(%d): ", (long long)(offset+i), (int)sizeof(uint32_t)) ;
	   fflush(stdout);
           for(j=0; j<16; j+=4)
             {
	       uint32_t v;
#ifdef MAPIT
                uint32_t *lptr = ptr+i+j ;

		v = *lptr;
#else
		lseek(fd, offset + i + j, SEEK_SET);
		read(fd, &v, 4);
#endif
                fprintf(stdout,"%8x ", v) ;
             }
           fprintf(stdout,"\n") ;
         }
      }
  }
  return 0 ;
}
