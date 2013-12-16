/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Dec 22 11:48:34 2011 EST
 * BASED ON THE FILE: bias.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: bias
 */
#include "bias_Worker.h"

BIAS_METHOD_DECLARATIONS;
RCCDispatch bias = {
 /* insert any custom initializations here */
 BIAS_DISPATCH
};

/*
 * Methods to implement for worker bias, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)timedOut;(void)newRunCondition;
 RCCPort
   *in = &self->ports[BIAS_IN],
   *out = &self->ports[BIAS_OUT];
 BiasProperties *props = self->properties;
 uint32_t
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }
 if (in->input.length % sizeof(uint32_t)) {
   self->errorString = "input message size not a multiple of data type";
   return RCC_ERROR;
 }
 for (unsigned n = in->input.length / sizeof(uint32_t); n; n--)
   *outData++ = *inData++ + props->biasValue;
 out->output.length = in->input.length;
 out->output.u.operation = in->input.u.operation;
 return in->input.length ? RCC_ADVANCE : RCC_ADVANCE_DONE;
}
