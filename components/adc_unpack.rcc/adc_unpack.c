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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Jul 28 07:09:04 2010 EDT
 * BASED ON THE FILE: adc_unpack.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: adc_unpack
 */
#include "adc_unpack_Worker.h"

#include <string.h>

ADC_UNPACK_METHOD_DECLARATIONS;
RCCDispatch adc_unpack =
{
  /* insert any custom initializations here */
  ADC_UNPACK_DISPATCH
};

/*
 * Methods to implement for worker adc_unpack, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  Adc_unpackProperties* props = ( Adc_unpackProperties* ) self->properties;

  props->n_bytes = 0;
  props->n_buffers = 0;
  props->sample_swap = 0;

  return RCC_OK;
}

static RCCResult start ( RCCWorker* self )
{
  ( void ) self;
  /* Nothing to do */
  return RCC_OK;
}

static RCCResult stop ( RCCWorker* self )
{
  ( void ) self;
  /* Nothing to do */
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
  Adc_unpackProperties* props = ( Adc_unpackProperties* ) self->properties;

  RCCPort* in = &self->ports [ ADC_UNPACK_IN ];

  RCCPort* out = &self->ports [ ADC_UNPACK_OUT ];

  const uint32_t* src = ( const uint32_t* ) in->current.data;

  uint32_t* dst = ( uint32_t* ) out->current.data;

  if ( src && dst )
  {
    props->n_buffers++;
    props->n_bytes += in->input.length;
  }

  if ( props->sample_swap  )
  {
    do_sample_swap ( src, dst, in->input.length );
  }
  else /* Bypass */
  {
    memcpy ( dst, src, in->input.length );
  }

  out->output.length = in->input.length;
  out->output.u.operation = in->input.u.operation;

  return RCC_ADVANCE;
}
