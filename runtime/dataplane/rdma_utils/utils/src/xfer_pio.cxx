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

#include <stdio.h>
#include <stdlib.h>
#include "OcpiOsDataTypes.h"
#include "OcpiOsMisc.h"
#include "DtOsDataTypes.h"
#include "DtSharedMemoryInterface.h"
#include "DtTransferInterface.h"
#include "xfer_if.h"
#include "xfer_internal.h"

using namespace DataTransfer;

#define USE_BYTE_TRANSFERS
#ifdef USE_BYTE_TRANSFERS
void
xfer_pio_action_transfer(PIO_transfer transfer)
{

  /* Get the alignments */
  uint32_t src_al = (uint32_t)((uintptr_t)transfer->src_va & 3);
  uint32_t dst_al = (uint32_t)((uintptr_t)transfer->dst_va & 3);
  uint32_t i;

  /* Check src and dst alignment */
  if (src_al != dst_al) {
    /* 
     * src and dst addresses don't have same alignment
     * we will need to transfer everything as bytes.
     */
    char *src = (char *)transfer->src_va;
    char *dst = (char *)transfer->dst_va;
    size_t nbytes = transfer->nbytes;


    if (src == 0) {
      for (i=0; i < nbytes; i++)
        *dst++ = 0;
    }
    else {
      for (i=0; i < nbytes; i++)
        *dst++ = *src++;
    }
  }
  else if (src_al > 0) {
    /*
     * We need to perform 'src_al' number of byte transfers
     * before we are word aligned, then we can use word
     * transfers.
     */
    char *src_b = (char *)transfer->src_va;
    char *dst_b = (char *)transfer->dst_va;

    int32_t *src_w;
    int32_t *dst_w;

    for (i=0; i < src_al; i++)
      *dst_b++ = *src_b++;

    /* Get the word pointers */
    src_w = (int32_t *)src_b;
    dst_w = (int32_t *)dst_b;

    /* Get the word count and remainder */
    size_t nwords = (transfer->nbytes - src_al) / 4;
    size_t rem_nwords = (transfer->nbytes - src_al) % 4;

    if (src_w == 0) {
      for (i=0; i < nwords; i++)    
        *dst_w++ = 0;
    }
    else {
      for (i=0; i < nwords; i++)    
        *dst_w++ = *src_w++;
    }
    
    if (rem_nwords) {
      /* Set the byte pointers */
      src_b = (char *)src_w;
      dst_b = (char *)dst_w;

      /* Process remainder in bytes */
      if (src_w == 0) {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = 0;
      }
      else {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = *src_b++;
      }
    }
  }
  else {
    /* Get the word pointers */
    int32_t *src_w = (int32_t *)transfer->src_va;
    int32_t *dst_w = (int32_t *)transfer->dst_va;

    size_t nwords = transfer->nbytes / 4;
    size_t rem_nwords = transfer->nbytes % 4;

    if (src_w == 0) {
      for (i=0; i < nwords; i++)    
        *dst_w++ = 0;
    }
    else {
      for (i=0; i < nwords; i++)    
        *dst_w++ = *src_w++;
    }
    
    if (rem_nwords) {
      /* Set the byte pointers */
      char *src_b = (char *)src_w;
      char *dst_b = (char *)dst_w;

      /* Process remainder in bytes */
      if (src_w == 0) {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = 0;
      }
      else {
        for (i=0; i < rem_nwords; i++)
          *dst_b++ = *src_b++;
      }
    }
  }

  //#define TRACE_PIO_XFERS  
#ifdef TRACE_PIO_XFERS
  int32_t *src1 = (int32_t *)transfer->src_va;
  printf("^^^^ copying %zd bytes from %zx to %zx\n",
	 (size_t)(transfer->nbytes),(size_t)(transfer->src_off),
	 (size_t)(transfer->dst_off));
  printf("source wrd 1 = %d wrd2 = %d\n", src1[0], src1[1] );
#endif

}
#else

void
xfer_pio_action_transfer(PIO_transfer transfer)
{
  uint32_t nwords = ((transfer->nbytes + 5) / 8) ;

#ifdef DEBUG
  {
        unsigned int M=50;
        char* c=(char*)src;
        printf("\n\nSource Data(0x%x) Target(0x%x):\n", src, dst );
        for (int n=0; n<M; n++ ) {
                printf("%d ", c[n]);
        }
  }
#endif



#ifdef IP_DEBUG_SUPPORT
  if ( nwords == 1 ) {
    int32_t *src1 = (int32_t *)transfer->src_va;
    int32_t *dst1 = (int32_t *)transfer->dst_va;    
    //    if ( ((src1[0] < 3) && (src1[1] < 3)) || ((src1[0] > 2048) && (src1[1] > 2048)) )  {
    if ( ((src1[0] < 3) && (src1[1] < 3)) || ((src1[0] > 2048) && (src1[1] > 2048)) )  {

      /*
      if ( (src1[0] == 1 ) || (src1[1] == 1 ) ) {
        printf("patching flag 1->0 \n");
        dst[0] = 1;
      }
      else {
        dst1[0] = 0;
      }
      */
      
      dst1[0] = 1;
      //      dst1[1] = 1;
    }
    else {
      dst1[0] = src1[0];
    }
  }
  else {
    int32_t *src1 = (int32_t *)transfer->src_va;
    int32_t *dst1 = (int32_t *)transfer->dst_va;    
    for (int32_t i=0; i < nwords*2; i++) {
      dst1[i] = src1[i];
    }
  }
#endif



  int32_t *src1 = (int32_t *)transfer->src_va;
  int32_t *dst1 = (int32_t *)transfer->dst_va;    

  //#define TRACE_PIO_XFERS  
#ifdef TRACE_PIO_XFERS
  printf("^^^^ copying %d bytes from %llx to %llx\n", transfer->nbytes,transfer->src_off,transfer->dst_off);
  printf("source wrd 1 = %d, %d\n", src1[0], src1[1] );
  printf("dst va = %p\n", dst1 );
#endif


  for (uint32_t i=0; i < nwords*2; i++) {
    dst1[i] = src1[i];
  }
  
  
  
}
#endif


int32_t
xfer_pio_create(DataTransfer::SmemServices* src, DataTransfer::SmemServices* dst, PIO_template *pio_templatep)
{
  void *src_va;
  void *dst_va;

  // Map source and destination
  src_va = src->map (0, 0);
  dst_va = dst->map (0, 0);
  ocpiDebug("Template src_va %p dst_va %p", src_va, dst_va);

  /* Allocate template */
  PIO_template pio_template = new struct pio_template_;

  /* Insert addresses to template */
  pio_template->src_va = src_va;
  pio_template->dst_va = dst_va;
  pio_template->s_smem = src;
  pio_template->t_smem = dst;

  /* Set the out parameter */
  *pio_templatep = pio_template;

  return 0;
}

#define OF_WINDOW 0x1000000

void
xfer_pio_modify( PIO_transfer transfer, int,  DtOsDataTypes::Offset *noff,  DtOsDataTypes::Offset *ooff )
{
  ooff[0] = transfer->src_off;
  transfer->src_va = (char*)transfer->src_va - transfer->src_off;
  transfer->src_off = *noff;
  transfer->src_va = (char*)transfer->src_va + *noff;
}



int32_t
xfer_pio_copy(PIO_template pio_template, DtOsDataTypes::Offset src_os, DtOsDataTypes::Offset dst_os,
              size_t nbytes, int32_t flags, PIO_transfer *pio_transferp)
{
  /* Allocate transfer */
  PIO_transfer pio_transfer = new struct pio_transfer_;

  /* Check to see if this is a flag transfer */
  if (flags & XFER_FLAG) {
    pio_transfer->src_va = 0;
    pio_transfer->src_stride = 1;
    pio_transfer->dst_va = ((char *)pio_template->src_va + src_os);
    pio_transfer->dst_stride = 1;
    pio_transfer->nbytes = nbytes;
    pio_transfer->next = 0;
  }
  else {
    /* Populate transfer */
#if CONTIGUOUS_MAP
    pio_transfer->src_va = ((char *)pio_template->src_va + src_os);
#else
    pio_transfer->src_va = pio_template->s_smem->map(src_os, nbytes);
#endif
    pio_transfer->src_off = src_os;
    pio_transfer->dst_off = dst_os;
    pio_transfer->src_stride = 1;
        
    //#define MULTI_MAP
#ifdef MULTI_MAP
    if ( dst_os >= OF_WINDOW ) {
                
#ifndef NDEBUG
      printf("*** Got a window bigger than original map, offset = 0x%x !!\n", dst_os );
#endif
                
      pio_template->rdst_va[pio_template->rdst] = pio_template->t_smem->map (dst_os, nbytes);
      pio_transfer->dst_va = (char*)pio_template->rdst_va[pio_template->rdst];
      pio_template->rdst++;
    }
    else {
      pio_transfer->dst_va = ((char *)pio_template->dst_va + dst_os);
    }
#else
#if CONTIGUOUS_MAP
    pio_transfer->dst_va = ((char *)pio_template->dst_va + dst_os);
#else
    pio_transfer->dst_va = pio_template->t_smem->map(dst_os, nbytes);
#endif
#endif
                
    pio_transfer->dst_stride = 1;
    pio_transfer->nbytes = nbytes;
    pio_transfer->next = 0;
  }

  ocpiDebug("xfer_pio_copy: dst_va: %p", pio_transfer->dst_va);
  /* Set the output parameter */
  *pio_transferp = pio_transfer;

  /* Return success */
  return 0;
}


int32_t
xfer_pio_group(PIO_transfer *members, int32_t, PIO_transfer *pio_transferp)
{
  /* Check for null member array */
  if (!members)
    return 1;

  PIO_transfer transfer = new struct pio_transfer_;
  uint32_t index = 0;

  /* Check the allocation worked */
  if (!transfer)
    return 1;

  /* Check the list is not empty */
  if (!(members[0]))
    return 1;

  /* Copy the first member into transfer */
  *transfer = *(members[0]);

  /* Local Variables */
  PIO_transfer xf = transfer;

  if (members[index]) {
    do {
      /* Local Variables */
      PIO_transfer tmp;

      /* Walk to the end of the current transfer */
      while (xf->next) {
        /* Allocate a new space to copy the transfer into */
        tmp = new struct pio_transfer_;
        /* Copy the next transfer */
        *tmp = *(xf->next);
        /* Set the correct next pointer */
        xf->next = tmp;
        /* Point xf to the new transfer */
        xf = tmp;
      }

      /* Increment the index, and check for end of list */
      if (!(members[++index]))
        break;

      /* Allocate a new member */
      tmp = new struct pio_transfer_;

      /* Copy the member into the new space */
      *tmp = *(members[index]);

      /* Append the new transfer */
      xf->next = tmp;

      /* Point xf to the new transfer */
      xf = tmp;

    } while (members[index]);
  }

  /* Set the out parameter */
  *pio_transferp = transfer;

  /* Return success */
  return 0;
}

#if 0
int32_t
xfer_pio_start(PIO_transfer pio_transfer, int32_t)
{
  PIO_transfer transfer = pio_transfer;

  do {
    /* Perform the described transfer */
    action_pio_transfer(transfer);

  } while ((transfer = transfer->next));

  return 0;
}
#endif
int32_t
xfer_pio_release(PIO_transfer pio_transfer)
{
  /* Local Variables */
  PIO_transfer xf = pio_transfer;

  /* Free the transfer list */
  do {
    /* Local Variables */
    PIO_transfer tmp;
    /* Get the next transfer (if any) */
    tmp = xf->next;
    /* Free the current transfer */
    delete xf;
    /* Copy the next transfer to xf */
    xf = tmp;

  } while (xf);

  return 0;
}

int32_t
xfer_pio_destroy(PIO_template pio_template)
{
  /* Free the transfer template */
  delete pio_template;
  return 0;
}
