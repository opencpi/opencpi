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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue April 26 01:56:18 2011 EDT
 * BASED ON THE FILE: good_features_to_track.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: good_features_to_track
 */
#include "good_features_to_track_Worker.h"
#include <string.h>
#include <stdlib.h>

#define FRAME_BYTES (p->height * p->width * sizeof(float)) // pixels per frame

typedef struct {
  uint16_t init;
  float *tmp;
  float **tmpCorners;
  float *gridx, *gridy;
  int *gridSize;
} GFState;

static uint32_t sizes[] = {sizeof(GFState), 0 };

GOOD_FEATURES_TO_TRACK_METHOD_DECLARATIONS;
RCCDispatch good_features_to_track = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  GOOD_FEATURES_TO_TRACK_DISPATCH
};

static RCCResult start(RCCWorker *self)
{
  GFState *myState = self->memories[0];  
  Good_features_to_trackProperties *p = self->properties;
  if ( myState->init == 0 ) {
    myState->tmp = (float *) malloc(p->height * p->width * sizeof(float));    
    myState->tmpCorners = malloc((p->height-1)*(p->width-1)*sizeof(float *));
    myState->gridx = malloc(p->height * p->width * sizeof(float));
    myState->gridy = malloc(p->height * p->width * sizeof(float));
    int minDistance = p->min_distance;
    if(minDistance < 0)
      minDistance = 7.0;
    const int cell_size = (int) (minDistance + 0.5);
    const int grid_width = (p->width + cell_size - 1) / cell_size;
    const int grid_height = (p->height + cell_size - 1) / cell_size;
    myState->gridSize = malloc(grid_height * grid_width * sizeof(int));
  }
  myState->init = 1;
  return RCC_OK;
}

static RCCResult release(RCCWorker *self)
{
  GFState *myState = self->memories[0];  
  // more cleanup
  if ( myState->init ) {
    free( myState->tmp );
    free( myState->gridx );
    free( myState->gridy );
    free( myState->tmpCorners);
    free( myState->gridSize );    
  }
  return RCC_OK;
}

// properties (and defaults)
// simple routines: OpenCV and STL substitutes
#define zmin(x, y) ((x) < (y) ? (x) : (y))
#define zmax(x, y) ((x) > (y) ? (x) : (y))

int greaterThanFloatPtr(const void *a, const void *b) {
  float x = **(float **)a;
  float y = **(float **)b;
  if(x > y) return -1;
  if(x < y) return 1;
  return 0;
}

// main loop
static size_t calc_features( GFState *myState, int H, int W, float *src, float *dst,
            int maxCorners, double qualityLevel, double minDistance)
{

  if(maxCorners < 0)
    maxCorners = 100;
  if(qualityLevel < 0)
    qualityLevel = 0.03;
  if(minDistance < 0)
    minDistance = 7.0;

  float *eig = src;

  int x, y;
  double maxVal = 0;
  // minMaxLoc( eig, 0, &maxVal, 0, 0, mask );
  for( y = 0; y < H; y++ ) {
    for( x = 0; x < W; x++ ) {
      float val = eig[y*W+x]; // eig.at<float>(y, x);
      if( val > maxVal )
        maxVal = val;
    }
  }

  double threshold = maxVal * qualityLevel;
  for( y = 0; y < H; y++ ) {
    for( x = 0; x < W; x++ ) {
      float val = eig[y*W+x];
      if(val < threshold)
        eig[y*W+x] = 0;
    }
  }

  for( y = 1; y < H - 1; y++ ) {
    for( x = 1; x < W - 1; x++ ) {
      float val = eig[y*W+x];
      int dx, dy;
      for( dy = -1; dy <= 1; dy++ )
        for( dx = -1; dx <= 1; dx++ )
          val = zmax(eig[(y+dy)*W+x+dx], val);

      myState->tmp[y*W+x] = val;
    }
    // border
    myState->tmp[y*W] = eig[y*W];
    myState->tmp[y*W+W-1] = eig[y*W+W-1];
  }
  // border
  for( x = 0; x < W; x++ ) {
    myState->tmp[x] = eig[x];
    myState->tmp[(H-1)*W+x] = eig[(H-1)*W+x];
  }

  size_t tmpCornersSize = 0;

  // collect list of pointers to features - put them into temporary image
  for( y = 1; y < H - 1; y++ )
  {
    float* eig_data = eig + y*W;
    const float* tmp_data = (const float*) (myState->tmp + y*W);

    for( x = 1; x < W - 1; x++ )
    {
      float val = eig_data[x];
      if( val != 0 && val == tmp_data[x])
        myState->tmpCorners[tmpCornersSize++] = eig_data + x;
    }
  }

  size_t i, j, total = tmpCornersSize, ncorners = 0;

  // sort features
  qsort( myState->tmpCorners, total, sizeof(float *), greaterThanFloatPtr );

  // Partition the image into larger grids
  const int cell_size = (int) (minDistance + 0.5);
  const int grid_width = (W + cell_size - 1) / cell_size;
  const int grid_height = (H + cell_size - 1) / cell_size;
  for( i = 0; i < (size_t)(grid_height * grid_width); i++ )
    myState->gridSize[i] = 0;

  minDistance *= minDistance;

  for( i = 0; i < total; i++ )
  {
    int ofs = (int)(myState->tmpCorners[i] - eig);
    y = (int)(ofs / W);
    x = (int)(ofs - y*W);

    size_t good = 1;

    int x_cell = x / cell_size;
    int y_cell = y / cell_size;

    int x1 = x_cell - 1;
    int y1 = y_cell - 1;
    int x2 = x_cell + 1;
    int y2 = y_cell + 1;

    // boundary check
    x1 = zmax(0, x1);
    y1 = zmax(0, y1);
    x2 = zmin(grid_width-1, x2);
    y2 = zmin(grid_width-1, y2);

    int xx, yy;
    int hw = H * W / (grid_height * grid_width);
    for( yy = y1; yy <= y2; yy++ )
    {
      for( xx = x1; xx <= x2; xx++ )
      {   
        int ind = yy*grid_width + xx;

        if( myState->gridSize[ind] )
        {
          for(j = 0; j < (size_t)myState->gridSize[ind]; j++)
          {
            float dx = x - myState->gridx[ind*hw + j];
            float dy = y - myState->gridy[ind*hw + j];

            if( dx*dx + dy*dy < minDistance )
            {
              good = 0; // false
              goto break_out;
            }
          }
        }                
      }
    }

break_out:

    if(good)
    {
      int ind = y_cell*grid_width + x_cell;
      myState->gridx[ind*hw + myState->gridSize[ind]] = (float) x;
      myState->gridy[ind*hw + myState->gridSize[ind]] = (float) y;
      myState->gridSize[ind]++;

      dst[2*ncorners] = x;
      dst[2*ncorners+1] = y;
      ncorners++;

        if( maxCorners > 0 && (int)ncorners == maxCorners )
          break;
    }
  }

  return ncorners;
}

/*
 * Methods to implement for worker good_features_to_track, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  GFState *myState = self->memories[0];  
  Good_features_to_trackProperties *p = self->properties;
  RCCPort *in = &self->ports[GOOD_FEATURES_TO_TRACK_IN],
          *out = &self->ports[GOOD_FEATURES_TO_TRACK_OUT];

  if ( (in->input.length>0) && (in->input.length>FRAME_BYTES) ) {
    return RCC_ERROR;
  }
  size_t ncorners = calc_features( myState, 
				   p->height, p->width,
                    (float *) in->current.data,
                    (float *) out->current.data,
                    p->max_corners,
                    p->quality_level,
                    p->min_distance );
  out->output.u.operation = in->input.u.operation;
  out->output.length = 2 * ncorners * sizeof(float);
  return RCC_ADVANCE;
}
