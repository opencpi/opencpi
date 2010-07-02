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

#include <sys/types.h>
#include <fcntl.h>

#include <sys/mman.h>

//#define MAPIT
uint64_t
atoi_any(char *arg)
{
  int value ;

#if 1
  return strtoull(arg, 0, 0);
#else
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
      *nbytes_arg = argv[2],
      *update_value_arg = argv[3] ;
    int rc, fd ;
    uint64_t offset = atoi_any(offset_arg) ;
    int nbytes = atoi_any(nbytes_arg) ;
    int mmoffset = offset&(PAGE-1) ;
    int mmbase = offset&(~(PAGE-1)) ;
    int update_value = (argc>3)?atoi_any(update_value_arg):0 ;
    int update = (argc>3) ;
    void *ptr ;

    printf("offset=%08x nbytes=%d mmoffset=%08x mmbase=%08x\n",
	offset, nbytes, mmoffset, mmbase) ;
    
    if((fd = open("/dev/mem", O_RDWR))==-1)
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

    ptr += mmoffset ;
#endif

    if(update)
      {
        * (int *) ptr = update_value ;
	nbytes=4 ;
      }

      {
       int addr=offset ;
       int i, j ;
       for(i=0; i<nbytes; i+=16)
         {
           fprintf(stdout,"%8x(%d): ", offset+i, (int)sizeof(uint32_t)) ;
	   fflush(stdout);
           for(j=0; j<16; j+=4)
             {
	       uint32_t v;
#ifdef MAPIT
                uint32_t *lptr = ptr+i+j ;

		v = *lptr;
#else
		int x = lseek(fd, offset + i + j, SEEK_SET);
		int y = read(fd, &v, 4);
#endif
                fprintf(stdout,"%8x ", v) ;
             } 
           fprintf(stdout,"\n") ;
         }          
      }
  }
  return 0 ;
}
