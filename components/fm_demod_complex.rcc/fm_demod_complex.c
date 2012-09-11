#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Aug  6 05:48:41 2012 PDT
 * BASED ON THE FILE: fm_demod_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: fm_demod_complex
 */
#include "fm_demod_complex_Worker.h"
#include "signal_utils.h"


typedef struct {
  double lastAngle;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};


FM_DEMOD_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch fm_demod_complex = {
 /* insert any custom initializations here */
  .memSizes = mysizes,
 FM_DEMOD_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker fm_demod_complex, based on metadata.
 */
double myATan2(double y, double x)
{
  double coeff_1 = PI/4;
  double coeff_2 = 3*coeff_1;
  double abs_y = fabs(y)+1e-10;      // kludge to prevent 0/0 condition
  double r,angle;
  if (x>=0)
    {
      r = (x - abs_y) / (x + abs_y);
      angle = coeff_1 - coeff_1 * r;
    }
  else
    {
      r = (x + abs_y) / (abs_y - x);
      angle = coeff_2 - coeff_1 * r;
    }
  if (y < 0)
    return(-angle);     // negate if in quad III or IV
  else
    return(angle);
}

static void
sync( RCCWorker * self )
{
  MyState *s = self->memories[0];
  memset( s,0,sizeof(MyState));
}

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  MyState *s = self->memories[0];
  Fm_demod_complexProperties *p = self->properties;

 RCCPort
   *in = &self->ports[FM_DEMOD_COMPLEX_IN],
   *out = &self->ports[FM_DEMOD_COMPLEX_OUT];
   
 Fm_demod_complexInIq
   *inData = in->current.data;

 Fm_demod_complexOutData 
   *outData = out->current.data;

 if (in->input.length/2 > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }

 out->output.u.operation = in->input.u.operation;
 switch( in->input.u.operation ) {

 case FM_DEMOD_COMPLEX_IN_IQ:
   {
     unsigned len = byteLen2Complex(in->input.length);
     unsigned samp;
     for ( samp=0; samp<len; samp++ ) {
       double angle = myATan2(Uscale(inData->data[0].I),Uscale(inData->data[0].Q));
       if ( p->emitAngle ) {
	 outData->real[0] = angle;	 
       }
       else {
	 outData->real[0] =  angle - s->lastAngle;
       }
       s->lastAngle = angle;
     }
     out->output.length = in->input.length/2;
   }
   break;

 case FM_DEMOD_COMPLEX_IN_SYNC:
   sync( self );
 case FM_DEMOD_COMPLEX_IN_TIME:
   memcpy( outData, inData, in->input.length);
   out->output.length = in->input.length;
   break;
   
 };

 return RCC_ADVANCE;
}
