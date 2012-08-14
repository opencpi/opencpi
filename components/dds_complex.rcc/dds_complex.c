/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 22 10:02:41 2012 EDT
 * BASED ON THE FILE: dds_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: dds_complex
 */
#include "dds_complex_Worker.h"

DDS_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch dds_complex = {
 /* insert any custom initializations here */
 DDS_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker dds_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;


 RCCPort
   *in = &self->ports[DDS_COMPLEX_IN],
   *out = &self->ports[DDS_COMPLEX_OUT];

 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 printf("In dds_complex.c got data = %s\n", inData);

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }



 return RCC_ADVANCE;
}
