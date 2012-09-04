#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Aug  6 05:48:41 2012 PDT
 * BASED ON THE FILE: fm_demod_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: fm_demod_complex
 */
#include "fm_demod_complex_Worker.h"

FM_DEMOD_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch fm_demod_complex = {
 /* insert any custom initializations here */
 FM_DEMOD_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker fm_demod_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[FM_DEMOD_COMPLEX_IN],
   *out = &self->ports[FM_DEMOD_COMPLEX_OUT];
   

 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }

 switch( in->input.u.operation ) {

 case FM_DEMOD_COMPLEX_IN_IQ:
   //   processSignalData( self  );

 case FM_DEMOD_COMPLEX_IN_SYNC:
   //   processSyncSignal( self  );

 case FM_DEMOD_COMPLEX_IN_TIME:
   //   processTimeSignal( self );
   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   out->output.u.operation = in->input.u.operation;
   break;
   
 };

 return RCC_ADVANCE;
}
