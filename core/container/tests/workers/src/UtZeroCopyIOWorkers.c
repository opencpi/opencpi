// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

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
#include <CpiTimeEmitC.h>
#endif
#include "UtZeroCopyIOWorkers.h"

enum PPortIds {
  ProducerWorker_Data_Out_Port=0,
  LastPort
};

static RCCResult ProducerInitialize(RCCWorker *this_)
{
  ProducerWorkerProperties *props = this_->properties;
  props->buffersProcessed = 0;
  props->bytesProcessed = 0;
  props->transferMode = ProducerSend;
  return RCC_OK;
}


static RCCResult release(RCCWorker *this_)
{
  return RCC_OK;
}

static RCCResult test(RCCWorker *this_ )
{
  return RCC_OK;
}


static int r_count = 0;
static RCCResult ProducerWorker_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  uint32_t n;
  uint32_t len;
  int      *b;
  ProducerWorkerProperties *props = this_->properties;
  char* out_buffer = (char*)this_->ports[ProducerWorker_Data_Out_Port].current.data;

  if ( props->buffersProcessed == props->run2BufferCount ) {
    return RCC_OK;
  }

#ifdef TIME_IT
  CPI_TIME_EMIT_C( "Producer Start" );
#endif

#ifndef NDEBUG
  printf("Producing buffer number %d\n", props->buffersProcessed );
#endif

  len = this_->ports[ProducerWorker_Data_Out_Port].current.maxLength;

#ifndef NDEBUG
  printf("Producing len = %d\n", len );
#endif

  len = len - (props->buffersProcessed%127);

  b = (int*)out_buffer;
  *b = props->buffersProcessed;
  for ( n=4; n<len; n++ ) out_buffer[n] = (char)(n+props->buffersProcessed)%23; 

  props->buffersProcessed++; 

#ifdef TIME_IT
  CPI_TIME_EMIT_C( "Producer Start Send" ); 
#endif

#ifndef NDEBUG
  printf("Producer is producing\n"); 
#endif

  this_->ports[ProducerWorker_Data_Out_Port].output.length = len;
  this_->ports[ProducerWorker_Data_Out_Port].output.u.operation = (props->buffersProcessed-1)%256;

  if ( props->transferMode == ProducerSend ) {
    this_->container->send( &this_->ports[ProducerWorker_Data_Out_Port], 
			  &this_->ports[ProducerWorker_Data_Out_Port].current, 
			    this_->ports[ProducerWorker_Data_Out_Port].output.u.operation, len );
  }
  else {
     this_->container->advance( &this_->ports[ProducerWorker_Data_Out_Port], 0 );
  }

  props->bytesProcessed += len;

#ifdef TIME_IT
  CPI_TIME_EMIT_C( "Producer End Send" ); 
#endif
	
  return RCC_OK;

}

#define PROD_PROPERTY_SIZE  sizeof( ProducerWorkerProperties )
static int32_t ProdportRunConditions[] = { (1<<ProducerWorker_Data_Out_Port) , 0 };
static RCCRunCondition RCCRunConditions[] = { {ProdportRunConditions}, {0} , {0} };

RCCDispatch UTZCopyProducerWorkerDispatchTable = { RCC_VERSION, 0, 1, 
						   PROD_PROPERTY_SIZE, 0 , 0,
						   ProducerInitialize, NULL, NULL, release, NULL, NULL, NULL, ProducerWorker_run,
						   /*workerRunConditions*/ NULL, NULL, 0 };





/************** Consumer ****************/



enum ConsumerPortIds {
  ConsumerWorker_Data_In_Port=0,
};

// #define CONSUMER_TAKE_COUNT (MIN_CONSUMER_BUFFERS-10)

#define CONSUMER_TAKE_COUNT 2
static RCCResult ConsumerInitialize(RCCWorker *this_)
{
  uint32_t i;
  ConsumerWorkerStaticMemory *mem = (ConsumerWorkerStaticMemory*)this_->memories[0];
  ConsumerWorkerProperties *props = this_->properties;
  props->buffersProcessed = 0;
  props->bytesProcessed = 0;
  props->passfail = 1;
  props->droppedBuffers = 0;
  props->transferMode = ConsumerConsume;
  props->releaseBufferIndex = 0;
  props->takenBufferIndex = 0;
  for ( i=0; i<CONSUMER_TAKE_COUNT; i++ ) {
	props->takenBuffers[i].data = NULL;
  }
  return RCC_OK;
}
		
		
		
static RCCResult ConsumerAfterConfigure( RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  ConsumerWorkerStaticMemory *static_mem = this_->memories[0];
  ConsumerWorkerProperties *props = this_->properties;
      
  return RCC_OK;
}
	
	
static RCCResult ConsumerBeforeQuery(RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  ConsumerWorkerStaticMemory *static_mem = this_->memories[0];
  ConsumerWorkerProperties *props = this_->properties;
      
  return RCC_OK;
}
	

static RCCResult ConsumerWorker_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
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

#ifdef TIME_IT
  CPI_TIME_EMIT_C( "Consumer Start" );
#endif
		    
#ifdef TIME_TP
  if ( mem->b_count == 0 ) {
    get_timestamp( &mem->startTime );
  }
#endif
		      
  len = this_->ports[ConsumerWorker_Data_In_Port].input.length;

#ifdef TESTOC
  printf("OC = %d\n", this_->ports[ConsumerWorker_Data_In_Port].input.u.operation );  
  if ( this_->ports[ConsumerWorker_Data_In_Port].input.u.operation !=  props->buffersProcessed ) {
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
  if ( *b != props->buffersProcessed ) {
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
    CPI_TIME_EMIT_C( "Consumer Start Release" );
#endif
    this_->container->release( &this_->ports[ConsumerWorker_Data_In_Port].current ); 
#ifdef TIME_IT
    CPI_TIME_EMIT_C( "Consumer End Release" );
#endif
  }
  else {

	  if ( props->takenBuffers[props->releaseBufferIndex].data ) {
        this_->container->take( &this_->ports[ConsumerWorker_Data_In_Port],
		   &props->takenBuffers[props->releaseBufferIndex], &props->takenBuffers[props->takenBufferIndex] );
        props->releaseBufferIndex =  (props->releaseBufferIndex + 1)%CONSUMER_TAKE_COUNT;
	  }
	  else {
        this_->container->take( &this_->ports[ConsumerWorker_Data_In_Port],
		   NULL, &props->takenBuffers[props->takenBufferIndex] );
	  }

      
	  // Take the buffers for a simulated sliding window algorithm
	  props->takenBufferIndex =  (props->takenBufferIndex + 1)%CONSUMER_TAKE_COUNT;
 
  }

  return RCC_OK;

}
						      				      
						      
#define CON_PROPERTY_SIZE  sizeof( ConsumerWorkerProperties )
static uint32_t memSizes[] = {sizeof(ConsumerWorkerStaticMemory), 1024*10, 0 };
static int32_t ConsumerPortRunConditions[] = { (1<<ConsumerWorker_Data_In_Port)  , 0 }; 
static RCCRunCondition workerRunConditions[] = { ConsumerPortRunConditions, 0 , 0 };
static RCCPortInfo ConsumerPortInfo[] = { {0,1024,MIN_CONSUMER_BUFFERS}, {RCC_NO_ORDINAL,0,0} };
RCCDispatch UTZCopyConsumerWorkerDispatchTable = { RCC_VERSION, 1, 0, 
						   CON_PROPERTY_SIZE, memSizes, 0, 
						   ConsumerInitialize, NULL, NULL, release, NULL, 
						   ConsumerAfterConfigure, ConsumerBeforeQuery, 
						   ConsumerWorker_run,
						   /*workerRunConditions*/ NULL, ConsumerPortInfo, 0 };
							    
							        
							    
/****************  Loop Back  *******************/
	 
	 
enum PortIds {
  LoopbackWorker_Data_In_Port=0,
  LoopbackWorker_Data_Out_Port=1,
};
	 
	 
static RCCResult LBInitialize(RCCWorker *this_)
{
  LoopbackWorkerStaticMemory *mem = (LoopbackWorkerStaticMemory*)this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;
  return RCC_OK;
}
	
	
	
static RCCResult LBAfterConfigure(RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  LoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;
  return RCC_OK;
}
	
	
static RCCResult LBBeforeQuery(RCCWorker *this_ )
{
  LoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;
  return RCC_OK;
}
	
	
static int runc=0;
static RCCResult LoopbackWorker_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  uint32_t len;
  uint32_t cplen;
    
  LoopbackWorkerStaticMemory *mem = this_->memories[0];
  LoopbackWorkerProperties *props = this_->properties;
	
  char* in_buffer = (char*)this_->ports[LoopbackWorker_Data_In_Port].current.data;
  char* out_buffer = (char*)this_->ports[LoopbackWorker_Data_Out_Port].current.data;
	    
  len = this_->ports[LoopbackWorker_Data_In_Port].input.length;
 
	      
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
		
  switch( runc ) {
		  
    // Send input buffer to output port
  case 0:
    {
      this_->container->send( &this_->ports[LoopbackWorker_Data_Out_Port], 
			      &this_->ports[LoopbackWorker_Data_In_Port].current, 0x54, len );
    }
    break;
		      
    // copy input to ouput and send data
  case 1:
    {
      // First we need to get an output buffer
      if ( ! out_buffer ) {
	this_->container->request( &this_->ports[LoopbackWorker_Data_Out_Port], 0 );
      }
      out_buffer = (char*)this_->ports[LoopbackWorker_Data_Out_Port].current.data;
      if ( ! out_buffer ) {
	runc--;
	return RCC_OK;
      }
	  cplen = (len < this_->ports[LoopbackWorker_Data_Out_Port].current.maxLength )
		  ? len : this_->ports[LoopbackWorker_Data_Out_Port].current.maxLength;
      memcpy(out_buffer,in_buffer,cplen);
      this_->container->send( &this_->ports[LoopbackWorker_Data_Out_Port], 
			      &this_->ports[LoopbackWorker_Data_Out_Port].current, 0x54, len );
      this_->container->release( &this_->ports[LoopbackWorker_Data_In_Port].current );
    }
    break;
		      
		      
    // copy input to output and used RCC_ADVANCE
  case 2:
    {
      // First we need to get an output buffer
      if ( ! out_buffer ) {
	this_->container->request( &this_->ports[LoopbackWorker_Data_Out_Port], 0 );
      }
      out_buffer = (char*)this_->ports[LoopbackWorker_Data_Out_Port].current.data;
      this_->ports[LoopbackWorker_Data_Out_Port].output.length = len;
      if ( ! out_buffer ) {
	runc--;
	return RCC_OK;
      }
     cplen = (len < this_->ports[LoopbackWorker_Data_Out_Port].current.maxLength )
		  ? len : this_->ports[LoopbackWorker_Data_Out_Port].current.maxLength;
      memcpy(out_buffer,in_buffer,cplen);
      return RCC_ADVANCE;
    }
    break;
		      
  }
		  
  return RCC_OK;
}
		    
		    
		    
		    
/*
 * The following code is generated by the tool
 */
	 
#define LB_PROPERTY_SIZE      sizeof( LoopbackWorkerProperties )
static uint32_t LBmemSizes[] = {sizeof(LoopbackWorkerStaticMemory), 0 };
static int32_t LBPortRunConditions[] = { (1<<LoopbackWorker_Data_In_Port) /* | (1<<LoopbackWorker_Data_Out_Port)*/ , 0 };
static RCCRunCondition LBWorkerRunConditions[] = { LBPortRunConditions, 0 , 0 };
static RCCPortInfo portInfo[] = { {1,1024,2}, {RCC_NO_ORDINAL,0,0} };
RCCDispatch UTZCopyLoopbackWorkerDispatchTable = { RCC_VERSION, 1, 1, 
						   LB_PROPERTY_SIZE, LBmemSizes, 0,
						   LBInitialize, NULL, NULL, release, NULL, LBAfterConfigure, LBBeforeQuery, 
						   LoopbackWorker_run,
						   LBWorkerRunConditions, NULL, 0};


DllDispatchEntry ZeroCopyWorkerDispatchTables[] = {
  {"Consumer", &UTZCopyConsumerWorkerDispatchTable},
  {"Loopback", &UTZCopyLoopbackWorkerDispatchTable},
  {"Producer", &UTZCopyProducerWorkerDispatchTable},
  {NULL,NULL}
};


























