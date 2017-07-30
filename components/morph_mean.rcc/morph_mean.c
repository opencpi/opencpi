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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun 29 10:49:36 2010 EDT
 * BASED ON THE FILE: morph_mean.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: morph_mean
 */
#include "morph_mean_Worker.h"

#include <stdio.h>
#include <string.h>

#if defined ( __PPC__ )
#include <altivec.h>
#endif

MORPH_MEAN_METHOD_DECLARATIONS;
RCCDispatch morph_mean =
{
  /* insert any custom initializations here */
  MORPH_MEAN_DISPATCH
};

/*
 * Methods to implement for worker morph_mean, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  ( void ) self;
  /* Nothing to do */
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

static float sum_across ( const float* buffer, size_t n_elems )
{
  size_t n;

  __vector float* v_buf = ( __vector float* ) buffer;

  __vector float v0, v1, v2, v3;

  __vector float part_v0 = { 0.0, 0.0, 0.0, 0.0 };
  __vector float part_v1 = { 0.0, 0.0, 0.0, 0.0 };
  __vector float part_v2 = { 0.0, 0.0, 0.0, 0.0 };
  __vector float part_v3 = { 0.0, 0.0, 0.0, 0.0 };

  for ( n = 0; n < n_elems; n += 16 )
  {
    v0 = *v_buf++;
    v1 = *v_buf++;
    v2 = *v_buf++;
    v3 = *v_buf++;

    part_v0 = vec_add ( part_v0, v0 );
    part_v1 = vec_add ( part_v1, v1 );
    part_v2 = vec_add ( part_v2, v2 );
    part_v3 = vec_add ( part_v3, v3 );
  }

  part_v0 = vec_add ( part_v1, part_v0 );
  part_v2 = vec_add ( part_v3, part_v2 );
  part_v0 = vec_add ( part_v2, part_v0 );

  part_v1 = vec_mergeh ( part_v0, part_v0 );
  part_v2 = vec_mergel ( part_v0, part_v0 );
  part_v0 = vec_add ( part_v1, part_v2 );
  part_v1 = vec_mergeh ( part_v0, part_v0 );
  part_v2 = vec_mergel ( part_v0, part_v0 );

  part_v0 = vec_add ( part_v1, part_v2 );

  float sum;

  vec_ste ( part_v0, 0, &sum );

  return sum;
}

#endif

static float vsum ( const float* p, size_t n_elems )
{
  float sum = 0.0;

#if defined ( __PPC__ )

  sum = sum_across ( p, n_elems );

#else
  size_t n;

  for ( n = 0; n < n_elems; n++ )
  {
    sum += p [ n ];
  }
#endif

  return sum;
}

static void mean_filter ( const float* p_src,
                          float* p_dst,
                          size_t window_length,
                          size_t n_elems )
{
  size_t n;

  size_t sum_len;

  if ( window_length > n_elems )
  {
    window_length = n_elems;
  }

  size_t half_window_length = window_length / 2;

  for ( n = 0; n < half_window_length; n++ )
  {
    sum_len = n + half_window_length;

    p_dst [ n ] = vsum ( p_src, sum_len ) / ( float ) sum_len;
  }

  sum_len = window_length;

  size_t iw = window_length;

  float sum = 0.0;

  const float scale = 1.0 / ( float ) window_length;

  for ( n = half_window_length; n < n_elems - half_window_length ; n++ )
  {
    if ( iw >= window_length )
    {
      sum = vsum ( &( p_src [ n - half_window_length ] ), sum_len );
      iw = 0;
    }
    else
    {
      sum = sum - ( p_src [ n - half_window_length ] +
                    p_src [ n + half_window_length ] );
    }
    p_dst [ n ] = sum * scale;
    iw++;
  }

  for ( n = n_elems - half_window_length; n < n_elems; n++ )
  {
    sum_len = n_elems - n + half_window_length;

    p_dst [ n ] = vsum ( &( p_src [ n - half_window_length ] ),
                         sum_len ) / ( float ) sum_len;
  }
}

static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  ( void ) timedOut;
  ( void ) newRunCondition;

  Morph_meanProperties* props = ( Morph_meanProperties* ) self->properties;

  RCCPort* in = &self->ports [ MORPH_MEAN_IN ];

  float* p_src = ( float* ) in->current.data;

  RCCPort* out = &self->ports [ MORPH_MEAN_OUT ];

  float* p_dst = ( float* ) out->current.data;

  size_t n_elems = in->input.length / sizeof ( *p_src );

  mean_filter ( p_src, p_dst, props->window_length, n_elems );

  out->output.u.operation = in->input.u.operation;

  out->output.length = in->input.length;

  return RCC_ADVANCE;
}
