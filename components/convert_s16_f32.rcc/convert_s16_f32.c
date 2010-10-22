
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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Jul 27 09:25:58 2010 EDT
 * BASED ON THE FILE: convert_s16_f32.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: convert_s16_f32
 */
#include "convert_s16_f32_Worker.h"

#if defined ( __PPC__ )
#include <altivec.h>
#endif

CONVERT_S16_F32_METHOD_DECLARATIONS;
RCCDispatch convert_s16_f32 =
{
  /* insert any custom initializations here */
  CONVERT_S16_F32_DISPATCH
};

/*
 * Methods to implement for worker convert_s16_f32, based on metadata.
*/


static void do_convert_s16_f32 ( const int16_t* src,
                                 float* dst,
                                 float* scale,
                                 float* bias,
                                 size_t n_elems )
{
#if defined ( __PPC__ )

  /*
    Assume pointers are 16-byte aligned and n_elems is a multiple
    of 32.
    Ignores scale an bias
  */
  ( void ) scale;
  ( void ) bias;

  __vector signed short* v_src = ( __vector signed short* ) ( src );

  __vector float* v_dst = ( __vector float* ) ( dst );

  __vector signed short v0_src_s16,
                        v1_src_s16,
                        v2_src_s16,
                        v3_src_s16;

  __vector float v0_src_f32,
                 v1_src_f32,
                 v2_src_f32,
                 v3_src_f32,
                 v4_src_f32,
                 v5_src_f32,
                 v6_src_f32,
                 v7_src_f32;

  size_t n;

  for ( n = 0; n < n_elems; n += 32 )
  {
    v0_src_s16 = *v_src++;
    v1_src_s16 = *v_src++;
    v2_src_s16 = *v_src++;
    v3_src_s16 = *v_src++;

    v0_src_f32 = vec_ctf ( vec_unpackh ( v0_src_s16 ), 0 );
    v1_src_f32 = vec_ctf ( vec_unpackl ( v0_src_s16 ), 0 );

    v2_src_f32 = vec_ctf ( vec_unpackh ( v1_src_s16 ), 0 );
    v3_src_f32 = vec_ctf ( vec_unpackl ( v1_src_s16 ), 0 );

    v4_src_f32 = vec_ctf ( vec_unpackh ( v2_src_s16 ), 0 );
    v5_src_f32 = vec_ctf ( vec_unpackl ( v2_src_s16 ), 0 );

    v6_src_f32 = vec_ctf ( vec_unpackh ( v3_src_s16 ), 0 );
    v7_src_f32 = vec_ctf ( vec_unpackl ( v3_src_s16 ), 0 );

    *v_dst++ = v0_src_f32;
    *v_dst++ = v1_src_f32;
    *v_dst++ = v2_src_f32;
    *v_dst++ = v3_src_f32;
    *v_dst++ = v4_src_f32;
    *v_dst++ = v5_src_f32;
    *v_dst++ = v6_src_f32;
    *v_dst++ = v7_src_f32;
  }
#else
  size_t n;

  for ( n = 0; n < n_elems; n++ )
  {
    dst [ n ] = ( float ) ( *scale * src [ n ] + *bias );
  }
#endif
}

static RCCResult initialize ( RCCWorker* self )
{
  Convert_s16_f32Properties* props =
                             ( Convert_s16_f32Properties* ) self->properties;

  props->scale = 1.0;
  props->bias = 0.0;

  return RCC_OK;
}

static RCCResult start ( RCCWorker* self )
{
  /* Nothing to do */
  ( void ) self;
  return RCC_OK;
}

static RCCResult stop ( RCCWorker* self )
{
  /* Nothing to do */
  ( void ) self;
  return RCC_OK;
}

static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  ( void ) timedOut;
  ( void ) newRunCondition;  
  Convert_s16_f32Properties* props =
                             ( Convert_s16_f32Properties* ) self->properties;

  RCCPort* in = &self->ports [ CONVERT_S16_F32_IN ];

  RCCPort* out = &self->ports [ CONVERT_S16_F32_OUT ];

  float scale = ( float ) props->scale;
  float bias = ( float ) props->bias;

  do_convert_s16_f32 ( ( int16_t* ) in->current.data,
                       ( float* ) out->current.data,
                       &scale,
                       &bias,
                       in->input.length / sizeof ( int16_t ) );

  out->output.length = 2 * in->input.length;
  out->output.u.operation = in->input.u.operation;

  return RCC_ADVANCE;
}
