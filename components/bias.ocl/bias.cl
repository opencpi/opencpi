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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Mar  9 18:00:06 2015 EDT
 * BASED ON THE FILE: bias.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: bias
 */

#include "bias-worker.h"

static OCLResult
bias_start(BiasWorker *self, __global BiasProperties *properties) {
  (void)self;
  return OCL_OK;
}

static OCLResult
bias_run(BiasWorker *self, __global BiasProperties *properties) {
  size_t nElems                = self->ports.in.current.length / sizeof(uint32_t);
  __global const uint32_t *src = (__global uint32_t *)self->ports.in.current.data;
  __global uint32_t* dst       = (__global uint32_t *)self->ports.out.current.data;

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
  self->ports.out.current.length = self->ports.in.current.length;
  self->ports.out.current.opCode = self->ports.in.current.opCode;
  return nElems ? OCL_ADVANCE : OCL_ADVANCE_DONE;
}
