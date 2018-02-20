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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Nov  4 16:58:40 2013 EST
 * BASED ON THE FILE: replicate.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: replicate
 */
#include "replicate_Worker.h"

REPLICATE_METHOD_DECLARATIONS;

// Initialized to zero by the container
typedef struct {
  unsigned inPos, outPos, repCount;
} MyState;

RCCDispatch replicate = {
 /* insert any custom initializations here */
  .memSize = sizeof(MyState),
 REPLICATE_DISPATCH
};

/*
 * Methods to implement for worker replicate, based on metadata.
 */
static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  RCCPort
    *in = &self->ports[REPLICATE_IN],
    *out = &self->ports[REPLICATE_OUT];
  char
    *inData = in->current.data,
    *outData = out->current.data;
  MyState *s = self->memory;
  ReplicateProperties *p = self->properties;
  (void)timedOut;(void)newRunCondition;

  // We know there is a current input and current output buffer.
  // We know that inPos, outPos, and repCount all start out zero
  while (s->inPos < in->input.length && s->outPos < out->current.maxLength) {
    outData[s->outPos++] = inData[s->inPos];
    if (++s->repCount >= p->factor) {
      s->inPos++;
      s->repCount = 0;
    }
  }
  // We have run out of room in input or output buffer
  if (s->inPos >= in->input.length) {
    self->container.advance(in, 0);
    s->inPos = 0;
  }
  if (s->outPos >= out->current.maxLength) {
    out->output.length = s->outPos;
    self->container.advance(out, 0);
    s->outPos = 0;
  }
  return in->input.length == 0 ? RCC_DONE : RCC_OK;
}  
