
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
 * Coverage test for loopback worker using Zero copy I/O.  
 *
 * 06/23/09 - John Miller
 * Added line to delete container factory.
 *
 * 06/03/09 - John Miller
 * Initial Version 
 */

#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <OcpiTransportServer.h>
#include <OcpiTransportClient.h>
#include <OcpiRDTInterface.h>
#include <test_utilities.h>

#include <UtZeroCopyIOWorkers.h>

#include <OcpiThread.h>

using namespace OCPI::DataTransport;
using namespace DataTransport::Interface;
using namespace OCPI::Container;
using namespace OCPI;
using namespace OCPI::CONTAINER_TEST;

static const char* g_ep1    = "ocpi-smb-pio://test1:9000000.1.20";
static const char* g_ep2    = "ocpi-smb-pio://test2:9000000.2.20";
static const char* g_ep3    = "ocpi-smb-pio://test3:9000000.3.20";
static int   OCPI_RCC_DATA_BUFFER_SIZE   = 512;
static int   OCPI_USE_POLLING            = 1;

static CWorker PRODUCER(0,1),  LOOPBACK(1,1), CONSUMER(1,0);

#define PRODUCER_OUTPUT_PORT  PORT_0
#define CONSUMER_INPUT_PORT   PORT_0
#define LOOPBACK_INPUT_PORT   PORT_0
#define LOOPBACK_OUTPUT_PORT  PORT_1


static void createWorkers(std::vector<CApp>& ca )
{
  try {
    PRODUCER.worker = OCPI::CONTAINER_TEST::createWorker(ca[PRODUCER.cid], &UTZCopyProducerWorkerDispatchTable );
    LOOPBACK.worker = OCPI::CONTAINER_TEST::createWorker(ca[LOOPBACK.cid], &UTZCopyLoopbackWorkerDispatchTable );
    CONSUMER.worker = OCPI::CONTAINER_TEST::createWorker(ca[CONSUMER.cid], &UTZCopyConsumerWorkerDispatchTable );
  }
  CATCH_ALL_RETHROW( "creating workers" )

    }

static void createPorts( std::vector<CApp>& ca )
{
  ( void ) ca;
  try { 
    PRODUCER.pdata[PRODUCER_OUTPUT_PORT].port = &
      PRODUCER.worker->createOutputPort( 
                                        PRODUCER_OUTPUT_PORT,
                                        PRODUCER.pdata[PRODUCER_OUTPUT_PORT].bufferCount,
                                        OCPI_RCC_DATA_BUFFER_SIZE, NULL);                
  }
  CATCH_ALL_RETHROW( "creating producer source port" )

    try {
      CONSUMER.pdata[CONSUMER_INPUT_PORT].port = &
        CONSUMER.worker->createInputPort( 
                                         CONSUMER_INPUT_PORT,
                                         CONSUMER.pdata[CONSUMER_INPUT_PORT].bufferCount,
                                         OCPI_RCC_DATA_BUFFER_SIZE,NULL);
    }
  CATCH_ALL_RETHROW("creating consumer target port")

    try {
      LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT].port = &
        LOOPBACK.worker->createOutputPort(
                                          LOOPBACK_OUTPUT_PORT,
                                          LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT].bufferCount,
                                          OCPI_RCC_DATA_BUFFER_SIZE, NULL);
      LOOPBACK.pdata[LOOPBACK_INPUT_PORT].port = &
        LOOPBACK.worker->createInputPort(
                                         LOOPBACK_INPUT_PORT,
                                         LOOPBACK.pdata[LOOPBACK_INPUT_PORT].bufferCount,
                                         OCPI_RCC_DATA_BUFFER_SIZE,NULL);
    }
  CATCH_ALL_RETHROW( "creating loopback ports")
    }


static void connectWorkers(std::vector<CApp>& ca )
{
  ( void ) ca;
  PRODUCER.pdata[PRODUCER_OUTPUT_PORT].port->connect( *LOOPBACK.pdata[LOOPBACK_INPUT_PORT].port,0,0 );
  LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT].port->connect( *CONSUMER.pdata[CONSUMER_INPUT_PORT].port,0,0 );
}



#define BUFFERS_2_PROCESS 200;
static void initWorkerProperties(int mode, std::vector<CApp>& ca )
{
  ( void ) ca;
  int32_t  tprop[5], offset, nBytes;

  // Set the producer buffer run count property to 0
  offset = offsetof(ProducerWorkerProperties,run2BufferCount);
  nBytes = sizeof( uint32_t );
  tprop[0] = BUFFERS_2_PROCESS;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  // Set the producer buffers processed count
  offset = offsetof(ProducerWorkerProperties,buffersProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  // Set the producer bytes processed count
  offset = offsetof(ProducerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  // Set the consumer passfail property to passed
  offset = offsetof(ConsumerWorkerProperties,passfail);
  nBytes = sizeof( uint32_t );
  tprop[0] = 1;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer dropped buffers count
  offset = offsetof(ConsumerWorkerProperties,droppedBuffers);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffer run count property to 0
  offset = offsetof(ConsumerWorkerProperties,run2BufferCount);
  nBytes = sizeof( uint32_t );
  tprop[0] = BUFFERS_2_PROCESS;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffers processed count
  offset = offsetof(ConsumerWorkerProperties,buffersProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffers processed count
  offset = offsetof(ConsumerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the loopback mode
  offset = offsetof(LoopbackWorkerProperties,transferMode);
  nBytes = sizeof( uint32_t );
  tprop[0] = mode;
  LOOPBACK.worker->write( offset, nBytes, &tprop[0]);
  LOOPBACK.worker->afterConfigure();

}



static bool run_zcopy_test(std::vector<CApp>& ca, std::vector<CWorker*>& workers, int mode) 
{
  bool passed = true;

  initWorkerProperties(mode, ca);
  enableWorkers(ca, workers);

  ConsumerWorkerProperties cprops;
  ProducerWorkerProperties pprops;

  int count = 6;
  while ( count > 0 ) {

    // Read the consumer properties to monitor progress
    CONSUMER.worker->read(   0, 
			     sizeof(ConsumerWorkerProperties), 
			     &cprops);

    if ( cprops.buffersProcessed == cprops.run2BufferCount  ) {

      if ( cprops.droppedBuffers ) {
        printf("\nConsumer dropped %d buffers\n", cprops.droppedBuffers );
        passed = false;
        break;
      }

      // Make sure that the consumer got the same data
      PRODUCER.worker->read(   0, 
			       sizeof(ProducerWorkerProperties), 
			       &pprops);

      if ( cprops.bytesProcessed != pprops.bytesProcessed ) {
        printf("Producer produced %d bytes of data, consumer got %d bytes of data\n",
               pprops.bytesProcessed, cprops.bytesProcessed );
        passed = false;
        break;
      }
      else {
        break;
      }

    }
    OCPI::OS::sleep( 1000 );
    count--;
  }

  if ( count == 0 ) {
    printf("\nTest timed out\n");
    passed = false;
  }

  if ( ! passed ) {
    PRODUCER.worker->read(   0, 
			     sizeof(ProducerWorkerProperties), 
			     &pprops);
    printf("\nTest failed results:\n");
    printf("   Producer produced %d buffers, consumer received %d buffers\n", 
           pprops.buffersProcessed, cprops.buffersProcessed );
  }

  disableWorkers(ca, workers );
  return passed;
}

int run_zc_test( const char* test_name, std::vector<CApp>& ca, std::vector<CWorker*>& workers)
{
  bool test_rc;
  int testPassed = 1;

  createWorkers( ca );
  createPorts( ca );
  connectWorkers( ca );


  test_rc = run_zcopy_test(ca, workers, LBSendOnly);
  if ( test_rc == false ) testPassed = 0;
  printf("\n%s Send Only:       %s\n",  test_name, test_rc ? "PASSED" : "FAILED" );

  test_rc = run_zcopy_test(ca, workers, LBAdvanceOnly);
  if ( test_rc == false ) testPassed = 0;
  printf("\n%s Advance Only:    %s\n",  test_name, test_rc ? "PASSED" : "FAILED" );

  test_rc = run_zcopy_test(ca, workers, LBZCopyOnly);
  if ( test_rc == false ) testPassed = 0;
  printf("\n%s Zero copy only:  %s\n",  test_name, test_rc ? "PASSED" : "FAILED" );

  test_rc = run_zcopy_test(ca, workers, LBMix);
  if ( test_rc == false ) testPassed = 0;
  printf("\n%s Mixed Send:      %s\n",  test_name, test_rc ? "PASSED" : "FAILED" );

  disconnectPorts( ca, workers );
  destroyWorkers( ca, workers );

  return testPassed;

}

int config_and_run_zcopy_container_tests(std::vector<CApp>& ca, std::vector<CWorker*>& workers,
                                         int cmap[], int bcmap[] )
{
  char tnamebuf[256];
  sprintf(tnamebuf, "ZCopy Test: container map %d,%d,%d buffer map %d,%d,%d,%d",
          cmap[0], cmap[1], cmap[2], bcmap[0], bcmap[1], bcmap[2], bcmap[3] );

  PRODUCER = cmap[0];
  PRODUCER.pdata[PRODUCER_OUTPUT_PORT].bufferCount = bcmap[0];
  LOOPBACK = cmap[1];
  LOOPBACK.pdata[LOOPBACK_INPUT_PORT].bufferCount  = bcmap[1];
  LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT].bufferCount = bcmap[2];
  CONSUMER = cmap[2];
  CONSUMER.pdata[CONSUMER_INPUT_PORT].bufferCount  = bcmap[3];

  return run_zc_test( tnamebuf, ca, workers );

}


static int bcmap[][4] = {
  { 1,1,1,1 },
  { 2,2,2,2 },
  { 1,2,3,4 },
  { 4,5,10,1 },
  { 4,5,1,14 },
  { 1,1,10,5 },
  { 10,1,1,5 },
  { 1,10,10,15 },
  { 15,1,1,1 }
};


#ifdef NO_MAIN
int unit_test_zcopy_main( int argc, char** argv)
#else
int  main( int argc, char** argv)
#endif
{
  ( void ) argc;
  ( void ) argv;
  int test_rc = 1;
  DataTransfer::EventManager* event_manager;
  std::vector<const char*> endpoints;
  endpoints.push_back( g_ep1 );
  endpoints.push_back( g_ep2 );
  endpoints.push_back( g_ep3 );
  std::vector<CApp> ca;

  try {
    ca = 
      createContainers(endpoints, event_manager, (bool)OCPI_USE_POLLING);
  }
  catch( std::string& err ) {
    printf("Got a string exception while creating containers = %s\n", err.c_str() );
    exit(-1);
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("Create containers failed with exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    exit(-1);
  }
  catch( ... ) {
    printf("Got an unknown exception while creating containers\n");
    exit(-1);
  }
        
#if 0
  // Create a dispatch thread
  DThreadData tdata;
  tdata.run =1;
  tdata.containers = ca;
  tdata.event_manager = event_manager;
  OCPI::Util::Thread* t = runTestDispatch(tdata);
#endif

  std::vector<CWorker*> workers;
  workers.push_back( &PRODUCER );
  workers.push_back( &CONSUMER );
  workers.push_back( &LOOPBACK );
  

  int cmap[3];
  int bccount =0;


  cmap[0] = 0; cmap[1] = 0; cmap[2] =1;
  test_rc &= config_and_run_zcopy_container_tests(ca,workers,cmap,  bcmap[6] );

  cmap[0] = 2; cmap[1] = 0; cmap[2] =1;
  test_rc &= config_and_run_zcopy_container_tests(ca,workers,cmap,  bcmap[6] );


  for ( int i=0; i<3; i++ ) {
    cmap[0] = i;
    for ( int ii=0; ii<3; ii++ ) {
      cmap[1] = ii;
      for ( int iii=0; iii<3; iii++ ) {
        cmap[2] = iii;
        test_rc &= config_and_run_zcopy_container_tests(ca,workers,cmap,
                                                        bcmap[(bccount++)%9] );
      }
    }
  }


#if 0
  tdata.run=0;
  t->join();
#endif
  destroyContainers( ca, workers );

  return test_rc;
}




