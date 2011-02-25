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

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <math.h>
#include <sstream>
#include <fcntl.h>

#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <OcpiContainerInterface.h>
#include <DtTransferInternal.h>
#include <OcpiContainerPort.h>
#include <OcpiWorker.h>
#include <ConsumerWorker.h>
#include <ProdWorker.h>
#include <LoopbackWorker.h>
#include <test_utilities.h>
#include <OcpiDriver.h>
#include <OcpiUtilCommandLineConfiguration.h>

using namespace OCPI::Container;
using namespace OCPI;

// Constants
static Worker * WORKER_PRODUCER_ID;
static Worker * WORKER_CONSUMER_ID;
static Worker * WORKER_LOOPBACK_ID;
static bool loopback;

int  OCPI_RCC_DATA_BUFFER_SIZE   = 10*1024;
int  OCPI_RCC_CONT_NBUFFERS      = 3;

volatile int OCPI_RUN_TEST = 1;
volatile int OCPI_YIELD = 1;

static OCPI::Container::Interface*     gpp_container;
static OCPI::Container::Application*   gpp_app;
static OCPI::Container::Interface*     loopback_container;
static OCPI::Container::Application*   loopback_app;

Port *pc_inputPort, *pc_outputPort, *lb_inputPort, *lb_outputPort;

// Command line Configuration
class OcpiRccBinderConfigurator
  : public OCPI::Util::CommandLineConfiguration
{
public:
  OcpiRccBinderConfigurator ();

public:
  bool help;
  bool verbose;
  std::string host;
  std::string protocol;
  int  protocol_index;
  std::string endpoint;
  int  endpoint_index;
  std::string server;
  int iters;
  bool show_drivers;
  std::string xml_config;
private:
  static CommandLineConfiguration::Option g_options[];
};
static  OcpiRccBinderConfigurator config;

OcpiRccBinderConfigurator::
OcpiRccBinderConfigurator ()
  : OCPI::Util::CommandLineConfiguration (g_options),
    help (false),
    verbose (false),
    protocol_index(-1),
    endpoint_index(0),
    iters(2000000),
    show_drivers(false),
    xml_config("../dconf.xml")
  
{
}

OCPI::Util::CommandLineConfiguration::Option
OcpiRccBinderConfigurator::g_options[] = {
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "sd", "Show all the available drivers",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::show_drivers), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "surl", "Used in the client test to identify the server url we wll connect to",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::server), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "endpoint", "Use this endpoint for testing",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::endpoint), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "ei", "Use this endpoint (index) for testing",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::endpoint_index), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "protocol", "Use this protocol for testing",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::protocol), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "pi", "Protocol Index, Can be used in place of the string",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::protocol_index), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "h", "Act as the client(slave), use this address to connect to the server",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::host), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::BOOLEAN,
    "verbose", "Be verbose",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::verbose), 0 },
  { OCPI::Util::CommandLineConfiguration::OptionType::LONG,
    "iters", "Number of iterations for test loop",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::iters), 0 },
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



static int socket_fd;
/****************************************************************************** 
 *
 ******************************************************************************/
static 
int client_connect(const char *servername,int port) {
    
  struct addrinfo *res, *t;
  struct addrinfo hints;
  memset( &hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  char *service;
  int n;
  int sockfd = -1;

  if (asprintf(&service, "%d", port) < 0)
    return -1;

  n = getaddrinfo(servername, service, &hints, &res);

  if (n < 0) {
    fprintf(stderr, "%s for %s:%d\n", gai_strerror(n), servername, port);
    return n;
  }

  for (t = res; t; t = t->ai_next) {
    sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
    if (sockfd >= 0) {
      if (!connect(sockfd, t->ai_addr, t->ai_addrlen))
	break;
      close(sockfd);
      sockfd = -1;
    }
  }

  freeaddrinfo(res);

  if (sockfd < 0) {
    fprintf(stderr, "Couldn't connect to %s:%d\n", servername, port);
    return sockfd;
  }
  return sockfd;
}


/****************************************************************************** 
 *
 ******************************************************************************/
static 
int server_connect(int port)
{
  struct addrinfo *res, *t;
  struct addrinfo hints;
  memset( &hints, 0, sizeof(hints));
  hints.ai_flags    = AI_PASSIVE;
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  char *service;
  int sockfd = -1, connfd;
  int n;

  if (asprintf(&service, "%d", port) < 0)
    return -1;

  n = getaddrinfo(NULL, service, &hints, &res);

  if (n < 0) {
    fprintf(stderr, "%s for port %d\n", gai_strerror(n), port);
    return n;
  }

  for (t = res; t; t = t->ai_next) {
    sockfd = socket(t->ai_family, t->ai_socktype, t->ai_protocol);
    if (sockfd >= 0) {
      n = 1;

      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);

      if (!bind(sockfd, t->ai_addr, t->ai_addrlen))
	break;
      close(sockfd);
      sockfd = -1;
    }
  }

  freeaddrinfo(res);

  if (sockfd < 0) {
    fprintf(stderr, "Couldn't listen to port %d\n", port);
    return sockfd;
  }

  listen(sockfd, 1);
  connfd = accept(sockfd, NULL, 0);
  if (connfd < 0) {
    perror("server accept");
    fprintf(stderr, "accept() failed\n");
    close(sockfd);
    return connfd;
  }

  close(sockfd);
  return connfd;
}

static 
int swrite( std::string & s )
{
  uint32_t  l = s.size();
  write( socket_fd, &l, 4 );  
  write( socket_fd, s.c_str(), l );  
}

static 
int
sread( std::string & s  )
{
  uint32_t l;
  uint32_t size;
  char buf[2048];
  l = read( socket_fd, &size, 4 );
  ocpiAssert( l > 0 );  
  l = read( socket_fd, buf, size); 
  ocpiAssert( l == size);  
  s.assign(buf,l);
  return size;
}

static 
void setupForPCMode()
{
  try {
    WORKER_CONSUMER_ID = &gpp_app->createWorker( NULL,NULL, (const char*)&ConsumerWorkerDispatchTable, NULL );
    WORKER_PRODUCER_ID = &gpp_app->createWorker( NULL,NULL, (const char*)&ProducerWorkerDispatchTable, NULL);
  }
  CATCH_ALL_RETHROW( "creating workers" )
  try { 
    pc_outputPort = &WORKER_PRODUCER_ID->createOutputPort( 0,
                                                          OCPI_RCC_CONT_NBUFFERS,
                                                          OCPI_RCC_DATA_BUFFER_SIZE, NULL);
  }
  CATCH_ALL_RETHROW( "creating output port" );
        
  try {

    //    static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-socket-rdma"),
    static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-ofed-rdma"),
    //    static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-smb-pio"),
                                                                                  OCPI::Util::PVEnd };
  pc_inputPort = &WORKER_CONSUMER_ID->createInputPort( 0,
                                                       OCPI_RCC_CONT_NBUFFERS,
                                                       OCPI_RCC_DATA_BUFFER_SIZE,
                                                       c_port_props
                                                       );
  }
  CATCH_ALL_RETHROW("creating input port")

  // Now we need to make the connections.  We are connecting our producer to the loopback consumer
  // and the loopback consumer to our producer.
  try {
    std::string desc, fb;
    desc = pc_inputPort->getInitialProviderInfo(NULL);
    swrite( desc );
    sread( desc );

    fb = pc_inputPort->setInitialUserInfo( desc );
    swrite( fb );
    sread( desc );
    pc_inputPort->setFinalUserInfo( desc );

    sread( desc );
    fb  = pc_outputPort->setInitialProviderInfo( NULL, desc );
    swrite( fb );
    sread( desc );
    fb = pc_outputPort->setFinalProviderInfo( desc);
    swrite( fb );
    printf("Done setting fc\n");

  }
  catch( ... ) {                
    printf("gpp: Caught an unknown exception while connecting external ports\n" );
    throw;
  }
  printf("Setup is complete\n");
}

static 
void setupForLoopbackMode() 
{
  printf("*** Create worker\n");
  try {
    WORKER_LOOPBACK_ID = &loopback_app->createWorker(NULL,NULL, (const char*)&LoopbackWorkerDispatchTable, NULL );
  }
  CATCH_ALL_RETHROW( "creating workers");

  printf("*** Create ports\n");
  try {
    lb_outputPort = &WORKER_LOOPBACK_ID->createOutputPort( 0,
                                                          OCPI_RCC_CONT_NBUFFERS,
                                                          OCPI_RCC_DATA_BUFFER_SIZE, NULL);

    //        static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-smb-pio"),
    static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-ofed-rdma"),
                                               OCPI::Util::PVEnd };
    lb_inputPort = &WORKER_LOOPBACK_ID->createInputPort( 1,
							 OCPI_RCC_CONT_NBUFFERS,
                                                         OCPI_RCC_DATA_BUFFER_SIZE,
                                                         c_port_props
                                                         );
  }
  CATCH_ALL_RETHROW( "creating ports");

  try {
    std::string desc, fb;
    
    sread( desc );
    fb  = lb_outputPort->setInitialProviderInfo( NULL, desc );
    swrite( fb );
    sread( desc );
    fb = lb_outputPort->setFinalProviderInfo( desc);
    swrite( fb );
    desc = lb_inputPort->getInitialProviderInfo(NULL);
    swrite( desc );
    sread( desc );
    fb = lb_inputPort->setInitialUserInfo( desc );
    swrite( fb );
    sread( desc );
    lb_inputPort->setFinalUserInfo( desc );
  }
  CATCH_ALL_RETHROW( "connecting ports");

}

static 
bool parseArgs( int argc, char** argv)
{
  bool ret =false;
  for ( int n=0; n<argc; n++ ) {
    if (strcmp(argv[n],"-loopback") == 0 ) {
      printf("Setting up for loopback mode\n");
      ret = true;
    }
  }
  return ret;
}

extern OCPI::Util::DriverManager dm;
static 
int gpp_cont(int /* argc */, char** /* argv */)
{

  DataTransfer::EventManager* event_manager = NULL;

    try {

      static OCPI::Util::PValue cprops[] = {
          OCPI::Util::PVBool("polling",1),
          OCPI::Util::PVEnd };

        dm.discoverDevices(0,0);


      // First thing here on the GPP container is that we need to create the workers and
      // the worker ports.  We will use OCPIRDT mode 3 for this test. 
      if ( ! loopback ) {
        try { 

          // Create the container
          OCPI::Util::Device* d = dm.getDevice( cprops, "RCC");
          if ( ! d ) {
            throw OCPI::Util::EmbeddedException("No Containers found\n");
          }
          gpp_container = static_cast<OCPI::Container::Interface*>(d);
          gpp_app = gpp_container->createApplication();

        }
        CATCH_ALL_RETHROW( "creating container");

      }
      else {

        OCPI::Util::Device* d = dm.getDevice( cprops, "RCC" );
        if ( ! d ) {
          throw OCPI::Util::EmbeddedException("No Containers found\n");
        }
        loopback_container = static_cast<OCPI::Container::Interface*>(d);
        loopback_app = loopback_container->createApplication();

      }

      // We can either take on the role of the producer/consumer or the loopback
      if ( loopback ) {
        setupForLoopbackMode();
        WORKER_LOOPBACK_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
        WORKER_LOOPBACK_ID->control( WCI_CONTROL_START, WCI_DEFAULT );
      }
      else {
        try {
          setupForPCMode();
        }
        CATCH_ALL_RETHROW("setting mode");

        try {
          WORKER_PRODUCER_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
          WORKER_PRODUCER_ID->control( WCI_CONTROL_START, WCI_DEFAULT );
          WORKER_CONSUMER_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
          WORKER_CONSUMER_ID->control( WCI_CONTROL_START, WCI_DEFAULT );
        }
        CATCH_ALL_RETHROW("initializing workers");

      }

      OCPI_RUN_TEST = 1;

      if ( event_manager ) {
        printf("Running with an event manager\n");
      }
      else {
        printf("Running without a event manager\n");
      }      
      
      while( OCPI_RUN_TEST ) {
        if (!loopback) {
          if ( event_manager ) {
            do {
              gpp_container->dispatch(event_manager);
            } while(1);
          }
          else {
            gpp_container->dispatch( event_manager );
          }
        }
        else {
          loopback_container->dispatch( event_manager );
        }

	// Give the network driver some time
	OCPI::OS::sleep( 0 );        

      }

      // Cleanup
      if ( loopback ) {
        delete loopback_app;
        delete loopback_container;
        loopback_container = NULL;
      }
      else {
        OCPI::OS::sleep( 3000 );
        delete gpp_app;
        delete gpp_container;
        gpp_container = NULL; 
      }
    }
    catch ( int& ii ) {
      printf("gpp: Caught an int exception = %d\n", ii );
    }
    catch( std::string& stri ) {
      printf("gpp: Caught a string exception = %s\n", stri.c_str() );
    }
    catch ( OCPI::Util::EmbeddedException& eex ) {                        
      printf(" \n gpp main: Caught an embedded exception");  
      printf( " error number = %d", eex.m_errorCode );                        
      printf(" Error = %s\n", eex.getAuxInfo() );
    }                                                               
    catch( ... ) {
      printf("gpp: Caught an unknown exception\n" );
    }

    printf("gpp_container: Good Bye\n");

    return 0;
}

static 
void mh(int sn )
{
  ( void ) sn;
  exit(-1);
}


int main( int argc, char** argv)
{
  SignalHandler sh(mh);
  DataTransfer::XferFactoryManager & fm = DataTransfer::XferFactoryManager::getFactoryManager();  

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

  // Print out the available protocols
  std::vector<std::string> protolist  = fm.getListOfSupportedProtocols();  
  if ( protolist.size() == 0 ) {
    printf("There are no resistered transfer drivers linked into this test, exiting\n");
    exit(-1);
  }
  std::vector<std::string>::iterator epit;
  printf("\n\n List of supported transfer driver id's:\n");
  for ( epit=protolist.begin(); epit!=protolist.end(); epit++ ){
    printf("  %s\n", (*epit).c_str() );
  }

  // Print out the available endpoints
  std::vector<std::string>  eplist  = fm.getListOfSupportedEndpoints();
  printf("\n\n List of supported endpoints:\n");
  for ( epit=eplist.begin(); epit!=eplist.end(); epit++ ){
    printf("  %s\n", (*epit).c_str() );
  }
  if ( config.show_drivers ) {
    exit(1);
  }
  if ( config.protocol_index > 0 ) {
    if ( config.protocol_index >= (int)protolist.size() ) {
      printf("Invalid --pe argument, maximum value is %d\n", (int)protolist.size() );
      exit(-1);
    }
    std::string p = protolist[config.protocol_index];
    std::vector<std::string>::iterator it;
    int n=0;
    for ( it=eplist.begin(); it!=eplist.end(); it++ ) {      
      if ( strncmp( p.c_str(), (*it).c_str(),p.size()) == 0 ) {
	config.endpoint_index = n;
	break;
      }
      n++;
    }
  }

  // Get the selected factory for testing;
  if ( config.endpoint.size() == 0 ) {
    config.endpoint = eplist[config.endpoint_index];
  }

  if ( config.host.empty() ) {  
    socket_fd = server_connect(18077);
    loopback = false;
  }
  else {
    socket_fd = client_connect(config.host.c_str() ,18077);
    loopback = true;
  }

  // Start the container in a thead
  gpp_cont(argc,argv);

}

