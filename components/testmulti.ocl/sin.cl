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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Sep  9 10:27:13 2011 EDT
 * BASED ON THE FILE: sin.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the OCL implementation skeleton for worker: sin
 */

#include "sin-worker.h"

/*
 * Required work group size for worker sin run() function.
 */
#define OCL_WG_X 64
#define OCL_WG_Y 1
#define OCL_WG_Z 1

/*
 * Methods to implement for worker sin, based on metadata.
 */

OCLResult sin_run(__local OCLWorkerSin* self) {
  const size_t n_elems = self->in.current.length / sizeof(float);
  __global const float* src = (__global float *)self->in.current.data;
  __global float* dst = (__global float *)self->out.current.data;
  size_t gid = get_global_id(0);

  if (gid < n_elems) {
    dst[gid] = sin(src[gid]);
    self->out.current.length = self->in.current.length;
    self->out.current.opCode = self->in.current.opCode;
  }
  return OCL_ADVANCE;
}
