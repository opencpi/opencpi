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
 * Generic Loopback workers used for various tests
 *
 * 06/23/09 - John Miller
 * Added code to request buffer if one is not available.
 *
 * 06/01/09 - John Miller
 * Initial Version
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <CpiTimeEmitC.h>
#include "UtGenericLoopbackWorkers.h"


enum PPortIds {
  UTGProducerWorker_Data_Out_Port0=0,
  UTGProducerWorker_Data_Out_Port1,
  UTGProducerWorker_Data_Out_Port2,
  LastPort
};

static RCCResult UTGProducerInitialize(RCCWorker *this_)
{
  UTGProducerWorkerProperties *props = this_->properties;
  props->buffersProcessed = 0;
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


#ifdef TEST_ONE_SHOT
volatile int PROD_PRODUCE=0;
#endif

static RCCResult UTGProducerWorker_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  uint32_t n;
  uint32_t len;
  int      *b;
  UTGProducerWorkerProperties *props = this_->properties;
  char* out_buffer = (char*)this_->ports[UTGProducerWorker_Data_Out_Port0].current.data;

#ifdef TEST_ONE_SHOT
  if ( PROD_PRODUCE == 0 ) {
    return RCC_OK;
  }
  PROD_PRODUCE = 0;
#endif


  if ( ! out_buffer ) {
    this_->container->request( &this_->ports[UTGProducerWorker_Data_Out_Port0], 0 );
  }
  out_buffer = (char*)this_->ports[UTGProducerWorker_Data_Out_Port0].current.data;
  if ( ! out_buffer ) {
    return RCC_OK;
  }

  if ( props->buffersProcessed == props->run2BufferCount ) {
    return RCC_OK;
  }


  CPI_TIME_EMIT_C( "Producer Start" );

#ifndef NDEBUG
  printf("Producing buffer number %d\n", props->buffersProcessed );
#endif

  len = this_->ports[UTGProducerWorker_Data_Out_Port0].current.maxLength;

#ifndef NDEBUG
  printf("lenght = %d\n", len);
#endif

  b = (int*)out_buffer;
  *b = props->buffersProcessed;
  for ( n=4; n<len; n++ ) out_buffer[n] = (char)(n+props->buffersProcessed)%23; 

  props->buffersProcessed++; 
 
  CPI_TIME_EMIT_C( "Producer Start Send" );

#ifndef NDEBUG
  printf("Producer is producing\n"); 
#endif

  this_->ports[UTGProducerWorker_Data_Out_Port0].output.length = len;
  this_->ports[UTGProducerWorker_Data_Out_Port0].output.u.operation = props->buffersProcessed%256;
  this_->container->send( &this_->ports[UTGProducerWorker_Data_Out_Port0], 
			  &this_->ports[UTGProducerWorker_Data_Out_Port0].current, 0x54, len );

  CPI_TIME_EMIT_C( "Producer Start End" );
	
  return RCC_OK;

}

#define PROD_PROPERTY_SIZE  sizeof( UTGProducerWorkerProperties )
static int32_t ProdportRunConditions[] = { (1<<UTGProducerWorker_Data_Out_Port0) , 0 };
static RCCRunCondition RCCRunConditions[] = { {ProdportRunConditions}, {0} , {0} };
static RCCPortInfo ProdPortInfo[] = { {0, 1024*2, 1}, {RCC_NO_ORDINAL, 0, 0} };
static int32_t UTGProducerPortRunConditions[] = { (1<<UTGProducerWorker_Data_Out_Port0) , 0 };
static RCCRunCondition UTGProducerWorkerRunConditions[] = { UTGProducerPortRunConditions, 0 , 0 };
#define NUM_OUTPUTS 3
#define OPTIONAL_CONNECTIONS_MASK 0xe
RCCDispatch UTGProducerWorkerDispatchTable = { RCC_VERSION, 0, NUM_OUTPUTS, 
					       PROD_PROPERTY_SIZE, 0 , 0,
					       UTGProducerInitialize, NULL, NULL, release, NULL, NULL, NULL, UTGProducerWorker_run,
					       UTGProducerWorkerRunConditions, NULL, OPTIONAL_CONNECTIONS_MASK };





/************** UTGConsumer ****************/



enum UTGConsumerPortIds {
  UTGConsumerWorker_Data_In_Port0=0,
  UTGConsumerWorker_Data_In_Port1=1,
  UTGConsumerWorker_Data_In_Port2=2,
  UTGConsumerWorker_Data_In_Port3=3
};


static RCCResult UTGConsumerInitialize(RCCWorker *this_)
{
  UTGConsumerWorkerStaticMemory *mem = (UTGConsumerWorkerStaticMemory*)this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;
  props->buffersProcessed = 0;
  props->passfail = 1;
  props->droppedBuffers = 0;
  if ( props->testControlErrors == 1 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_ERROR;
  }
  else if ( props->testControlErrors == 2 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_FATAL;
  }
  return RCC_OK;
}



static RCCResult UTGConsumerStart(RCCWorker *this_)
{
  UTGConsumerWorkerStaticMemory *mem = (UTGConsumerWorkerStaticMemory*)this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;
  if ( props->testControlErrors == 1 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_ERROR;
  }
  else if ( props->testControlErrors == 2 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_FATAL;
  }
  return RCC_OK;
}



static RCCResult UTGConsumerStop(RCCWorker *this_)
{
  UTGConsumerWorkerStaticMemory *mem = (UTGConsumerWorkerStaticMemory*)this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;
  if ( props->testControlErrors == 1 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_ERROR;
  }
  else if ( props->testControlErrors == 2 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_FATAL;
  }

  return RCC_OK;
}


static RCCResult UTGConsumerRelease(RCCWorker *this_)
{
  UTGConsumerWorkerStaticMemory *mem = (UTGConsumerWorkerStaticMemory*)this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;
  if ( props->testControlErrors == 1 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_ERROR;
  }
  else if ( props->testControlErrors == 2 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_FATAL;
  }

  return RCC_OK;
}


static RCCResult UTGConsumerTest(RCCWorker *this_)
{
  UTGConsumerWorkerStaticMemory *mem = (UTGConsumerWorkerStaticMemory*)this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;
  if ( props->testControlErrors == 1 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_ERROR;
  }
  else if ( props->testControlErrors == 2 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_FATAL;
  }

  return RCC_OK;
}




static RCCResult UTGConsumerAfterConfigure( RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  UTGConsumerWorkerStaticMemory *static_mem = this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;

  if ( props->testConfigErrors == 1 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_ERROR;
  }
  else if ( props->testControlErrors == 2 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_FATAL;
  }

  return RCC_OK;
}


static RCCResult UTGConsumerBeforeQuery(RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  UTGConsumerWorkerStaticMemory *static_mem = this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;

  if ( props->testConfigErrors == 1 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_ERROR;
  }
  else if ( props->testControlErrors == 2 ) {
    this_->errorString = ERROR_TEST_STRING;
    return RCC_FATAL;
  }

  return RCC_OK;
}


static RCCResult UTGConsumerWorker_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  int ncount, *b;
  uint32_t len,n;
  UTGConsumerWorkerStaticMemory *mem = this_->memories[0];
  UTGConsumerWorkerProperties *props = this_->properties;
  int passed = 1;

#ifdef TIME_TP
  double          usecs;
  Timespec        cTime;
#endif

  char* in_buffer = (char*)this_->ports[UTGConsumerWorker_Data_In_Port0].current.data;

  CPI_TIME_EMIT_C( "Consumer Start" );

#ifdef TIME_TP
  if ( mem->b_count == 0 ) {
    get_timestamp( &mem->startTime );
  }
#endif

  len = this_->ports[UTGConsumerWorker_Data_In_Port0].input.length;

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
    if ( (in_buffer[n] != (char)(n+props->buffersProcessed)%23) && (ncount++ < 100000) ) {
      printf("UTGConsumer(b-> %d): Data integrity error(%d) !!, expected %d, got %d\n", 
	     props->buffersProcessed,n, (char)(n+props->buffersProcessed)%23, in_buffer[n]);
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

  CPI_TIME_EMIT_C( "Consumer Start Release" );
  this_->container->release( &this_->ports[UTGConsumerWorker_Data_In_Port0].current ); 
  CPI_TIME_EMIT_C( "Consumer End Release" );
  return RCC_OK;
}


/*
 * The following code is generated by the tool
 */

#define CON_PROPERTY_SIZE  sizeof( UTGConsumerWorkerProperties )
static uint32_t memSizes[] = {sizeof(UTGConsumerWorkerStaticMemory), 1024*10, 0 };
static int32_t CPortRunConditions[] = { (1<<UTGConsumerWorker_Data_In_Port0)  , 0 }; 
static RCCRunCondition CWorkerRunConditions[] = { CPortRunConditions, 0 , 0 };
#define CoptionalPorts (						\
			(1<<UTGConsumerWorker_Data_In_Port1) | (1<<UTGConsumerWorker_Data_In_Port2) \
			| (1<<UTGConsumerWorker_Data_In_Port3) )
static RCCPortInfo UTGConsumerPortInfo[] = { {0, 1024*2, 3}, {RCC_NO_ORDINAL, 0, 0} };
#define COPTIONAL_CONNECTIONS_MASK 0xe
RCCDispatch UTGConsumerWorkerDispatchTable = { RCC_VERSION, 4, 0, 
					       CON_PROPERTY_SIZE, memSizes, 0, 
					       UTGConsumerInitialize, UTGConsumerStart, UTGConsumerStop, UTGConsumerRelease, UTGConsumerTest, 
					       UTGConsumerAfterConfigure, UTGConsumerBeforeQuery, 
					       UTGConsumerWorker_run,
					       /*CWorkerRunConditions*/NULL, NULL, COPTIONAL_CONNECTIONS_MASK };












/****************  Loop Back  *******************/


enum PortIds {
  UTGLoopbackWorker_Data_In_Port0  =0,
  UTGLoopbackWorker_Data_In_Port1  =1,
  UTGLoopbackWorker_Data_Out_Port0 =2,
  UTGLoopbackWorker_Data_Out_Port1 =3,
  UTGLoopbackWorker_Data_Out_Port2 =4,
};


static RCCResult LBInitialize(RCCWorker *this_)
{
  UTGLoopbackWorkerStaticMemory *mem = (UTGLoopbackWorkerStaticMemory*)this_->memories[0];
  UTGLoopbackWorkerProperties *props = this_->properties;
  return RCC_OK;
}



static RCCResult LBAfterConfigure(RCCWorker *this_ )
{
  /* This only works for simple memory configurations */
  UTGLoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  UTGLoopbackWorkerProperties *props = this_->properties;
  return RCC_OK;
}


static RCCResult LBBeforeQuery(RCCWorker *this_ )
{
  UTGLoopbackWorkerStaticMemory *static_mem = this_->memories[0];
  UTGLoopbackWorkerProperties *props = this_->properties;
  return RCC_OK;
}


static int runc=0;
static RCCResult UTGLoopbackWorker_run(RCCWorker *this_,RCCBoolean timedout,RCCBoolean *newRunCondition)
{
  uint32_t len;

  UTGLoopbackWorkerStaticMemory *mem = this_->memories[0];
  UTGLoopbackWorkerProperties *props = this_->properties;

  char* in_buffer = (char*)this_->ports[UTGLoopbackWorker_Data_In_Port0].current.data;
  char* out_buffer = (char*)this_->ports[UTGLoopbackWorker_Data_Out_Port0].current.data;

  len = this_->ports[UTGLoopbackWorker_Data_In_Port0].input.length;
  runc = 0;

  switch( runc ) {

    // Send input buffer to output port
  case 0:
    {
      this_->container->send( &this_->ports[UTGLoopbackWorker_Data_Out_Port0], 
			      &this_->ports[UTGLoopbackWorker_Data_In_Port0].current, 0x54, len );
    }
    break;

    // copy input to ouput and send data
  case 1:
    {
      // First we need to get an output buffer
      if ( ! out_buffer ) {
	this_->container->request( &this_->ports[UTGLoopbackWorker_Data_Out_Port0], 0 );
      }
      out_buffer = (char*)this_->ports[UTGLoopbackWorker_Data_Out_Port0].current.data;
      if ( ! out_buffer ) {
	runc--;
	return RCC_OK;
      }
      memcpy(out_buffer,in_buffer,len);
      this_->container->send( &this_->ports[UTGLoopbackWorker_Data_Out_Port0], 
			      &this_->ports[UTGLoopbackWorker_Data_Out_Port0].current, 0x54, len );
      this_->container->release( &this_->ports[UTGLoopbackWorker_Data_In_Port0].current );
    }
    break;


    // copy input to output and used RCC_ADVANCE
  case 2:
    {
      // First we need to get an output buffer
      if ( ! out_buffer ) {
	this_->container->request( &this_->ports[UTGLoopbackWorker_Data_Out_Port0], 0 );
      }
      out_buffer = (char*)this_->ports[UTGLoopbackWorker_Data_Out_Port0].current.data;
      this_->ports[UTGLoopbackWorker_Data_Out_Port0].output.length = len;
      if ( ! out_buffer ) {
	runc--;
	return RCC_OK;
      }
      memcpy(out_buffer,in_buffer,len);
      return RCC_ADVANCE;
    }
    break;

  }

  return RCC_OK;
}




/*
 * The following code is generated by the tool
 */

#define LB_PROPERTY_SIZE      sizeof( UTGLoopbackWorkerProperties )
static uint32_t LBmemSizes[] = {sizeof(UTGLoopbackWorkerStaticMemory), 0 };
static int32_t LBPortRunConditions[] = { (1<<UTGLoopbackWorker_Data_In_Port0) /* | (1<<UTGLoopbackWorker_Data_Out_Port)*/ , 0 };
static RCCRunCondition LBWorkerRunConditions[] = { LBPortRunConditions, 0 , 0 };
static RCCPortInfo portInfo[] = { {0, 1024*3, 1}, {RCC_NO_ORDINAL, 0, 0} };
RCCDispatch UTGLoopbackWorkerDispatchTable = { RCC_VERSION, 2, 3, 
					       LB_PROPERTY_SIZE, LBmemSizes, 0,
					       LBInitialize, NULL, NULL, release, NULL, LBAfterConfigure, LBBeforeQuery, 
					       UTGLoopbackWorker_run,
					       LBWorkerRunConditions, NULL, 
					       (1<<UTGLoopbackWorker_Data_In_Port1)  |
					       (1<<UTGLoopbackWorker_Data_Out_Port1) |
					       (1<<UTGLoopbackWorker_Data_Out_Port2) };



























