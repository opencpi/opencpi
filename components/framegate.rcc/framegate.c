/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Jun 27 08:07:19 2010 EDT
 * BASED ON THE FILE: framegate.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: framegate
 */
#include <string.h>
#include "framegate_Worker.h"

FRAMEGATE_METHOD_DECLARATIONS;
RCCDispatch framegate = {
  /* insert any custom initializations here */
  FRAMEGATE_DISPATCH
};

/*
 * Methods to implement for worker framegate, based on metadata.
 *
 * We pass one frame, then only skip whole frames, until we have skipped the gateSize
 */

static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  FramegateProperties* p = self->properties;
  RCCPort *in = &self->ports[FRAMEGATE_IN];

  if (p->skip_nbytes) { // We are skipping.  Only advance the input
    p->skip_nbytes = 
      in->input.length > p->skip_nbytes ? 0 : p->skip_nbytes - in->input.length;
    self->container->advance(in, 0);
    return RCC_OK;
  } else { // We are not skipping.  Copy and advance in and out.
    RCCPort *out = &self->ports[FRAMEGATE_OUT];
    memcpy(out->current.data, in->current.data, in->input.length);
    out->output.u.operation = in->input.u.operation;
    out->output.length = in->input.length;
    p->skip_nbytes = p->gateSize;
    return RCC_ADVANCE;
  }
}
