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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Sep 16 08:51:29 2013 EDT
 * BASED ON THE FILE: (null)
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: real_digitizer
 */
#include "real_digitizer_Worker.h"
#include "signal_utils.h"
typedef struct {
	uint16_t mask;
	uint16_t data;
} State;
static size_t mysizes[] = {sizeof(State), 0 };

REAL_DIGITIZER_METHOD_DECLARATIONS;
RCCDispatch real_digitizer = {
 /* insert any custom initializations here */
 .memSizes = mysizes,
 REAL_DIGITIZER_DISPATCH
};

/*
 * Methods to implement for worker real_digitizer, based on metadata.
 */
static RCCResult start(RCCWorker *self)
{
	State *myState = self->memories[0];
	Real_digitizerProperties *p = self->properties;

	myState->mask = 0;
	myState->data = 0;
	if (p->need_sync != 0)
	{
	   p->sync_criteria_met = 0;
	}
	else
	{
	   p->sync_criteria_met = 1;
	}

	return RCC_OK;
}
static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;

   RCCPort *in = &self->ports[REAL_DIGITIZER_IN];
   RCCPort *out = &self->ports[REAL_DIGITIZER_OUT];
   Real_digitizerInData  *inData  = in->current.data;
   Real_digitizerOutData *outData = out->current.data;
   State *myState = self->memories[0];
	 Real_digitizerProperties *p = self->properties;
   unsigned len = byteLen2Real(in->input.length);
   unsigned int i,j = 0;
   unsigned short data = 0;
   unsigned short mask = 0x8000;

   if (in->input.length == 0)
   {
     out->output.length =  0;
     self->container.advance(out,0);
     return RCC_OK;
   }

   //printf("digitizer got a packet %x %x\n", inData->real[0], inData->real[1]);
   if(myState->mask > 0)
   {
     data = myState->data;
     mask =  myState->mask;
     myState->mask = 0;
   }

   for (i = 0; i<len; i++ )
   {
     if (!p->sync_criteria_met)
     {
       myState->data = myState->data << 1;
       if (Uscale(inData->real[i]) > 0)
         myState->data++;

       if (myState->data == 0xCEFA)
       {
         p->sync_criteria_met = 1;
         if(p->enable_printing)
         {
           printf("real_digitizer: sync pattern 0xFACE found\n");
         }
       }
     }
     else
     {
		   if (Uscale(inData->real[i]) > 0)
			 {
			   data += mask;
			 }

       //printf("index: %i data: %x mask: %x input: %x\n", i, data, mask, DisplayShort(inData->real[i]));
       mask = mask >> 1;
       if (mask == 0x0000)
       {
         mask = 0x8000;
         outData->real[j] = data; //(data<<8) | (data>>8);
         //printf("data out is : %x is : %x\n\n", data, outData->real[j]);
         j++;
         data = 0;
       }
   	 }
   }
   if (mask != 0x8000)
   {
     myState->mask  = mask;
   	 myState->data = data;
   	 //printf("mask %x\n" , mask);
   }

   //printf("bytes out: %i\n ", real2bytes(j));
   out->output.length =  real2bytes(j);
   out->output.u.operation = in->input.u.operation;

   if (out->output.length == 0)
   {
	   self->container.advance(in,0);
		 return RCC_OK;
	 }
	 else
	 {
	   return RCC_ADVANCE;
	 }
}

