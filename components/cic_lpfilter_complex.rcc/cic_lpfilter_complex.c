#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 22 10:04:14 2012 EDT
 * BASED ON THE FILE: cic_lpfilter_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: cic_lpfilter_complex
 */
#include "cic_lpfilter_complex_Worker.h"
#include "signal_utils.h"

#define STAGES 2
#define II 0
#define QQ 1

typedef struct {
  int32_t slow_acc[2][STAGES+1];
  int32_t slow_del[2][STAGES+1];
  int32_t fast_acc[2][STAGES+1];
  int32_t dfactor;
  unsigned sample;
  unsigned out_idx;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};


CIC_LPFILTER_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch cic_lpfilter_complex = {
 /* insert any custom initializations here */
  .memSizes = mysizes,
 CIC_LPFILTER_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker cic_hpfilter_complex, based on metadata.
 */
static void
sync( RCCWorker * self )
{
  MyState *s = self->memories[0];
  Cic_lpfilter_complexProperties *p = self->properties;
  memset( s,0,sizeof(MyState));
  if ( p->M == 0 ) {
    p->M = 1;
  }
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
  Cic_lpfilter_complexProperties *p = self->properties;

 RCCPort
   *in = &self->ports[CIC_LPFILTER_COMPLEX_IN],
   *out = &self->ports[CIC_LPFILTER_COMPLEX_OUT];
   
 Cic_lpfilter_complexInIq
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }
 
 switch( in->input.u.operation ) {

 case CIC_LPFILTER_COMPLEX_IN_IQ:
   {
     unsigned out_idx = s->out_idx;
     unsigned len = byteLen2Complex(in->input.length);
     unsigned i;
     len = min(out->current.maxLength-out_idx, len);
     for ( s->sample=0; s->sample<len; s->sample++ ) {

       // I
       s->fast_acc[II][0] = inData->data[s->sample].I;
       for ( i=1; i<=STAGES; i++ ) {
	 s->fast_acc[II][i] = s->fast_acc[II][i] - s->fast_acc[II][i-1];
       }
       s->slow_acc[II][0] = s->fast_acc[II][STAGES];
       for ( i=0; i<STAGES; i++ ) {
	 s->slow_acc[II][i+1] = s->slow_acc[II][i] - s->slow_del[II][i];       
	 s->slow_del[II][i+1] = s->slow_acc[II][i];
       }

       // Q
       s->fast_acc[QQ][0] = inData->data[s->sample].Q;
       for ( i=1; i<=STAGES; i++ ) {
	 s->fast_acc[QQ][i] = s->fast_acc[QQ][i] - s->fast_acc[QQ][i-1];
       }
       s->slow_acc[QQ][0] = s->fast_acc[QQ][STAGES];
       for ( i=0; i<STAGES; i++ ) {
	 s->slow_acc[QQ][i+1] = s->slow_acc[QQ][i] - s->slow_del[QQ][i];       
	 s->slow_del[QQ][i+1] = s->slow_acc[QQ][i];
       }

       // Generate the decimated output
       if ( ((s->dfactor++)%p->M) == 0 ) {
	 double gain = Gain( p->gain);
	 outData->data[out_idx].I = Scale( Uscale(s->slow_acc[II][STAGES]) * gain);
	 outData->data[out_idx].Q = Scale( Uscale(s->slow_acc[QQ][STAGES]) * gain);	 
	 double v = scabs( outData->data[out_idx].I, outData->data[out_idx].Q );
	 if ( v > Uscale( p->peakDetect ) ) {
	   p->peakDetect = Scale( v );
	 }
	 out_idx++;
       }
     }

     // To optimize throughput we will pack output buffers
     unsigned outb = Complex2bytes(out_idx);
     if ( outb < out->current.maxLength-Complex2bytes(1) ) {
       s->out_idx = out_idx;
     }
     else {
       out->output.length = outb;
       s->out_idx = 0;
       self->container.advance( out, 0 );
     }
     if ( s->sample >= byteLen2Complex(in->input.length) ) {
       s->sample=0;
       self->container.advance( in, 0 );
     }
     
   }
   break;  

 case CIC_LPFILTER_COMPLEX_IN_SYNC:
   sync( self  );
 case CIC_LPFILTER_COMPLEX_IN_TIME:
   self->container.send( out, &in->current, in->input.u.operation, in->input.length );
   return RCC_OK;
   break;

 };

 return RCC_OK;

}
