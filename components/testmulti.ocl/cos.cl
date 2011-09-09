/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Sep  9 10:27:13 2011 EDT
 * BASED ON THE FILE: cos.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: cos
 */

#include "cos_Worker.h"

/*
 * Required work group size for worker cos run() function.
 */
#define OCL_WG_X 64
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
 * Methods to implement for worker cos, based on metadata.
 */

OCLResult cos_run ( __local OCLWorkerCos* self,
                    OCLBoolean timedOut,
                    __global OCLBoolean* newRunCondition )
{
  (void)timedOut;(void)newRunCondition;

  const size_t n_elems = self->in.attr.length / sizeof ( float );

  __global const float* src = ( __global float* ) self->in.current.data;

  __global float* dst = ( __global float* ) self->out.current.data;

  int gid = get_global_id ( 0 );

  if ( gid >= n_elems )
  {
    return OCL_ADVANCE;
  }

  dst [ gid ] = cos ( src [ gid ] );

  self->out.attr.length = self->in.attr.length;
  self->out.attr.u.operation = self->in.attr.u.operation;


  return OCL_ADVANCE;
}
