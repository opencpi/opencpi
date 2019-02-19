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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "copy_Worker.h"

COPY_METHOD_DECLARATIONS;
RCCDispatch copy = {
  /* insert any custom initializations here */
  COPY_DISPATCH
};

/*
 * Methods to implement for worker copy, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort
    *in = &self->ports[COPY_IN],
    *out = &self->ports[COPY_OUT];
  assert(in->input.length <= out->current.maxLength);
  //  fprintf(stderr, "Sending %u %zu\n", in->input.u.operation, in->input.length);
#if 0 // This zero-copy mode doesn't work for some reason
  self->container.send(out, &in->current, in->input.u.operation, in->input.length);
  return RCC_OK;
#else
  out->output.length = in->input.length;
  if (in->input.length == 0)
    return RCC_ADVANCE_DONE;
  memcpy(out->current.data, in->current.data, in->input.length);
  return RCC_ADVANCE;
#endif
}
