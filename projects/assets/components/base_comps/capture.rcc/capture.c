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
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Oct 24 15:27:00 2012 EDT
 * BASED ON THE FILE: gen/pattern.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: pattern
 */
#include <string.h>
#include <stdbool.h>
#include "capture_Worker.h"

CAPTURE_METHOD_DECLARATIONS;
RCCDispatch capture = {
 /* insert any custom initializations here */
 CAPTURE_DISPATCH
};

/*
 * Methods to implement for worker pattern, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  CaptureProperties *p = self->properties;
  RCCPort *in = self->ports;
  uint32_t 
    *indata = (uint32_t*)in->current.data,
    *meta = p->metadata[p->nextMeta++],
    *mydata = p->data + p->nextData,
    *myend = p->data + sizeof(p->data)/sizeof(*p->data),
    nwords = (in->input.length + sizeof(uint32_t) - 1) / sizeof(uint32_t);
  bool done = false;
  
  meta[0] = in->input.length;
  meta[1] = in->input.u.operation;
  p->metadataCount++;
  while (nwords--) {
    *mydata++ = *indata++;
    p->dataCount++;
    if (mydata >= myend) {
      if (p->control & 2) {
	done = true;
	break;
      } else {
	p->nextData = 0;
	mydata = p->data;
      }
    }
  }
  if (p->nextMeta >= sizeof(p->metadata)/(sizeof(uint32_t) * 4)) {
    if (p->control & 2)
      done = true;
    else
      p->nextMeta = 0;
  }
  if (in->input.length == 0)
    done = true;
  return done ? RCC_ADVANCE_DONE : RCC_ADVANCE;
}
