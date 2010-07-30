/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Jul 29 08:03:24 2010 EDT
 * BASED ON THE FILE: copy.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: copy
 */
#include "copy_Worker.h"

COPY_METHOD_DECLARATIONS;
RCCDispatch copy = {
  /* insert any custom initializations here */
  COPY_DISPATCH
};

/*
 * Methods to implement for worker copy, based on metadata.
*/

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  RCCPort
    *in = &self->ports[COPY_IN],
    *out = &self->ports[COPY_OUT];
#if 0
  self->container->send(out, in->current, in->input.u.operation, in->input.length);
  return RCC_OK;
#else
  memcpy(self->ports[COPY_OUT].current.data, in->current.data, in->input.length);
  out->output.u.operation = in->input.u.operation;
  out->output.length = in->input.length;
  return RCC_ADVANCE;
#endif
}
