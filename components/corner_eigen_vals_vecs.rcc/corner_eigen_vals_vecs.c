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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 10 01:54:59 2011 EDT
 * BASED ON THE FILE: corner_eigen_vals_vecs.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: corner_eigen_vals_vecs
 */
#include "corner_eigen_vals_vecs_Worker.h"

#include <stdlib.h>
#include <string.h>

CORNER_EIGEN_VALS_VECS_METHOD_DECLARATIONS;
RCCDispatch corner_eigen_vals_vecs = {
  /* insert any custom initializations here */
  CORNER_EIGEN_VALS_VECS_DISPATCH
};

static void
calc_corner_eigen_vals_vecs( int H, int W, char *src,
                     float* cov )
{
    double scale = 1.0 / (255.0 * 12); 

    int i, j;

    // need 32F type
    float *Dx = (float *) malloc(H * W * sizeof(float));
    float *Dy = (float *) malloc(H * W * sizeof(float));

    for( i = 1; i < H - 1; i++ ) {
      uint8_t *l0 = (uint8_t *) (src + (i-1)*W);
      uint8_t *l1 = (uint8_t *) (src + (i)*W);
      uint8_t *l2 = (uint8_t *) (src + (i+1)*W);
      for( j = 1; j < W - 1; j++ ) {
        float dx = l0[j+1] + 2*l1[j+1] + l2[j+1] - l0[j-1] - 2*l1[j-1] - l2[j-1];
        float dy = l2[j-1] + 2*l2[j] + l2[j+1] - l0[j+1] - 2*l0[j] - l0[j-1];

        Dx[i*W+j] = dx * scale;
        Dy[i*W+j] = dy * scale;
      }
      // borders
      Dx[i*W] = Dy[i*W] = 0;
      Dx[i*W+W-1] = Dy[i*W+W-1] = 0;
    }
    for( j = 0; j < W; j++) {
      // borders
      Dx[j] = Dy[j] = 0;
      Dx[(H-1)*W+j] = Dy[(H-1)*W+j] = 0;
    }

    for( i = 0; i < H; i++ )
    {
        // float* cov_data = (float*)(cov.data + i*cov.step);
        float *cov_data = (float *) cov + i*W*3;
        const float* dxdata = (const float*)(Dx + i*W);
        const float* dydata = (const float*)(Dy + i*W);

        for( j = 0; j < W; j++ )
        {
            float dx = dxdata[j];
            float dy = dydata[j];

            cov_data[j*3] = dx*dx;
            cov_data[j*3+1] = dx*dy;
            cov_data[j*3+2] = dy*dy;
        }
    }
    // cleanup
    free( Dx );
    free( Dy );

    // boxFilter(cov, cov, cov.depth(), Size(block_size, block_size),
    //     Point(-1,-1), false, borderType ); // normalize = false
    float *cov_tmp = (float *) malloc(H * W* sizeof(float) * 3);
    for( i = 1; i < H - 1; i++ ) {
      float *l0 = (float *) cov + (i-1)*W*3;
      float *l1 = (float *) cov + i*W*3;
      float *l2 = (float *) cov + (i+1)*W*3;
      float *cov_dst = (float *) cov_tmp + i*W*3;
      int k;
      for( k = 0; k < 3; k++ ) {
        for( j = 1; j < W - 1; j++ ) {
          cov_dst[3*j+k] = l0[3*j+k] + l0[3*(j-1)+k] + l0[3*(j+1)+k]
                         + l1[3*j+k] + l1[3*(j-1)+k] + l1[3*(j+1)+k]
                         + l2[3*j+k] + l2[3*(j-1)+k] + l2[3*(j+1)+k];
        }
      }
    }
    for( i = 1; i < H - 1; i++ ) {
      float *cov_src = (float *) cov_tmp + i*W*3;
      float *cov_dst = (float *) cov + i*W*3;
      for( j = 1; j < W - 1; j++ ) {
        cov_dst[3*j] = cov_src[3*j];
        cov_dst[3*j+1] = cov_src[3*j+1];
        cov_dst[3*j+2] = cov_src[3*j+2];
      }
    }

    // cleanup
    free( cov_tmp );

}


/*
 * Methods to implement for worker corner_eigen_vals_vecs, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  Corner_eigen_vals_vecsProperties *p = self->properties;
  RCCPort *in = &self->ports[CORNER_EIGEN_VALS_VECS_IN],
          *out = &self->ports[CORNER_EIGEN_VALS_VECS_OUT];
  const RCCContainer *c = self->container;

  calc_corner_eigen_vals_vecs( p->height, p->width,
                        (char *) in->current.data,
                        (float *) out->current.data );

  out->output.u.operation = in->input.u.operation;
  out->output.length = p->height * p->width * sizeof(float) * 3;
  return RCC_ADVANCE;
}
