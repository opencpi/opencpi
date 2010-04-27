#include <stdio.h>
#include <sstream>
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <CpiTransportServer.h>
#include <CpiTransportClient.h>
#include <CpiRDTInterface.h>
#include <CpiThread.h>
#include <CpiBuffer.h>


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


using namespace CPI::DataTransport;
using namespace DataTransport::Interface;
using namespace CPI::Container;
using namespace CPI;

/*
 *  These are the global parameters that are configured in the vxWorks startup script
 */
/*
char* CPI_RCC_CONT_COMMS_EP    = "cpi-smb-pio://s:300000.1.20";
char* CPI_RCC_LBCONT_COMMS_EP  = "cpi-smb-pio://lb:300000.3.20";
*/

const char* CPI_RCC_CONT_COMMS_EP    = "cpi-socket-rdma://mfs-opencpi-1;40005:600000.2.8";
const char* CPI_RCC_LBCONT_COMMS_EP  = "cpi-socket-rdma://mfs-opencpi-1;40006:600000.4.8";

int  CPI_RCC_DATA_BUFFER_SIZE   = 1024;
int  CPI_RCC_CONT_NBUFFERS      = 1;


// Program globals
static std::string server_end_point;
static std::string loopback_end_point;
static volatile int circuit_count=0;
static MessageCircuit     *gpp_circuits[10];
static MessageCircuit     *loopback_circuit;
static Server *server=NULL;
static Client *client=NULL;


#define CS_DISPATCH if (server)server->dispatch();if(client)client->dispatch();

class TransportSEventHandler : public ServerEventHandler
{
public:

  void newMessageCircuitAvailable( MessageCircuit* new_circuit )
  {
    //		printf("TransportEventHandler::newCircuitAvailable new circuit available\n");
    gpp_circuits[circuit_count++] = new_circuit;
  }

  void dataAvailable( MessageCircuit* circuit ) {

  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  void error( CPI::Util::EmbeddedException& ex )
  {
    printf("TransportEventHandler: Got an exception, (%d%s)\n", ex.getErrorCode(), ex.getAuxInfo() );
  }

};


class TransportCEventHandler : public ClientEventHandler
{
public:

  /**********************************
   *  This method gets called when data is available on a circuit
   **********************************/	
  void dataAvailable( MessageCircuit* circuit )
  {

  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  virtual void error( CPI::Util::EmbeddedException& ex )
  {

  }

};

bool parseArgs( int argc, char** argv)
{
  bool ret =false;
  for ( int n=0; n<argc; n++ ) {
    if (strcmp(argv[n],"-loopback") == 0 ) {
      printf("Setting up for loopback mode\n");
      ret = true;
    }
    else if ( strcmp(argv[n],"-sep") == 0 ) {
      server_end_point = argv[n+1];
      n++;
    }
  }
  return ret;
}


int gpp_cont(int argc, char** argv)
{
  printf("In gpp_cont, Instrumentation turned on\n");
  bool loopback;

  try {
    server_end_point = CPI_RCC_CONT_COMMS_EP;
    loopback = parseArgs(argc,argv);
    TransportCEventHandler* eh=NULL;
    TransportSEventHandler *tcb=NULL;
    if ( !loopback ) {

      // Create the server endpoint and its processing thread
      tcb = new TransportSEventHandler();
      server = new Server( server_end_point, tcb );
      printf("setServerURL \"%s\"\n", server_end_point.c_str() );

      // We need a client to continue
      while ( circuit_count == 0 ) {
	server->dispatch();
	CPI::OS::sleep( 500 );
	printf("Waiting for a client to connect\n");
      }

      

      printf("***** Got a new client !! \n");
      for (int n=0; n<10; n++) {
	CPI::OS::sleep( 500 );
	server->dispatch();
      }

      printf("About to start sending data\n");
      
      int msg_count = 0;
      while( msg_count < 100 ) {
	CPI::DataTransport::Buffer* buffer;	
	buffer = gpp_circuits[0]->getSendMessageBuffer();
	if ( buffer ) {
	  sprintf((char*)buffer->getBuffer(),"message %d\n", msg_count++ );
	  gpp_circuits[0]->sendMessage( buffer, strlen((char*)buffer->getBuffer()) + 1 );
	}
	CPI::OS::sleep( 5 );
	server->dispatch();
      }


    }
    else {
      loopback_end_point = CPI_RCC_LBCONT_COMMS_EP;
      eh = new TransportCEventHandler();
      client = new Client( loopback_end_point, 1024, eh );
      loopback_circuit = client->createCircuit( server_end_point );   

      printf("***** Established a new connection  !! \n");

      while( 1) {
	client->dispatch();
	CPI::DataTransport::Buffer* buffer;	
	CPI::OS::sleep( 5 );
	if ( loopback_circuit->messageAvailable() ) {
	  buffer = loopback_circuit->getNextMessage();
	  if ( buffer ) {
	    printf("Message from server = %s\n", (char*)buffer->getBuffer() );
	    loopback_circuit->freeMessage( buffer );
	  }
	}      
      }
    }
  }
  catch ( ... ) {
    printf("Failed with exception\n");
  }

  return 0;
}

int main( int argc, char** argv)
{
  // Start the container in a thead
  return gpp_cont(argc,argv);
}

