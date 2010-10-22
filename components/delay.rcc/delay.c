
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
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Jun 26 11:04:28 2010 EDT
 * BASED ON THE FILE: delay.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: delay
 */
#include "delay_Worker.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

/* ---- Worker Memories Table -------------------------------------------- */

typedef struct MmapInfo
{
  int fd;
  uint8_t* p_mem;
  size_t n_bytes;
  size_t rd_off;
  size_t wr_off;
} MmapInfo;

#define MMAP_INFO_INDEX 0

static uint32_t memories [ ] =
{
  sizeof ( MmapInfo  ), /* mmap() file information */
  0
};

/* ---- Worker Dispatch Table -------------------------------------------- */

DELAY_METHOD_DECLARATIONS;
RCCDispatch delay =
{
  /* insert any custom initializations here */
  .memSizes = memories,
  .test = 0,
  .afterConfigure = 0,
  .beforeQuery = 0,
  .runCondition = 0,
  .portInfo = 0,
  .optionallyConnectedPorts = 0,

  DELAY_DISPATCH
};

/*
 * Methods to implement for worker delay, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  DelayProperties* p = ( DelayProperties* ) self->properties;

  p->dlyCtrl = 0;
  p->dlyHoldoffBytes = 0;
  p->dlyHoldoffCycles = 0;
#if WAS
  p->mesgWtCount = 0;
  p->mesgRdCount = 0;
  p->bytesWritten = 0;
  p->portStatus = 0;
  p->reserved = 0;
  p->WSI_S_pMesgCount = 0;
  p->WSI_S_iMesgCount = 0;
  p->WSI_S_tBusyCount = 0;
  p->WSI_M_pMesgCount = 0;
  p->WSI_M_iMesgCount = 0;
  p->WSI_M_tBusyCount = 0;
  p->wmemiWrReq = 0;
  p->wmemiRdReq = 0;
  p->wmemiRdResp = 0;
  p->dlyWordsStored = 0;
  p->dlyRdCredit = 0;
  p->dlyWAG = 0;
  p->dlyRAG = 0;
#endif
  return RCC_OK;
}

static RCCResult start ( RCCWorker* self )
{
  DelayProperties* p = ( DelayProperties* ) self->properties;

  if ( p->dlyCtrl )
  {
    size_t delay_n_bytes = 0;

    if ( p->dlyHoldoffBytes )
    {
      delay_n_bytes = p->dlyHoldoffBytes;
    }
    else if ( p->dlyHoldoffCycles )
    {
      /* FPGA IP writes 4 bytes per cycle */
      const uint32_t n_fpga_bytes_per_cycle = 4;
      delay_n_bytes = p->dlyHoldoffCycles * n_fpga_bytes_per_cycle;
    }

    MmapInfo* p_map_info = ( MmapInfo* ) self->memories [ MMAP_INFO_INDEX ];

    p_map_info->fd = open ( "/data1/bh_delay.bin", O_RDWR );

    if ( p_map_info->fd == -1 )
    {
      perror ( "open(/data1/bh_delay.bin)" );
      return RCC_ERROR;
    }

    p_map_info->p_mem = mmap ( 0,
                               delay_n_bytes,
                               PROT_WRITE | PROT_READ,
                               MAP_SHARED,
                               p_map_info->fd,
                               0 );

    if ( p_map_info->p_mem == MAP_FAILED )
    {
      perror ( "mmap()" );
      return RCC_ERROR;
    }

    bzero ( p_map_info->p_mem, delay_n_bytes );
    p_map_info->rd_off = 0;
    p_map_info->wr_off = 0;
    p_map_info->n_bytes = delay_n_bytes;

  } /* End: if ( p->dlyCtrl ) */

  return RCC_OK;
}

static RCCResult stop ( RCCWorker* self )
{
  MmapInfo* p_map_info = ( MmapInfo* ) self->memories [ MMAP_INFO_INDEX ];

  if ( p_map_info )
  {
    if ( p_map_info->fd > 0 )
    {
      int rc = close ( p_map_info->fd );
      p_map_info->fd = -1;

      if ( rc )
      {
        perror ( "close()" );
        return RCC_ERROR;
      }
    }

    if ( p_map_info->p_mem )
    {
      int rc = munmap ( p_map_info->p_mem, p_map_info->n_bytes );
      p_map_info->p_mem = 0;

      if ( rc )
      {
        perror ( "munmap()" );
        return RCC_ERROR;
      }
    }
  } /* End: if ( p_map_info ) */


  return RCC_OK;
}

static RCCResult release ( RCCWorker* self )
{
  ( void ) self;
  /* Nothing to do */
  return RCC_OK;
}

static void read_delayed_data ( MmapInfo* p_map_info,
                                uint8_t* p_dst,
                                size_t n_bytes )
{

  if ( ( p_map_info->rd_off + n_bytes ) < p_map_info->n_bytes )
  {
    memcpy ( p_dst, &( p_map_info->p_mem [ p_map_info->rd_off ] ), n_bytes );

    p_map_info->rd_off += n_bytes;
  }
  else /* Wrap around */
  {
    size_t partial_n_bytes = p_map_info->n_bytes - p_map_info->rd_off;

    memcpy ( p_dst,
             &( p_map_info->p_mem [ p_map_info->rd_off ] ),
             partial_n_bytes );

    p_map_info->rd_off = 0;

    size_t remainder_n_bytes = n_bytes - partial_n_bytes;

    memcpy ( &( p_dst [ partial_n_bytes ] ),
             &( p_map_info->p_mem [ p_map_info->rd_off ] ),
             remainder_n_bytes );

    p_map_info->rd_off += remainder_n_bytes;
  }
}

static void write_delayed_data ( MmapInfo* p_map_info,
                                 const uint8_t* p_src,
                                 size_t n_bytes )
{
  if ( ( p_map_info->wr_off + n_bytes ) < p_map_info->n_bytes )
  {
    memcpy ( &( p_map_info->p_mem [ p_map_info->wr_off ] ), p_src, n_bytes );

    p_map_info->wr_off += n_bytes;
  }
  else /* Wrap around */
  {
    size_t partial_n_bytes = p_map_info->n_bytes - p_map_info->wr_off;

    memcpy ( &( p_map_info->p_mem [ p_map_info->wr_off ] ),
             p_src,
             partial_n_bytes );

    p_map_info->wr_off = 0;

    size_t remainder_n_bytes = n_bytes - partial_n_bytes;

    memcpy ( &( p_map_info->p_mem [ p_map_info->wr_off ] ),
             &( p_src [ partial_n_bytes ] ),
             remainder_n_bytes );

    p_map_info->wr_off += remainder_n_bytes;
  }
}

/*
  We assume that we read and write the same buffer sizes and at the
  same frequency so we do not have to worry about the write pointer or
  read pointer passing each other.
*/
static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  ( void ) timedOut;
  ( void ) newRunCondition;
  const uint8_t* p_src =
                  ( const uint8_t* ) self->ports [ DELAY_IN ].current.data;

  uint8_t* p_dst = ( uint8_t* ) self->ports [ DELAY_OUT ].current.data;

  size_t n_bytes = self->ports [ DELAY_IN ].current.maxLength;

  DelayProperties* p = ( DelayProperties* ) self->properties;

  if ( p->dlyCtrl )
  {
    MmapInfo* p_map_info = ( MmapInfo* ) self->memories [ MMAP_INFO_INDEX ];

    /* Read delayed data */
    read_delayed_data ( p_map_info, p_dst, n_bytes );

    self->ports [ DELAY_OUT ].output.length = n_bytes;

    /* Write new data */
    write_delayed_data ( p_map_info, p_src, n_bytes );
  }
  else  /* No delay */
  {
    memcpy ( p_dst, p_src, n_bytes );

    self->ports [ DELAY_OUT ].output.length = n_bytes;
  }

  return RCC_ADVANCE;
}
