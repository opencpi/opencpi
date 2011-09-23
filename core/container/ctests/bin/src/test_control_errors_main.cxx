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
 * Container control errors test
 *
 * 06/23/09 - John Miller
 * Added line to delete container factory.
 *
 * 06/10/09 - John Miller
 * Initial Version
 *
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
#include <OcpiUtilCommandLineConfiguration.h>
#include "UtGenericLoopbackWorkers.h"
#include <OcpiThread.h>

using namespace OCPI::DataTransport;
using namespace DataTransport::Interface;
using namespace OCPI::Container;
using namespace OCPI;
using namespace OCPI::CONTAINER_TEST;
namespace OM = OCPI::Metadata;
namespace OU = OCPI::Util;

static const char* g_ep[]    =  {
  "ocpi-smb-pio://test1:9000000.1.20",
  "ocpi-smb-pio://test2:9000000.2.20",
  "ocpi-smb-pio://test3:9000000.3.20"
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

// Configuration
static  OcpiRccBinderConfigurator config;

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
  PRODUCER.worker->write( offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  // Set the producer bytes processed count
  offset = offsetof(UTGProducerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  PRODUCER.worker->write( offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();


  // Set the consumer passfail property to passed
  offset = offsetof(UTGConsumerWorkerProperties,passfail);
  nBytes = sizeof( uint32_t );
  tprop[0] = 1;
  CONSUMER.worker->write( offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer dropped buffers count
  offset = offsetof(UTGConsumerWorkerProperties,droppedBuffers);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write( offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffer run count property to 0
  offset = offsetof(UTGConsumerWorkerProperties,run2BufferCount);
  nBytes = sizeof( uint32_t );
  tprop[0] = BUFFERS_2_PROCESS;
  CONSUMER.worker->write( offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffers processed count
  offset = offsetof(UTGConsumerWorkerProperties,buffersProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write( offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set the consumer buffers processed count
  offset = offsetof(UTGConsumerWorkerProperties,bytesProcessed);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write( offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  // Set producer error mode property
  offset = offsetof(UTGProducerWorkerProperties,testControlErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();

  offset = offsetof(UTGProducerWorkerProperties,testConfigErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  PRODUCER.worker->write(  offset, nBytes, &tprop[0]);
  PRODUCER.worker->afterConfigure();


  // Set loopback error mode property
  offset = offsetof(UTGLoopbackWorkerProperties,testControlErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  LOOPBACK.worker->write( offset, nBytes, &tprop[0]);
  LOOPBACK.worker->afterConfigure();

  offset = offsetof(UTGLoopbackWorkerProperties,testConfigErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  LOOPBACK.worker->write(  offset, nBytes, &tprop[0]);
  LOOPBACK.worker->afterConfigure();


  // Set consumer error mode property
  offset = offsetof(UTGConsumerWorkerProperties,testControlErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write( offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

  offset = offsetof(UTGConsumerWorkerProperties,testConfigErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  CONSUMER.worker->write( offset, nBytes, &tprop[0]);
  CONSUMER.worker->afterConfigure();

}




#define SET_LAST_ERROR_UNKNOWN_WORKER "Unknown Worker Type"
#define SET_LAST_ERROR_WORKER_IN_UNUSABLE_STATE "Worker in Unusable State"
#define SET_LAST_ERROR_INVALID_SEQUENCE "Invalid Control Sequence"
#define SET_LAST_ERROR_ALL_REQUIRED_PORTS_NOT_CONNECTED "Not all required ports are connected"
#define SET_LAST_ERROR_PORT_COUNT_MISMATCH "Input/Output port count mismatch between worker and meta-data"
#define SET_LAST_ERROR_TO_TEST_NOT_IMPLEMENTED "Test Not Implemented"
#define SET_LAST_ERROR_TO_PROPTERY_OVERRUN "Property Overrun error"




/*
 *  Test the before/after property methods
 */
static int config_and_run_misc_cont_tests(const char *test_name, std::vector<CApp>& ca,
                                          std::vector<CWorker*>& workers
                                          )
{
  ( void ) test_name;
  int testPassed = 1;
  uint32_t err_code = OU::NO_ERROR_;
  std::string err_str;
  int tmpi = 0;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create workers\n");
    throw;
  }

  try {
    initWorkerProperties( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to initialize properties\n");
    throw;
  }

  TRY_AND_SET(err_code, err_str, "", PRODUCER.worker->test());
  if ( err_code != OU::TEST_NOT_IMPLEMENTED ) {  // Test failed, there should have been an error reported
    TUPRINTF("Testing worker test method, without test defined returned success\n");
    testPassed = false;
    goto done;
  }

  try {
    createPorts( ca, workers );
  }
  catch ( ... ) {
    ocpiAssert ( 0 );
    TUPRINTF("Failed to initialize properties\n");
    throw;
  }

  TRY_AND_SET(err_code, err_str, "", PRODUCER.worker->start());
  if ( err_code != OU::PORT_NOT_CONNECTED ) {
    TUPRINTF("Bad error code returned\n");
    testPassed = false;
    goto done;
  }

  try {
    connectWorkers( ca, workers );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create workers\n");
    throw;
  }


  tmpi = UTGProducerWorkerDispatchTable.numInputs;
  UTGProducerWorkerDispatchTable.numInputs = 10;
  TRY_AND_SET(err_code, err_str, "", PRODUCER.worker->start());
  if ( err_code != OU::PORT_COUNT_MISMATCH ) {
    UTGProducerWorkerDispatchTable.numInputs  = tmpi;
    TUPRINTF("Bad error code returned, not port_count_mismatch\n");
    testPassed = false;
    goto done;
  }

 done:
  UTGProducerWorkerDispatchTable.numInputs  = tmpi;

  try {
    disableWorkers(ca,workers);
  }
  catch(...){}

  try {
    disconnectPorts( ca, workers );
  }
  catch(...){}

  try {
    destroyWorkers( ca, workers );
  }
  catch(...){}

  return testPassed;

}



/*
 *  Test the before/after property methods
 */
static int config_and_run_BA_method(const char *test_name, std::vector<CApp>& ca,
                                    std::vector<CWorker*>& workers
                                    )
{
  ( void ) test_name;
  int testPassed = 1;
  uint32_t err_code;
  std::string err_str;
  int32_t  tprop[5], offset, nBytes;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create workers\n");
    throw;
  }

  try {
    initWorkerProperties( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to initialize properties\n");
    throw;
  }

  // Set consumer error mode property
  offset = offsetof(UTGConsumerWorkerProperties,testConfigErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  TRY_AND_SET(err_code, err_str, "",
	      CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
	      CONSUMER.worker->afterConfigure(););
  if ( err_code != OU::NO_ERROR_) {
    TUPRINTF("returned failure code, not success\n");
    testPassed = false;
    goto done;
  }
  TRY_AND_SET(err_code, err_str, "", CONSUMER.worker->beforeQuery());
  if ( err_code != OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
    TUPRINTF("returned failure code, not success\n");
    testPassed = false;
    goto done;
  }

  tprop[0] = 1;
  TRY_AND_SET(err_code, err_str, ERROR_TEST_STRING,
	      CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
	      CONSUMER.worker->afterConfigure(););
  if ( err_str != ERROR_TEST_STRING ) {
    testPassed = false;
  }
  if ( err_code == OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
    TUPRINTF("Expected error return code, not success\n");
    testPassed = false;
    goto done;
  }

  TRY_AND_SET(err_code, err_str, "", CONSUMER.worker->beforeQuery());
  if ( err_str != ERROR_TEST_STRING ) {
    testPassed = false;
  }
  if ( err_code == OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
    TUPRINTF("Expected error return code, not success\n");
    testPassed = false;
    goto done;
  }


 done:
  destroyWorkers(ca,workers);


  return testPassed;
}





/*
 *  Test the read write properties methods
 */
static int config_and_run_read_write_method(const char *test_name, std::vector<CApp>& ca,
                                            std::vector<CWorker*>& workers
                                            )
{
  ( void ) test_name;
  int testPassed = 1;
  uint32_t err_code;
  std::string err_str;
  int32_t  tprop[5], offset, nBytes;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create workers\n");
    throw;
  }

  try {
    initWorkerProperties( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to initialize properties\n");
    throw;
  }


  // Set consumers last property and make sure it works
  offset = offsetof(UTGConsumerWorkerProperties,lastProp);
  nBytes = sizeof( uint32_t );
  tprop[0] = 10;
  TRY_AND_SET(err_code, err_str, "",
	      CONSUMER.worker->write(  offset, nBytes, &tprop[0]));
  if ( err_code != OU::NO_ERROR_ ) {
    TUPRINTF("returned failure code, not success\n");
    testPassed = false;
    goto done;
  }

  // read it back
  offset = offsetof(UTGConsumerWorkerProperties,lastProp);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  TRY_AND_SET(err_code, err_str, "",
	      CONSUMER.worker->read( offset, nBytes, &tprop[0]));
  if ( err_code != OU::NO_ERROR_ ) {
    TUPRINTF("returned failure code, not success\n");
    testPassed = false;
    goto done;
  }
  if ( tprop[0] != 10 ) {
    TUPRINTF("value read is not equal to value written\n");
    testPassed = false;
    goto done;
  }


  offset = offsetof(UTGConsumerWorkerProperties,lastProp)+1;
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  TRY_AND_SET(err_code, err_str, "",
	      CONSUMER.worker->write( offset, nBytes, &tprop[0]));
  if ( err_code == OU::NO_ERROR_ ) {
    TUPRINTF("Expected error return code, not success\n");
    testPassed = false;
    goto done;
  }

  offset = offsetof(UTGConsumerWorkerProperties,lastProp)+1;
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  TRY_AND_SET(err_code, err_str, "",
	      CONSUMER.worker->read( offset, nBytes, &tprop[0]));
  if ( err_code == OU::NO_ERROR_ ) {
    TUPRINTF("Expected error return code, not success\n");
    testPassed = false;
    goto done;
  }


 done:
  destroyWorkers( ca, workers );

  return testPassed;
}






/*
 *  Test the "test" method
 */
static int config_and_run_test_method(const char *test_name, std::vector<CApp>& ca,
                                      std::vector<CWorker*>& workers
                                      )
{
  ( void ) test_name;
  int testPassed = 1;
  uint32_t err_code;
  std::string err_str;
  int32_t  tprop[5], offset, nBytes;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create workers\n");
    throw;
  }

  try {
    initWorkerProperties( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to initialize properties\n");
    throw;
  }

  // Set consumer error mode property
  offset = offsetof(UTGConsumerWorkerProperties,testControlErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  TRY_AND_SET(err_code, err_str, "",
	      CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
	      // Try setting the state
	      CONSUMER.worker->test());
  if ( err_code != OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
    TUPRINTF("test() returned failure code, not success\n");
    testPassed = false;
    goto done;
  }
  if ( err_str != "" ) {
    testPassed = false;
    goto done;
  }

  // Set consumer error mode property
  offset = offsetof(UTGConsumerWorkerProperties,testControlErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 1;
  TRY_AND_SET(err_code, err_str, ERROR_TEST_STRING,
	      CONSUMER.worker->write(  offset, nBytes, &tprop[0]);
	      // Now test should fail and return a string
	      CONSUMER.worker->test());
  if ( err_code == OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
    TUPRINTF("Expected error return code, not success\n");
    testPassed = false;
    goto done;
  }
  if ( err_str != ERROR_TEST_STRING ) {
    testPassed = false;
    goto done;
  }



 done:
  try {
    disableWorkers(ca,workers);
    disconnectPorts( ca, workers );
    destroyWorkers( ca, workers );
  }
  catch(...){
  };


  return testPassed;
}









/*
 *  Call the worker "state" method and make sure that it returns success. Then call the "state" method again
 *  and make sure we get an invalid sequence error back.
 */
static int config_and_run_state_error_test1(const char *test_name, std::vector<CApp>& ca,
                                            std::vector<CWorker*>& workers, OM::Worker::ControlOperation state
                                            )
{
  ( void ) test_name;
  int testPassed = 1;
  uint32_t err_code;
  std::string err_str;
  int32_t  tprop[5], offset, nBytes;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create workers\n");
    throw;
  }

  try {
    initWorkerProperties( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to initialize properties\n");
    throw;
  }

  // Set consumer error mode property
  offset = offsetof(UTGConsumerWorkerProperties,testControlErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 0;
  TRY_AND_SET(err_code, err_str, "",
	      CONSUMER.worker->write(  offset, nBytes, &tprop[0]));
  if ( err_code != OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
    TUPRINTF("return() to return failure code, not success\n");
    testPassed = false;
    goto done;
  }


  try {
    createPorts( ca, workers );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create ports\n");
    throw;
  }

  try {
    connectWorkers( ca, workers);
  }
  catch ( ... ) {
    TUPRINTF("Failed to connect worker ports\n");
    throw;
  }

  if ( state == OM::Worker::OpStop ) {
    try {
      enableWorkers( ca, workers );
    }
    catch ( ... ) {
      TUPRINTF("Failed to enable workers\n");
      throw;
    }
  }

  // Try setting the state if not initialize
  if (state != OM::Worker::OpInitialize) {
    TRY_AND_SET(err_code, err_str, "",
		switch (state) {
		case OM::Worker::OpStart:   CONSUMER.worker->start(); break;
		case OM::Worker::OpStop:   CONSUMER.worker->stop(); break;
		default:;
		});
    if ( err_code != OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
      TUPRINTF("init() to return failure code, not success\n");
      testPassed = false;
      goto done;
    }
    if ( err_str != "" ) {
      testPassed = false;
      goto done;
    }
  }

  // Try to set state again and make sure the container complains.
  TRY_AND_SET(err_code, err_str, "",
	      switch (state) {
	      case OM::Worker::OpStart:   CONSUMER.worker->start(); break;
	      case OM::Worker::OpStop:   CONSUMER.worker->stop(); break;
	      default:;
	      });
  if ( err_code != OU::INVALID_CONTROL_SEQUENCE ) {  // Test failed, there should have been an error reported
    TUPRINTF("Wrong return code\n");
    testPassed = false;
    goto done;
  }

 done:
  try {
    disableWorkers(ca,workers);
    disconnectPorts( ca, workers );
    destroyWorkers( ca, workers );
  }
  catch(...){
  };


  return testPassed;
}



/*
 *  Force the worker to create an error on "state" and make sure we get back the error string and
 *  status correctly.  Then call "state" again and make sure we get a worker unusable error back.
 */
static int config_and_run_state_error_test2(const char *test_name, std::vector<CApp>& ca,
                                            std::vector<CWorker*>& workers, OM::Worker::ControlOperation state
                                            )
{
  ( void ) test_name;
  int testPassed = 1;
  uint32_t err_code;
  std::string err_str;
  int32_t  tprop[5], offset, nBytes;

  try {
    createWorkers( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create workers\n");
    throw;
  }

  try {
    initWorkerProperties( ca );
  }
  catch ( ... ) {
    TUPRINTF("Failed to initialize properties\n");
    throw;
  }

  try {
    createPorts( ca, workers );
  }
  catch ( ... ) {
    TUPRINTF("Failed to create ports\n");
    throw;
  }

  try {
    connectWorkers( ca, workers);
  }
  catch ( ... ) {
    TUPRINTF("Failed to connect worker ports\n");
    throw;
  }

  if ( state == OM::Worker::OpStop ) {
    try {
      enableWorkers( ca, workers);
    }
    catch ( ... ) {
      TUPRINTF("Failed to enable workers\n");
      throw;
    }
  }

  // Set consumer error mode property
  offset = offsetof(UTGConsumerWorkerProperties,testControlErrors);
  nBytes = sizeof( uint32_t );
  tprop[0] = 1;
  TRY_AND_SET(err_code, err_str, ERROR_TEST_STRING,
	      CONSUMER.worker->write( offset, nBytes, &tprop[0]);
	      switch (state) {
	      case OM::Worker::OpStart:   CONSUMER.worker->start(); break;
	      case OM::Worker::OpStop:   CONSUMER.worker->stop(); break;
	      default:;
	      });
  if ( err_code == OU::NO_ERROR_ ) {  // Test failed, there should have been an error reported
    TUPRINTF("Expected error return code, not success\n");
    testPassed = false;
  }
  if ( err_str != ERROR_TEST_STRING ) {
    testPassed = false;
  }

  tprop[0] = 2;
  TRY_AND_SET(err_code, err_str, ERROR_TEST_STRING,
	      CONSUMER.worker->write( offset, nBytes, &tprop[0]);
	      switch (state) {
	      case OM::Worker::OpStart:   CONSUMER.worker->start(); break;
	      case OM::Worker::OpStop:   CONSUMER.worker->stop(); break;
	      default:;
	      });
  if ( err_code == OU::NO_ERROR_ ) {
    TUPRINTF("Expected worker to return FATAL falure\n");
    testPassed = false;
  }
  if ( err_str != ERROR_TEST_STRING ) {
    testPassed = false;
  }

  TRY_AND_SET(err_code, err_str, "",
	      switch (state) {
	      case OM::Worker::OpStart:   CONSUMER.worker->start(); break;
	      case OM::Worker::OpStop:   CONSUMER.worker->stop(); break;
	      default:;
	      });
  if ( err_code == OU::NO_ERROR_ ) {
    TUPRINTF("Expected worker to be unusable\n");
    testPassed = false;
  }
  if ( err_code != OU::WORKER_UNUSABLE) {
    testPassed = false;
  }

  try {
    disableWorkers( ca, workers );
    disconnectPorts( ca, workers );
    destroyWorkers( ca, workers );
  }
  catch( ... ) {
    // User in unusable state so we expect a complaint
  }

  return testPassed;
}


#ifdef NO_MAIN
int test_control_errors_main( int argc, char** argv)
#else
int  main( int argc, char** argv)
#endif
{
  int test_rc = 1;
  int oa_test_rc = 1;
  DataTransfer::EventManager* event_manager;
  int cmap[3];
  const char* test_name;


  try {
    config.configure (argc, argv);
  }
  catch (const std::string & oops) {
    std::cerr << "Error: " << oops << std::endl;
    return false;
  }
  if (config.help) {
    printUsage (config, argv[0]);
    return false;
  }
  g_testUtilVerbose = config.verbose;
  cmap[0] = cmap[1] = cmap[2] = 0;

  if ( config.verbose ) {
    TUPRINTF("endpoints count = %u\n", 
              ( unsigned int ) config.endpoints.size() );
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
  catch( std::string& err ) {
    printf("Got a string exception while creating containers = %s\n", err.c_str() );
    exit( -1 );
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("Create containers failed with exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    exit(-1);
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


  test_name = "Misc. Control Error Tests";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_misc_cont_tests( test_name, ca, workers   );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;



  test_name = "Property read/write Control Error Test";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_read_write_method( test_name, ca, workers );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;


  test_name = "Before/After Config Error Test";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_BA_method( test_name, ca, workers );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;



#if 0

  test_name = "init() Control Error Test 1";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_state_error_test1( test_name, ca, workers, OM::Worker::OpInitialize );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;

  test_name = "init() Control Error Test 2";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_state_error_test2( test_name, ca, workers, OM::Worker::OpInitialize );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;
#endif

  test_name = "start() Control Error Test 1";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_state_error_test1( test_name, ca, workers, OM::Worker::OpStart );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;




  test_name = "start() Control Error Test 2";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_state_error_test2( test_name, ca, workers, OM::Worker::OpStart );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;




  test_name = "stop() Control Error Test 1";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_state_error_test1( test_name, ca, workers, OM::Worker::OpStop );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;

  test_name = "test() Method Test";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_test_method( test_name, ca, workers   );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;


  test_name = "stop() Control Error Test 2";
  try {
    printf("\n\nRunning test (%s): \n", test_name );
    test_rc &= config_and_run_state_error_test2( test_name, ca, workers, OM::Worker::OpStop  );
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
  printf(" Test:  %s\n",   test_rc ? "PASSED" : "FAILED" );
  oa_test_rc &= test_rc; test_rc=1;




#if 0
  tdata.run=0;
  t->join();
  delete t;
#endif
  destroyContainers( ca, workers );

  return !oa_test_rc;
}




