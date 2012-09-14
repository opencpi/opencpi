#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Aug 16 09:34:46 2012 PDT
 * BASED ON THE FILE: noise_gen_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: noise_gen_complex
 */
#include "noise_gen_complex_Worker.h"
#include "signal_utils.h"

#define BIT(v,x) (v>>x)
#define SETBIT( t,v,x) t = ((t&~(1<<x))|(v<<x))

typedef struct {
  uint16_t rand_i;
  uint16_t rand_q;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};

NOISE_GEN_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch noise_gen_complex = {
 /* insert any custom initializations here */
  .memSizes = mysizes,
 NOISE_GEN_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker noise_gen_complex, based on metadata.
 */
static RCCResult
start(RCCWorker *self) {
  MyState *s = self->memories[0];
  s->rand_i = 0x1;
  s->rand_q = 0x2267;
  return RCC_OK;
}

static void
random_noise( MyState * s )
{

#ifdef HW_ALGO
  s->rand_i = (s->rand_i&0x7ffe) | (((s->rand_i>>14)&1)^((s->rand_i>>13)&1));
  s->rand_q = (s->rand_q&0x7ffe) | (((s->rand_q>>14)&1)^((s->rand_q>>13)&1));  

  SETBIT(s->rand_i, BIT(s->rand_q,14) & BIT(s->rand_i,3),15);
  SETBIT(s->rand_i, BIT(s->rand_i,8) | BIT(s->rand_i,11),14);
  SETBIT(s->rand_i, BIT(s->rand_i,3) ^ BIT(s->rand_q,4),13);
  SETBIT(s->rand_i, !BIT(s->rand_q,13) ,12);
  SETBIT(s->rand_i, BIT(s->rand_i,12) ,11);
  SETBIT(s->rand_i, BIT(s->rand_q,13) ,10);
  SETBIT(s->rand_i, BIT(s->rand_q,5) ^ BIT(s->rand_i,6) ,9);
  SETBIT(s->rand_i, BIT(s->rand_i,2) & BIT(s->rand_i,7) ,8);
  SETBIT(s->rand_i, BIT(s->rand_i,14) ^ BIT(s->rand_q,3) ,7);
  SETBIT(s->rand_i, BIT(s->rand_i,7) ,6);
  SETBIT(s->rand_i, BIT(s->rand_i,9) | BIT(s->rand_i,8) ,5);
  SETBIT(s->rand_i, BIT(s->rand_i,6) & BIT(s->rand_i,1) ,4);
  SETBIT(s->rand_i, BIT(s->rand_q,2) & BIT(s->rand_q,0) ,3);
  SETBIT(s->rand_i, BIT(s->rand_q,1) ,2);
  SETBIT(s->rand_i, !BIT(s->rand_q,3) ,1);
  SETBIT(s->rand_i, BIT(s->rand_i,11) | BIT(s->rand_i,8) ,0);

  SETBIT(s->rand_q, BIT(s->rand_i,14) , 15);
  SETBIT(s->rand_q, BIT(s->rand_q,5) & BIT(s->rand_i,2),14);
  SETBIT(s->rand_q, BIT(s->rand_i,1) ^ BIT(s->rand_i,0),13);
  SETBIT(s->rand_q, BIT(s->rand_q,12) ,12);
  SETBIT(s->rand_q, BIT(s->rand_i,1) ^ BIT(s->rand_i,0) ,11);
  SETBIT(s->rand_q, BIT(s->rand_q,12) ,10);
  SETBIT(s->rand_q, BIT(s->rand_q,13) | BIT(s->rand_i,5) ,9);
  SETBIT(s->rand_q, BIT(s->rand_q,4) ,8);
  SETBIT(s->rand_q, !BIT(s->rand_i,7) ,7);
  SETBIT(s->rand_q, !BIT(s->rand_q,6) ,6);
  SETBIT(s->rand_q, BIT(s->rand_q,5) ,5);
  SETBIT(s->rand_q, BIT(s->rand_i,4) & BIT(s->rand_i,8) ,4);
  SETBIT(s->rand_q, BIT(s->rand_q,1) & BIT(s->rand_i,7) ,3);
  SETBIT(s->rand_q, BIT(s->rand_i,13) ,2);
  SETBIT(s->rand_q, BIT(s->rand_i,9) ,1);
  SETBIT(s->rand_q, !BIT(s->rand_q,12) ,0);
#else 
  s->rand_i = rand();
  s->rand_q = rand();
  s->rand_i |= s->rand_q << 1;
  s->rand_q |= s->rand_i << 1;
#endif


}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;
  Noise_gen_complexProperties *p = self->properties;
  MyState *s = self->memories[0];

 RCCPort
   *in = &self->ports[NOISE_GEN_COMPLEX_IN],
   *out = &self->ports[NOISE_GEN_COMPLEX_OUT];

 Noise_gen_complexInIq
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }

 switch( in->input.u.operation ) {

 case NOISE_GEN_COMPLEX_IN_IQ:
   {
     random_noise( s );
     if ( ! p->mask ) {
       self->container.send( out, &in->current,in->input.u.operation, in->input.length );
       return RCC_OK;
     }
     unsigned i;
     unsigned len = byteLen2Complex(in->input.length);
     for ( i=0; i<len; i++ ) {
       outData->data[i].I = inData->data[i].I ^ (s->rand_i & p->mask );
       outData->data[i].Q = inData->data[i].Q ^ (s->rand_q & p->mask );
     }
   }
   break;

 case NOISE_GEN_COMPLEX_IN_SYNC:
 case NOISE_GEN_COMPLEX_IN_TIME:
   self->container.send( out, &in->current, in->input.u.operation, in->input.length );
   return RCC_OK;

 };

 out->output.u.operation = in->input.u.operation;
 out->output.length = in->input.length;
 return RCC_ADVANCE;

}
