
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
 * self FILE WAS ORIGINALLY GENERATED ON Thu Jun  3 08:57:49 2010 EDT
 * BASED ON THE FILE: adc.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * self file contains the RCC implementation skeleton for worker: adc
 */

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "adc_Worker.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327
#endif


/* ---- Worker Memories Table -------------------------------------------- */

#define FILE_POINTER_INDEX 0

static size_t memories [ ] =
{
  sizeof ( FILE* ), /* Input data file pointer */
  0
};

/* ---- Worker Dispatch Table -------------------------------------------- */

ADC_METHOD_DECLARATIONS;
RCCDispatch adc = {
  /* insert any custom initializations here */
  .memSizes = memories,

  .test = 0,
  .afterConfigure = 0,
  .beforeQuery = 0,
  .runCondition = 0,
  .portInfo = 0,
  .optionallyConnectedPorts = 0,

  ADC_DISPATCH
};

/*
 * Methods to implement for worker adc, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  // First 1KB is the XM header.
  const char* datafile = "/data1/bh/bhwbinput32k.tmp";

  FILE** file = ( FILE** ) self->memories [ FILE_POINTER_INDEX ];

  *file = fopen ( datafile, "rb" );

  if ( !( *file ) )
  {
    fprintf ( stderr, "\n\nError: Unable to open input file %s\n", datafile );
    return RCC_OK;
  }

  return RCC_OK;
}

static RCCResult release ( RCCWorker* self )
{
  if ( self->memories [ FILE_POINTER_INDEX ] )
  {
    FILE* file = ( FILE* ) self->memories [ FILE_POINTER_INDEX ];
    fclose ( file );
    file = 0;
  }

  return RCC_OK;
}


#if 1

static void cosine_generator ( double* data,
                               double frequency,
                               size_t n_elems )
{
  double phase_increment = 2.0 * M_PI / frequency;

  size_t n;
  for ( n = 0; n < n_elems; n++ )
  {
    data [ n ] = cos ( phase_increment * n );
  }
}

static void scale_f32_to_s16 ( double* data_f32,
                               int16_t* data_s16,
                               size_t n_elems )
{
  size_t n;

  /* ---- Find the old minimum and maximum ----------------------------- */

  double old_min = data_f32 [ 0 ];

  for ( n = 0; n < n_elems; n++ )
  {
    old_min = ( data_f32 [ n ] < old_min ) ? data_f32 [ n ] : old_min;
  }

  double old_max = data_f32 [ 0 ];

  for ( n = 0; n < n_elems; n++ )
  {
    old_max = ( data_f32 [ n ] > old_max ) ? data_f32 [ n ] : old_max;
  }

  /* ---- Scale to the new minimum and maximum ------------------------- */

  const double new_min = -32768;
  const double new_max = 32767;

  const double scale = ( old_max - old_min ) / ( new_max - new_min );

  const double shift = old_min - ( scale * new_min );

  for ( n = 0; n < n_elems; n++ )
  {
    data_s16 [ n ] = ( int16_t ) ( ( data_f32 [ n ] - shift ) / scale );
  }
}

static double waveform_f64 [ 2048 ];
static int16_t waveform_s16 [ 2048 ];

#endif

static RCCResult start ( RCCWorker* self )
{
  ( void ) self; /* Nothing to do */

  cosine_generator ( waveform_f64, 32.0, 2048 );

  scale_f32_to_s16 ( waveform_f64, waveform_s16, 2048 );

  return RCC_OK;
}

static RCCResult stop ( RCCWorker* self )
{
  ( void ) self; /* Nothing to do */
  return RCC_OK;
}


#if defined ( __PPC__ )

#include <altivec.h>

/*
  Assume 16-byte alignment of src and dst. Assume n_bytes is a multiple
  of 64 bytes.
*/

static void do_sample_swap ( const uint32_t* src,
                             uint32_t* dst,
                             size_t n_bytes )
{
  __vector unsigned char unpack_and_swap = {  2,  3,  0,  1,
                                              6,  7,  4,  5,
                                             10,  11, 8,  9,
                                             14,  15, 12, 13 };

  __vector unsigned char v0, v1, v2, v3;

  __vector unsigned char* v_src = ( __vector unsigned char* ) src;
  __vector unsigned char* v_dst = ( __vector unsigned char* ) dst;

  size_t n;

  for ( n = 0; n < n_bytes; n += 64 )
  {
    v0 = *v_src++;
    v1 = *v_src++;
    v2 = *v_src++;
    v3 = *v_src++;

    v0 = vec_perm ( v0, v0, unpack_and_swap );
    v1 = vec_perm ( v1, v1, unpack_and_swap );
    v2 = vec_perm ( v2, v2, unpack_and_swap );
    v3 = vec_perm ( v3, v3, unpack_and_swap );

    *v_dst++ = v0;
    *v_dst++ = v1;
    *v_dst++ = v2;
    *v_dst++ = v3;
  }
}

#else

static void do_sample_swap ( const uint32_t* src,
                             uint32_t* dst,
                             size_t n_bytes )
{
  size_t n;

  for ( n = 0; n < n_bytes / sizeof ( *src ); n++ )
  {
    uint16_t sample0 = ( uint16_t ) ( src [ n ] & 0x0000FFFF );

    uint16_t sample1 = ( uint16_t ) ( ( src [ n ] >> 16 ) & 0x0000FFFF );

    dst [ n ] = ( sample0 << 16 ) | sample1;
  }
}
#endif


static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  ( void ) timedOut;
  ( void ) newRunCondition;

  uint8_t* buffer = ( uint8_t* ) self->ports [ ADC_OUT ].current.data;

#if 0
  FILE* file = *( ( FILE** ) self->memories [ FILE_POINTER_INDEX ] );

  size_t n_bytes = self->ports [ ADC_OUT ].current.maxLength;

  size_t offset = 0;

  while ( n_bytes )
  {
    size_t n_read = fread ( &( buffer [ offset ] ),
                            sizeof ( uint8_t ),
                            n_bytes,
                            file );
    offset += n_read;
    n_bytes -= n_read;

    if ( feof ( file ) )
    {
      rewind ( file );
    }

  } /* End: while ( n_bytes ) */
#else
  memcpy ( buffer, waveform_s16, 4096 );
#endif

  /* Swap the 16-bit samples within a 32-bit word to match the swap
     done by the ADC IP.
  */
  do_sample_swap ( ( uint32_t* ) buffer,
                   ( uint32_t* ) buffer,
                   self->ports [ ADC_OUT ].current.maxLength );

  self->ports [ ADC_OUT ].output.length =
                               self->ports [ ADC_OUT ].current.maxLength;

  return RCC_ADVANCE;
}
