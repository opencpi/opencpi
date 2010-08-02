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
