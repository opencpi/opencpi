/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Oct 24 15:27:00 2012 EDT
 * BASED ON THE FILE: gen/pattern.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: pattern
 */
#include <string.h>
#include "pattern_Worker.h"

PATTERN_METHOD_DECLARATIONS;
RCCDispatch pattern = {
 /* insert any custom initializations here */
 PATTERN_DISPATCH
};

/*
 * Methods to implement for worker pattern, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  PatternProperties *p = self->properties;
  RCCPort *out = self->ports;
  const uint32_t *meta = p->metadata[p->nextMeta++];
  unsigned length = meta[0];
  uint8_t *data = (uint8_t*)p->data + p->nextData;
  unsigned left = sizeof(p->data) - p->nextData;

  if (!p->messagesToSend)
    return RCC_DONE;
  if (length > out->current.maxLength) {
    self->container.setError("metadata length %u (0x%x) exceeds buffer size", length, length);
    return RCC_ERROR;
  }
  if (length > sizeof(p->data)) {
    self->container.setError("data length %u (0x%x) exceeds data memory size (%u)",
			     length, length, sizeof(p->data));
    return RCC_ERROR;
  }
  out->output.length = length;
  out->output.u.operation = meta[1];
  if (length > left) {
    memcpy(out->current.data, data, left);
    memcpy(out->current.data + left, p->data, length - left);
  } else
    memcpy(out->current.data, data, length);
  if (!(p->control & 2))
    p->nextData = (p->nextData + length) % sizeof(p->data);
  if (p->nextMeta >= p->metadataCount)
    p->nextMeta = 0;
  p->messagesSent++;
  p->dataSent += length;
  return --p->messagesToSend ? RCC_ADVANCE : RCC_ADVANCE_DONE;
}
