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


static void
processSignalData( RCCWorker * self )
{
  Noise_gen_complexProperties *p = self->properties;

  RCCPort
    *in = &self->ports[NOISE_GEN_COMPLEX_IN],
    *out = &self->ports[NOISE_GEN_COMPLEX_OUT];
 
  Noise_gen_complexInIq    *inData = in->current.data;
  Noise_gen_complexOutIq    *outData = out->current.data;
  out->output.length = in->input.length;
  out->output.u.operation = in->input.u.operation;
  

}



static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[NOISE_GEN_COMPLEX_IN],
   *out = &self->ports[NOISE_GEN_COMPLEX_OUT];

 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }

 switch( in->input.u.operation ) {

 case NOISE_GEN_COMPLEX_IN_IQ:
   processSignalData( self  );

 case NOISE_GEN_COMPLEX_IN_SYNC:
 case NOISE_GEN_COMPLEX_IN_TIME:
   self->container.send( &self->ports[NOISE_GEN_COMPLEX_OUT], 
			 &self->ports[NOISE_GEN_COMPLEX_IN].current, in->input.u.operation, 
			 in->input.length );
   return RCC_OK;

 };

 return RCC_ADVANCE;
}
