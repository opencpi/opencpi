/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <string.h>
#include "dds_producer_Worker.h"

typedef struct {
  uint32_t p_count;
  uint64_t d_count;
  uint32_t u1;
} State;

static uint32_t sizes[] = {sizeof(State), 0 };

DDS_PRODUCER_METHOD_DECLARATIONS;
RCCDispatch dds_producer = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  DDS_PRODUCER_DISPATCH
};

/*
 * Methods to implement for worker hello, based on metadata.
 */


static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  State *myState = self->memories[0];  


  // We will run 10 times then delay to allow an external app to run and provide data to the consumer
  if ( myState->p_count++ > 10 )  {
    return RCC_OK;
  }



  RCCPort *out = &self->ports[DDS_PRODUCER_OUT];

  Dds_producerOutSamples * sample = (Dds_producerOutSamples *)out->current.data;

  sample->userId = 10;
  sample->u1 = myState->u1++;
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
