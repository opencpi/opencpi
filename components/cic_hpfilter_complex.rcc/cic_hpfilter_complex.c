/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
static size_t mysizes[] = {sizeof(MyState), 0};

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

RCCResult sendOutput( RCCWorker * self, MyState * s, RCCPort * out, unsigned samp, unsigned len ) 
{
  if ( samp >= len ) {
    s->input_idx = 0;
    return RCC_ADVANCE;
  }
  else {
    self->container.advance(out,0);
    s->input_idx = samp;
    return RCC_OK;
  }
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
 out->output.length = in->input.length;
 switch( in->input.u.operation ) {

 case CIC_HPFILTER_COMPLEX_IN_IQ:
   {
     unsigned out_idx = 0;
     unsigned len = byteLen2Complex(in->input.length);
     unsigned i, samp;

#ifndef NDEBUG
      printf("%s got %zu bytes of data\n", __FILE__,  in->input.length);
#endif

     // We may need to generate more output data from the last input
     unsigned max_out = byteLen2Complex(out->current.maxLength);
     if ( s->remainder ) {
       for ( i=0; (i<s->remainder) && (out_idx<max_out); i++, out_idx++ ) {
	 outData->data[out_idx].I = s->fast_acc[II][STAGES];
	 outData->data[out_idx].Q = s->fast_acc[QQ][STAGES];
       }
       s->remainder -= i;
       if ( out_idx >= max_out ) {
	 return sendOutput( self, s, out, s->input_idx, byteLen2Complex(in->input.length));
       }
     }

     len = min(len,max_out);
     for ( samp=s->input_idx; samp<len; samp++ ) {

       // I
       s->slow_acc[II][0] = inData->data[samp].I;
       s->fast_acc[II][0] = s->fast_acc[II][0] + s->slow_acc[II][STAGES];
       for ( i=0; i<STAGES; i++ ) {
	 s->slow_acc[II][i+1] = s->slow_acc[II][i] - s->slow_del[II][i];
	 s->slow_del[II][i] = s->slow_acc[II][i];
       }
       for ( i=1; i<=STAGES; i++ ) {
	 s->fast_acc[II][i] = s->fast_acc[II][i] + s->fast_acc[II][i-1];       
       }

       // Q
       s->slow_acc[QQ][0] = inData->data[samp].Q;
       s->fast_acc[QQ][0] = s->fast_acc[QQ][0] + s->slow_acc[QQ][STAGES];
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
	 if ( out_idx  >= max_out ) {
	   s->remainder = p->M-i;
	   return sendOutput( self, s, out, samp,  byteLen2Complex(in->input.length));
	 }
	 double gain = Gain( p->gain);
	 outData->data[out_idx].I = Scale( Uscale(s->fast_acc[II][STAGES]) * gain);
	 outData->data[out_idx].Q = Scale( Uscale(s->fast_acc[QQ][STAGES]) * gain);	 
	 double v = scabs( outData->data[out_idx].I, outData->data[out_idx].Q );
	 if ( v > Uscale( p->peakDetect ) ) {
	    p->peakDetect = Scale( v );
	 }
       }
     }
     return sendOutput( self, s, out, samp, byteLen2Complex(in->input.length));
   }
   break;  

 case CIC_HPFILTER_COMPLEX_IN_SYNC:
   sync( self  );
 case CIC_HPFILTER_COMPLEX_IN_TIME:
   self->container.send( out, &in->current, in->input.u.operation, in->input.length);
   break;

 }

 return RCC_OK;
}
