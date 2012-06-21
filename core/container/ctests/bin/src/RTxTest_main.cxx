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
#include <errno.h>
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
#include <OcpiUtilCommandLineConfiguration.h>

using namespace OCPI::Container;
using namespace OCPI;

// Constants
static Worker * WORKER_PRODUCER_ID;
static Worker * WORKER_CONSUMER_ID;
static Worker * WORKER_LOOPBACK_ID;
static bool loopback;

int  OCPI_RCC_DATA_BUFFER_SIZE   = 10*1024;
int  OCPI_RCC_CONT_NBUFFERS      = 1;

volatile int OCPI_RUN_TEST = 1;
volatile int OCPI_YIELD = 1;

static OCPI::API::Container*     gpp_container;
static OCPI::API::ContainerApplication*   gpp_app;
static OCPI::API::Container*     loopback_container;
static OCPI::API::ContainerApplication*   loopback_app;

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
  std::string transport;
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
    iters(2000),
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
  { OCPI::Util::CommandLineConfiguration::OptionType::STRING,
    "transport", "Use this transport for testing",
    OCPI_CLC_OPT(&OcpiRccBinderConfigurator::transport), 0 },
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
  free(service);

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
  free(service);
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
void swrite( std::string & s )
{
  ocpiDebug("Entering write %d", s.size());
  uint32_t  l = s.size();
  ocpiCheck(write( socket_fd, &l, 4 ) == 4);
  if (l)
    ocpiCheck(write( socket_fd, s.data(), l ) == l);  
}

static 
int
sread( std::string & s  )
{
  ocpiDebug("Entering read");
  uint32_t size;
  char buf[2048];
  ssize_t r = read(socket_fd, &size, 4);
  if (r == 0)
    return 0;
  if (r != 4)
    throw OCPI::Util::Error("Descriptor socket read error %d %d", r, errno);
  if (size) {
    ocpiCheck(read( socket_fd, buf, size) == size);
    s.assign(buf, size);
  } else
    s.clear();
  ocpiDebug("Leaving sread %d", size);
  return size;
}

static void
setupInputPort(Port *ip, const OCPI::API::PValue *props) {
  std::string desc, fb;
  ip->getInitialProviderInfo(props, desc);
  ocpiAssert(desc.size());
  swrite( desc );
  ocpiCheck(sread( desc ));
  ip->setInitialUserInfo( desc, fb );
  swrite( fb );
  if (fb.size() && sread( desc ))
    ip->setFinalUserInfo( desc );
}
static void
setupOutputPort(Port *op, const OCPI::API::PValue */*props */) {
  std::string desc, fb;
  ocpiCheck(sread( desc ));
  op->setInitialProviderInfo( NULL, desc, fb );
  swrite( fb );
  if (fb.size() && sread( desc )) {
    op->setFinalProviderInfo( desc, fb);
    swrite( fb );
  }
}
static 
void setupForPCMode(const OCPI::API::PValue *props)
{
  try {
    WORKER_CONSUMER_ID = OCPI::CONTAINER_TEST::createWorker(gpp_app, &ConsumerWorkerDispatchTable);
    WORKER_PRODUCER_ID = OCPI::CONTAINER_TEST::createWorker(gpp_app,&ProducerWorkerDispatchTable );
  }
  CATCH_ALL_RETHROW( "creating workers" )
  try { 
    pc_outputPort = &WORKER_PRODUCER_ID->createOutputPort( 0,
                                                          OCPI_RCC_CONT_NBUFFERS,
                                                          OCPI_RCC_DATA_BUFFER_SIZE, props);
  }
  CATCH_ALL_RETHROW( "creating output port" );
        
  try {

  pc_inputPort = &WORKER_CONSUMER_ID->createInputPort( 0,
                                                       OCPI_RCC_CONT_NBUFFERS,
                                                       OCPI_RCC_DATA_BUFFER_SIZE,
                                                       props
                                                       );
  }
  CATCH_ALL_RETHROW("creating input port")

  // Now we need to make the connections.  We are connecting our producer to the loopback consumer
  // and the loopback consumer to our producer.
  try {
    setupInputPort(pc_inputPort, props);
    setupOutputPort(pc_outputPort, props);
    ocpiDebug("Done setting pc");
  }
  catch( ... ) {                
    throw OCPI::Util::Error("gpp: Caught an unknown exception while connecting external ports");
  }
  ocpiDebug("Setup is complete");
}

static 
void setupForLoopbackMode(const OCPI::API::PValue *props)
{
  ocpiDebug("*** Create worker");
  try {
    WORKER_LOOPBACK_ID = OCPI::CONTAINER_TEST::createWorker(loopback_app, &LoopbackWorkerDispatchTable);
  }
  CATCH_ALL_RETHROW( "creating workers");

  ocpiDebug("*** Create ports");
  try {
    lb_outputPort = &WORKER_LOOPBACK_ID->createOutputPort( 0,
                                                          OCPI_RCC_CONT_NBUFFERS,
                                                          OCPI_RCC_DATA_BUFFER_SIZE, props);

    //    static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-smb-pio"),
    //						//static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-ofed-rdma"),
    //                                               OCPI::Util::PVEnd };
    lb_inputPort = &WORKER_LOOPBACK_ID->createInputPort( 1,
							 OCPI_RCC_CONT_NBUFFERS,
                                                         OCPI_RCC_DATA_BUFFER_SIZE,
                                                         props
                                                         );
  }
  CATCH_ALL_RETHROW( "creating ports");

  try {
    setupOutputPort(lb_outputPort, props);
    setupInputPort(lb_inputPort, props);
  }
  CATCH_ALL_RETHROW( "connecting ports");

}
#if 0
static 
bool parseArgs( int argc, char** argv)
{
  bool ret =false;
  for ( int n=0; n<argc; n++ ) {
    if (strcmp(argv[n],"-loopback") == 0 ) {
      ocpiDebug("Setting up for loopback mode");
      ret = true;
    }
  }
  return ret;
}
#endif
static 
int gpp_cont()
{

  DataTransfer::EventManager* event_manager = NULL;

    try {

      static OCPI::Util::PValue cprops[] = {
          OCPI::Util::PVBool("polling",1),
          OCPI::Util::PVEnd };

      OCPI::API::Container *c;
      OCPI::API::ContainerApplication *a;

      try { 
	c =  OCPI::API::ContainerManager::find("rcc", NULL, cprops);
	if ( ! c )
	  throw OCPI::Util::EmbeddedException("No Containers found");
	a = c->createApplication();
      }
      CATCH_ALL_RETHROW( "creating container");

      
      // the worker ports.  We will use OCPIRDT mode 3 for this test. 
      if ( ! loopback ) {
          gpp_container = c;
          gpp_app = a;
      }
      else {
        loopback_container = c;
	loopback_app = a;
      }

      
      // We can either take on the role of the producer/consumer or the loopback
      OCPI::Container::Worker *worker;
      // static OCPI::Util::PValue port_props[] = {OCPI::Util::PVString("transport","ocpi-socket-rdma"),
      // static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-ofed-rdma"),
      // static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-smb-pio"),
      const OCPI::API::PValue port_props[] = {OCPI::Util::PVString("transport", config.transport.c_str()),
					       OCPI::Util::PVEnd };
      if ( loopback ) {
        setupForLoopbackMode(port_props);
	worker = WORKER_LOOPBACK_ID;
        WORKER_LOOPBACK_ID->start();
      }
      else {
        try {
          setupForPCMode(port_props);
        }
        CATCH_ALL_RETHROW("setting mode");

	worker = WORKER_CONSUMER_ID;
        try {
          WORKER_PRODUCER_ID->start();
          WORKER_CONSUMER_ID->start();
        }
        CATCH_ALL_RETHROW("initializing workers");

      }

      OCPI_RUN_TEST = 1;

      if ( event_manager ) {
        ocpiDebug("Running with an event manager");
      }
      else {
        ocpiDebug("Running without a event manager");
      }      
      
      unsigned sofar = 0;
      while( OCPI_RUN_TEST && sofar < (unsigned)config.iters) {
	OS::Timer timer(2,0);
	if (!worker->wait(&timer)) {
	  ocpiInfo(" worker terminated");
	  break;
	}
	worker->read(0, sizeof(uint32_t), &sofar);
	ocpiInfo(" received %u", sofar);
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
      ocpiBad("gpp: Caught an int exception = %d", ii );
    }
    catch ( OCPI::Util::EmbeddedException& eex ) {                        
      ocpiBad("gpp main: Caught an embedded exception");  
      ocpiBad("error number = %d", eex.m_errorCode );                        
      ocpiBad("Error = %s", eex.getAuxInfo() );
    }                                                               
    catch( std::string& stri ) {
      ocpiBad("gpp: Caught a string exception = %s", stri.c_str() );
    }
    catch( ... ) {
      ocpiBad("gpp: Caught an unknown exception" );
    }

    ocpiDebug("gpp_container: Good Bye");

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
  if (config.transport.empty())
    config.transport = config.protocol;
  if (config.help) {
    printUsage (config, argv[0]);
    return false;
  }

  // Print out the available protocols
  std::vector<std::string> protolist  = fm.getListOfSupportedProtocols();  
  if ( protolist.size() == 0 ) {
    ocpiBad("There are no resistered transfer drivers linked into this test, exiting");
    exit(-1);
  }
  std::vector<std::string>::iterator epit;
  ocpiDebug("List of supported transfer driver id's:");
  for ( epit=protolist.begin(); epit!=protolist.end(); epit++ ){
    ocpiDebug("  %s", (*epit).c_str() );
  }

  // Print out the available endpoints
  std::vector<std::string>  eplist  = fm.getListOfSupportedEndpoints();
  ocpiDebug("List of supported endpoints:");
  for ( epit=eplist.begin(); epit!=eplist.end(); epit++ ){
    ocpiDebug("  %s", (*epit).c_str() );
  }
  if ( config.show_drivers ) {
    exit(1);
  }
  if ( config.protocol_index >= 0 ) {
    if ( config.protocol_index >= (int)protolist.size() ) {
      ocpiBad("Invalid --pe argument, maximum value is %d", (int)protolist.size() );
      exit(-1);
    }
    std::string p = protolist[config.protocol_index];
    config.protocol = p;
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
    ocpiCheck((socket_fd = server_connect(18077)) >= 0);
    loopback = false;
  }
  else {
    ocpiCheck((socket_fd = client_connect(config.host.c_str() ,18077)) >= 0);
    loopback = true;
  }

  // Start the container in a thead
  gpp_cont();

}

