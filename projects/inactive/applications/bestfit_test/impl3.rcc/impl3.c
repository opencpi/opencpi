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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "impl3_Worker.h"
#include <math.h>

#define S_SIZE 5
typedef struct {
  double wave[S_SIZE];
} MyState;

static size_t sizes[] = {sizeof(MyState), 0};
#define PI 3.14159265

IMPL3_METHOD_DECLARATIONS;
RCCDispatch impl3 = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  IMPL3_DISPATCH
};

static RCCResult initialize ( RCCWorker* self )
{
  //  Impl3Properties* p = ( Impl3Properties* ) self->properties;
  MyState *s = self->memories[0];
  double interval = 2.0*PI/S_SIZE;
  int n;
  for ( n=0; n<S_SIZE; n++) {
    s->wave[n] = sin( interval * n );
  }
  return RCC_OK;
}

static RCCResult start ( RCCWorker* self )
{
  (void)self;
  return RCC_OK;
}

#define RU(x) (x+.5)
double
sin_wave( double d, MyState *s )
{
  double r=0;
  double i = 2*3.14/S_SIZE;
  int v = (int) RU(d/i)%S_SIZE;
  r = s->wave[v];
  return r;
}

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *out = &self->ports[IMPL3_OUT];
  // Impl3Properties* p = ( Impl3Properties* ) self->properties;
  MyState *s = self->memories[0];
  printf("IMPL 3 selected\n");
  double freq=6;
  double gain=100;
  double phase= 0;
  double bias=0;
  double noise=0;
  double interval = (2*PI)/(360/freq);
  unsigned int n;
  uint16_t *data = (uint16_t*)out->current.data;
  for ( n=0; n<out->current.maxLength/2; n++) {
    data[n] = ( gain*sin_wave(interval*n,s) + phase)+ rand()*(noise/50*(gain/20)) + bias;    
  }
  return RCC_ADVANCE;
}
