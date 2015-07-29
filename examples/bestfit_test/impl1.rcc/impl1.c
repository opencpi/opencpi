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
