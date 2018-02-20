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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue April 26 01:55:32 2011 EDT
 * BASED ON THE FILE: min_eigen_val.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: min_eigen_val
 */
#include "min_eigen_val_Worker.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#define FRAME_BYTES (p->height * p->width * sizeof(float)) // pixels per frame

MIN_EIGEN_VAL_METHOD_DECLARATIONS;
RCCDispatch min_eigen_val = {
  /* insert any custom initializations here */
  MIN_EIGEN_VAL_DISPATCH
};

static void
calc_min_eigen_val( int H, int W, const float* _cov, float* _dst )
{
    int i, j;

    for( i = 0; i < H; i++ )
    {
        const float* cov = (const float*) (_cov + i*W*3);
        float* dst = (float*) (_dst + i*W);
        for( j = 0; j < W; j++ )
        {
            double a = cov[j*3]*0.5;
            double b = cov[j*3+1];
            double c = cov[j*3+2]*0.5;
            dst[j] = (float)((a + c) - sqrt((a - c)*(a - c) + b*b));
        }
    }
}


/*
 * Methods to implement for worker min_eigen_val, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  ( void ) timedOut;
  ( void ) newRunCondition;
  Min_eigen_valProperties *p = self->properties;
  RCCPort *in = &self->ports[MIN_EIGEN_VAL_IN],
          *out = &self->ports[MIN_EIGEN_VAL_OUT];

  calc_min_eigen_val( p->height, p->width,
                    (float *) in->current.data,
                    (float *) out->current.data);

  out->output.u.operation = in->input.u.operation;
  out->output.length = p->height * p->width * sizeof(float);

  return RCC_ADVANCE;
}
