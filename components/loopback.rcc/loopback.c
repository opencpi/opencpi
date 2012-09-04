#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Aug  6 05:56:05 2012 PDT
 * BASED ON THE FILE: loopback.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: loopback
 */
#include "loopback_Worker.h"

LOOPBACK_METHOD_DECLARATIONS;
RCCDispatch loopback = {
 /* insert any custom initializations here */
 LOOPBACK_DISPATCH
};

/*
 * Methods to implement for worker loopback, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[LOOPBACK_IN],
   *out = &self->ports[LOOPBACK_OUT];
   
 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   printf("loopback: ERROR: ouput buffer is too small\n");
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }

 switch( in->input.u.operation ) {

 case 3:
   printf("In loopback  got data = %s, len = %d\n", inData, in->input.length );
   
 default:
   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   out->output.u.operation = in->input.u.operation;
   break;

 };

 return RCC_ADVANCE;
}
