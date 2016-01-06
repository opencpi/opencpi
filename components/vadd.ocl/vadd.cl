/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Aug 24 11:50:23 2011 EDT
 * BASED ON THE FILE: vadd.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: vadd
 */

#include "vadd-worker.h"

/*
 * Required work group size for worker vadd run() function.
 */
#define OCL_WG_X 64
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
 * Methods to implement for worker vadd, based on metadata.
 */

static OCLResult
vadd_run(VaddWorker* self, __global VaddProperties *properties) {
  const size_t n_elems = self->in0.current.length / sizeof ( float );
  __global const float
    *src0 = (__global float*)self->in0.current.data,
    *src1 = (__global float*)self->in1.current.data;
  __global float* dst = (__global float*)self->out.current.data;
  size_t gid = get_global_id(0);

  if (gid >= n_elems)
    return OCL_DONE;
  dst[gid] = src0[gid] + src1[gid];
  self->out.current.length = self->in0.current.length;
  self->out.current.opCode = self->in0.current.opCode;
  return OCL_ADVANCE;
}

