#define DEBUG 1
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
 *  John Miller -  12/2010
 *  Initial version
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstddef>

#include "OcpiOsAssert.h"
#include "OcpiOsMisc.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilEzxml.h"
#include "XferEndPoint.h"
#include "XferServices.h"
#include "XferFactory.h"
#include "XferManager.h"
#include "OcpiUtilCommandLineConfiguration.h"

namespace XF = DataTransfer;
static int socket_fd;
/****************************************************************************** 
 *
 ******************************************************************************/
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

  
  const char* env = getenv("OCPI_TT_IP_ADDR");
  if( !env || (env[0] == 0)) {
    printf("You can select the interface by setting OCPI_TT_IP_ADDR\n");
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
  }
  else {

    struct sockaddr_in myaddr;

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons((uint16_t)port);
    inet_aton("192.168.100.101", (in_addr*)&myaddr.sin_addr.s_addr);

    sockfd  = socket(PF_INET, SOCK_STREAM, 0);

    printf("BINDING TO  ALTERNATE SOCKET\n");

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof n);

    if ( bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) ) {
      close(sockfd);
      sockfd = -1;
    } 
       
  }


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



/****************************************************************************** 
 *
 ******************************************************************************/
struct HSDesc {
  std::string url;
  uint64_t  cookie;
  HSDesc():cookie(0){}
};



static 
void 
ex_data( HSDesc & ep1, HSDesc & ep2  )
{
  char buf[512];
  write( socket_fd, &ep1.cookie, 8 );  
  size_t us =  ep1.url.size()+1;
  write( socket_fd, &us, sizeof(uint32_t));
  write( socket_fd, ep1.url.c_str(), ep1.url.size()+1 );
  ssize_t l = read( socket_fd, &ep2.cookie, 8); 
  printf("Read %zd bytes, expected %d\n", l , 8);
  l = read( socket_fd, &us, sizeof(uint32_t));    
  ocpiAssert( l == sizeof(uint32_t));
  l = read( socket_fd, buf, us);
  printf("READ (%s) from socket\n", buf );
  ocpiAssert( l > 0 );
  ep2.url = buf;
}

static 
void
hand_shake( HSDesc & my_ep, HSDesc & other_ep )
{
  ex_data( my_ep, other_ep);
}

using namespace OCPI::Util;

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
    iters(10),
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

struct ConnectMemLayout {
  volatile uint32_t   connected;
  uint8_t    url[256];
};
const int BUFFER_SIZE=2;

struct TestMemLayout {
  uint8_t     data[BUFFER_SIZE];
  uint32_t    nbytes;
  volatile uint32_t    full_flag;
};
const int BUFFER_COUNT = 20;
struct MemLayout {
  ConnectMemLayout connection;
  volatile TestMemLayout    buffers[BUFFER_COUNT];
  char *           scratch;
};

class TestBase {
public:
  TestBase( DataTransfer::XferFactory * factory, std::string & ep )
    :m_factory(factory),m_endpoint_url(ep)
  {
    // Get the resources that we will need for the test
    m_endpoint = &factory->getEndPoint( ep, true );
    m_smem     = &m_endpoint->sMemServices();


    printf("About to call MAP !!\n");


    m_Txmem     = (MemLayout*)m_smem->mapTx(0, m_endpoint->size());
    m_Rxmem     = (MemLayout*)m_smem->mapRx(0, m_endpoint->size());

    printf("m_Rxmem after map = %p\n", m_Rxmem);

  }

  ~TestBase()
  {
    m_smem->unMap();
    delete m_endpoint;
  }

  void createXferServices( std::string & other_ep ) {
    m_other_endpoint_url = other_ep;
    m_other_endpoint = &m_factory->getEndPoint( other_ep, false );
    m_other_smem = &m_other_endpoint->sMemServices();

    printf("About to getXferServices\n");

    m_xferServices = &m_factory->getTemplate(*m_endpoint, *m_other_endpoint);
  }
  
  void getConnectionCookie( HSDesc & desc ) {
    desc.cookie = m_xferServices->getConnectionCookie();
  }

  void finalize( HSDesc & desc ) {
    m_xferServices->finalize( desc.cookie );
  }

  void createBufferXferLists()
  {
    // Here for completness we will create a transfer for each of our buffers to each of the other sides buffers
    for ( int n=0; n<BUFFER_COUNT; n++ ) {
      for ( int y=0; y<BUFFER_COUNT; y++ ) {
	size_t s_ff_off = offsetof( struct MemLayout, buffers[0].full_flag ) + sizeof( TestMemLayout ) * n;
	size_t s_data_off = offsetof( struct MemLayout, buffers[0].data ) + sizeof( TestMemLayout ) * n;
	size_t s_nbytes_off = offsetof( struct MemLayout, buffers[0].nbytes ) + sizeof( TestMemLayout ) * n;
	size_t t_ff_off = offsetof( struct MemLayout, buffers[0].full_flag ) + sizeof( TestMemLayout ) * y;
	size_t t_data_off = offsetof( struct MemLayout, buffers[0].data ) + sizeof( TestMemLayout ) * y;
	size_t t_nbytes_off = offsetof( struct MemLayout, buffers[0].nbytes ) + sizeof( TestMemLayout ) * y;
	DataTransfer::XferRequest * req = m_xferServices->createXferRequest();
	req->copy(OCPI_UTRUNCATE(XF::Offset, s_data_off), OCPI_UTRUNCATE(XF::Offset, t_data_off),
		  BUFFER_SIZE, DataTransfer::XferRequest::DataTransfer );    
	req->copy(OCPI_UTRUNCATE(XF::Offset, s_nbytes_off), OCPI_UTRUNCATE(XF::Offset, t_nbytes_off),
		  sizeof(uint32_t), DataTransfer::XferRequest::None );    
	req->copy(OCPI_UTRUNCATE(XF::Offset, s_ff_off), OCPI_UTRUNCATE(XF::Offset, t_ff_off),
		  sizeof(uint32_t) , DataTransfer::XferRequest::FlagTransfer );	

	m_reqs[ n ][ y ] = req;
      }
    }
  }

  volatile uint8_t * getMsgBuffer( int buffer_id, bool Tx )
  {
    if ( Tx ) {
      return m_Txmem->buffers[buffer_id].data;
    }
    else {
      return m_Rxmem->buffers[buffer_id].data;
    }
  }

  void produce( int sbid, int tbid, uint32_t nbytes ) {

    //    ocpiAssert(     m_reqs [ sbid ] [ tbid ] ->getStatus() == DataTransfer::XferRequest::CompleteSuccess );
    m_Txmem->buffers[sbid].full_flag = 1;
    m_Txmem->buffers[sbid].nbytes = nbytes;


    printf("Posting %d bytes to buffer %d,%d \n", nbytes, sbid, tbid );


    m_reqs [ sbid ] [ tbid ] ->post();

#ifdef DEBUG
    while (  m_reqs [ sbid ] [ tbid ] ->getStatus() != DataTransfer::XferRequest::CompleteSuccess ) {      
      printf("Waiting for the posted message to be sent\n");
      OCPI::OS::sleep( 1000 );      
    }
#endif

  }

  void consume( int buffer_id ) {
    m_Txmem->buffers[buffer_id].full_flag = 0;    
  }

  uint32_t waitForMsg( int buffer_id ) {
    while ( m_Rxmem->buffers[buffer_id].full_flag == 0 ) {
           OCPI::OS::sleep( 0 );
    }
    //    OCPI::OS::sleep( 1000 );          
    printf("The full flag =%d\n", m_Rxmem->buffers[buffer_id].full_flag );
    printf("N BYTES  =%d\n", m_Rxmem->buffers[buffer_id].nbytes );
    return  m_Rxmem->buffers[buffer_id].nbytes;
  }

  int32_t checkForMsg( int buffer_id ) {
    if ( m_Rxmem->buffers[buffer_id].full_flag != 0 ) {
      return  m_Rxmem->buffers[buffer_id].nbytes;
    }
    return -1;
  }

  const std::string & endpoint()
  {
    return m_endpoint->name();
  }


protected:
  DataTransfer::XferFactory  * m_factory;
  std::string                  m_endpoint_url;
  DataTransfer::EndPoint     * m_endpoint;
  DataTransfer::SmemServices * m_smem;
  MemLayout                  * m_Txmem;
  MemLayout                  * m_Rxmem;
  DataTransfer::XferServices * m_xferServices;
  std::string                  m_other_endpoint_url;
  DataTransfer::EndPoint     * m_other_endpoint;
  DataTransfer::SmemServices * m_other_smem;

  // transfer lists
  DataTransfer::XferRequest * m_reqs[ BUFFER_COUNT] [ BUFFER_COUNT ];

};



class ServerTest : public TestBase {
public:
  ServerTest( DataTransfer::XferFactory * factory, std::string & ep )
    :TestBase(factory,ep){}

  bool clientConnected(){

    if ( m_Rxmem->connection.connected ) {
      printf("The client URL is (%s)\n",  m_Rxmem->connection.url );
      std::string oep =  (char*)m_Rxmem->connection.url;
      return true;
    }
    return false;
  }

  void setClientEp( std::string & cep )
  {
    createXferServices( cep );    
  }
  
 
  void reflectTillDone()
  {

    printf("******** Reflect till done\n");

    // Here we just take full buffers from our input and send them back
    int count=0;

    for (;;) {

      printf("*****  About to wait for message\n");
      int c = waitForMsg( 0 );
      printf("Got a  message\n");

      if (  c == 0 ) {
	printf("Received %d buffers\n", count );
	printf("Terminating\n");

	// Allow any pending writes to complete
	OCPI::OS::sleep( 1000 );
	break;
      }

      if ( (count%1) == 0 ) 
	printf("Server: b(%d) got a message and sending it back, nbytes = %d\n", count, m_Rxmem->buffers[0].nbytes );


      // We will move the message to our buffer #3 and send it back from there
      memcpy( (void*)&m_Txmem->buffers[3], (void*)&m_Rxmem->buffers[0], sizeof(TestMemLayout) );

      produce( 3, 0, m_Txmem->buffers[0].nbytes );
      consume(0);
      
      count++;
    }
  }
  
};

class ClientTest : public TestBase {
public:

  ClientTest( DataTransfer::XferFactory * factory, std::string & ep  )
    :TestBase(factory,ep){}

  void connect( std::string & ) {

    printf("IN client connect\n");
    
    
    // Create the transfer that sends our URL
    int con_off = offsetof( struct ConnectMemLayout, connected );
    int url_off = offsetof( struct ConnectMemLayout, url );

    // We will use our scratch as a staging area
    int s_start_off = offsetof( struct MemLayout, scratch );

    // Now create the request to send the server our url so it can talk back to us
    DataTransfer::XferRequest * server_init_req = m_xferServices->createXferRequest();
    server_init_req->copy( s_start_off + url_off, url_off, 256, DataTransfer::XferRequest::DataTransfer );
    server_init_req->copy( s_start_off + con_off, con_off, 4, DataTransfer::XferRequest::FlagTransfer );    

    // Now init the data in our local SMB
    MemLayout *scratch = (MemLayout*) m_smem->mapTx( s_start_off, sizeof(MemLayout) );
    strcpy( (char*)scratch->connection.url, (char*)endpoint().c_str() );
    scratch->connection.connected = 1;

    printf("About to post the server connect request\n");
    OCPI::OS::sleep( 1000 );

    server_init_req->post();

    printf("Waiting for request to complete\n");
    while( server_init_req->getStatus() != DataTransfer::XferRequest::CompleteSuccess ) {
      //      printf("request is still pending\n");
      OCPI::OS::sleep( 10 );
    }
    printf("Sent the server connect request\n");
    delete server_init_req;
  }

};


int main( int argc, char** argv )
{
  XF::XferManager & fm = XF::getManager();  

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


#ifdef JWH
  ezxml_t xml_data = NULL;
  const char *err;
  if (config.xml_config.empty())
    printf("No Driver configuration XML file specified\n");
  else if ((err = OCPI::Util::EzXml::ezxml_parse_file(config.xml_config.c_str(), xml_data))) {
    printf("Error parsing file: %s\n", err);
    exit(1);
  } else
    printf("Top level XML node name = %s\n", ezxml_name(xml_data));
  fm.configure ( xml_data );
#endif


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

#if 0
  // Print out the available endpoints
  std::vector<std::string>  eplist  = fm.getListOfSupportedEndpoints();
  printf("\n\n List of supported endpoints:\n");
  for ( epit=eplist.begin(); epit!=eplist.end(); epit++ ){
    printf("  %s\n", (*epit).c_str() );
  }
#endif
  if ( config.show_drivers ) {
    exit(1);
  }
  if ( config.protocol_index > 0 ) {
    if ( config.protocol_index >= (int)protolist.size() ) {
      printf("Invalid --pe argument, maximum value is %d\n", (int)protolist.size() );
      exit(-1);
    }
    std::string p = protolist[config.protocol_index];
    
    printf("Selected protocol = %s\n", p.c_str() );

#if 0
    std::vector<std::string>::iterator it;
    int n=0;
    for ( it=eplist.begin(); it!=eplist.end(); it++ ) {      
      if ( strncmp( p.c_str(), (*it).c_str(),p.size()) == 0 ) {
	config.endpoint_index = n;
	break;
      }
      n++;
    }
#endif
  }

#if 0
  // Get the selected factory for testing;
  if ( config.endpoint.size() == 0 ) {
    config.endpoint = eplist[config.endpoint_index];
  }
#endif

  DataTransfer::XferFactory * factory = fm.find( config.endpoint.c_str(), NULL );  
  ocpiAssert( factory );

  bool tpassed = true;
  try {

    // Now we will test
    if ( config.host.empty() ) {

      printf("\n\n Server Endpoint to use for client connect \n\n");
      ServerTest  server( factory, config.endpoint  );
      printf("%s\n", server.endpoint().c_str() );

      socket_fd = server_connect(18077);
      HSDesc cdesc, sdesc;
      sdesc.url = server.endpoint();

      // For the first handshake we only hand over our url
      hand_shake( sdesc, cdesc );

      // Now we get the client url and connection cookie
      hand_shake(  sdesc, cdesc );

      printf("Server: in HS 1, client ep = %s\n", cdesc.url.c_str());

      // Now we get the other url and allocate our resources.
      server.setClientEp( cdesc.url );


      server.finalize( cdesc );

      printf("Server: in HS 2\n");

      // Now we can give the client our cookie
      server.getConnectionCookie( sdesc );      
      hand_shake( sdesc, cdesc );      

      printf("Server: in HS 3\n");

      server.createBufferXferLists();

      printf("Done creating buffers\n");

      while ( ! server.clientConnected() ) { 
	OCPI::OS::sleep( 0 );
      }
      printf("Server: -> Found a client\n");

      // Now just listen for data and send back what we receive
      server.reflectTillDone();

    }
    else {

      ClientTest client( factory, config.endpoint  );
      //      sleep( 2 );

      printf("Client URL  = \n");
      printf("%s\n\n", client.endpoint().c_str() );

      printf("Connecting to host = %s\n", config.host.c_str() );


      socket_fd = client_connect(config.host.c_str() ,18077);


      HSDesc cdesc, sdesc;

      // Get the servers url
      hand_shake( cdesc, sdesc );

      // Start the connection process
      client.createXferServices( sdesc.url );
      client.getConnectionCookie( cdesc );
      cdesc.url = client.endpoint();

      // Give the server our url and cookie
      hand_shake( cdesc, sdesc );

      // Get the servers connection cookie
      hand_shake( cdesc, sdesc );


      printf("About to finalize the connection\n");
      client.finalize( sdesc );
        
      client.createBufferXferLists();
      client.connect( config.server );

      printf("Client: Running ramp test\n");

      int count=0;

      // For this test we will use buffer 2 as our output buffer and buffer 0 as input
      uint8_t * out_data = (uint8_t*)client.getMsgBuffer(2, true);
      uint8_t * in_data = (uint8_t*)client.getMsgBuffer(0, false);

      for ( int n=0; n<config.iters; n++ ) {

	for ( int y=1; y<BUFFER_SIZE; y++ ) {
	  count++;

	  // Clear the data available flag
	  client.consume(0);

	  int z;
	  for ( z=0; z<y; z++ ) {
	    out_data[z] = (uint8_t)((z + y + 11)%256);
	  }

	  //	  client.produce( 1,4,y );
	  //	  client.produce( 3,5,y );
	  //	  client.produce( 6,6,y );
	  //      client.produce( 4,7,y );


	  client.produce( 2,0,y );

	  //	  OCPI::OS::sleep( 1000 );

	  int c = client.waitForMsg(0);

	  if ( c != y ) {
	    printf("Error: B(%d) Expected %d bytes from server, got %d bytes back\n", count,y, c);
	    tpassed = false;
	    continue;
	  }
	  for ( z=0; z<y; z++ ) {
	    if ( in_data[z] != out_data[z] ) {
	      int o = (int)out_data[z];
	      int i = (int)in_data[z];	      
	      printf("Error: Bad Data : Expected %d from server, got %d at %d\n",o,i,z);
	      tpassed = false;
	    }
	  }
	}
      }

      // Done
      client.produce( 2,0,0);	
    }

    

    OCPI::OS::sleep( 1000 );
  }
  catch ( OCPI::Util::EmbeddedException & ex ) {
    printf("Caught a 'OCPI::Util::EmbeddedException' \n");
    printf(" Error codes = %d, aux = %s\n", ex.getErrorCode(), ex.getAuxInfo() );
    tpassed = false;
  }
  catch ( std::string & str ) {
    printf("ERROR: caught exception %s\n", str.c_str() );
    tpassed = false;
  }
  catch ( ... ) {
    printf("Caught an unknown exception\n");
    tpassed = false;
  }

  // Print out the test results
  if ( ! tpassed ) {
    printf("Transfer test failed\n");
  }
  else {
    printf("Transfer test passed\n");
  }

  return tpassed ? 1 : -1;
}
