/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Jun  7 16:40:56 2014 EDT
 * BASED ON THE FILE: bias_param.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the bias_param worker in C
 */

#include "bias_param_Worker.h"

BIAS_PARAM_METHOD_DECLARATIONS;
RCCDispatch bias_param = {
 /* insert any custom initializations here */
 BIAS_PARAM_DISPATCH
};

/*
 * Methods to implement for worker bias_param, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 RCCPort
   *in = &self->ports[BIAS_PARAM_IN],
   *out = &self->ports[BIAS_PARAM_OUT];
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
   *outData++ = *inData++ + PARAM_biasValue();
 out->output.length = in->input.length;
 out->output.u.operation = in->input.u.operation;
 return in->input.length ? RCC_ADVANCE : RCC_ADVANCE_DONE;
}
