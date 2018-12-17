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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Nov 18 08:15:26 2013 EST
 * BASED ON THE FILE: Baudtracking_simple.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: Baudtracking_simple
 */
#include "Baudtracking_simple_Worker.h"
#include "signal_utils.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
   double sumArray[100];
   double savedData[100];
   unsigned savedIndex;
   unsigned int centerSamp;
   unsigned int baudAvrCounter;
   unsigned int dropSamples;
   int start;
   int driftCount;
} State;
static size_t mysizes[] = {sizeof(State), 0 };

BAUDTRACKING_SIMPLE_METHOD_DECLARATIONS;
RCCDispatch Baudtracking_simple = {
 /* insert any custom initializations here */
 .memSizes = mysizes,
 BAUDTRACKING_SIMPLE_DISPATCH
};

/*
 * Methods to implement for worker Baudtracking_simple, based on metadata.
 */
static void realignData(RCCWorker *self, int drift)
{
	int myDrift = drift;
	State *myState = self->memories[0];
	Baudtracking_simpleProperties *p = self->properties;

	while (myDrift > 0)
	{
		myState->savedData[drift-myDrift] = myState->savedData[p->SPB-myDrift];
		myDrift--;
	}
}
static void getData(RCCWorker *self, unsigned int current_index[])
{
	Baudtracking_simpleProperties *p = self->properties;
    RCCPort *in = &self->ports[BAUDTRACKING_SIMPLE_IN];
    Baudtracking_simpleInData *inData = in->current.data;
    State *myState = self->memories[0];
    unsigned len = ComplexLen2Real(in->input.length);
    int i;

    //if (current_index[0] == len)
    	//current_index[0]++;

    for(i = myState->savedIndex;
      ((i < p->SPB) && (current_index[0] < len));
        i++)
	  {
    	if (myState->dropSamples == 0)
    	{
    	    myState->savedData[i]= Uscale(inData->real[current_index[0]]);
		    myState->savedIndex++;
    	}
    	else
    	{
    		myState->dropSamples--;
    		i--;
    	}
		  current_index[0]++;
		  //printf("end1\n");
	  }
}

static RCCResult start(RCCWorker *self)
{
   State *myState = self->memories[0];
   Baudtracking_simpleProperties *p = self->properties;
   unsigned int i;

   if (p->SPB <= 0)
   {
     return self->container.setError("error invalid setting for SPB property", strerror(1));
   }

   myState->savedIndex = 0;
   myState->centerSamp = p->SPB/2;
   myState->baudAvrCounter = 1;

   for(i = 0; i < p->SPB; i++)
   {
	   myState->sumArray[i] = 0;
   }

   myState->start = 0;
   myState->driftCount = 0;
   myState->dropSamples = 0;

   return RCC_OK;
}

static RCCResult run(RCCWorker *self,
		             RCCBoolean timedOut,
		             RCCBoolean *newRunCondition)
{

   (void)self;
   (void)timedOut;
   (void)newRunCondition;
   Baudtracking_simpleProperties *p = self->properties;
   RCCPort *in = &self->ports[BAUDTRACKING_SIMPLE_IN];
   RCCPort *out = &self->ports[BAUDTRACKING_SIMPLE_OUT];
   Baudtracking_simpleOutData *outData = out->current.data;
   State *myState = self->memories[0];
   //unsigned len = ComplexLen2Real(in->input.length);
   int drift;
   unsigned int i;
   double peakVal = 0;
   unsigned int peakSample = 0;
   unsigned int dataOut_index = 0;
   unsigned int current_index[1] = {0};
   unsigned int centerSamp = p->SPB/2;
   out->output.length = 0;

   //if (in->input.length > out->current.maxLength/p->SPB)
   //{
   //  printf("in buffer is: %i out buffer: %i\n", in->input.length, out->current.maxLength);
   //   self->container.setError( "output buffer too small" );
   //   return RCC_ERROR;
  // }

   if (in->input.length == 0)
   {
     out->output.length =  0;
     self->container.advance(out,0);
     return RCC_OK;
   }

   if ( p->bypass )
   {
     out->output.length =  in->input.length;
     memcpy(out->current.data, in->current.data, out->output.length);
     return RCC_ADVANCE;
   }
   else
   {
    getData(self, current_index);
    dataOut_index = 0;

    // BaudAvrCount really should have been a ushort property, but to maintain
    // backwards compatibility, we leave it as a short and error when its value
    // is <= 0
    uint16_t uBaudAvrCount = 0;
    if (p->BaudAvrCount <= 0)
    {
      const char* msg = "BaudAvrCount property set to a negative value";
      return self->container.setError(msg, strerror(1));
    }
    else
    {
      uBaudAvrCount = (uint16_t) p->BaudAvrCount & 0x7fff;
    }

    while(myState->savedIndex == p->SPB)
    {
       for(i = 0; i < p->SPB; i++)
       {
         myState->sumArray[i] += fabs(myState->savedData[i]);
       }

       /*for(i = 0; i < p->SPB; i++){
          if ((myState->start>110) && (myState->baudAvrCounter != 4))
          {
            if(i ==0 ) printf("sum\n");
            printf("%f ", myState->sumArray[i]);
            if (((i+1) % 10) == 0 ) printf("\n");
          }
        }*/
       myState->baudAvrCounter++;

       drift = 0;
       if(myState->baudAvrCounter > uBaudAvrCount)
       {
         for(i = 0; i < p->SPB; i++)
         {
           if (myState->sumArray[i] > peakVal)
           {
             peakVal = myState->sumArray[i];
             peakSample = i;
           }
         }
         if (myState->driftCount < 20)
         {
           myState->driftCount++;
           //printf("drift is: %i\n", peakSample - centerSamp);
         }
         peakVal = 0;
         drift = peakSample - centerSamp;
         if ((abs(drift) > 5) && (myState->driftCount == 20))
         {
           drift = 0;
         }

         myState->baudAvrCounter = 1;

         for(i = 0; i < p->SPB; i++)
         {
           myState->sumArray[i] = 0;
         }

           //current_index[0] += drift;

          // printf("\ndrift: %i current_index: %i len: %i\n",
          //	   drift,
            //   current_index[0],
              // len);
         }

         outData->real[dataOut_index] =
         Scale(myState->savedData[centerSamp+drift]);
         out->output.length += 2;
         dataOut_index++;
         myState->savedIndex = 0;
         if (drift < 0 )
         {
           realignData(self, -drift);
           myState->savedIndex = -drift;
         }
         else if (drift > 0)
         {
           myState->dropSamples = drift;
         }
         getData(self, current_index);
      }
   }

   if (out->output.length == 0)
   {
       self->container.advance(in,0);
	     return RCC_OK;
   }
   else
   {
	   //printf("baud: %u\n", out->output.length);
       return RCC_ADVANCE;
   }
}

