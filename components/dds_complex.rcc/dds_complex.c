/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 22 10:02:41 2012 EDT
 * BASED ON THE FILE: dds_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: dds_complex
 */
#include "dds_complex_Worker.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "signal_utils.h"

// We will run when we get an input buffer, OR if we have an output buffer
static uint32_t portRunConditionMasks[] = { 1<<DDS_COMPLEX_IN , 1<<DDS_COMPLEX_OUT, 0 };
static RCCRunCondition workerRunCondition = { portRunConditionMasks, 0 , 0 };

typedef struct {
  double curAngle;
} State;

static size_t sizes[] = {sizeof(State), 0 };

DDS_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch dds_complex = {
  /* insert any custom initializations here */
  .runCondition = &workerRunCondition,
  .memSizes = sizes,
  DDS_COMPLEX_DISPATCH
};

static void 
sync( RCCWorker *self )
{
  Dds_complexProperties *p = self->properties;
  State *myState = self->memories[0];  
  myState->curAngle = 0 + p->syncPhase;
}

/*
 * Methods to implement for worker dds_complex, based on metadata.
 */
static RCCResult start(RCCWorker *self )
{
  sync( self );
  return RCC_OK;
}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  State *myState = self->memories[0];  
  Dds_complexProperties *p = self->properties;

  RCCPort
    *in = &self->ports[DDS_COMPLEX_IN],
    *out = &self->ports[DDS_COMPLEX_OUT];

  Dds_complexOutIq 
    *outData = (Dds_complexOutIq*)out->current.data;

  uint16_t *inData;

  if ( in->current.data ) { 
    inData = in->current.data;

    switch( in->input.u.operation ) {

    case DDS_COMPLEX_IN_SYNC:
      sync( self  );
      break;

    case DDS_COMPLEX_IN_TIME:
      // Empty
      break;
    };
  }
  
  // Generate the next signal buffer
  if ( outData ) {
    unsigned int len = byteLen2Complex(out->current.maxLength);
    unsigned int n;
    for ( n=0; n<len; n++ ) {
      outData->data[n].I = Scale( sin(myState->curAngle) );
      outData->data[n].Q = Scale( cos(myState->curAngle) );    
      myState->curAngle += p->phaseIncrement;
      if( myState->curAngle > 2*PI ) {
	myState->curAngle = 0;
      }
    }
    out->output.length = len * 4;
    out->output.u.operation = DDS_COMPLEX_OUT_IQ;
  }

  return RCC_ADVANCE;
}
