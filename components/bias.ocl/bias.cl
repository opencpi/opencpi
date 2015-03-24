/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Mar  9 18:00:06 2015 EDT
 * BASED ON THE FILE: bias.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: bias
 */

#include "bias-worker.h"

/*
 * Required work group size for worker bias run() function.
 */
#define OCL_WG_X 1
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
 * Methods to implement for worker bias, based on metadata.
 */

OCLResult bias_run(BiasWorker *self, __global BiasProperties *properties) {
  const size_t n_elems = self->in.current.length / sizeof(float);
  __global const uint32_t* src = (__global uint32_t *)self->in.current.data;
  __global uint32_t* dst = (__global uint32_t *)self->out.current.data;
  size_t gid = get_global_id(0);

  if (self->logLevel >= 10)
    printf("src %p dst %p gid %d Got length: %d\n", src, dst, gid, n_elems);
  for (unsigned n = 0; n < n_elems; n++)
    dst[n] = src[n] + properties->biasValue;
  self->out.current.length = self->in.current.length;
  self->out.current.opCode = self->in.current.opCode;
  return n_elems ? OCL_ADVANCE : OCL_ADVANCE_DONE;
}
