#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal_utils.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Aug  6 05:50:19 2012 PDT
 * BASED ON THE FILE: fsk_mod_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: fsk_mod_complex
 */
#include "fsk_mod_complex_Worker.h"

typedef struct {
  double accum;
} State;
static uint32_t sizes[] = {sizeof(State), 0 };

FSK_MOD_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch fsk_mod_complex = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  FSK_MOD_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker fsk_mod_complex, based on metadata.
 */

static void
doFsk( RCCWorker * self )
{
  State *myState = self->memories[0];  

  RCCPort
    *in = &self->ports[FSK_MOD_COMPLEX_IN],
    *out = &self->ports[FSK_MOD_COMPLEX_OUT];
 
  Fsk_mod_complexInData *inData = in->current.data;
  Fsk_mod_complexOutIq  *outData = out->current.data;

  unsigned len = byteLen2Real(in->input.length);
  unsigned int i;
  unsigned rval, ival;
  for ( i=0; i<len; i++ ) {
    myState->accum += Uscale( inData->real.data[i] );
    {
      const double
	pi_2 = 0x3fff,
	one  = 0x3fff;
      int16_t rdiv, idiv, angle;
      if (myState->accum > pi_2) {
	rval = 0;
	ival = one;
	angle = pi_2;
      } else if (myState->accum < -pi_2) {
	rval = 0;
	ival = -one;
	angle = -pi_2;
      } else {
	rval = one;
	ival = 0;
	angle = 0;
      }

      unsigned count;
      for ( count = 0; count < 12; count++) {
	static int16_t angles[] = {
	  0x2000, 0x12e4, 0x09fb, 0x0511, 0x028b, 0x0146, 0x00a3, 0x0051, 0x0029, 0x0014, 0x000a, 0x0005
	};
	int16_t mask = -1 << (16 - count);
	rdiv = (rdiv & mask) | ((rval >> count) & ~mask);
	idiv = (idiv & mask) | ((ival >> count) & ~mask);
	if (myState->accum > angle) {
	  angle -= angles[count];
	  rval += idiv;
	  ival -= rdiv;
	} else {
	  angle += angles[count];
	  rval -= idiv;
	  ival += rdiv;
	}
      }
      if (myState->accum) {
	outData->data.data[i].I = Scale( one );
	outData->data.data[i].Q = 0;
      } else {
	outData->data.data[i].I = Scale( rval );
	outData->data.data[i].Q = Scale( ival );
      }
    }
  }

}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

  RCCPort
    *in = &self->ports[FSK_MOD_COMPLEX_IN],
    *out = &self->ports[FSK_MOD_COMPLEX_OUT];

  uint16_t
    *inData = in->current.data,
    *outData = out->current.data;

  switch( in->input.u.operation ) {

  case FSK_MOD_COMPLEX_IN_DATA:
    doFsk( self  );
    break;

  case FSK_MOD_COMPLEX_IN_SYNC:
  case FSK_MOD_COMPLEX_IN_TIME:
    memcpy( outData, inData, in->input.length);
    out->output.length = in->input.length;
    out->output.u.operation = in->input.u.operation;
    break;

  };

  return RCC_ADVANCE;
}
