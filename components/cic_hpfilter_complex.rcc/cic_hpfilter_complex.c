#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Aug  5 14:08:12 2012 PDT
 * BASED ON THE FILE: cic_hpfilter_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: cic_hpfilter_complex
 */
#include "cic_hpfilter_complex_Worker.h"
#include "signal_utils.h"


#define STAGES 4
#define II 0
#define QQ 1

typedef struct {
  int64_t slow_acc[2][STAGES+1];
  int64_t slow_del[2][STAGES+1];
  int64_t fast_acc[2][STAGES+1];
  unsigned input_idx;
  unsigned remainder;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};

CIC_HPFILTER_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch cic_hpfilter_complex = {
 /* insert any custom initializations here */
  .memSizes = mysizes,
 CIC_HPFILTER_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker cic_hpfilter_complex, based on metadata.
 */



static void
sync( RCCWorker * self )
{
  MyState *s = self->memories[0];
  memset( s,0,sizeof(MyState));
}


static RCCResult
start(RCCWorker *self) {
  sync( self );
  return RCC_OK;
}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  MyState *s = self->memories[0];
  Cic_hpfilter_complexProperties *p = self->properties;

 RCCPort
   *in = &self->ports[CIC_HPFILTER_COMPLEX_IN],
   *out = &self->ports[CIC_HPFILTER_COMPLEX_OUT];
   
 Cic_hpfilter_complexInIq
   *inData = in->current.data,
   *outData = out->current.data;
 
 out->output.u.operation = in->input.u.operation;
 switch( in->input.u.operation ) {

 case CIC_HPFILTER_COMPLEX_IN_IQ:
   {
     unsigned out_idx = 0;
     unsigned len = byteLen2Complex(in->input.length);
     unsigned i, samp;
     for ( samp=s->input_idx; samp<len; samp++ ) {
       if ( ((out_idx+1)*4) >= out->current.maxLength ) {  // We ran out of output buffer
	 s->input_idx = samp;
	 self->container.send( &self->ports[CIC_HPFILTER_COMPLEX_OUT], 
			       &self->ports[CIC_HPFILTER_COMPLEX_OUT].current, in->input.u.operation, 
			       out_idx*4 );
	 if ( samp < len ) {
	   return RCC_OK;
	 }
	 else {
	   return RCC_ADVANCE;
	 }
       }
       // We may need to generate more output data from the last input
       if ( s->remainder ) {
	 for ( i=0; (i<s->remainder) && ((out_idx*4) <= out->current.maxLength); i++, out_idx++ ) {
	   outData->data[out_idx].I = s->fast_acc[II][STAGES];
	   outData->data[out_idx].Q = s->fast_acc[QQ][STAGES];
	 }
	 s->remainder -= i;
	 if ( ((out_idx+1)*4) >= out->current.maxLength ) {
	   self->container.send( &self->ports[CIC_HPFILTER_COMPLEX_OUT], 
				 &self->ports[CIC_HPFILTER_COMPLEX_OUT].current, in->input.u.operation, 
				 out_idx*4 );
	   if ( samp < len ) {
	     return RCC_OK;
	   }
	   else {
	     return RCC_ADVANCE;
	   }
	 }
       }

       // I
       s->slow_acc[II][0] = inData->data[samp].I;
       s->fast_acc[II][0] = s->fast_acc[II][0] + s->slow_acc[II][STAGES+1];
       for ( i=0; i<STAGES; i++ ) {
	 s->slow_acc[II][i+1] = s->slow_acc[II][i] - s->slow_del[II][i];
	 s->slow_del[II][i] = s->slow_acc[II][i];
       }
       for ( i=1; i<=STAGES; i++ ) {
	 s->fast_acc[II][i] = s->fast_acc[II][i] + s->fast_acc[II][i-1];       
       }

       // Q
       s->slow_acc[QQ][0] = inData->data[samp].Q;
       s->fast_acc[QQ][0] = s->fast_acc[QQ][0] + s->slow_acc[QQ][STAGES+1];
       for ( i=0; i<STAGES; i++ ) {
	 s->slow_acc[QQ][i+1] = s->slow_acc[QQ][i] - s->slow_del[QQ][i];
	 s->slow_del[QQ][i] = s->slow_acc[QQ][i];
       }
       for ( i=1; i<=STAGES; i++ ) {
	 s->fast_acc[QQ][i] = s->fast_acc[QQ][i] + s->fast_acc[QQ][i-1];       
       }

       // Generate the interpolated output 
       // We are not really interpolating here, just copying the last calculated value
       for ( i=0; i<p->M; i++, out_idx++ ) {
	 if ( ((out_idx+1)*4) >= out->current.maxLength ) {
	   s->remainder = p->M-i;
	   s->input_idx = samp;
	   self->container.send( &self->ports[CIC_HPFILTER_COMPLEX_OUT], 
				 &self->ports[CIC_HPFILTER_COMPLEX_OUT].current, in->input.u.operation, 
				 out_idx*4 );
	   if ( samp < len ) {
	     return RCC_OK;
	   }
	   else {
	     return RCC_ADVANCE;
	   }
	 }
	 double gain = Gain( p->gain);
	 outData->data[out_idx].I = Scale( Uscale(s->fast_acc[II][STAGES]) * gain);
	 outData->data[out_idx].Q = Scale( Uscale(s->fast_acc[QQ][STAGES]) * gain);	 
	 double v = scabs( outData->data[out_idx].I, outData->data[out_idx].Q );
	 if ( v > Uscale( p->peakDetect ) ) {
	    p->peakDetect = Scale( v );
	 }
	 out_idx++;
       }
     }
   }
   s->input_idx=0;
   break;  

 case CIC_HPFILTER_COMPLEX_IN_SYNC:
   sync( self  );
 case CIC_HPFILTER_COMPLEX_IN_TIME:
   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   break;

 };


 return RCC_ADVANCE;
}
