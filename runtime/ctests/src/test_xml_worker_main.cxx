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
 * Coverage test for data transfer roles.  The container is used as a test bench
 * for this test.
 *
 *    John Miller - 1/10/10
 *    Initial Version
 */

#include <vector>
#include <string>
#include <stdio.h>
#include <sstream>
#include <stdlib.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <OcpiRDTInterface.h>
#include <test_utilities.h>
#include <OcpiUtilCommandLineConfiguration.h>
#include <UtZeroCopyIOWorkers.h>
#include <OcpiTimeEmit.h>

#include <OcpiThread.h>

using namespace OCPI::Container;
using namespace OCPI;
using namespace OCPI::CONTAINER_TEST;


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
printUsage (OcpiRccBinderConfigurator & a_config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  a_config.printOptions (std::cout);
}

void sig_handler( int signum )
{ 
  ( void ) signum;
  exit(-1);
}


#ifdef NO_MAIN
int test_ap_main( int argc, char** argv)
#else
int  main( int argc, char** argv)
#endif
{

  SignalHandler sh(sig_handler);

  int test_rc = 1;
  DataTransfer::EventManager* event_manager;
  //int cmap[3];

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
  //cmap[0] = 0; cmap[1] = 1; cmap[2] = 2;

  std::vector<const char*> endpoints;
  std::vector<CApp> ca;
  try {
    ca =
      createContainers(endpoints, event_manager, (bool)1);
  }
  catch( OCPI::Util::EmbeddedException& ex ) {
    printf("Create containers failed with exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    exit(-1);
  }
  catch( std::string& err ) {
    printf("Got a string exception while creating containers = %s\n", err.c_str() );
    exit(-1);
  }
  catch( ... ) {
    printf("Got an unknown exception while creating containers\n");
    exit(-1);
  }

  // Create a dispatch thread
#if 0
  DThreadData tdata;
  tdata.run =1;
  tdata.containers = ca;
  tdata.event_manager = event_manager;
  OCPI::Util::Thread* t = runTestDispatch(tdata);
#endif

  try {
  // New library-based components
  OCPI::API::Worker
    & consumer = ca[0].app->createWorker("testConsumer", "ocpi.Consumer" ),
    & producer = ca[0].app->createWorker("testProducer", "ocpi.Producer" );
  OCPI::API::Port & pout = producer.getPort( "Out" );
  OCPI::API::Port & cin = consumer.getPort( "In" );


  //  std::string p = cin.getInitialProviderInfo() ;
  //  std::string flowc = pout.setFinalProviderInfo( p );
  //  cin.setFinalUserInfo( flowc );

  static OCPI::API::PValue props[] = {OCPI::Util::PVString("protocol","ocpi-smb-pio"),
				       OCPI::Util::PVEnd };
  
  cin.connect( pout, props );

  OCPI::API::Property doubleT ( consumer, "doubleT" );
  OCPI::API::Property passFail ( consumer, "passfail" );
  OCPI::API::Property boolT ( consumer, "boolT" );
  OCPI::API::Property run2BufferCount ( consumer, "run2BufferCount" );
  OCPI::API::Property longlongT ( consumer, "longlongT" );
  OCPI::API::Property buffersProcessed ( consumer, "buffersProcessed" );
  OCPI::API::Property floatST ( consumer, "floatST" );
  OCPI::API::Property droppedBuffers ( consumer, "droppedBuffers" );
  OCPI::API::Property bytesProcessed ( consumer, "bytesProcessed" );
  OCPI::API::Property transferMode ( consumer, "transferMode" );


  OCPI::API::Property Prun2BufferCount ( producer, "run2BufferCount" );
  OCPI::API::Property PbuffersProcessed ( producer, "buffersProcessed" );
  OCPI::API::Property PbytesProcessed ( producer, "bytesProcessed" );
  OCPI::API::Property PtransferMode ( producer, "transferMode" );


  // Extra for test case only
  doubleT.setDoubleValue( 167.82 );
  boolT.setBoolValue( 1 );
  longlongT.setLongLongValue( 1234567890 );
  float fv[] = {1.1f, 2.2f ,3.3f, 4.4f, 5.5f, 6.6f};
  floatST.setFloatSequenceValue( fv, 6 );

  // Set consumer properties
  passFail.setULongValue( 1 );
  droppedBuffers.setULongValue( 0 );
  run2BufferCount.setULongValue( 250  );
  buffersProcessed.setULongValue( 0 );
  bytesProcessed.setULongValue( 0 );
  transferMode.setULongValue( ConsumerConsume );
  consumer.afterConfigure();


  // Set producer properties
  Prun2BufferCount.setULongValue( 250 );
  PbuffersProcessed.setULongValue( 0 );
  PbytesProcessed.setULongValue( 0 );
  producer.afterConfigure();


  //  consumer.initialize();
  consumer.start();
  //  producer.initialize();
  producer.start();


  // Let test run for a while
  int count = 5;
  do {
    uint32_t bp = buffersProcessed.getULongValue();
    if ( bp == 250 ) {
      printf(" Test: PASSED\n");
      break;
    }
    OCPI::OS::sleep( 1000 );
  } while ( count-- );

  uint32_t tbp = buffersProcessed.getULongValue();
  if ( tbp != 250 ) {
    printf("Test: FAILED!!, tried to process 250 buffers, only processed %d buffers\n", tbp );
  }


#if 0
  tdata.run=0;
  t->join();
  delete t;
#endif

  } catch (std::string & oops) {
    std::cout << "Error: " << oops << std::endl;
    return 1;
  }
  return !test_rc;
}
