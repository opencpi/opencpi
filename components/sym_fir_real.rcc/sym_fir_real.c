
/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Aug  5 14:13:52 2012 PDT
 * BASED ON THE FILE: sym_fir_real.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: sym_fir_real
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sym_fir_real_Worker.h"
#include "signal_utils.h"
#include "sym_fir.h"


#define NTAPS (mems(Sym_fir_realProperties,taps)/sizeof(int16_t))

typedef struct {
  double   taps[NTAPS];
} State;
static uint32_t sizes[] = {sizeof(State), 0 };



SYM_FIR_REAL_METHOD_DECLARATIONS;
RCCDispatch sym_fir_real = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  SYM_FIR_REAL_DISPATCH
};

/*
 * Methods to implement for worker sym_fir_real, based on metadata.
 */

/*
 * Methods to implement for worker dds_complex, based on metadata.
 */
static RCCResult start(RCCWorker *self )
{
  int i,j;
  Sym_fir_realProperties *p = self->properties;
  State *myState = self->memories[0];  
  for ( i=0; i<(int)NTAPS/2; i++ ) {
    myState->taps[i] = (double)p->taps[i];
  }
  for ( j=i; i>=0; j++,i-- ) {
    myState->taps[j] = (double)p->taps[i];
  }  
  return RCC_OK;
}



static double
apply_filter( double taps[],  Sym_fir_realInData * input )
{
  unsigned	i = 0;
  double	
    acum0 = 0,
    acum1 = 0,
    acum2 = 0,
    acum3 = 0;
  unsigned	n = (NTAPS / UnRoll) * UnRoll;
  for (i = 0; i < n; i += UnRoll){
    acum0 += taps[i + 0] * Uscale( input->real[i + 0] );
    acum1 += taps[i + 1] * Uscale( input->real[i + 1] );
    acum2 += taps[i + 2] * Uscale( input->real[i + 2] );
    acum3 += taps[i + 3] * Uscale( input->real[i + 3] );
  }
  for (; i < NTAPS; i++)
    acum0 += taps[i] * Uscale( input->real[i] );
  return (acum0 + acum1 + acum2 + acum3);
}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;  

  Sym_fir_realProperties *p = self->properties;
  State *myState = self->memories[0];  

  RCCPort
    *in = &self->ports[SYM_FIR_REAL_IN],
    *out = &self->ports[SYM_FIR_REAL_OUT];

  Sym_fir_realInData
    *inData = in->current.data,
    *outData = out->current.data;

  if (in->input.length > out->current.maxLength) {
    self->container.setError( "output buffer too small" );
    return RCC_ERROR;
  }

  switch( in->input.u.operation ) {

  case SYM_FIR_REAL_IN_DATA:
    {
#ifndef NDEBUG
      printf("%s got %d bytes of data\n", __FILE__,  in->input.length);
#endif

      if ( p->bypass ) {
	self->container.send( out, &in->current, in->input.u.operation, in->input.length);
	return RCC_OK;
      }
      else {
	unsigned i;
	double gain = Gain( p->gain);
	unsigned len = byteLen2Real(in->input.length) - UnRoll;
	for ( i=0; i<len; i++ ) {
	  double v = apply_filter( myState->taps, inData );
	  if ( fabs(v) > p->peakDetect ) {
	    p->peakDetect = Scale( fabs(v) );
	  }
	  outData->real[i] =  Scale( gain * v );
	}
      }
    }
    break;

  case SYM_FIR_REAL_IN_SYNC:
  case SYM_FIR_REAL_IN_TIME:
    self->container.send( out, &in->current, in->input.u.operation, in->input.length);
    return RCC_OK;
    break;

  };

  out->output.length = in->input.length;
  out->output.u.operation = in->input.u.operation;
  return RCC_ADVANCE;
}
