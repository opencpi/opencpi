/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Sep  9 10:27:13 2011 EDT
 * BASED ON THE FILE: sin.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: sin
 */

#include "sin_Worker.h"

/*
 * Required work group size for worker sin run() function.
 */
#define OCL_WG_X 64
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
 * Methods to implement for worker sin, based on metadata.
 */

OCLResult sin_run ( __local OCLWorkerSin* self,
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

  dst [ gid ] = sin ( src [ gid ] );

  self->out.attr.length = self->in.attr.length;
  self->out.attr.u.operation = self->in.attr.u.operation;


  return OCL_ADVANCE;
}
