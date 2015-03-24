/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Aug 24 11:47:41 2011 EDT
 * BASED ON THE FILE: vsadd.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: vsadd
 */

#include "vsadd-worker.h"

/*
 * Required work group size for worker vsadd run() function.
 */
#define OCL_WG_X 64
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
 * Methods to implement for worker vsadd, based on metadata.
 */

OCLResult vsadd_run(VsaddWorker* self, __global VsaddProperties *properties) {
  const size_t n_elems = self->in.current.length / sizeof(float);
  __global const float* src = (__global float *)self->in.current.data;
  __global float* dst = (__global float *)self->out.current.data;
  size_t gid = get_global_id(0);

  if (gid >= n_elems)
    return OCL_DONE;
  dst[gid] = src[gid] + properties->scalar;
  self->out.current.length = self->in.current.length;
  self->out.current.opCode = self->in.current.opCode;
  return OCL_ADVANCE;
}

