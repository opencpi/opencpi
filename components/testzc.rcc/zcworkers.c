
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


/*
 * Zero copy I/O workers for test
 *
 * 06/01/09 - John Miller
 * Initial Version
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef TIME_IT
#include <OcpiTimeEmitC.h>
#endif
#include "Consumer_Worker.h"
#include "Loopback_Worker.h"
#include "Producer_Worker.h"

/************** Producer ****************/

PRODUCER_METHOD_DECLARATIONS;
RCCDispatch Producer = {
  /* insert any custom initializations here */
  PRODUCER_DISPATCH
};

#define ProducerSend    0
#define ProducerAdvance 1

static RCCResult ProducerInitialize(RCCWorker *this_)
{
  ProducerProperties *props = this_->properties;
  props->buffersProcessed = 0;
  props->bytesProcessed = 0;
  props->transferMode = ProducerSend;
  return RCC_OK;
}

#if 0
// We don't support shared methods yet.
static RCCResult release(RCCWorker *this_)
{
  return RCC_OK;
}

static RCCResult test(RCCWorker *this_ )
{
  return RCC_OK;
}
#endif


//static int r_count = 0;
static RCCResult ProducerRun(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  ( void ) timedout;
  ( void ) newRunCondition;
  uint32_t n;
  uint32_t len;
  int      *b;
  ProducerProperties *props = this_->properties;
  RCCPort *out = &this_->ports[PRODUCER_OUT];
  char* out_buffer = out->current.data;

  if ( props->buffersProcessed == props->run2BufferCount ) {
    return RCC_OK;
  }

#ifdef TIME_IT
  OCPI_TIME_EMIT_C( "Producer Start" );
#endif

#ifndef NDEBUG
  printf("Producing buffer number %d\n", props->buffersProcessed );
#endif

  len = out->current.maxLength;

#ifndef NDEBUG
  printf("Producing len = %d\n", len );
#endif

  len = len - (props->buffersProcessed%127);

  b = (int*)out_buffer;
  *b = props->buffersProcessed;
  for ( n=4; n<len; n++ ) out_buffer[n] = (char)(n+props->buffersProcessed)%23; 

  props->buffersProcessed++; 

#ifdef TIME_IT
  OCPI_TIME_EMIT_C( "Producer Start Send" ); 
#endif

#ifndef NDEBUG
  printf("Producer is producing\n"); 
#endif

  out->output.length = len;
  out->output.u.operation = (props->buffersProcessed-1)%256;

  if ( props->transferMode == ProducerSend ) {
    this_->container.send(out, &out->current, out->output.u.operation, len);
  }
  else {
     this_->container.advance( out, 0 );
  }

  props->bytesProcessed += len;

#ifdef TIME_IT
  OCPI_TIME_EMIT_C( "Producer End Send" ); 
#endif
        
  return RCC_OK;

}

/************** Consumer ****************/

#define MIN_CONSUMER_BUFFERS 4
#define ConsumerConsume 0 
#define ConsumerTake    1

struct ConsumerStaticMemory_ {
  uint32_t     place_holder;
#ifdef TIME_TP
  Timespec        startTime;
#endif
  RCCBuffer takenBuffers[MIN_CONSUMER_BUFFERS];
};
typedef struct ConsumerStaticMemory_ ConsumerStaticMemory;

static uint32_t memSizes[] = {sizeof(ConsumerStaticMemory), 0 };
static RCCPortInfo ConsumerPortInfo[] = { {0,0,MIN_CONSUMER_BUFFERS}, {RCC_NO_ORDINAL,0,0} };

CONSUMER_METHOD_DECLARATIONS;
RCCDispatch Consumer = {
  /* insert any custom initializations here */
  .memSizes = memSizes,
  .portInfo = ConsumerPortInfo,
  CONSUMER_DISPATCH
};

// #define CONSUMER_TAKE_COUNT (MIN_CONSUMER_BUFFERS-10)

#define CONSUMER_TAKE_COUNT 2
static RCCResult ConsumerInitialize(RCCWorker *this_)
{
  uint32_t i;
  ConsumerStaticMemory *mem = (ConsumerStaticMemory*)this_->memories[0];
  ConsumerProperties *props = this_->properties;
  props->buffersProcessed = 0;
  props->bytesProcessed = 0;
  props->passfail = 1;
  props->droppedBuffers = 0;
  props->transferMode = ConsumerConsume;
  props->releaseBufferIndex = 0;
  props->takenBufferIndex = 0;
  for ( i=0; i<CONSUMER_TAKE_COUNT; i++ ) {
        mem->takenBuffers[i].data = NULL;
  }
  return RCC_OK;
}
                
static RCCResult ConsumerAfterConfigure( RCCWorker *this_ )
{
  //  ConsumerStaticMemory *static_mem = this_->memories[0];

#ifdef DEBUG_PROPS
  ConsumerProperties *props = this_->properties;
  printf("doubleT value = %f", props->doubleT);
  printf("passfail value = %d", props->passfail);
  printf("run2BufferCount value = %d", props->run2BufferCount);
  printf("longlongT value = %lld", props->longlongT);
  printf("buffersProcessed value = %d", props->buffersProcessed);
  printf("droppedBuffers value = %d", props->droppedBuffers);  
  printf("bytesProcessed value = %d", props->bytesProcessed);  
#else
  ( void ) this_;
#endif
      
  return RCC_OK;
}
        
        
static RCCResult ConsumerBeforeQuery(RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  //  ConsumerStaticMemory *static_mem = this_->memories[0];
  //  ConsumerProperties *props = this_->properties;
  ( void ) this_;    
  return RCC_OK;
}
        

static RCCResult ConsumerRun(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  ( void ) timedout;
  ( void ) newRunCondition;
  int ncount, *b;
  uint32_t len,n;
  ConsumerStaticMemory *mem = this_->memories[0];
  ConsumerProperties *props = this_->properties;
  RCCPort *in = &this_->ports[CONSUMER_IN];
  int passed = 1;

            
#ifdef TIME_TP
  double          usecs;
  Timespec        cTime;
#endif
                
  char* in_buffer = in->current.data;

#ifdef TIME_IT
  OCPI_TIME_EMIT_C( "Consumer Start" );
#endif
                    
#ifdef TIME_TP
  if ( mem->b_count == 0 ) {
    get_timestamp( &mem->startTime );
  }
#endif
                      
  len = in->input.length;

#ifdef TESTOC
  printf("OC = %d\n", in->input.u.operation );  
  if ( in->input.u.operation !=  props->buffersProcessed ) {
    passed = 0;
    printf("ERROR!! op code is not correct !!\n");
  }
#endif

  props->bytesProcessed += len;
                          
  if ( len == 0 ) {
#ifndef NDEBUG
    printf("Error !! Got a Zero length buffer\n"); 
#endif
    props->droppedBuffers++;
    return RCC_ADVANCE;
  }
  len -= 4;
                              
  b = (int*)(in_buffer);
  if ( *b != (int)props->buffersProcessed ) {
#ifndef NDEBUG
    printf("ERROR!! Dropped a buffer, got buffer %d, expected %d\n", 
           *b, props->buffersProcessed );
#endif     
    props->droppedBuffers++;
    /* resync */
    props->buffersProcessed = *b;
  }
                                  
  ncount = 0;
  for (n=4; n<len+4; n++) {
    if ( (in_buffer[n] != (char)(n+props->buffersProcessed)%23) && (ncount++ < 100) ) {
                                        
#ifndef NDEBUG
      printf("\nConsumer(b-> %d): Data integrity error(%d) !!, expected %d, got %d\n", 
             props->buffersProcessed,n, (char)(n+props->buffersProcessed)%23, in_buffer[n]);
#endif
      passed = 0;
    }
  }
  if ( passed ) {
#ifndef NDEBUG
    printf("Buffer %d data integrity test passed\n", props->buffersProcessed); 
#endif
  }
  else {
    props->passfail = 0;
  }
                                        
  if ( props->droppedBuffers ) {
#ifndef NDEBUG
    printf("ERROR!! Dropped %d buffer(s)\n", props->droppedBuffers);
#endif
  } 
                                          
  props->buffersProcessed++; 
                                            
#ifdef TIME_TP
  if ( (props->buffersProcessed%50000) == 0 ) { 
    get_timestamp( &cTime );
    usecs = elapsed_usecs ( &mem->startTime, &cTime );
    printf ( "xfer n_bytes %lld %16.4f usecs/transfer %16.4f MB/s\n",
             (unsigned long long)(mem->b_count * len),
             ( usecs / ( double ) mem->b_count ),
             ( ( double ) mem->b_count * ( double )len ) / usecs );
  }
#endif
                                
  if ( props->transferMode == ConsumerConsume ) {
#ifdef TIME_IT
    OCPI_TIME_EMIT_C( "Consumer Start Release" );
#endif
    this_->container.advance( in,0 ); 
#ifdef TIME_IT
    OCPI_TIME_EMIT_C( "Consumer End Release" );
#endif
  }
  else {

          if ( mem->takenBuffers[props->releaseBufferIndex].data ) {
        this_->container.take( in,
                   &mem->takenBuffers[props->releaseBufferIndex], &mem->takenBuffers[props->takenBufferIndex] );
        props->releaseBufferIndex =  (props->releaseBufferIndex + 1)%CONSUMER_TAKE_COUNT;
          }
          else {
        this_->container.take( in,
                   NULL, &mem->takenBuffers[props->takenBufferIndex] );
          }

      
          // Take the buffers for a simulated sliding window algorithm
          props->takenBufferIndex =  (props->takenBufferIndex + 1)%CONSUMER_TAKE_COUNT;
 
  }

  return RCC_OK;

}
                                                            
/****************  Loop Back  *******************/
         
struct LoopbackStaticMemory_ {
  uint32_t ph;
};
typedef struct LoopbackStaticMemory_ LoopbackStaticMemory;
static uint32_t LBmemSizes[] = {sizeof(LoopbackStaticMemory), 0 };
// This doesn't do anything, but it is a good compilation test
static uint32_t LBPortRunConditionMasks[] = { (1<<LOOPBACK_IN) | (1<<LOOPBACK_OUT), 0 };
static RCCRunCondition LBWorkerRunCondition = { LBPortRunConditionMasks, 0 , 0 };

LOOPBACK_METHOD_DECLARATIONS;
RCCDispatch Loopback = {
  /* insert any custom initializations here */
  .memSizes = LBmemSizes,
  .runCondition = &LBWorkerRunCondition,
  LOOPBACK_DISPATCH
};
         
static RCCResult LoopbackInitialize(RCCWorker *this_)
{
  //LoopbackStaticMemory *mem = this_->memories[0];
  //LoopbackProperties *props = this_->properties;
  ( void ) this_;
  return RCC_OK;
}
        
        
        
static RCCResult LoopbackAfterConfigure(RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  //LoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  //LoopbackWorkerProperties *props = this_->properties;
  ( void ) this_;
  return RCC_OK;
}
        
        
static RCCResult LoopbackBeforeQuery(RCCWorker *this_ )
{
  //LoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  //LoopbackWorkerProperties *props = this_->properties;
  ( void ) this_;
  return RCC_OK;
}
        
        
static int runc=1;
static RCCResult LoopbackRun(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  ( void ) timedout;
  ( void ) newRunCondition;
  uint32_t len;
  uint32_t cplen;
  int      oc;
    
  //LoopbackStaticMemory *mem = this_->memories[0];
  //LoopbackProperties *props = this_->properties;
  RCCPort
    *in = &this_->ports[LOOPBACK_IN],
    *out = &this_->ports[LOOPBACK_IN];
        
  char* in_buffer = in->current.data;
  char* out_buffer = out->current.data;
            
  len = in->input.length;
  oc  = in->input.u.operation;

#ifndef NDEBUG
  printf("In LB run, len = %d\n", len);
#endif

  //  uint32_t * d = (uint32_t*)in_buffer;

  /*
  printf("Data words = %d, %d, %d, %d\n", d[0], d[1], d[2], d[3] );
  */
 

  /*              
  switch ( props->transferMode ) {
  case LBZCopyOnly:
    runc = 0;
    break;
                    
  case LBAdvanceOnly:
    runc = 2;
    break;
                    
  case LBSendOnly:
    runc = 1;
    break;
                    
  case LBMix:
    runc = (++runc)%3;
    break;                
  }
  */

  runc=1;
                
  switch( runc ) {
                  
    // Send input buffer to output port
  case 0:
    {
      this_->container.send( out, &in->current, 0x54, len );
    }
    break;
                      
    // copy input to ouput and send data
  case 1:
    {
      // First we need to get an output buffer
      if ( ! out_buffer ) {
        this_->container.request( out, 0 );
      }
      out_buffer = out->current.data;
      if ( ! out_buffer ) {
        runc--;
        return RCC_OK;
      }
      cplen = (len < out->current.maxLength )
	? len : out->current.maxLength;
      memcpy(out_buffer,in_buffer,cplen);

      /*      printf("NOT SENDING LB DATA FOR DEBUG!!\n"); */

      this_->container.send( out, &out->current, oc, len );



      this_->container.advance(in, 0 );
    }
    break;
                      
                      
    // copy input to output and used RCC_ADVANCE
  case 2:
    {
      // First we need to get an output buffer
      if ( ! out_buffer ) {
        this_->container.request( out, 0 );
      }
      out_buffer = out->current.data;
      out->output.length = len;
      if ( ! out_buffer ) {
        runc--;
        return RCC_OK;
      }
     cplen = (len < out->current.maxLength )
                  ? len : out->current.maxLength;
      memcpy(out_buffer,in_buffer,cplen);
      return RCC_ADVANCE;
    }
    break;
                      
  }
                  
  return RCC_OK;
}




























