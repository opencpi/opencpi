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

SYM_FIR_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch sym_fir_complex = {
 /* insert any custom initializations here */
 SYM_FIR_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker sym_fir_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[SYM_FIR_COMPLEX_IN],
   *out = &self->ports[SYM_FIR_COMPLEX_OUT];


 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 switch( in->input.u.operation ) {

 case SYM_FIR_COMPLEX_IN_MESSAGE:

   {
     if (in->input.length > out->current.maxLength) {
       self->errorString = "output buffer too small";
       return RCC_ERROR;
     }
     printf("In sym_fir_complex  got data = %s, len = %d\n", inData, in->input.length );
     memcpy( outData, inData, in->input.length);
     out->output.length = in->input.length;
     out->output.u.operation = in->input.u.operation;
   }
   break;

 case SYM_FIR_COMPLEX_IN_IQ:
   //   processSignalData( self  );

 case SYM_FIR_COMPLEX_IN_SYNC:
   //   processSyncSignal( self  );

 case SYM_FIR_COMPLEX_IN_TIME:
   //   processTimeSignal( self );

   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   out->output.u.operation = in->input.u.operation;
   break;

 };

 return RCC_ADVANCE;
}
