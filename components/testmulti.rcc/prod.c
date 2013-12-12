
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
#if 0 // until we have a functioning AEP..
#include <OcpiTimeEmitC.h>
#else
#define OCPI_TIME_EMIT_C(x)
#endif
#include "ProducerWorker_Worker.h"

typedef enum  {
        P_Old_Input,
        P_New_Input
} P_MyWorkerState;

struct ProducerWorkerStaticMemory_ {
        uint32_t startIndex;
        P_MyWorkerState state;
        char overlap[100];
        uint32_t b_count;
        int32_t longProperty;
};

typedef struct ProducerWorkerStaticMemory_ ProducerWorkerStaticMemory;

static size_t memSizes[] = {sizeof(ProducerWorkerStaticMemory), 1024*10, 0 };
PRODUCERWORKER_METHOD_DECLARATIONS;
RCCDispatch ProducerWorker = {
  /* insert any custom initializations here */
  .memSizes = memSizes,
  PRODUCERWORKER_DISPATCH
};

void sleep( int n);
static ProducerWorkerProperties old_props;
static RCCResult initialize(RCCWorker *this_)
{
  int n;
  ProducerWorkerStaticMemory *mem = (ProducerWorkerStaticMemory*)this_->memories[0];
  ProducerWorkerProperties *props = this_->properties;
  mem->state = P_New_Input;
  mem->startIndex = 0;
  mem->b_count = 0;
  props->p1 = 0;
  props->p2 = 1;
  props->p3 = 2;
  props->p4 = 3;
  for ( n=0; n<5; n++ ) {
    props->p5[n] = 4+n;
  }
  props->p6 = 500;
  old_props = *props;
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
  int n;

  /* This only works for simple memory configurations */
  ProducerWorkerProperties *props = this_->properties;

  printf("In After Configure\n");
  printf("P1 = %d\n", props->p1 );
  printf("P2 = %d\n", props->p2 );
  printf("P3 = %d\n", props->p3 );
  printf("P4 = %hhd\n", props->p4 );
  for ( n=0; n<5; n++ ) {
    printf("p5[%d] = %d\n", n, props->p5[n] );
  }
  printf("P6 = %hd\n", props->p6 );
  
  return RCC_OK;
}


static RCCResult beforeQuery(RCCWorker *this_)
{
  ( void ) this_;
  return RCC_OK;
}



int g_prod_p_count=0;
void PrintProdStatus()
{
  printf("Produced buffer count = %d\n", g_prod_p_count );
}

//static int r_count = 0;
static RCCResult run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  ( void ) timedout;
  ( void ) newRunCondition;
  uint32_t n;
  uint32_t len;
  int      *b;

  ProducerWorkerStaticMemory *mem = this_->memories[0];
  RCCPort *out = &this_->ports[PRODUCERWORKER_DATA_OUT_PORT];
  
  /* Generate a simple pattern and send it */
  char* out_buffer = out->current.data;

#ifndef NDEBUG
  printf("Out maxlen = %zu\n", out->current.maxLength ); 
#endif

  OCPI_TIME_EMIT_C( "Producer Start" );

#define LIMIT_PRODUCTION__
#ifdef LIMIT_PRODUCTION
#define PP_LIMIT 2
  if ( ++r_count > PP_LIMIT ) {
    if ( r_count == (PP_LIMIT+1)) {
      printf("**** NOT PRODUCING ANY MORE DATA ****\n"); 
    }
    return RCC_OK;
  }
#endif

#ifndef NDEBUG
    printf("Producing buffer number %d\n", mem->b_count );
#endif

  len = out->current.maxLength;

#define GEN_PATTERN
#ifdef GEN_PATTERN
  b = (int*)out_buffer;
  *b = mem->b_count;
  for ( n=4; n<len; n++ ) out_buffer[n] = (char)(n+mem->b_count)%23; 
#endif

  mem->b_count++; 
  g_prod_p_count =  mem->b_count;

  OCPI_TIME_EMIT_C( "Producer Start Send" );


  /*  printf("Producer is producing with a length = %d\n", len);  */


  this_->container.send( out, &out->current, 0x54, len );

#define DELAY_AFTER_SEND__
#ifdef DELAY_AFTER_SEND
    printf("Just produced, sleeping for 1 seconds\n");
    sleep( 1 );
    printf("Done sleeping ... \n");
#endif

  out->output.length = len;
  out->output.u.operation = 0;

  OCPI_TIME_EMIT_C( "Producer End Send" );
        
  return RCC_OK;
}













