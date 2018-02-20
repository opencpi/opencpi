/*
  =====
  Copyright (C) 2011 Massachusetts Institute of Technology


  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  ======
*/

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Feb 28 02:52:23 2011 EST
 * BASED ON THE FILE: erode.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: erode
 */
#include <string.h>
#include "erode_Worker.h"

// Algorithm specifics:
typedef uint8_t Pixel;      // the data type of pixels
#define FRAME_BYTES (p->height * p->width * sizeof(Pixel)) // pixels per frame
#define MAX UINT8_MAX       // the maximum pixel value
#define KERNEL_SIZE 3       // the size of the kernel

// Define state between runs.  Will be initialized to zero for us.
#define LINE_BYTES (p->width * sizeof(Pixel))
#define HISTORY_SIZE KERNEL_SIZE
typedef struct {
  unsigned inLine;                 // The next line# of input I expect
  RCCBuffer buffers[HISTORY_SIZE]; // buffer history (ring)
} ErodeState;

static size_t sizes[] = {sizeof(ErodeState), 0};

ERODE_METHOD_DECLARATIONS;
RCCDispatch erode = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  ERODE_DISPATCH
};


// Compute one line of output
inline static void
doLine(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    Pixel t = l0[i];
    // unroll loops for speed
    if(l0[i-1] < t)	t = l0[i-1];
    if(l0[i+1] < t)	t = l0[i+1];
    if(l1[i] < t)		t = l1[i];
    if(l1[i-1] < t)	t = l1[i-1];
    if(l1[i+1] < t)	t = l1[i+1];
    if(l2[i] < t)		t = l2[i];
    if(l2[i-1] < t)	t = l2[i-1];
    if(l2[i+1] < t)	t = l2[i+1];
    // minimum of 3x3 region
    out[i] = t;
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

// A run condition for flushing zero-length message
// Basically we only wait for an output buffer, not an input
// This will be unnecessary when we have a separate EOS indication.
static RCCPortMask endMask[] = {1 << ERODE_OUT, 0};
static RCCRunCondition end = {endMask, 0, 0};

/*
 * Methods to implement for worker erode, based on metadata.
 */

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ErodeProperties *p = self->properties;
  ErodeState *s = self->memories[0];
  RCCPort *in = &self->ports[ERODE_IN],
    *out = &self->ports[ERODE_OUT];
  const RCCContainer *c = &self->container;  
  (void)timedOut;

  if ( (in->input.length>0) && (in->input.length>FRAME_BYTES) ) {
    return RCC_ERROR;
  }

  // Arrange to send the zero-length message after the last line of last image
  // This will be unnecessary when EOS indication is fixed

  // End state:  just send the zero length message to indicate "done"
  // This will be unnecessary when EOS indication is fixed
  if (in->input.length == 0) {
    self->runCondition = &end;
    *newRunCondition = 1;
    out->output.length = 0;
    c->advance(out, 0);
    return RCC_DONE;
  }

  // Current buffer
  unsigned cur = s->inLine % HISTORY_SIZE;

  // First line: do nothing
  // Second line
  if(s->inLine == 1) {
    memset(out->current.data, 0, LINE_BYTES);
    out->output.length = LINE_BYTES;
    c->advance(out, LINE_BYTES);
  }
  // Middle line
  else if(s->inLine > 1) {
    doLine(s->buffers[(cur - 2 + HISTORY_SIZE) % HISTORY_SIZE].data,
	   s->buffers[(cur - 1 + HISTORY_SIZE) % HISTORY_SIZE].data,
	   in->current.data,
	   out->current.data,
	   p->width);
    out->output.length = LINE_BYTES;
    c->advance(out, LINE_BYTES);
  }

  // Go to next
  unsigned prev = (cur - 2 + HISTORY_SIZE) % HISTORY_SIZE;
  if(s->inLine < HISTORY_SIZE - 1)
    c->take(in, NULL, &s->buffers[cur]);
  else
    c->take(in, &s->buffers[prev], &s->buffers[cur]);
  s->inLine++;

  return RCC_OK;
}
