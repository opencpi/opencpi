
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


#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "OcpiOsMisc.h"
#include "OcpiOsAssert.h"
#include "DtIntEventHandler.h"
#include "OcpiThread.h"
#include "OcpiPValue.h"
#include "OcpiUtilCommandLineConfiguration.h"
#include "ContainerWorker.h"
#include "ContainerPort.h"
#include "ConsumerWorker.h"
#include "ProdWorker.h"
#include "test_utilities.h"

#define OCPI_RCC_DATA_BUFFER_SIZE 32

// const char* OCPI_RCC_CONT_EP          = "ocpi-smb-pio:GPPSMB:900000.18.20";
// const char* OCPI_RCC_CONT_EP          = "ocpi-dma-pio:1.0:900000.3.20";
// const char* OCPI_RCC_CONT_EP          = "ocpi-dma-pio:1.0.900000:900000.3.20";


class OcpiRccBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccBinderConfigurator ();

public:
  bool help;
  bool verbose;
  bool standalone;
  std::string pdfpath;
  long msgSize;
  long nBuffers;
  std::string endpoint;
private:
  static CommandLineConfiguration::Option g_options[];
};

// Configuration
static  OcpiRccBinderConfigurator config;

OcpiRccBinderConfigurator::
OcpiRccBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
    standalone(false),
    msgSize( OCPI_RCC_DATA_BUFFER_SIZE ),
    nBuffers(2)
			  //    endpoint(OCPI_RCC_CONT_EP)
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccBinderConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Set this containers endpoint",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::endpoint), 0 },
   { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "nBuffers", "Number of buffers",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::nBuffers), 0 },
   { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "msgSize", "Message size",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::msgSize), 0 },
   { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "pdfpath", "Port descriptor file",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::pdfpath), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "standalone", "Run this test and connect the producer directly to the input",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::standalone), 0 },
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


#define PORT_0 0
#define PORT_1 1

#define GPP_CONT_UID 1


// using namespace OCPI::DataTransport;

using namespace OCPI::Container;
using namespace OCPI;


// Constants
static Worker * WORKER_OUTPUT_ID;
static Worker * WORKER_INPUT_ID;

int OCPI_USE_POLLING=1;

// Program globals
static OCPI::API::Container* gpp_container;
static OCPI::API::ContainerApplication *gpp_app;
Port *inputPort, *outputPort;

#ifdef NEED_CI
void  dumpPortData( PortData& pd )
{

  printf("CD mode = %d\n", pd.connectionData.data.mode);
  printf("CD role = %d\n", pd.connectionData.data.role);

  if ( pd.connectionData.data.mode == OCPI::RDT::OutputDescType ) {
    printf("Output desc\n");
  }
  else{
    printf("Input desc\n");
  }
  printf("  desc.nbuffers = %d\n",  pd.connectionData.data.desc.nBuffers );
  printf( " desc.dataBufferBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.dataBufferBaseAddr );
  printf( " desc.dataBufferPitch = %d\n",  pd.connectionData.data.desc.dataBufferPitch );
  printf( " desc.dataBufferSize = %d\n",  pd.connectionData.data.desc.dataBufferSize );
  printf( " desc.metaDataBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.metaDataBaseAddr );
  printf( " desc.metaDataPitch = %d\n",  pd.connectionData.data.desc.metaDataPitch );
  printf( " desc.fullFlagBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.fullFlagBaseAddr );
  printf( " desc.fullFlagSize = %d\n",  pd.connectionData.data.desc.fullFlagSize );
  printf( " desc.fullFlagPitch = %d\n",  pd.connectionData.data.desc.fullFlagPitch );
  printf( " desc.fullFlagValue = 0x%llx\n",  (long long)pd.connectionData.data.desc.fullFlagValue );
  printf( " desc.emptyFlagBaseAddr = 0x%llx\n",  (long long)pd.connectionData.data.desc.emptyFlagBaseAddr );
  printf( " desc.emptyFlagSize = %d\n",  pd.connectionData.data.desc.emptyFlagSize );
  printf( " desc.emptyFlagPitch = %d\n",  pd.connectionData.data.desc.emptyFlagPitch );
  printf( " desc.emptyFlagValue = 0x%llx\n",  (long long)pd.connectionData.data.desc.emptyFlagValue );
  printf( " desc.oob.oep = %s\n", pd.connectionData.data.desc.oob.oep );

}
#endif



#define MAX_DESC_LEN 1024
void readPortDecsFile( std::string& rpl_output, std::string& rpl_input )
{
  if ( config.verbose )
    printf("About to read the desc file (%s)\n", config.pdfpath.c_str() );

  int pfd;
  std::string pfile = config.pdfpath;
  pfile += ".user";
  if ( (pfd = open( pfile.c_str(), O_RDONLY )) < 0 ) {
    printf("Could not read the port descriptor file (%s)\n",  pfile.c_str() );
    printf("Good bye\n");
    exit(-1);
  }

  int cfd;
  std::string cfile = config.pdfpath;
  cfile += ".provider";
  if ( (cfd = open( cfile.c_str(), O_RDONLY )) < 0 ) {
    printf("Could not read the port descriptor file (%s)\n",  cfile.c_str() );
    printf("Good bye\n");
    exit(-1);
  }
  ssize_t br;

  // Read the input desc
  char buf[MAX_DESC_LEN];
  br = read(cfd, buf, MAX_DESC_LEN);
  if ( (br==0) || (br >= MAX_DESC_LEN)  ) {
    printf("(2)Attempted to read %d bytes and only read %zd bytes from descriptor file\n", MAX_DESC_LEN, br );
    exit(-1);
  }
  rpl_input.assign( buf, br );


  br = read(pfd, buf, MAX_DESC_LEN);
  if ( br >= MAX_DESC_LEN ) {
    printf("Attempted to read %d bytes and only read %zd bytes from descriptor file\n", MAX_DESC_LEN, br );
    exit(-1);
  }
  rpl_output.assign( buf, br );

  printf("\n\n\n*****    INPUT   *********\n" );
//  dumpPortData( rpl_input );

  printf("\n\n\n*****    OUTPUT   *********\n" );
//  dumpPortData( rpl_output );

  printf("\n\n\n");

  if ( config.verbose )
    printf("Done\n");


  close( pfd );
  close( cfd );
}


void writePortDecsFile( std::string& rpl_output, std::string& rpl_input )
{
  ssize_t br;

  int pfd;
  std::string pfile = config.pdfpath;
  pfile += ".user";
  if ( (pfd = open( pfile.c_str(), O_CREAT | O_RDWR , S_IRWXU | S_IRWXG | S_IRWXO )) < 0 ) {
    printf("Could not read the port descriptor file (%s)\n",  pfile.c_str() );
    printf("Good bye\n");
    exit(-1);
  }

  int cfd;
  std::string cfile = config.pdfpath;
  cfile += ".provider";
  if ( (cfd = open( cfile.c_str(), O_CREAT | O_RDWR , S_IRWXU | S_IRWXG | S_IRWXO )) < 0 ) {
    printf("Could not read the port descriptor file (%s)\n",  cfile.c_str() );
    printf("Good bye\n");
    exit(-1);
  }

  br = write(cfd, rpl_input.c_str(), rpl_input.length() );
  if ( (br==0) || (br >= MAX_DESC_LEN) ) {
    printf("(2)Attempted to write %d bytes and only wrote %zd bytes from descriptor file\n", MAX_DESC_LEN, br );
    exit(-1);
  }

  br = write(pfd, rpl_output.c_str(), rpl_output.length() );
  if ((br==0) || ( br >= MAX_DESC_LEN )) {
    printf("Attempted to write %d bytes and only wrote %zd bytes from descriptor file\n", MAX_DESC_LEN, br );
    exit(-1);
  }

  close( pfd );
  close( cfd );
}


void setupForPCMode()
{
  try {
    WORKER_INPUT_ID = OCPI::CONTAINER_TEST::createWorker( gpp_app, &ConsumerWorkerDispatchTable  );
    WORKER_OUTPUT_ID = OCPI::CONTAINER_TEST::createWorker( gpp_app, &ProducerWorkerDispatchTable );
  }
  CATCH_ALL_RETHROW( "creating workers" )


 try {
      outputPort = &  WORKER_OUTPUT_ID->createOutputPort( PORT_0,
                                                            config.nBuffers,
                                                            config.msgSize, NULL);
  }
  CATCH_ALL_RETHROW( "creating source port" );


  try {


    static OCPI::Util::PValue tprops[] = {
      //      OCPI::Util::PVString("endpoint","ocpi-dma-pio:0.0:300000.1.10"),
      OCPI::Util::PVEnd };

    inputPort = & WORKER_INPUT_ID->createInputPort(  PORT_0,
                                                    config.nBuffers,
                                                    config.msgSize,
                                                    tprops
                                                    );
  }
  CATCH_ALL_RETHROW("creating target port")


  if ( config.standalone ) {
    std::string spd, cpd;
    OCPI::Container::Port::packPortDesc( outputPort->getData().data, spd );
    OCPI::Container::Port::packPortDesc( inputPort->getData().data, cpd );
    writePortDecsFile( spd , cpd );
  }


  // Now we need to make the connections.  We are connecting our output to the loopback input
  // and the loopback input to our output.
  std::string localShadowPort;

  // Read in the rpl port descriptors
  std::string remoteTargetPort;
  std::string remoteSourcePort;
  readPortDecsFile( remoteSourcePort, remoteTargetPort );


#ifdef PORT_COMPLETE
  if ( ! config.standalone ) {
    // PATCH the descriptor
    reinterpret_cast<PortData*>(&remoteSourcePort)->connectionData.port  =
      reinterpret_cast<PortData*>(outputPort)->connectionData.port;
    remoteSourcePort.connectionData.cid = 100;
    remoteTargetPort.connectionData.cid = 100;
  }
#endif



  // We will connect our output to the remote input.  We got the inputs descriptor out of band
  // but it is complete and will allow us to establish this connection without any other information.  The
  // Input however is waiting for us to provide it with feedback control information so that it can complete
  // its portion of this conection.  We are not able to provide this information until we create the connections
  // since it is dependent on our "shadow" target buffers.

  if ( config.verbose )
    printf("About to connect target port\n");
  try {

    outputPort->setFinalProviderInfo( remoteTargetPort, localShadowPort );
  }
  CATCH_ALL_RETHROW("creating target port")

  // Here we eat the shadow port info since the rpl is currently passive
  if ( config.standalone ) {
    std::string scp;
    OCPI::Container::Port::packPortDesc(inputPort->getData().data, scp);
    writePortDecsFile( localShadowPort, scp );
    readPortDecsFile( remoteSourcePort, remoteTargetPort );
  }

    // Again we ignor this step for our test
    // Then we send the Loopback container our input descriptor
    // std::string scp = gpp_container->packPortDesc( inputPort );
    // gppSendInputDescPacked( gpp_circuits[0], scp );


    // The OCPI output descriptor contains the outputs "shadow port" buffer empty flag
    // address information.  The id's in the descriptor relate to our input port.  This
    // info will let us know how to tell the output when we are done with a buffer.
    printf("About to configure the SFC\n");
    //    dumpPortData( remoteSourcePort );

    inputPort->setFinalUserInfo( remoteSourcePort );

    if ( config.verbose )
      printf("Done setting up the GPP container\n");

}


int gpp_cont(int argc, char** argv)
{
  ( void ) argc;
  ( void ) argv; 
  //  DataTransfer::EventManager* gpp_event_manager;
  static OCPI::Util::PValue container_props[] = {OCPI::Util::PVString("endpoint",""), OCPI::Util::PVEnd };

#if 0
  gpp_container = static_cast<OCPI::Container::Interface*>(d);
  ocpiAssert( dynamic_cast<OCPI::Container::Interface*>(d) );
  gpp_event_manager = gpp_container->getEventManager();
  gpp_app = gpp_container->createApplication();
#else
      try { 
	gpp_container =  OCPI::API::ContainerManager::find("rcc", NULL, container_props);
	if ( ! gpp_container)
	  throw OCPI::Util::EmbeddedException("No Containers found\n");
	gpp_app = gpp_container->createApplication();
      }
      CATCH_ALL_RETHROW( "creating container");
#endif

  try {

#ifdef DONE
    try {
      gpp_cFactory =  new OCPI::Container::Factory(OCPI::Container::Factory::RCC,OCPI_USE_POLLING);
    }
    CATCH_ALL_RETHROW( "creating container");

    // First thing here on the GPP container is that we need to create the workers and
    // the worker ports.  We will use OCPIRDT mode 3 for this test.
    OCPI::Container::StartupParams params;
    try {
      params.endpoint = config.endpoint.c_str();
      gpp_container = gpp_cFactory->create( GPP_CONT_UID, params );
      gpp_event_manager = gpp_cFactory->getEventManager();
      gpp_app = &gpp_container->createApplicationContext();
    }
    CATCH_ALL_RETHROW( "creating container");
#endif





    try {
      setupForPCMode();
    }
    CATCH_ALL_RETHROW("setting mode");

    try {

      // Enable the workers
      WORKER_OUTPUT_ID->afterConfigure();
      WORKER_INPUT_ID->afterConfigure();
      WORKER_OUTPUT_ID->start();
      WORKER_INPUT_ID->start();

    }
    CATCH_ALL_RETHROW("initializing workers");


    // Now we will just enter a processing loop
    int lc=0;
    int OCPI_RUN_TEST = 1;
#if 0
    int event_id;
    OCPI::OS::uint64_t evalue;

    if ( gpp_event_manager ) {
      printf("Running with an event manager\n");
    }
    else {
      printf("Running without a event manager\n");
    }
#endif

    while( OCPI_RUN_TEST ) {

      gpp_container->run(100, true);
#if 0
      if ( gpp_event_manager ) {
        do {
          gpp_container->dispatch( gpp_event_manager);
          if ( gpp_event_manager->waitForEvent( 100, event_id, evalue ) == DataTransfer::EventTimeout ) {
            printf("We have not recieved an event for 100 uSec.\n");
          }
          else {
            gpp_container->dispatch( gpp_event_manager );
            break;
          }
        } while(1);
      }
      else {
        gpp_container->dispatch( gpp_event_manager );
      }
#endif

      OCPI::OS::sleep( 500 );
      lc++;

      //#define LIMIT_RUN
#ifdef LIMIT_RUN
      if ( lc > (1000 * 10) ) {
        OCPI_RUN_TEST = 0;
      }
      OCPI::OS::sleep( 1 );
#endif

    }

    printf("About to cleanup \n");

    // Cleanup
    OCPI::OS::sleep( 3000 );
    delete gpp_app;
    gpp_container->stop();
    delete gpp_container;
    gpp_container = NULL;

  }
  catch ( int& ii ) {
    printf("gpp: Caught an int exception while %s = %d\n", "main" ,ii );
  }
  catch ( OCPI::Util::EmbeddedException& eex ) {
    printf(" gpp: Caught an embedded exception while %s:\n", "main");
    printf( " error number = %d", eex.m_errorCode );
    printf( " aux info = %s\n", eex.m_auxInfo.c_str() );
  }
  catch( std::string& stri ) {
    printf("gpp: Caught a string exception while %s = %s\n","main", stri.c_str() );
  }
  catch( ... ) {
    printf("gpp: Caught an unknown exception while %s\n","main" );
  }

  printf("gpp_container: Good Bye\n");

  return 0;
}


int main( int argc, char** argv)
{
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

  if ( config.pdfpath.length() == 0 ) {
    printf("You must tell me where the rpl port descritor file lives\n");
    printUsage (config, argv[0]);
    return false;
  }

  // Start the container in a thead
  gpp_cont(argc,argv);
}


