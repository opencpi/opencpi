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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Mar 08 14:37:21 2011 EST
 * BASED ON THE FILE: canny.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: canny
 */
#include <string.h>
#include <stdlib.h>
#include "canny_Worker.h"

// Algorithm specifics:
typedef unsigned char uchar;
typedef uint8_t Pixel;       // the data type of pixels
typedef uint16_t PixelTemp;  // the data type for intermediate pixel math
#define MAX UINT8_MAX        // the maximum pixel value
#define FRAME_BYTES (p->height * p->width * sizeof(Pixel)) // pixels per frame

typedef struct {
  uint16_t init;
  uint16_t *dx,*dy;
  char* buffer;
  uchar **stack;
} CState;

static size_t sizes[] = {sizeof(CState), 0 };

CANNY_METHOD_DECLARATIONS;
RCCDispatch canny = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  CANNY_DISPATCH
};


static RCCResult start(RCCWorker *self)
{
  CState *myState = self->memories[0];  
  CannyProperties *p = self->properties;
  if ( myState->init == 0 ) {
    myState->dx = (uint16_t *) malloc( p->width * p->height * sizeof(uint16_t) );
    myState->dy = (uint16_t *) malloc( p->width * p->height * sizeof(uint16_t) );
    myState->buffer =  (char *) malloc( (p->width+2)*(p->height+2) + (p->width+2)*3*sizeof(int) );
    myState->stack = (uchar **) malloc( p->width*p->height*sizeof(uchar) );
  }
  myState->init = 1;
  return RCC_OK;
}


static RCCResult release(RCCWorker *self)
{
  CState *myState = self->memories[0];  
  if ( myState->init ) {
    free( myState->dx );
    free( myState->dy );
    free( myState->stack );
    free( myState->buffer );
  }
  return RCC_OK;
}

// Compute one line of output
static void
doFrame( CState *myState,
	int H, int W, Pixel *srcdata, Pixel *dstdata,
        double low_thresh, double high_thresh) {

  // sanity checks
  if(low_thresh < 1e-9)
    low_thresh = 10;
  if(high_thresh < 1e-9)
    high_thresh = 100;
  if(low_thresh > high_thresh) {
    double temp = high_thresh;
    high_thresh = low_thresh;
    low_thresh = temp;
  }

  int i, j;
  int mapstep;//, maxsize;

#define ind(i,j) ((i)*W+(j))

  for(i = 1; i + 1 < H; i++) {
    for(j = 1; j + 1 < W; j++) {
      myState->dx[ind(i, j)] = srcdata[ind(i-1, j+1)] - srcdata[ind(i-1, j-1)]
                  + 2 * ( srcdata[ind(i, j+1)] - srcdata[ind(i, j-1)] )
                  + srcdata[ind(i+1, j+1)] - srcdata[ind(i+1, j-1)];
      myState->dy[ind(i, j)] = srcdata[ind(i+1, j-1)] - srcdata[ind(i-1, j-1)]
                  + 2 * ( srcdata[ind(i+1, j)] - srcdata[ind(i-1, j)] )
                  + srcdata[ind(i+1, j+1)] - srcdata[ind(i-1, j+1)];
    }
  }
  // fill in edges
  for(i = 0; i < H; i++)
    myState->dx[ind(i, 0)] = myState->dx[ind(i, W-1)] = myState->dy[ind(i, 0)] = myState->dy[ind(i, W-1)] = 0;
  for(j = 0; j < W; j++)
    myState->dx[ind(0, j)] = myState->dx[ind(H-1, j)] = myState->dy[ind(0, j)] = myState->dy[ind(H-1, j)] = 0;

#undef ind

  // setting thresholds
  int low, high;
  low = (int) low_thresh;
  high = (int) high_thresh;

  uchar* map;
  int* mag_buf[3];
  mag_buf[0] = (int*)myState->buffer;
  mag_buf[1] = mag_buf[0] + W + 2;
  mag_buf[2] = mag_buf[1] + W + 2;
  map = (uchar*)(mag_buf[2] + W + 2);
  mapstep = W + 2;

  // allocate stack directly
  uchar **stack_top = 0, **stack_bottom = 0;

  // maxsize = H * W;
  stack_top = stack_bottom = &myState->stack[0];

  memset( mag_buf[0], 0, (W+2)*sizeof(int) );
  memset( map, 1, mapstep );
  memset( map + mapstep*(H + 1), 1, mapstep );

  /* sector numbers 
     (Top-Left Origin)

     1   2   3
   *  *  * 
   * * *  
   0*******0
   * * *  
   *  *  * 
   3   2   1
   */

#define CANNY_PUSH(d)    *(d) = (uchar)2, *stack_top++ = (d)
#define CANNY_POP(d)     (d) = *--stack_top


  // calculate magnitude and angle of gradient, perform non-maxima supression.
  // fill the map with one of the following values:
  //   0 - the pixel might belong to an edge
  //   1 - the pixel can not belong to an edge
  //   2 - the pixel does belong to an edge
  for( i = 0; i <= H; i++ )
  {
    int* _mag = mag_buf[(i > 0) + 1] + 1;
    const short* _dx = (short*)(myState->dx + W*i);
    const short* _dy = (short*)(myState->dy + W*i);
    uchar* _map;
    int x, y;
    int magstep1, magstep2;
    int prev_flag = 0;

    if( i < H )
    {
      _mag[-1] = _mag[W] = 0;

      // Use L1 norm
      for( j = 0; j < W; j++ )
        _mag[j] = abs(_dx[j]) + abs(_dy[j]);
    }
    else
      memset( _mag-1, 0, (W + 2)*sizeof(int) );

    // at the very beginning we do not have a complete ring
    // buffer of 3 magnitude rows for non-maxima suppression
    if( i == 0 )
      continue;

    _map = map + mapstep*i + 1;
    _map[-1] = _map[W] = 1;

    _mag = mag_buf[1] + 1; // take the central row
    _dx = (short*)(myState->dx + W*(i-1));
    _dy = (short*)(myState->dy + W*(i-1));

    magstep1 = (int)(mag_buf[2] - mag_buf[1]);
    magstep2 = (int)(mag_buf[0] - mag_buf[1]);

    for( j = 0; j < W; j++ )
    {
#define CANNY_SHIFT 15
#define TG22  (int)(0.4142135623730950488016887242097*(1<<CANNY_SHIFT) + 0.5)

      x = _dx[j];
      y = _dy[j];
      int s = x ^ y;
      int m = _mag[j];

      x = abs(x);
      y = abs(y);
      if( m > low )
      {
        int tg22x = x * TG22;
        int tg67x = tg22x + ((x + x) << CANNY_SHIFT);

        y <<= CANNY_SHIFT;

        if( y < tg22x )
        {
          if( m > _mag[j-1] && m >= _mag[j+1] )
          {
            if( m > high && !prev_flag && _map[j-mapstep] != 2 )
            {
              CANNY_PUSH( _map + j );
              prev_flag = 1;
            }
            else
              _map[j] = (uchar)0;
            continue;
          }
        }
        else if( y > tg67x )
        {
          if( m > _mag[j+magstep2] && m >= _mag[j+magstep1] )
          {
            if( m > high && !prev_flag && _map[j-mapstep] != 2 )
            {
              CANNY_PUSH( _map + j );
              prev_flag = 1;
            }
            else
              _map[j] = (uchar)0;
            continue;
          }
        }
        else
        {
          s = s < 0 ? -1 : 1;
          if( m > _mag[j+magstep2-s] && m > _mag[j+magstep1+s] )
          {
            if( m > high && !prev_flag && _map[j-mapstep] != 2 )
            {
              CANNY_PUSH( _map + j );
              prev_flag = 1;
            }
            else
              _map[j] = (uchar)0;
            continue;
          }
        }
      }
      prev_flag = 0;
      _map[j] = (uchar)1;
    }

    // scroll the ring buffer
    _mag = mag_buf[0];
    mag_buf[0] = mag_buf[1];
    mag_buf[1] = mag_buf[2];
    mag_buf[2] = _mag;
  }

  // now track the edges (hysteresis thresholding)
  while( stack_top > stack_bottom )
  {
    uchar* m;

    CANNY_POP(m);

    if( !m[-1] )
      CANNY_PUSH( m - 1 );
    if( !m[1] )
      CANNY_PUSH( m + 1 );
    if( !m[-mapstep-1] )
      CANNY_PUSH( m - mapstep - 1 );
    if( !m[-mapstep] )
      CANNY_PUSH( m - mapstep );
    if( !m[-mapstep+1] )
      CANNY_PUSH( m - mapstep + 1 );
    if( !m[mapstep-1] )
      CANNY_PUSH( m + mapstep - 1 );
    if( !m[mapstep] )
      CANNY_PUSH( m + mapstep );
    if( !m[mapstep+1] )
      CANNY_PUSH( m + mapstep + 1 );
  }

  // the final pass, form the final image
  for( i = 0; i < H; i++ )
  {
    const uchar* _map = map + mapstep*(i+1) + 1;
    uchar* _dst = dstdata + i*W;

    for( j = 0; j < W; j++ )
      _dst[j] = (uchar)-(_map[j] >> 1);
  }

}

/*
 * Methods to implement for worker canny, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  CState *myState = self->memories[0];  
  CannyProperties *p = self->properties;
  RCCPort *in = &self->ports[CANNY_IN],
          *out = &self->ports[CANNY_OUT];

  if ( (in->input.length>0) && (in->input.length>FRAME_BYTES) ) {
    return RCC_ERROR;
  }

  // Run Canny edge detection
  doFrame(myState,
	  p->height, p->width,
          in->current.data,
          out->current.data,
          p->low_thresh, p->high_thresh);
  out->output.u.operation = in->input.u.operation;
  out->output.length = FRAME_BYTES;
  return RCC_ADVANCE;
}
