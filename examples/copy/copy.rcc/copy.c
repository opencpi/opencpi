/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "copy_Worker.h"

COPY_METHOD_DECLARATIONS;
RCCDispatch copy = {
  /* insert any custom initializations here */
  COPY_DISPATCH
};

/*
 * Methods to implement for worker copy, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort
    *in = &self->ports[COPY_IN],
    *out = &self->ports[COPY_OUT];
  assert(in->input.length <= out->current.maxLength);
  //  fprintf(stderr, "Sending %u %zu\n", in->input.u.operation, in->input.length);
#if 0 // This zero-copy mode doesn't work for some reason
  self->container.send(out, &in->current, in->input.u.operation, in->input.length);
  return RCC_OK;
#else
  memcpy(out->current.data, in->current.data, in->input.length);
  out->output.length = in->input.length;
  return RCC_ADVANCE;
#endif
}
