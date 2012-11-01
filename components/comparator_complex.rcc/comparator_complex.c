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
#include "comparator_complex_Worker.h"
#include "signal_utils.h"
#include "assert.h"

typedef struct {
  double deviation;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};


COMPARATOR_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch comparator_complex = {
  /* insert any custom initializations here */
  COMPARATOR_COMPLEX_DISPATCH
  .memSizes = mysizes
};


static RCCResult
start(RCCWorker *self) {
  Comparator_complexProperties *p = self->properties;
  MyState *s = self->memories[0];
  s->deviation = Uscale( p->deviation );

  // We do this since a single failure will turn this to a false but we still
  // want to run the entire test to generate the output files for debug and 
  // analysis
  p->passed = 1;

  return RCC_OK;
}


static void
runComplexTest( RCCWorker * self ) 
{
  MyState *s = self->memories[0];
  Comparator_complexProperties *p = self->properties;

  printf("In comparator::runComplexTest\n");

  RCCPort
    *in_unit_test = &self->ports[COMPARATOR_COMPLEX_IN_UNIT_TEST],
    *in_expected = &self->ports[COMPARATOR_COMPLEX_IN_EXPECTED],
    *out_delta = &self->ports[COMPARATOR_COMPLEX_OUT_DELTA],
    *out_actual = &self->ports[COMPARATOR_COMPLEX_OUT_ACTUAL];
   
  Comparator_complexIn_unit_testIq
    *inUTData = in_unit_test->current.data,
    *inEXData = in_expected->current.data,
    *outActualData = out_actual->current.data;

  int16_t
    *outDeltaData = out_delta->current.data;

  if ( in_unit_test->input.length != in_expected->input.length ) {
    printf("/n/n The data length coming from the unit under test and the expected results file differ\n");
    printf("UUT data length = %d, expected data length = %d\n", in_unit_test->input.length ,  in_expected->input.length );
    printf("This will cause the test to fail. The output of the UUT and the expected results data should be identical\n");
    p->passed = 0;
  }
  int len = byteLen2Complex( in_unit_test->input.length );
  int i;
  for ( i=0; i<len; i++ ) {
    double delta = fabs( scabs(inUTData->data[i].I, inUTData->data[i].Q) - 
			 scabs(inEXData->data[i].I, inEXData->data[i].Q) );

    //    printf("Calculated delta = %f \n", delta );
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
  Comparator_complexProperties *p = self->properties;

  // We run when we have all buffers avail, no need to check
  runComplexTest( self );

  return RCC_ADVANCE;
}
