#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Aug 16 09:34:46 2012 PDT
 * BASED ON THE FILE: noise_gen_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: noise_gen_complex
 */
#include "noise_gen_complex_Worker.h"

NOISE_GEN_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch noise_gen_complex = {
 /* insert any custom initializations here */
 NOISE_GEN_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker noise_gen_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[NOISE_GEN_COMPLEX_IN],
   *out = &self->ports[NOISE_GEN_COMPLEX_OUT];


 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 switch( in->input.u.operation ) {

 case NOISE_GEN_COMPLEX_IN_MESSAGE:

   {
     if (in->input.length > out->current.maxLength) {
       self->errorString = "output buffer too small";
       return RCC_ERROR;
     }
     printf("In noise_gen_complex  got data = %s, len = %d\n", inData, in->input.length );
     memcpy( outData, inData, in->input.length);
     out->output.length = in->input.length;
     out->output.u.operation = in->input.u.operation;
   }
   break;

 case NOISE_GEN_COMPLEX_IN_IQ:
   //   processSignalData( self  );

 case NOISE_GEN_COMPLEX_IN_SYNC:
   //   processSyncSignal( self  );

 case NOISE_GEN_COMPLEX_IN_TIME:
   //   processTimeSignal( self );
   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   out->output.u.operation = in->input.u.operation;
   break;

 };

 return RCC_ADVANCE;
}
