#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Aug  6 05:56:05 2012 PDT
 * BASED ON THE FILE: loopback_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: loopback_complex
 */
#include "loopback_complex_Worker.h"

LOOPBACK_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch loopback_complex = {
 /* insert any custom initializations here */
 LOOPBACK_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker loopback_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[LOOPBACK_COMPLEX_IN],
   *out = &self->ports[LOOPBACK_COMPLEX_OUT];
   

 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 switch( in->input.u.operation ) {


 case LOOPBACK_COMPLEX_IN_MESSAGE:

   {
     if (in->input.length > out->current.maxLength) {
       self->errorString = "output buffer too small";
       return RCC_ERROR;
     }
     printf("In loopback_complex  got data = %s, len = %d\n", inData, in->input.length );
     memcpy( outData, inData, in->input.length);
     out->output.length = in->input.length;
     out->output.u.operation = in->input.u.operation;
   }
   break;
   
 case LOOPBACK_COMPLEX_IN_IQ:
   //   processSignalData( self  );

 case LOOPBACK_COMPLEX_IN_SYNC:
   //   processSyncSignal( self  );

 case LOOPBACK_COMPLEX_IN_TIME:
   //   processTimeSignal( self );
   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   out->output.u.operation = in->input.u.operation;
   break;

 };

 return RCC_ADVANCE;
}
