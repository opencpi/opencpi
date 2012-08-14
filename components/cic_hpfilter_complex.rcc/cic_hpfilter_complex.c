#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Aug  5 14:08:12 2012 PDT
 * BASED ON THE FILE: cic_hpfilter_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: cic_hpfilter_complex
 */
#include "cic_hpfilter_complex_Worker.h"

CIC_HPFILTER_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch cic_hpfilter_complex = {
 /* insert any custom initializations here */
 CIC_HPFILTER_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker cic_hpfilter_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[CIC_HPFILTER_COMPLEX_IN],
   *out = &self->ports[CIC_HPFILTER_COMPLEX_OUT];
   

 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 switch( in->input.u.operation ) {

 case CIC_HPFILTER_COMPLEX_IN_MESSAGE:

   {
     if (in->input.length > out->current.maxLength) {
       self->errorString = "output buffer too small";
       return RCC_ERROR;
     }
     printf("In cic_hpfilter_complex  got data = %s, len = %d\n", inData, in->input.length );
     memcpy( outData, inData, in->input.length);
     out->output.length = in->input.length;
     out->output.u.operation = in->input.u.operation;
   }
   break;


 case CIC_HPFILTER_COMPLEX_IN_IQ:
   //   processSignalData( self  );


 case CIC_HPFILTER_COMPLEX_IN_SYNC:
   //   processSyncSignal( self  );


 case CIC_HPFILTER_COMPLEX_IN_TIME:
   //   processTimeSignal( self );
   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   out->output.u.operation = in->input.u.operation;
   break;

 };

 return RCC_ADVANCE;
}
