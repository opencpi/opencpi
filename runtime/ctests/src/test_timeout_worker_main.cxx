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
 * Container worker timeout test.
 *
 * 06/10/09 - John Miller
 * Initial version
 */

#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <OcpiRDTInterface.h>
#include <test_utilities.h>
#include <OcpiUtilCommandLineConfiguration.h>

#include "UtGenericLoopbackWorkers.h"

#include <OcpiThread.h>
using namespace OCPI::Container;
using namespace OCPI;
using namespace OCPI::CONTAINER_TEST;

static const char* g_ep[]    =  {
  "ocpi-smb-pio:test1:900000.1.20",
  "ocpi-smb-pio:test2:900000.2.20",
  "ocpi-smb-pio:test3:900000.3.20"
};
static int   OCPI_USE_POLLING            = 1;

static CWorker PRODUCER(0,3), LOOPBACK(2,3), CONSUMER(4,0);

#define PRODUCER_OUTPUT_PORT0  PORT_0
#define CONSUMER_INPUT_PORT0   PORT_0
#define CONSUMER_INPUT_PORT1   PORT_1
#define CONSUMER_INPUT_PORT2   PORT_2
#define LOOPBACK_INPUT_PORT0   PORT_0
#define LOOPBACK_INPUT_PORT1   PORT_1
#define LOOPBACK_OUTPUT_PORT0  PORT_2
#define LOOPBACK_OUTPUT_PORT1  PORT_3
#define LOOPBACK_OUTPUT_PORT2  PORT_4


class OcpiRccBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccBinderConfigurator ();

public:
  bool help;
  bool verbose;
  MultiString  endpoints;

private:
  static CommandLineConfiguration::Option g_options[];
};

OcpiRccBinderConfigurator::
OcpiRccBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccBinderConfigurator::g_options[] = {

  { OCPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
    "endpoints", "container endpoints",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::endpoints), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::help), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::END, 0, 0, 0, 0 }
};

static
void
printUsage (OcpiRccBinderConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}




static void createWorkers(std::vector<CApp>& ca )
{
  try {
    PRODUCER.worker = OCPI::CONTAINER_TEST::createWorker(ca[PRODUCER.cid], &UTGProducerWorkerDispatchTable );
    LOOPBACK.worker = OCPI::CONTAINER_TEST::createWorker(ca[LOOPBACK.cid], &UTGLoopbackWorkerDispatchTable );
    CONSUMER.worker = OCPI::CONTAINER_TEST::createWorker(ca[CONSUMER.cid], &UTGConsumerWorkerDispatchTable );
  }
  CATCH_ALL_RETHROW( "creating workers" )
    }


static int BUFFERS_2_PROCESS = 2000000;
static void initWorkerProperties( std::vector<CApp>& ca )
{
  ( void ) ca;
  int32_t  tprop[5], offset, nBytes;
  std::string err_str;


  // Set the producer buffer run count property to 0
  offset = offsetof(UTGProducerWorkerProperties,run2BufferCount);
  nBytes = sizeof( uint32_t );
  tprop[0] = BUFFERS_2_PROCESS;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  // Set the producer buffers processed count
  offset = offsetof(UTGProducerWorkerProperties,buffersProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  // Set the producer bytes processed count
  offset = offsetof(UTGProducerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  // Set the consumer passfail property to passed
  offset = offsetof(UTGConsumerWorkerProperties,passfail);
  nBytes = sizeof( uint32_t );
  tprop[0] = 1;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer dropped buffers count
  offset = offsetof(UTGConsumerWorkerProperties,droppedBuffers);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffer run count property to 0
  offset = offsetof(UTGConsumerWorkerProperties,run2BufferCount);
  nBytes = sizeof( uint32_t );
  tprop[0] = BUFFERS_2_PROCESS;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffers processed count
  offset = offsetof(UTGConsumerWorkerProperties,buffersProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffers processed count
  offset = offsetof(UTGConsumerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();
}



static bool run_lb_test(std::vector<CApp>& ca, std::vector<CWorker*>& workers, int& buffers )
{
  bool passed = true;

  try {
    initWorkerProperties(ca);
  }
  catch( ... ) {
    printf("Failed to init worker properties\n");
    throw;
  }

  try {
    enableWorkers(ca, workers);
  }
  catch( ... ) {
    printf("Failed to enable workers\n");
    throw;
  }

  UTGConsumerWorkerProperties cprops;
  UTGProducerWorkerProperties pprops;

  int count = 6;
  while ( count > 0 ) {

    // Read the consumer properties to monitor progress
    CONSUMER.worker->read(   0,
			     sizeof(UTGConsumerWorkerProperties),
			     &cprops);

    if ( cprops.buffersProcessed == cprops.run2BufferCount  ) {

      if ( cprops.droppedBuffers ) {
        printf("\nConsumer dropped %d buffers\n", cprops.droppedBuffers );
        passed = false;
        break;
      }

      // Make sure that the consumer got the same data
      PRODUCER.worker->read(   0,
			       sizeof(UTGProducerWorkerProperties),
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
			     sizeof(UTGProducerWorkerProperties),
			     &pprops);
    printf("\nTest failed results:\n");
    printf("   Producer produced %d buffers, consumer received %d buffers\n",
           pprops.buffersProcessed, cprops.buffersProcessed );
  }

  buffers = pprops.buffersProcessed;
  disableWorkers(ca, workers );
  return passed;
}


int config_and_run_timeout_test(const char *test_name, std::vector<CApp>& ca,
                                std::vector<CWorker*>& workers, int& buffers )
{
  ( void ) test_name;
  int tbtp = BUFFERS_2_PROCESS;
  BUFFERS_2_PROCESS = 30;
  bool test_rc;
  int testPassed = 1;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    BUFFERS_2_PROCESS = tbtp;
    printf("Failed to create workers\n");
    throw;
  }

  try {
    createPorts( ca, workers );
  }
  catch ( ... ) {
    BUFFERS_2_PROCESS = tbtp;
    printf("Failed to create ports\n");
    throw;
  }

  try {
    connectWorkers( ca, workers);
  }
  catch ( ... ) {
    BUFFERS_2_PROCESS = tbtp;
    printf("Failed to connect worker ports\n");
    throw;
  }

  test_rc = run_lb_test(ca, workers, buffers );
  if ( test_rc == false ) testPassed = 0;

  disconnectPorts( ca, workers );
  destroyWorkers( ca, workers );

  BUFFERS_2_PROCESS = tbtp;
  return testPassed;
}



#ifdef NO_MAIN
int test_timeout_main( int argc, char** argv)
#else
int  main( int argc, char** argv)
#endif
{
  OcpiRccBinderConfigurator config;
  int test_rc = 1;
  int oa_test_rc = 1;
  DataTransfer::EventManager* event_manager;
  int cmap[3];
  const char* test_name;
  int buffers;

  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return 1;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return 1;
  }
  g_testUtilVerbose = config.verbose;
  cmap[0] = cmap[1] = cmap[2] = 0;
  if ( config.verbose ) {
    printf("endpoints count = %zu\n", config.endpoints.size() );
  }

  for ( unsigned int n=0; n<config.endpoints.size(); n++ ) {
    g_ep[n] = (char*)config.endpoints[n].c_str();
    if ( config.verbose ) {
      printf(" Ep(%d) = (%s)\n", n, g_ep[n] );
    }
  }

  std::vector<const char*> endpoints;
  endpoints.push_back( g_ep[0] );
  endpoints.push_back( g_ep[1] );
  endpoints.push_back( g_ep[2] );

  std::vector<CApp> ca;

  try {
    ca =
      createContainers(endpoints, event_manager, (bool)OCPI_USE_POLLING);
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("Create containers failed with exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    exit(-1);
  }
  catch( std::string& err ) {
    printf("Got a string exception while creating containers = %s\n", err.c_str() );
    exit( -1 );
  }
  catch( ... ) {
    printf("Got an unknown exception while creating containers\n");
    exit( -1 );
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

  // Setup connection info
  PRODUCER = cmap[0];
  PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.worker = &LOOPBACK;
  PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].down_stream_connection.pid = LOOPBACK_INPUT_PORT0;

  LOOPBACK = cmap[1];
  LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.worker = &CONSUMER;
  LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].down_stream_connection.pid = CONSUMER_INPUT_PORT0;

  CONSUMER = cmap[2];

  PRODUCER.pdata[PRODUCER_OUTPUT_PORT0].bufferCount = 2;
  LOOPBACK.pdata[LOOPBACK_INPUT_PORT0].bufferCount  = 3;
  LOOPBACK.pdata[LOOPBACK_OUTPUT_PORT0].bufferCount = 5;
  CONSUMER.pdata[CONSUMER_INPUT_PORT0].bufferCount  = 1;



  RCCRunCondition rc;
#if 0 // this oesn't work because a null pointer means run all the time
  // Patch the producers run condition so that it only has a 1 second timout
  rc.portMasks = NULL;
  rc.usecs = 1000 * 1000 * 1;
  rc.timeout    = 1;
  UTGProducerWorkerDispatchTable.runCondition = &rc;

  test_name = "Producer run condition runs every second, NULL port ready mask";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_timeout_test( test_name, ca, workers, buffers );
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("failed with an exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    test_rc = 0;
  }
  catch ( std::string& str ) {
    printf(" failed with an exception %s\n",
           str.c_str() );
    test_rc = 0;
  }
  catch ( ... ) {
    test_rc = 0;
  }
  test_rc = !test_rc;
  if ( (buffers < 4) || (buffers > 6) ) {
    test_rc = 0;
  }
  printf(" Test: %s %d: %s\n", test_name, buffers, test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;
#endif

  // Patch the producers run condition so that it only has a 1 second timout
  RCCPortMask nilPortMask = 0;
  rc.portMasks = &nilPortMask;
  rc.timeout    = 1;
  rc.usecs = 1000 * 1000 *  1;
  UTGProducerWorkerDispatchTable.runCondition = &rc;

  test_name = "Producer run condition runs every second, nil port ready mask";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_timeout_test( test_name, ca, workers, buffers );
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("failed with an exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    test_rc = 0;
  }
  catch ( std::string& str ) {
    printf(" failed with an exception %s\n",
           str.c_str() );
    test_rc = 0;
  }
  catch ( ... ) {
    test_rc = 0;
  }
  test_rc = !test_rc;
  if ( (buffers < 5) || (buffers > 7) ) {
    test_rc = 0;
  }
  printf(" Test: %s: %s\n", test_name,  test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;



  // Patch the producers run condition so that it only has a 1 second timout
  RCCPortMask portMask[] = {1, 0};
  rc.portMasks = portMask;
  rc.timeout    = 1;
  rc.usecs = 1000 * 1000 *  1;
  UTGProducerWorkerDispatchTable.runCondition = &rc;

  test_name = "Producer run condition runs every second, with port ready mask";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_timeout_test( test_name, ca, workers, buffers );
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("failed with an exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    test_rc = 0;
  }
  catch ( std::string& str ) {
    printf(" failed with an exception %s\n",
           str.c_str() );
    test_rc = 0;
  }
  catch ( ... ) {
    test_rc = 0;
  }
  printf(" Test: %s: %s\n", test_name, test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;




#if 0
  tdata.run=0;
  t->join();
#endif
  destroyContainers( ca, workers );

  return !oa_test_rc;
}




