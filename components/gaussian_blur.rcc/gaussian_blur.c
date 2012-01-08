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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Feb 28 07:27:09 2011 EST
 * BASED ON THE FILE: gaussian_blur.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: gaussian_blur
 */
#include <math.h>
#include <string.h>
#include "gaussian_blur_Worker.h"

// Algorithm specifics:
typedef uint8_t Pixel;      // the data type of pixels
#define FRAME_BYTES (p->height * p->width * sizeof(Pixel)) // pixels per frame
typedef double PixelTemp;  // the data type for intermediate pixel math
#define MAX UINT8_MAX       // the maximum pixel value
#define KERNEL_SIZE 3       // the size of the kernel

// Define state between runs.  Will be initialized to zero for us.
#define LINE_BYTES (p->width * sizeof(Pixel))
#define HISTORY_SIZE KERNEL_SIZE
typedef struct {
  unsigned inLine;                 // The next line# of input I expect
  RCCBuffer buffers[HISTORY_SIZE]; // buffer history (ring)
} Gaussian_blurState;

static uint32_t sizes[] = {sizeof(Gaussian_blurState), 0};
static RCCPortInfo pinfo[] = { {GAUSSIAN_BLUR_IN,0,KERNEL_SIZE+1},{GAUSSIAN_BLUR_OUT,0,2},{-1,0,0} };

GAUSSIAN_BLUR_METHOD_DECLARATIONS;
RCCDispatch gaussian_blur = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  .portInfo = pinfo,
  GAUSSIAN_BLUR_DISPATCH
};

// Gaussian kernel
static double gaussian[3][3];

inline void
initKernel(double sigmaX, double sigmaY) {
  // default values 0.8
  if(sigmaX < 1e-9)
    sigmaX = 0.8;
  if(sigmaY < 1e-9)
    sigmaY = 0.8;

  unsigned i, j;
  double gx[3];
  double gy[3];
  for(i = 0; i < 3; i++) {
    gx[i] = exp(-0.5*(i-1)*(i-1)/sigmaX/sigmaX);
    gy[i] = exp(-0.5*(i-1)*(i-1)/sigmaY/sigmaY);
  }

  double sx = gx[0] + gx[1] + gx[2];
  double sy = gy[0] + gy[1] + gy[2];
  for(i = 0; i < 3; i++) {
    gx[i] /= sx;
    gy[i] /= sy;
  }

  for(i = 0; i < 3; i++)
    for(j = 0; j < 3; j++)
      gaussian[i][j] = gx[j] * gy[i];
}

// Compute one line of output
inline void
doLine(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    PixelTemp t =
      gaussian[0][0] * l0[i-1]
      + gaussian[0][1] * l0[i]
      + gaussian[0][2] * l0[i+1]
      + gaussian[1][0] * l1[i-1]
      + gaussian[1][1] * l1[i]
      + gaussian[1][2] * l1[i+1]
      + gaussian[2][0] * l2[i-1]
      + gaussian[2][1] * l2[i]
      + gaussian[2][2] * l2[i+1];
    out[i] = (Pixel) (t < 0 ? 0 : (t > MAX ? MAX : t) );
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

// A run condition for flushing zero-length message
// Basically we only wait for an output buffer, not an input
// This will be unnecessary when we have a separate EOS indication.
static RCCPortMask endMask[] = {1<<GAUSSIAN_BLUR_OUT, 0};
static RCCRunCondition end = {endMask, 0, 0};

/*
 * Methods to implement for worker gaussian_blur, based on metadata.
 */

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  Gaussian_blurProperties *p = self->properties;
  Gaussian_blurState *s = self->memories[0];
  RCCPort *in = &self->ports[GAUSSIAN_BLUR_IN],
    *out = &self->ports[GAUSSIAN_BLUR_OUT];
  const RCCContainer *c = &self->container;  
  (void)timedOut;

  if ( (in->input.length>0) && (in->input.length>FRAME_BYTES) ) {
    return RCC_ERROR;
  }

  // End state:  just send the zero length message to indicate "done"
  // This will be unnecessary when EOS indication is fixed

  // Arrange to send the zero-length message after the last line of last image
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

  // First line: init
  if(s->inLine == 0) {
    initKernel(p->sigmaX, p->sigmaY);
  }
  // Second line
  else if(s->inLine == 1) {
    memset(out->current.data, 0, LINE_BYTES);
    out->output.length = LINE_BYTES;
    c->advance(out, LINE_BYTES);
  }
  // Middle line
  else {
    doLine(s->buffers[(cur - 2 + HISTORY_SIZE) % HISTORY_SIZE].data,
	   s->buffers[(cur - 1 + HISTORY_SIZE) % HISTORY_SIZE].data,
	   in->current.data,
	   out->current.data,
	   p->width);
    out->output.length = LINE_BYTES;
    c->advance(out, LINE_BYTES);
  }

  // Go to next
  unsigned prev = (cur - HISTORY_SIZE) % HISTORY_SIZE;
  
  if(s->inLine < (HISTORY_SIZE - 1)) {
    c->take(in, NULL, &s->buffers[cur]);
  }
  else {
    c->take(in, &s->buffers[prev], &s->buffers[cur]);
  }
  s->inLine++;

  return RCC_OK;
}
