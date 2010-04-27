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

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <CpiContainerInterface.h>
#include <CpiWorker.h>
#include <CpiContainerPort.h>
#include <ConsumerWorker.h>
#include <ProdWorker.h>
#include <CpiThread.h>
#include <CpiPValue.h>
#include <CpiDriver.h>
#include <CpiUtilCommandLineConfiguration.h>


#define CPI_RCC_DATA_BUFFER_SIZE 32

// char* CPI_RCC_CONT_EP          = "cpi-smb-pio://GPPSMB:900000.18.20";
// char* CPI_RCC_CONT_EP          = "cpi-pci-pio://1.0:900000.3.20";
char* CPI_RCC_CONT_EP          = "cpi-pci-pio://1.0.900000:900000.3.20";


class CpiRccBinderConfigurator
  : public CPI::Util::CommandLineConfiguration
{
public:
  CpiRccBinderConfigurator ();

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
static  CpiRccBinderConfigurator config;

CpiRccBinderConfigurator::
CpiRccBinderConfigurator ()
  : CPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
    standalone(false),
    msgSize( CPI_RCC_DATA_BUFFER_SIZE ),
    nBuffers(2),
    endpoint(CPI_RCC_CONT_EP)
{
}

CPI::Util::CommandLineConfiguration::Option
CpiRccBinderConfigurator::g_options[] = {
  { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Set this containers endpoint",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::endpoint) },
   { CPI::Util::CommandLineConfiguration::OptionType::LONG,
    "nBuffers", "Number of buffers",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::nBuffers) },
   { CPI::Util::CommandLineConfiguration::OptionType::LONG,
    "msgSize", "Message size",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::msgSize) },
   { CPI::Util::CommandLineConfiguration::OptionType::STRING,
    "pdfpath", "Port descriptor file",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::pdfpath) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::verbose) },
  { CPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "standalone", "Run this test and connect the producer directly to the input",
    CPI_CLC_OPT(&CpiRccBinderConfigurator::standalone) },
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



#define CHECK_WCI_CONROL_ERROR(err, op) \
	if ( err != WCI_SUCCESS ) { \
	printf("ERROR: WCI control(%d) returned %d\n", op, err );\
	throw 1;\
	}

#define PORT_0 0
#define PORT_1 1

#define GPP_CONT_UID 1

#define CATCH_ALL_RETHROW( msg )					\
  catch ( int& ii ) {							\
    printf("gpp: Caught an int exception while %s = %d\n", msg,ii );	\
    throw;								\
  }									\
  catch( std::string& stri ) {						\
    printf("gpp: Caught a string exception while %s = %s\n", msg, stri.c_str() ); \
    throw;								\
  }									\
  catch ( CPI::Util::EmbeddedException& eex ) {				\
    printf(" gpp: Caught an embedded exception while %s:\n", msg);		\
    printf( " error number = %d", eex.m_errorCode );			\
    printf( " aux info = %s\n", eex.m_auxInfo.c_str() );	        \
    throw;								\
  }									\
  catch( ... ) {							\
    printf("gpp: Caught an unknown exception while %s\n",msg );		\
    throw;								\
  }

// using namespace CPI::DataTransport;

using namespace CPI::Container;
using namespace CPI;


// Constants
static Worker * WORKER_OUTPUT_ID;
static Worker * WORKER_INPUT_ID;

int CPI_USE_POLLING=1;

// Program globals
static CPI::Container::Interface* gpp_container;
static Application *gpp_app;
Port *inputPort, *outputPort;

#ifdef NEED_CI
void  dumpPortData( PortData& pd )
{

  printf("CD mode = %d\n", pd.connectionData.data.mode);
  printf("CD role = %d\n", pd.connectionData.data.role);

  if ( pd.connectionData.data.mode == CPI::RDT::OutputDescType ) {
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
    printf("(2)Attempted to read %d bytes and only read %lu bytes from descriptor file\n", MAX_DESC_LEN, br );
    exit(-1);
  }
  rpl_input.assign( buf, br );


  br = read(pfd, buf, MAX_DESC_LEN);
  if ( br >= MAX_DESC_LEN ) {
    printf("Attempted to read %d bytes and only read %lu bytes from descriptor file\n", MAX_DESC_LEN, br );
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
    printf("(2)Attempted to write %d bytes and only wrote %lu bytes from descriptor file\n", MAX_DESC_LEN, br );
    exit(-1);
  }

  br = write(pfd, rpl_output.c_str(), rpl_output.length() );
  if ((br==0) || ( br >= MAX_DESC_LEN )) {
    printf("Attempted to write %d bytes and only wrote %lu bytes from descriptor file\n", MAX_DESC_LEN, br );
    exit(-1);
  }

  close( pfd );
  close( cfd );
}


void setupForPCMode()
{
  try {
    WORKER_INPUT_ID = &gpp_app->createWorker( NULL, NULL, (const char*)&ConsumerWorkerDispatchTable  );
    WORKER_OUTPUT_ID = &gpp_app->createWorker( NULL, NULL, (const char*)&ProducerWorkerDispatchTable );
  }
  CATCH_ALL_RETHROW( "creating workers" )


 try { 
      outputPort = &  WORKER_OUTPUT_ID->createOutputPort( PORT_0,
							    config.nBuffers,
							    config.msgSize, NULL);
  }
  CATCH_ALL_RETHROW( "creating source port" );
	

  try {


    static CPI::Util::PValue tprops[] = {
      CPI::Util::PVString("endpoint","cpi-pci-pio://0.0:300000.1.10"),
      CPI::Util::PVEnd };

    inputPort = & WORKER_INPUT_ID->createInputPort(  PORT_0,
						    config.nBuffers,
						    config.msgSize,
						    tprops
						    );
  }
  CATCH_ALL_RETHROW("creating target port")


  if ( config.standalone ) {
    std::string spd = gpp_container->packPortDesc( *outputPort );
    std::string cpd = gpp_container->packPortDesc( *inputPort );
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
    localShadowPort = outputPort->setFinalProviderInfo( remoteTargetPort );
  }
  CATCH_ALL_RETHROW("creating target port")

  // Here we eat the shadow port info since the rpl is currently passive
  if ( config.standalone ) {
    std::string scp = gpp_container->packPortDesc(*inputPort);
    writePortDecsFile( localShadowPort, scp );
    readPortDecsFile( remoteSourcePort, remoteTargetPort );
  }

    // Again we ignor this step for our test
    // Then we send the Loopback container our input descriptor
    // std::string scp = gpp_container->packPortDesc( inputPort );
    // gppSendInputDescPacked( gpp_circuits[0], scp );


    // The CPI output descriptor contains the outputs "shadow port" buffer empty flag
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
  DataTransfer::EventManager* gpp_event_manager;
  static CPI::Util::PValue container_props[] = {CPI::Util::PVString("endpoint",""), CPI::Util::PVEnd };
  CPI::Util::DriverManager dm("Container");
  dm.discoverDevices(0,0);  
  CPI::Util::Device* d = dm.getDevice( container_props, 0 );

  gpp_container = static_cast<CPI::Container::Interface*>(d);
  cpiAssert( dynamic_cast<CPI::Container::Interface*>(d) );
  gpp_event_manager = gpp_container->getEventManager();
  gpp_app = gpp_container->createApplication();


  try { 

#ifdef DONE
    try {
      gpp_cFactory =  new CPI::Container::Factory(CPI::Container::Factory::RCC,CPI_USE_POLLING);
    }
    CATCH_ALL_RETHROW( "creating container");

    // First thing here on the GPP container is that we need to create the workers and
    // the worker ports.  We will use CPIRDT mode 3 for this test. 
    CPI::Container::StartupParams params;
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

    WCI_error wcie;
    try {

      // Enable the workers
      wcie = WORKER_OUTPUT_ID->control(  WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
      CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_INITIALIZE );

      wcie = WORKER_INPUT_ID->control(  WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
      CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_INITIALIZE );

      wcie = WORKER_OUTPUT_ID->control( WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
      CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG );

      wcie = WORKER_INPUT_ID->control( WCI_CONTROL_AFTER_CONFIG, WCI_DEFAULT );
      CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_AFTER_CONFIG );

      wcie = WORKER_OUTPUT_ID->control( WCI_CONTROL_START, WCI_DEFAULT );
      CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_START );

      wcie = WORKER_INPUT_ID->control( WCI_CONTROL_START, WCI_DEFAULT );
      CHECK_WCI_CONROL_ERROR( wcie, WCI_CONTROL_START );

    }
    CATCH_ALL_RETHROW("initializing workers");


    // Now we will just enter a processing loop
    int lc=0;
    int event_id;
    CPI::OS::uint64_t evalue;
    int CPI_RUN_TEST = 1;

    if ( gpp_event_manager ) {
      printf("Running with an event manager\n");
    }
    else {
      printf("Running without a event manager\n");
    }

    while( CPI_RUN_TEST ) {

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

      CPI::OS::sleep( 500 );
      lc++;

      //#define LIMIT_RUN
#ifdef LIMIT_RUN
      if ( lc > (1000 * 10) ) {
	CPI_RUN_TEST = 0;
      }
      CPI::OS::sleep( 1 );
#endif

    }

    printf("About to cleanup \n");

    // Cleanup
    CPI::OS::sleep( 3000 );
    delete gpp_app;
    gpp_container->stop( gpp_event_manager );
    delete gpp_container;
    gpp_container = NULL; 

  }
  catch ( int& ii ) {						      
    printf("gpp: Caught an int exception while %s = %d\n", "main" ,ii );
  }									
  catch( std::string& stri ) {						
    printf("gpp: Caught a string exception while %s = %s\n","main", stri.c_str() );
  }									
  catch ( CPI::Util::EmbeddedException& eex ) {				
    printf(" gpp: Caught an embedded exception while %s:\n", "main");	
    printf( " error number = %d", eex.m_errorCode );			
    printf( " aux info = %s\n", eex.m_auxInfo.c_str() );	        
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


