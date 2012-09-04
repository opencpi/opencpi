
/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 22 09:05:20 2012 EDT
 * BASED ON THE FILE: mixer_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: mixer_complex
 */
#include "mixer_complex_Worker.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <signal_utils.h>


static uint32_t runConditionMasks[] = { (1<<MIXER_COMPLEX_IN_DDS) | (1<<MIXER_COMPLEX_IN_IF) | (1<<MIXER_COMPLEX_OUT), 0 };
static RCCRunCondition rc = { runConditionMasks, 0 , 0 };

typedef struct {
  unsigned int curDdsIndex;
  unsigned int curIfIndex;
} State;
static uint32_t sizes[] = {sizeof(State), 0 };

MIXER_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch mixer_complex = {
 /* insert any custom initializations here */
  .runCondition = &rc,
  .memSizes = sizes,
 MIXER_COMPLEX_DISPATCH
};

static void
processSignalData( RCCWorker * self  )
{
  State *myState = self->memories[0];  

  RCCPort
    *in_if = &self->ports[MIXER_COMPLEX_IN_IF],
    *in_dds = &self->ports[MIXER_COMPLEX_IN_DDS],
    *out = &self->ports[MIXER_COMPLEX_OUT];

  Mixer_complexOutIq 
    *outData = (Mixer_complexOutIq*)out->current.data,
    *in_ifData = (Mixer_complexOutIq*)in_if->current.data, 
    *in_ddsData = (Mixer_complexOutIq*)in_dds->current.data;

  // We get vectors from both the dds and the if inputs, we can only process
  // the based on the shortest vector
  unsigned int len = min( byteLen2Complex(in_if->input.length)-myState->curIfIndex,
			  byteLen2Complex(in_dds->input.length)-myState->curDdsIndex);
  len = min(len, byteLen2Complex(out->output.length));



  // DDS * IF = out  -> (a+bi)(c+di) = (ac - bd) + (bc + ad)i  : (if.I + if.Q)(dds.I + dds.Q)
  unsigned int n;
  for ( n=0; n<len; n++ ) {
    double dtmp = ( Uscale(in_ifData->data.data[myState->curIfIndex].I) * Uscale(in_ddsData->data.data[myState->curDdsIndex].I) ) 
      - ( Uscale(in_ifData->data.data[myState->curIfIndex].Q) * Uscale(in_ddsData->data.data[myState->curDdsIndex].Q) );
    outData->data.data[n].I = Scale( dtmp );

    dtmp = ( Uscale(in_ifData->data.data[myState->curIfIndex].Q) * Uscale(in_ddsData->data.data[myState->curDdsIndex].I) ) 
      + ( Uscale(in_ifData->data.data[myState->curIfIndex].I) * Uscale(in_ddsData->data.data[myState->curDdsIndex].Q) );
    outData->data.data[n].Q = Scale( dtmp );

    myState->curDdsIndex++;
    myState->curIfIndex++;

  }
  
  // Figure out who to advance
  if ( myState->curIfIndex >= byteLen2Complex(in_if->input.length) ) {
    self->container.advance( in_if, 0);
    myState->curIfIndex = 0;
  }
  if ( myState->curDdsIndex >= byteLen2Complex(in_dds->input.length) ) {
    self->container.advance( in_dds, 0);
    myState->curDdsIndex = 0;
  }

  // Always advance the output
  self->container.advance( out, 0);  

}

void 
processSyncSignal( RCCWorker * self  )
{
  State *myState = self->memories[0];    

  RCCPort
    *out = &self->ports[MIXER_COMPLEX_OUT_SYNC_ONLY];

  // Flush our state
  myState->curDdsIndex = 0;
  myState->curIfIndex = 0;    

  // Tell the DDS to sync
  out->output.length = 0;
  out->output.u.operation = MIXER_COMPLEX_OUT_SYNC_ONLY_SYNC;
  self->container.advance( out, 0); 
}


void 
processTimeSignal( RCCWorker * self  )
{

  RCCPort*out = &self->ports[MIXER_COMPLEX_OUT_SYNC_ONLY];

  // Tell the DDS to sync
  out->output.length = 0;
  out->output.u.operation = MIXER_COMPLEX_OUT_SYNC_ONLY_TIME;
  self->container.advance( out, 0); 
}


/*
 * Methods to implement for worker mixer_complex, based on metadata.
 */
static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[MIXER_COMPLEX_IN_IF],
   *out = &self->ports[MIXER_COMPLEX_OUT];   

 uint16_t
   *inData = in->current.data,
   *outData = out->current.data;

 switch( in->input.u.operation ) {

 case MIXER_COMPLEX_IN_IF_IQ:
   processSignalData( self  );
   break;

 case MIXER_COMPLEX_IN_IF_SYNC:
   processSyncSignal( self  );
   break;

 case MIXER_COMPLEX_IN_IF_TIME:
   processTimeSignal( self );
   break;
   
 };

 return RCC_OK;
}
