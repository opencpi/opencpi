/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Nov  4 16:58:40 2013 EST
 * BASED ON THE FILE: replicate.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: replicate
 */
#include "replicate_Worker.h"

REPLICATE_METHOD_DECLARATIONS;

// Initialized to zero by the container
typedef struct {
  unsigned inPos, outPos, repCount;
} MyState;

static size_t memories[ ] =
{
  sizeof (MyState),
  0
};

RCCDispatch replicate = {
 /* insert any custom initializations here */
  .memSizes = memories,
 REPLICATE_DISPATCH
};

/*
 * Methods to implement for worker replicate, based on metadata.
 */
static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  RCCPort
    *in = &self->ports[REPLICATE_IN],
    *out = &self->ports[REPLICATE_OUT];
  char
    *inData = in->current.data,
    *outData = out->current.data;
  MyState *s = self->memories[0];
  ReplicateProperties *p = self->properties;
  (void)timedOut;(void)newRunCondition;

  // We know there is a current input and current output buffer.
  // We know that inPos, outPos, and repCount all start out zero
  while (s->inPos < in->input.length && s->outPos < out->current.maxLength) {
    outData[s->outPos++] = inData[s->inPos];
    if (++s->repCount >= p->factor) {
      s->inPos++;
      s->repCount = 0;
    }
  }
  // We have run out of room in input or output buffer
  if (s->inPos >= in->input.length) {
    self->container.advance(in, 0);
    s->inPos = 0;
  }
  if (s->outPos >= out->current.maxLength) {
    out->output.length = s->outPos;
    self->container.advance(out, 0);
    s->outPos = 0;
  }
  return in->input.length == 0 ? RCC_DONE : RCC_OK;
}  
