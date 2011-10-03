/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <string.h>
#include "dds_consumer_Worker.h"

DDS_CONSUMER_METHOD_DECLARATIONS;
RCCDispatch dds_consumer = {
  /* insert any custom initializations here */
  DDS_CONSUMER_DISPATCH
};

/*
 * Methods to implement for worker hello, based on metadata.
 */

// The message we send all the time
#define MESSAGE "Hello, world\n"

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *in = &self->ports[DDS_CONSUMER_IN];

  Dds_consumerInSamples * sample = (Dds_consumerInSamples *)in->current.data;

  printf( "\n\nConsumer got a message\n");
  printf( "  userId = %d\n", sample->userId);
  printf( "  u1 = %d\n", sample->u1 );
  printf( "  long seq len = %d\n", sample->long_seq_length );
  int n;
  for ( n=0; n<sample->long_seq_length; n++ ) {
    printf("    s[%d] = %d\n", n,sample->long_seq[n] );
  }
  printf( "  v_bool = %s\n", sample->v_bool == 0 ? "false" : "true" );
  printf( "  v_message = %s\n", sample->v_message );
  printf( "  v_short = %d\n", (int)sample->v_short );
  for ( n=0; n<5; n++ ) {
    //    printf("    s[%d] = %c\n", n,sample->v_oct_array[n] );
  }
  printf( "  v_longlong = %lld\n", sample->v_longlong );
  printf( "  v_double = %f\n", sample->v_double);

  return RCC_ADVANCE;
}
