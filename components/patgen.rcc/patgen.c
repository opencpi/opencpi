/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Oct 24 15:27:00 2012 EDT
 * BASED ON THE FILE: gen/patgen.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: patgen
 */
#include <string.h>
#include "patgen_Worker.h"
#include <unistd.h>

PATGEN_METHOD_DECLARATIONS;
RCCDispatch patgen = {
 /* insert any custom initializations here */
 PATGEN_DISPATCH
};

/*
 * Methods to implement for worker patgen, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  PatgenProperties *p = self->properties;
  RCCPort *out = self->ports;

  // Fixed length for now
  unsigned length = 1024;

  uint16_t * v = (uint16_t *)out->current.data;

  if (length > out->current.maxLength) {
    self->container.setError("length %u (0x%x) exceeds buffer size", length, length);
    return RCC_ERROR;
  }
  out->output.length = length*2;
  out->output.u.operation = 0;
  for ( int n=0; n<length; n++ ) {
    v[n] = n%512;
  }
  p->messagesSent++;
  
  //  usleep( 50000 );

  return RCC_ADVANCE;
}
