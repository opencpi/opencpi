#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Aug  5 14:09:56 2012 PDT
 * BASED ON THE FILE: sym_fir_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: sym_fir_complex
 */
#include "sym_fir_complex_Worker.h"
#include "sym_fir.h"
#include "signal_utils.h"

#define NTAPS (mems(Sym_fir_complexProperties,taps)/sizeof(int16_t))

typedef struct {
  double   taps[NTAPS];
} State;
static size_t sizes[] = {sizeof(State), 0 };


SYM_FIR_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch sym_fir_complex = {
 /* insert any custom initializations here */
  .memSizes = sizes,
 SYM_FIR_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker sym_fir_complex, based on metadata.
 */

/*
 * Methods to implement for worker dds_complex, based on metadata.
 */
static RCCResult start(RCCWorker *self )
{
  int i,j;
  Sym_fir_complexProperties *p = self->properties;
  State *myState = self->memories[0];  
  for ( i=0; i<(int)NTAPS/2; i++ ) {
    myState->taps[i] = (double)p->taps[i];
  }
  for ( j=i; i>0; j++,i-- ) {
    myState->taps[j] = (double)p->taps[i];
  }  
  return RCC_OK;
}



static void
apply_filter( double taps[], struct Sym_fir_complexInIqData * input, double * i, double * q )
{
  unsigned	n;
  double	
    acum0 = 0,
    acum1 = 0,
    acum2 = 0,
    acum3 = 0;
  unsigned	len = (NTAPS / UnRoll) * UnRoll;
  for (n = 0; n < len; n += UnRoll){
    acum0 += taps[n + 0] * Uscale( input[n + 0].I );
    acum1 += taps[n + 1] * Uscale( input[n + 1].I );
    acum2 += taps[n + 2] * Uscale( input[n + 2].I );
    acum3 += taps[n + 3] * Uscale( input[n + 3].I );
  }
  for (; n < NTAPS; n++)
    acum0 += taps[n] * Uscale( input[n].I );
  *i = acum0 + acum1 + acum2 + acum3;

  acum0 = acum1 = acum2 = acum3 = 0;
  for (n = 0; n < NTAPS-4; n += UnRoll){
    acum0 += taps[n + 0] * Uscale( input[n + 0].Q );
    acum1 += taps[n + 1] * Uscale( input[n + 1].Q );
    acum2 += taps[n + 2] * Uscale( input[n + 2].Q );
    acum3 += taps[n + 3] * Uscale( input[n + 3].Q );
  }
  for (; n < NTAPS; n++)
    acum0 += taps[n] * Uscale( input[n].Q );
  *q = acum0 + acum1 + acum2 + acum3;
}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  Sym_fir_complexProperties *p = self->properties;
  State *myState = self->memories[0];  

 RCCPort
   *in = &self->ports[SYM_FIR_COMPLEX_IN],
   *out = &self->ports[SYM_FIR_COMPLEX_OUT];

 Sym_fir_complexInIq
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->container.setError( "output buffer too small" );
   return RCC_ERROR;
 }

 switch( in->input.u.operation ) {

 case SYM_FIR_COMPLEX_IN_IQ:
    {
#ifndef NDEBUG
      printf("%s got %zu bytes of data\n", __FILE__,  in->input.length);
#endif

      if ( p->bypass ) {
	memcpy(inData,outData,in->input.length);
      }
      else if (in->input.length) {
	unsigned n;
	unsigned len = byteLen2Complex(in->input.length) - UnRoll;
	double gain = Gain( p->gain);
	for ( n=0; n<len; n++ ) {
	  double i,q;
	  apply_filter( myState->taps, &inData->data[n], &i, &q );
	  double v = scabs(i,q);
	  if ( v > Uscale( p->peakDetect ) ) {
	    p->peakDetect = Scale( v );
	  }

	  // Not sure if this is correct
	  outData->data[n].I = Scale( i * gain);
	  outData->data[n].Q = Scale( q * gain);	 
	}
      }
    }
    break;

 case SYM_FIR_COMPLEX_IN_SYNC:
 case SYM_FIR_COMPLEX_IN_TIME:
   self->container.send( out, &in->current, in->input.u.operation, in->input.length);
   return RCC_OK;
   break;

 };

 out->output.length = in->input.length;
 out->output.u.operation = in->input.u.operation;
 return RCC_ADVANCE;

}
