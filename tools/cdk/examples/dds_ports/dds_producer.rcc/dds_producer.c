/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <string.h>
#include "dds_producer_Worker.h"

DDS_PRODUCER_METHOD_DECLARATIONS;
RCCDispatch dds_producer = {
  /* insert any custom initializations here */
  DDS_PRODUCER_DISPATCH
};

/*
 * Methods to implement for worker hello, based on metadata.
 */

// The message we send all the time
#define MESSAGE "Hello, world\n"

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *out = &self->ports[DDS_PRODUCER_OUT];

  Dds_producerOutSamples * sample = (Dds_producerOutSamples *)out->current.data;

  sample->userId = 10;
  sample->u1 = 2222;
  sample->long_seq_length = 5;
  unsigned int n;
  for ( n=0; n<sample->long_seq_length; n++ ) {
    sample->long_seq[n] = 10*n;
  }
  sample->v_bool = 1;
  strcpy(sample->v_message, "Message from producer\n");
  sample->v_short  = 99;
  sample->v_longlong = 123456789;
  sample->v_double = 9876.543;


  out->output.length = sizeof(Dds_producerOutSamples);
  return RCC_ADVANCE;
}
