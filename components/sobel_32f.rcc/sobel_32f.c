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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri May 6 03:40:31 2011 EDT
 * BASED ON THE FILE: sobel_32f.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: sobel_32f
 */
#include "sobel_32f_Worker.h"

#include <string.h>
#include <stdlib.h>

typedef uint8_t Pixel;      // the data type of pixels
typedef int16_t PixelTemp;  // the data type for intermediate pixel math
#define MAX UINT8_MAX       // the maximum pixel value
#define FRAME_BYTES (p->height * p->width * sizeof(Pixel)) // pixels per frame

typedef struct {
  uint16_t init;
  unsigned lineAt;
  Pixel *img; 
} SState;

static uint32_t sizes[] = {sizeof(SState), 0 };

SOBEL_32F_METHOD_DECLARATIONS;
RCCDispatch sobel_32f = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  SOBEL_32F_DISPATCH
};

static RCCResult start(RCCWorker *self)
{
  SState *myState = self->memories[0];  
  Sobel_32fProperties *p = self->properties;
  if ( myState->init == 0 ) {
    myState->img = malloc(p->height * p->width * sizeof(Pixel));
  }
  myState->init = 1;
  return RCC_OK;
}

static RCCResult release(RCCWorker *self)
{
  SState *myState = self->memories[0];  
  if ( myState->init ) {
    free( myState->img );
  }
  return RCC_OK;
}

// X derivative
inline static void
doLineX(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    PixelTemp t =
      -1 * l0[i-1] + 1 * l0[i+1] +
      -2 * l1[i-1] + 2 * l1[i+1] +
      -1 * l2[i-1] + 1 * l2[i+1];
    out[i] = t < 0 ? 0 : (t > MAX ? MAX : t);
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

inline static void
doLineX_32f(Pixel *l0, Pixel *l1, Pixel *l2, float *out, unsigned width) {
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    float t = (float)
      -1 * l0[i-1] + 1 * l0[i+1] +
      -2 * l1[i-1] + 2 * l1[i+1] +
      -1 * l2[i-1] + 1 * l2[i+1];
    out[i] = t;
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

// Y derivative
inline static void
doLineY(Pixel *l0, Pixel *l1, Pixel *l2, Pixel *out, unsigned width) {
  ( void )l1;
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    PixelTemp t = 
      -1 * l0[i-1] + 1 * l2[i-1] +
      -2 * l0[i]   + 2 * l2[i]   +
      -1 * l0[i+1] + 1 * l2[i+1];
    out[i] = t < 0 ? 0 : (t > MAX ? MAX : t);
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

inline static void
doLineY_32f(Pixel *l0, Pixel *l1, Pixel *l2, float *out, unsigned width) {
  ( void )l1;
  unsigned i; // don't depend on c99 yet
  for (i = 1; i < width - 1; i++) {
    float t = (float)
      -1 * l0[i-1] + 1 * l2[i-1] +
      -2 * l0[i]   + 2 * l2[i]   +
      -1 * l0[i+1] + 1 * l2[i+1];
    out[i] = t;
  }
  out[0] = out[width-1] = 0; // boundary conditions
}

static unsigned 
update(SState *myState,
       int H, int W, Pixel *src, Pixel *dst, float *dst_32f,
       unsigned lines, unsigned xderiv) {

  unsigned i, j;
  for( i = 0; i < lines; i++ ) {
    int at = (myState->lineAt + i) * W;
    int src_at = i * W;
    for( j = 0; j < (unsigned)W; j++ )
      myState->img[at+j] = src[src_at+j];
  }

  unsigned produced = 0;
  for( i = 0; i < lines; i++ ) {
    int at = myState->lineAt + i;
    // at = 0 begin (zero lines)
    if( at == 1 ) {
      // first line
      memset(dst, 0, sizeof(Pixel) * W);
      memset(dst_32f, 0, sizeof(float) * W);
      dst += W;
      dst_32f += W;
      produced++;
    }
    else if( at > 1 ) {
      Pixel *img_cur = myState->img + at * W;
      if( xderiv ) {
        doLineX( img_cur - W, img_cur, img_cur + W, dst, W );
        doLineX_32f( img_cur - W, img_cur, img_cur + W, dst_32f, W );
      }
      else {
        doLineY( img_cur - W, img_cur, img_cur + W, dst, W );
        doLineY_32f( img_cur - W, img_cur, img_cur + W, dst_32f, W );
      }
      dst += W;
      dst_32f += W;
      produced++;
    }

    if( at == H - 1 ) {
      // end (two lines)
      memset(dst, 0, sizeof(Pixel) * W);
      memset(dst_32f, 0, sizeof(float) * W);
      dst += W;
      dst_32f += W;
      produced++;
    }
  }
  return produced;
}


/*
 * Methods to implement for worker sobel_32f, based on metadata.
 */

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  SState *myState = self->memories[0];  
  Sobel_32fProperties *p = self->properties;
  RCCPort *in = &self->ports[SOBEL_32F_IN],
    *out = &self->ports[SOBEL_32F_OUT],
    *out_32f = &self->ports[SOBEL_32F_OUT_32F];

  if ( (in->input.length>0) && (in->input.length>FRAME_BYTES) ) {
    return RCC_ERROR;
  }

  // update and produce gradients
  unsigned lines = in->input.length / p->width;
  unsigned produced = update( myState, 
			      p->height, p->width, in->current.data,
			      (Pixel *) out->current.data,
			      (float *) out_32f->current.data,
			      lines, p->xderiv );
  myState->lineAt += lines;

  // next image
  if( myState->lineAt == p->height ) {
    myState->lineAt = 0;
  }

  out->output.u.operation = in->input.u.operation;
  out->output.length = produced * p->width * sizeof(Pixel);
  out_32f->output.u.operation = in->input.u.operation;
  out_32f->output.length = produced * p->width * sizeof(float);

  return RCC_ADVANCE;
}
