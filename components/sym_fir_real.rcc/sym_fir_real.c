
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
#include <signal_utils.h>

#define NTAPS 256
#define UnRoll 4

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
  for ( i=0; i<NTAPS/2; i++ ) {
    myState->taps[i] = (double)p->taps[i];
  }
  for ( j=i; i>=0; j++,i-- ) {
    myState->taps[j] = (double)p->taps[i];
  }  
  return RCC_OK;
}




static double
apply_filter( State * myState,  Sym_fir_realInData * input )
{
  unsigned	i = 0;
  double	
    acum0 = 0,
    acum1 = 0,
    acum2 = 0,
    acum3 = 0;
  unsigned	n = (NTAPS / UnRoll) * UnRoll;
  for (i = 0; i < n; i += UnRoll){
    acum0 += myState->taps[i + 0] * Uscale( input->real.data[i + 0] );
    acum1 += myState->taps[i + 1] * Uscale( input->real.data[i + 1] );
    acum2 += myState->taps[i + 2] * Uscale( input->real.data[i + 2] );
    acum3 += myState->taps[i + 3] * Uscale( input->real.data[i + 3] );
  }
  for (; i < NTAPS; i++)
    acum0 += myState->taps[i] * Uscale( input->real.data[i] );
  return (acum0 + acum1 + acum2 + acum3);
}


static void
processSignalData( RCCWorker * self )
{
  State *myState = self->memories[0];  
  Sym_fir_realProperties *p = self->properties;

  RCCPort
    *in = &self->ports[SYM_FIR_REAL_IN],
    *out = &self->ports[SYM_FIR_REAL_OUT];
 
  Sym_fir_realInData    *inData = in->current.data;
  Sym_fir_realOutData   *outData = out->current.data;
  out->output.length = in->input.length;
  out->output.u.operation = SYM_FIR_REAL_OUT_DATA;
  
  if ( p->bypass ) {
    memcpy(inData,outData,in->input.length);
    return;
  }

  unsigned i;
  double gain = Gain( p->gain);
  unsigned len = byteLen2Real(in->input.length) - UnRoll;
  for ( i=0; i<len; i++ ) {
    double v = apply_filter( myState, inData );
    if ( fabs(v) > p->peakDetect ) {
      p->peakDetect = Scale( fabs(v) );
    }
    outData->real.data[i] =  Scale( gain * v );
  }
 
}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;  

  RCCPort
    *in = &self->ports[SYM_FIR_REAL_IN],
    *out = &self->ports[SYM_FIR_REAL_OUT];

  uint16_t
    *inData = in->current.data,
    *outData = out->current.data;

  if (in->input.length > out->current.maxLength) {
    self->errorString = "output buffer too small";
    return RCC_ERROR;
  }

  switch( in->input.u.operation ) {

  case SYM_FIR_REAL_IN_DATA:
    processSignalData( self  );
    break;

  case SYM_FIR_REAL_IN_SYNC:
  case SYM_FIR_REAL_IN_TIME:
    out->output.length = in->input.length;
    out->output.u.operation = in->input.u.operation;
    memcpy( outData, inData, in->input.length);
    break;

  };

  return RCC_ADVANCE;
}
