/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Aug 24 06:10:33 2012 PDT
 * BASED ON THE FILE: comparator.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: comparator
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "comparator_real_Worker.h"
#include "signal_utils.h"
#include "assert.h"

typedef struct {
  double deviation;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};

COMPARATOR_REAL_METHOD_DECLARATIONS;
RCCDispatch comparator_real = {
  /* insert any custom initializations here */
  COMPARATOR_REAL_DISPATCH
  .memSizes = mysizes
};


static RCCResult
start(RCCWorker *self) {
  Comparator_realProperties *p = self->properties;
  MyState *s = self->memories[0];
  s->deviation = Uscale( p->deviation );
  return RCC_OK;
}


/*
 * Methods to implement for worker comparator, based on metadata.
 */

static void
runRealTest( RCCWorker * self ) 
{
  MyState *s = self->memories[0];
  Comparator_realProperties *p = self->properties;

  RCCPort
    *in_unit_test = &self->ports[COMPARATOR_REAL_IN_UNIT_TEST],
    *in_expected = &self->ports[COMPARATOR_REAL_IN_EXPECTED],
    *out_delta = &self->ports[COMPARATOR_REAL_OUT_DELTA],
    *out_actual = &self->ports[COMPARATOR_REAL_OUT_ACTUAL];
   
  int16_t
    *inUTData = in_unit_test->current.data,
    *inEXData = in_expected->current.data,
    *outActualData = out_actual->current.data;

  int16_t
    *outDeltaData = out_delta->current.data;


  if ( in_unit_test->input.length != in_expected->input.length ) {
    fprintf( stderr, "/n/n The data length coming from the unit under test and the expected results file differ\n");
    fprintf( stderr, "UUT data length = %d, expected data length = %d\n", in_unit_test->input.length ,  in_expected->input.length );
    fprintf( stderr, "This will cause the test to fail. The output of the UUT and the expected results data should be identical\n");
    p->passed = 0;
  }

  int len = byteLen2Real(in_unit_test->input.length);
  int i;
  for ( i=0; i<len; i++ ) {
    double delta = fabs( (double)(inUTData[i]-inEXData[i]) );

    /*   if ( delta != 0 ) printf("Calculated delta(%d) = %f, %d,%d \n", i, delta, inUTData[i], inEXData[i] ); */
    if ( delta > s->deviation ) {
      p->passed = 0;
    }
    outDeltaData[i] = Scale( delta );
  }
  out_delta->output.u.operation = 0;
  out_delta->output.length = len;

  memcpy( outActualData, inUTData,  in_unit_test->input.length);
  out_actual->output.u.operation = in_unit_test->input.u.operation;
  out_actual->output.length = in_unit_test->input.length;

}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

  // MyState *s = self->memories[0];
  Comparator_realProperties *p = self->properties;

  // We do this since a single failure will turn this to a false but we still
  // want to run the entire test to generate the output files for debug and 
  // analysis
  p->passed = 1;

  // We run when we have all buffers avail, no need to check
  runRealTest( self );

  return RCC_ADVANCE;
}
