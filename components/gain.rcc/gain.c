/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Dec 22 11:48:34 2011 EST
 * BASED ON THE FILE: bias.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: bias
 */
#include "gain_Worker.h"

GAIN_METHOD_DECLARATIONS;
RCCDispatch gain = {
 /* insert any custom initializations here */
 GAIN_DISPATCH
};

/*
 * Methods to implement for worker gain, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {


  //  fprintf(stderr, "In gain RUN force to 1!!\n");

  (void)timedOut;(void)newRunCondition;
  RCCPort
    *in = &self->ports[GAIN_INPUTPORT],
    *out = &self->ports[GAIN_OUTPUTPORT];
  GainProperties *props = self->properties;
 
 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   fprintf(stderr, "In gain RUN ERROR output buffer too small \n");
   return RCC_ERROR;
 }
 if (in->input.length % sizeof(uint16_t)) {
   self->errorString = "input message size not a multiple of data type";
   fprintf(stderr, "In gain RUN ERROR size not a multiple of data type  \n");
   return RCC_ERROR;
 }
 for (unsigned n = in->input.length / sizeof(uint16_t); n; n--)
   *outData++ = *inData++ * props->gainValue;

  // fprintf(stderr, "In gain, setting len to %d  \n", in->input.length );

 out->output.length = in->input.length;
 out->output.u.operation = in->input.u.operation;
 return RCC_ADVANCE;

 // return in->input.length ? RCC_ADVANCE : RCC_ADVANCE_DONE;
}
