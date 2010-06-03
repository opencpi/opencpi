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
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <CpiTransportServer.h>
#include <CpiTransportClient.h>
#include <CpiRDTInterface.h>
#include <test_utilities.h>
#include <CpiUtilCommandLineConfiguration.h>
#include <UtZeroCopyIOWorkers.h>
#include <CpiTimeEmit.h>
#include <CpiProperty.h>

#include <CpiThread.h>

using namespace CPI::DataTransport;
using namespace DataTransport::Interface;
using namespace CPI::Container;
using namespace CPI;
using namespace CPI::CONTAINER_TEST;


class CpiRccBinderConfigurator
  : public CPI::Util::CommandLineConfiguration
{
public:
  CpiRccBinderConfigurator ();

public:
  bool help;
  bool verbose;
  MultiString  endpoints;

private:
  static CommandLineConfiguration::Option g_options[];
};

// Configuration
static  CpiRccBinderConfigurator config;

CpiRccBinderConfigurator::
CpiRccBinderConfigurator ()
  : CPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false)
{
}

CPI::Util::CommandLineConfiguration::Option
CpiRccBinderConfigurator::g_options[] = {
 
  { CPI::Util::CommandLineConfiguration::OptionType::MULTISTRING,
    "endpoints", "container endpoints",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::endpoints) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::verbose) },
  { CPI::Util::CommandLineConfiguration::OptionType::NONE,
    "help", "This message",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::help) },
  { CPI::Util::CommandLineConfiguration::OptionType::END }
};

static
void
printUsage (CpiRccBinderConfigurator & config,
            const char * argv0)
{
  std::cout << "usage: " << argv0 << " [options]" << std::endl
            << "  options: " << std::endl;
  config.printOptions (std::cout);
}

void sig_handler( int signum )
{
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
  int cmap[3];

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
  cmap[0] = 0; cmap[1] = 1; cmap[2] = 2;

  std::vector<char*> endpoints;
  std::vector<CApp> ca;
  try {
    ca = 
      createContainers(endpoints, event_manager, (bool)1);
  }
  catch( std::string& err ) {
    printf("Got a string exception while creating containers = %s\n", err.c_str() );
    exit(-1);
  }
  catch( CPI::Util::EmbeddedException& ex ) {
    printf("Create containers failed with exception. errorno = %d, aux = %s\n",
           ex.getErrorCode(), ex.getAuxInfo() );
    exit(-1);
  }
  catch( ... ) {
    printf("Got an unknown exception while creating containers\n");
    exit(-1);
  }
        
  // Create a dispatch thread
  DThreadData tdata;
  tdata.run =1;
  tdata.containers = ca;
  tdata.event_manager = event_manager;
  CPI::Util::Thread* t = runTestDispatch(tdata);


  // Create an artifact
  const char * w1Url = "/home/jmiller/projects/opencpi/core/container/linux-dll/test.dll";
  Artifact & art1 = ca[0].app-> loadArtifact(w1Url);
  Worker & consumer = ca[0].app->createWorker( art1,"Consumer",0 );
  Worker & producer = ca[0].app->createWorker( art1,"Producer",0 );

  CPI::Container::Port & pout = producer.getPort( "Out" );
  CPI::Container::Port & cin = consumer.getPort( "In" );

  std::string p = cin.getInitialProviderInfo() ;
  std::string flowc = pout.setFinalProviderInfo( p );
  cin.setFinalUserInfo( flowc );

  CPI::Container::Property doubleT ( consumer, "doubleT" );
  CPI::Container::Property passFail ( consumer, "passfail" );
  CPI::Container::Property boolT ( consumer, "boolT" );
  CPI::Container::Property run2BufferCount ( consumer, "run2BufferCount" );
  CPI::Container::Property longlongT ( consumer, "longlongT" );
  CPI::Container::Property buffersProcessed ( consumer, "buffersProcessed" );
  CPI::Container::Property floatST ( consumer, "floatST" );
  CPI::Container::Property droppedBuffers ( consumer, "droppedBuffers" );
  CPI::Container::Property bytesProcessed ( consumer, "bytesProcessed" );
  CPI::Container::Property transferMode ( consumer, "transferMode" );
  

  CPI::Container::Property Prun2BufferCount ( producer, "run2BufferCount" );
  CPI::Container::Property PbuffersProcessed ( producer, "buffersProcessed" );
  CPI::Container::Property PbytesProcessed ( producer, "bytesProcessed" );
  CPI::Container::Property PtransferMode ( producer, "transferMode" );
  

  // Extra for test case only
  doubleT.setDoubleValue( 167.82 );
  boolT.setBoolValue( 1 );
  longlongT.setLongLongValue( 1234567890 );  
  float fv[] = {1.1,2.2,3.3,4.4,5.5,6.6};
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


  consumer.initialize();
  consumer.start();
  producer.initialize();
  producer.start();

  
  // Let test run for a while
  int count = 5;
  do {
    uint32_t bp = buffersProcessed.getULongValue();
    if ( bp == 250 ) {
      printf("Test completed sucessfully !!\n");
      break;
    }
    CPI::OS::sleep( 1000 );
  } while ( count-- );

  uint32_t tbp = buffersProcessed.getULongValue();
  if ( tbp != 250 ) {
    printf("Test FAILED!!, tried to process 250 buffers, only processed %d buffers\n", tbp );
  }


  tdata.run=0;
  t->join();
  delete t;

  return !test_rc;
}


