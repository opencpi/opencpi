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
#include "impl2_Worker.h"
#include <math.h>

#define S_SIZE 15
typedef struct {
  double wave[S_SIZE];
} MyState;

static size_t sizes[] = {sizeof(MyState), 0};
#define PI 3.14159265

IMPL2_METHOD_DECLARATIONS;
RCCDispatch impl2 = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  IMPL2_DISPATCH
};

static RCCResult initialize ( RCCWorker* self )
{
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
  RCCPort *out = &self->ports[IMPL2_OUT];
  // Impl2Properties* p = ( Impl2Properties* ) self->properties;
  MyState *s = self->memories[0];
  printf("IMPL 2 selected\n");
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

