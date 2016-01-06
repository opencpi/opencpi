/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Mar  9 18:00:06 2015 EDT
 * BASED ON THE FILE: bias.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: bias
 */

#include "bias-worker.h"

static OCLResult
bias_run(BiasWorker *self, __global BiasProperties *properties) {
  size_t nElems                = self->in.current.length / sizeof(uint32_t);
  __global const uint32_t *src = (__global uint32_t *)self->in.current.data;
  __global uint32_t* dst       = (__global uint32_t *)self->out.current.data;

  // Do my part, given my global ID in the first dimension
  unsigned
    me = get_global_id(0),
    nKernels = get_global_size(0),
    nEach = (nElems + nKernels - 1) / nKernels,
    myBase = nEach * me;
  if (self->logLevel >= 10)
    printf((__constant char *)"bias.cl: me %u nKernels %u nEach %u myBase %u nElems %u\n",
	   me, nKernels, nEach, myBase, (unsigned)nElems);
  for (unsigned n = nEach; myBase < nElems && n; n--, myBase++)
    dst[myBase] = src[myBase] + properties->biasValue;
  // From here on everyone does the same thing
  self->out.current.length = self->in.current.length;
  self->out.current.opCode = self->in.current.opCode;
  return nElems ? OCL_ADVANCE : OCL_ADVANCE_DONE;
}
