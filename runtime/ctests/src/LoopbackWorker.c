
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <OcpiTimeEmitC.h>

#undef c_plus_plus
#include "LoopbackWorker.h"

enum PortIds {
  LoopbackWorker_Data_Out_Port=0,
  LoopbackWorker_Data_In_Port=1,
  LastPort
};


static RCCResult initialize(RCCWorker *this_)
{
  LoopbackWorkerStaticMemory *mem = (LoopbackWorkerStaticMemory*)this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;

  mem->state = LB_New_Input;
  mem->startIndex = 0;
  mem->overlapSize = 0;

  props->startIndex = mem->startIndex;
  props->longProperty = 0;

  return RCC_OK;
}


static RCCResult start(RCCWorker *this_)
{
  ( void ) this_;
  return RCC_OK;
}


static RCCResult stop(RCCWorker *this_)
{
  ( void ) this_;
  return RCC_OK;
}

static RCCResult release(RCCWorker *this_)
{
  ( void ) this_;
  return RCC_OK;
}

static RCCResult test(RCCWorker *this_)
{
  ( void ) this_;
  return RCC_OK;
}


static RCCResult afterConfigure(RCCWorker *this_)
{
  /* This only works for simple memory configurations */
  LoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;

  static_mem->longProperty = props->longProperty;
  return RCC_OK;
}


static RCCResult beforeQuery(RCCWorker *this_)
{
  /* This only works for simple memory configurations */
  LoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;

  props->startIndex = static_mem->startIndex;

  return RCC_OK;
}

static RCCResult LoopbackWorker_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  ( void ) timedout;
  ( void ) newRunCondition;
  size_t len;

  //  LoopbackWorkerStaticMemory *mem = this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;

  char* in_buffer = (char*)this_->ports[LoopbackWorker_Data_In_Port].current.data;
  char* out_buffer = (char*)this_->ports[LoopbackWorker_Data_Out_Port].current.data;

  /*  printf("In LoopbackWorker_run\n"); */

  len = this_->ports[LoopbackWorker_Data_In_Port].input.length;
  this_->ports[LoopbackWorker_Data_Out_Port].output.length = len;

#define CHECK_DATA
#ifdef CHECK_DATA
  { uint32_t *b = (uint32_t*)(in_buffer);
    uint32_t *mem = &props->longProperty;
    unsigned ncount, n, passed = 0;
#define RESYNC
#ifdef RESYNC

    //    sleep(2);

    if ( *b != *mem ) {
      printf("ERROR!! Dropped a buffer, got buffer %d, expected %d\n", 
	     *b, *mem );
      *mem = *b;
    }
#endif

    ncount = 0;
    for (n=4; n<len; n++) {
      if (in_buffer[n] != (char)(n+*mem)%23 && ncount++ < 100000) {
	printf("Consumer(%u, %zu, b-> %u): Data integrity error(%d) !!, expected %d, got %d\n", 
	       props->startIndex, len, *mem, n, (char)(n+*mem)%23, in_buffer[n]);
	passed = 0;
      }
    }
    if ( passed ) {
      if ( (*mem%500) == 0 ) 
	printf("Loopback: Buffer %d data integrity test passed\n", *mem);
    }
    (*mem)++;
  }
#endif


  //  printf("\n\n\n\ In LBW, got a good buffer of data !!\n\n\n");


#define LOOPBACK_DATA
#ifdef LOOPBACK_DATA
  memcpy(out_buffer,in_buffer,len);
  this_->ports[LoopbackWorker_Data_Out_Port].output.length = len;
#endif


#define NZCOPYIO

  /*
    #ifdef NZCOPYIO
    this_->container.send( &this_->ports[LoopbackWorker_Data_Out_Port], 
    &this_->ports[LoopbackWorker_Data_Out_Port].current, 0x54, len );
    this_->container.advance( &this_->ports[LoopbackWorker_Data_In_Port], 0 );
    #else
    this_->container.send( &this_->ports[LoopbackWorker_Data_Out_Port], 
    &this_->ports[LoopbackWorker_Data_In_Port].current, 0x54, len );
    #endif
  */

#if 0
  this_->container.send( &this_->ports[LoopbackWorker_Data_Out_Port], 
			 &this_->ports[LoopbackWorker_Data_Out_Port].current, 0x54, len );
  this_->container.advance( &this_->ports[LoopbackWorker_Data_In_Port], 0 );
#endif

  props->startIndex++;
  // Since we have used "send" in to out, or memcpy, send, and advance, we just return OK here.
  return RCC_ADVANCE;

}



/*
 * The following code is generated by the tool
 */
#define NUM_PORTS          2
#define NUM_INPUT_PORTS    1
#define NUM_OUTPUT_PORTS   1
#define PROPERTY_SIZE      sizeof( LoopbackWorkerProperties )
static size_t memSizes[] = {sizeof(LoopbackWorkerStaticMemory), 1024*10, 0 };

#ifdef NZCOPYIO
static uint32_t portRunConditions[] = { ((1<<LoopbackWorker_Data_In_Port) | (1<<LoopbackWorker_Data_Out_Port)), 0 };
#else
// static uint32_t portRunConditions[] = { (1<<LoopbackWorker_Data_In_Port), 0, 0 };
static uint32_t portRunConditions[] = { ((1<<LoopbackWorker_Data_In_Port) | (1<<LoopbackWorker_Data_Out_Port)), 0 };
#endif
static RCCRunCondition workerRunConditions[] = { { portRunConditions,0,0 }, {0,0,0} , { 0,0,0 } };
// static RCCPortInfo portInfo = { 0, 1024*12, 1 };
RCCDispatch LoopbackWorkerDispatchTable = { RCC_VERSION, NUM_INPUT_PORTS, NUM_OUTPUT_PORTS, 
                                            PROPERTY_SIZE, memSizes, 0,
                                            initialize, 
                                            start, 
                                            stop, 
                                            release, 
                                            test, 
                                            afterConfigure, 
                                            beforeQuery, 
                                            LoopbackWorker_run,
                                            workerRunConditions, 
                                            NULL, 
                                            0,
					    0};








