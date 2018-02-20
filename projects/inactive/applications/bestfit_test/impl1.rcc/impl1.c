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
#include <math.h>
#include "impl1_Worker.h"

IMPL1_METHOD_DECLARATIONS;
RCCDispatch impl1 = {
  /* insert any custom initializations here */
  IMPL1_DISPATCH
};

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *out = &self->ports[IMPL1_OUT];
  unsigned int n;
  printf("IMPL 1 selected\n");
  uint16_t *data = (uint16_t*)out->current.data;
  double freq=6;
  double gain=100;
  double phase= 0;
  double bias=0;
  double noise=0;
  double PI=acos(-1);
  double interval = (2*PI)/(360/freq);

  for ( n=0; n<out->current.maxLength/2; n++) {
    data[n] = (gain*sin( ( interval * n )  + phase)+ (rand()*(noise/50*(gain/20))))+bias;    
  }

  return RCC_ADVANCE;
}
