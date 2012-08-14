#include <stdio.h>
#include <string.h>
#include <stdlib.h>



/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Aug  6 05:50:19 2012 PDT
 * BASED ON THE FILE: fsk_mod_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: fsk_mod_complex
 */
#include "fsk_mod_complex_Worker.h"

FSK_MOD_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch fsk_mod_complex = {
 /* insert any custom initializations here */
 FSK_MOD_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker fsk_mod_complex, based on metadata.
 */


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

 case FSK_MOD_COMPLEX_IN_MESSAGE:

   {
     if (in->input.length > out->current.maxLength) {
       self->errorString = "output buffer too small";
       return RCC_ERROR;
     }
     printf("In fsk_mod_complex  got data = %s, len = %d\n", inData, in->input.length );
     memcpy( outData, inData, in->input.length);
     out->output.length = in->input.length;
     out->output.u.operation = in->input.u.operation;
   }
   break;


 case FSK_MOD_COMPLEX_IN_DATA:
   //   processSignalData( self  );

 case FSK_MOD_COMPLEX_IN_SYNC:
   //   processSyncSignal( self  );

 case FSK_MOD_COMPLEX_IN_TIME:
   //   processTimeSignal( self );

   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   out->output.u.operation = in->input.u.operation;
   break;

 };

 return RCC_ADVANCE;
}
