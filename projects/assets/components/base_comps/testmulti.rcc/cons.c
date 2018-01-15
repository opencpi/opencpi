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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if 0 // until we have a functioning AEP..
#include <OcpiTimeEmitC.h>
#else
#define OCPI_TIME_EMIT_C(x)
#endif
#include "ConsumerWorker_Worker.h"

typedef enum  {
        C_Old_Input,
        C_New_Input
} C_MyWorkerState;

struct ConsumerWorkerStaticMemory_ {
        uint32_t     startIndex;
        C_MyWorkerState state;
        uint32_t     b_count;
        uint32_t      longProperty;

#ifdef TIME_TP
        Timespec        startTime;
#endif

};

typedef struct ConsumerWorkerStaticMemory_ ConsumerWorkerStaticMemory;
static size_t memSizes[] = {sizeof(ConsumerWorkerStaticMemory), 1024*10, 0 };

CONSUMERWORKER_METHOD_DECLARATIONS;
RCCDispatch ConsumerWorker = {
  /* insert any custom initializations here */
  .memSizes = memSizes,
  CONSUMERWORKER_DISPATCH
};

void sleep( int n);

enum PortIds {
  ConsumerWorker_Data_In_Port=0,
  LastPort
};

static RCCResult initialize(RCCWorker *this_)
{
  ConsumerWorkerStaticMemory *mem = (ConsumerWorkerStaticMemory*)this_->memories[0];
  ConsumerWorkerProperties *props = this_->properties;
  mem->state = C_New_Input;
  mem->b_count = 0;
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
  ConsumerWorkerStaticMemory *static_mem = this_->memories[0];
  ConsumerWorkerProperties *props = this_->properties;
  static_mem->longProperty = props->longProperty;
  return RCC_OK;
}


static RCCResult beforeQuery(RCCWorker *this_)
{
  /* This only works for simple memory configurations */
  ConsumerWorkerStaticMemory *static_mem = this_->memories[0];
  ConsumerWorkerProperties *props = this_->properties;
  props->startIndex = static_mem->startIndex;
  return RCC_OK;
}

static int dropped_b=0;
static int report_er=0;
static RCCResult run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  ( void ) timedout;
  ( void ) newRunCondition;

  int ncount, *b;
  uint32_t len,n;
  ConsumerWorkerStaticMemory *mem = this_->memories[0];
  ConsumerWorkerProperties *props = this_->properties;
  int passed = 1;

#ifdef TIME_TP
  double          usecs;
  Timespec        cTime;
#endif

  char* in_buffer = (char*)this_->ports[ConsumerWorker_Data_In_Port].current.data;

  /*  printf("Cos buffer = %lld\n", in_buffer ); 
      printf("In maxlen = %d\n", this_->ports[ConsumerWorker_Data_In_Port].current.maxLength ); 
  */


    OCPI_TIME_EMIT_C( "Worker Consumer Start" );

#ifdef TIME_TP
  if ( mem->b_count == 0 ) {
    get_timestamp( &mem->startTime );
  }
#endif

  len = this_->ports[ConsumerWorker_Data_In_Port].input.length;

  /*
  printf("Consumer: got a buffer(%d) of data, len = %d first words = %d,%d \n", 
         mem->b_count, len, (int)in_buffer[0],(int)in_buffer[4] ); 
  */

  /* printf("Con: Actual len = %d, max = %d\n", len, this_->ports[ConsumerWorker_Data_In_Port].maxLength );
   */
  if ( len == 0 ) {
    printf("Error !! Got a Zero length buffer\n"); 
    return RCC_ADVANCE;
  }
  len -= 4;
  
#define CHECK_DATA
#ifdef CHECK_DATA
  b = (int*)(in_buffer);

#define RESYNC
#ifdef RESYNC
  if ( *b != (int)mem->b_count ) {
    printf("ERROR!! Dropped a buffer, got buffer %d, expected %d\n", 
           *b, mem->b_count );
    dropped_b++;
    /* resync */
    mem->b_count = *b;
  }
#endif

  ncount = 0;
  for (n=4; n<len+4; n++) {
    if ( (in_buffer[n] != (char)(n+mem->b_count)%23) && (ncount++ < 100000) ) {
      printf("Consumer(b-> %d): Data integrity error(%d) !!, expected %d, got %d\n", 
             mem->b_count,n, (char)(n+mem->b_count)%23, in_buffer[n]);
      passed = 0;
    }
  }
  props->startIndex++;
  if ( passed ) {
    if ( (mem->b_count%5000) == 0 ) { 
      printf("Buffer %d data integrity test passed\n", mem->b_count);
    } 
  }
#endif

  if ( ((report_er++)%10000) == 0 ) { 
    if ( dropped_b ) {
      printf("ERROR!! Dropped %d buffer(s)\n", dropped_b);
    }
  } 
  mem->b_count++; 
#ifdef TIME_TP
  if ( (mem->b_count%50000) == 0 ) { 
    get_timestamp( &cTime );
    usecs = elapsed_usecs ( &mem->startTime, &cTime );
    printf ( "xfer n_bytes %lld %16.4f usecs/transfer %16.4f MB/s\n",
             (unsigned long long)(mem->b_count * len),
             ( usecs / ( double ) mem->b_count ),
             ( ( double ) mem->b_count * ( double )len ) / usecs );
  }
#endif

  OCPI_TIME_EMIT_C("Consumer Start Release");
  this_->container.release( &this_->ports[ConsumerWorker_Data_In_Port].current ); 
  OCPI_TIME_EMIT_C("Consumer End Release");
  return RCC_OK;
}
