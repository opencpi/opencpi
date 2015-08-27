/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <string.h>
#include "hello_Worker.h"

HELLO_METHOD_DECLARATIONS;
RCCDispatch hello = {
  /* insert any custom initializations here */
  HELLO_DISPATCH
};

/*
 * Methods to implement for worker hello, based on metadata.
 */

// The message we send all the time
#define MESSAGE "Hello, world."

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *out = &self->ports[HELLO_OUT];

  if (sizeof(MESSAGE) > out->current.maxLength)
    return RCC_FATAL;
  strcpy(out->current.data, MESSAGE);
  out->output.length = sizeof(MESSAGE);
  return RCC_ADVANCE;
}
