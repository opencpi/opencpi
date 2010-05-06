#define USE_FS


/*
 * This is a test container that runs on a GPP.  It is used as a loopback tester for ay other CPI compatible
 * conatiner.  The workers in this container generate a test pattern and send it to the remote worker.  It is 
 * expected that the remote worker send the same data back.
 * When it is setup as a server, it will wait until another container makes a connection and will wait 
 * until that container sends its consumer descriptor.  This container will send back its producer descriptor.
 * Then it will exchange its consumer descriptor.
 */


#include <stdio.h>
#include <CpiOsMisc.h>
#include <CpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <CpiContainerInterface.h>
#include <CpiContainerPort.h>
#include <CpiContainerPort.h>
#include <CpiWorker.h>
#include <CpiRDTInterface.h>
#include <ConsumerWorker.h>
#include <ProdWorker.h>
#include <LoopbackWorker.h>
#include <test_utilities.h>

#include <CpiTransportServer.h>
#include <CpiTransportClient.h>
#include <CpiBuffer.h>

#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define PORT_0 0
#define PORT_1 1

#define GPP_CONT_UID 1
#define FCN_CONT_UID 2

struct  PortSetupMessage_t {
  CPI::RDT::PortDescriptorTypes    messageType;
  CPI::OS::uint32_t                length;
};



using namespace CPI::Container;
using namespace CPI;


// Constants
static Worker * WORKER_PRODUCER_ID;
static Worker * WORKER_CONSUMER_ID;
static Worker * WORKER_LOOPBACK_ID;
static bool loopback;


/*
 *  These are the global parameters that are configured in the vxWorks startup script
 */

/*
const char* CPI_RCC_CONT_COMMS_EP    = "cpi-smb-pio://s:300000.1.2";
const char* CPI_RCC_LBCONT_COMMS_EP  = "cpi-smb-pio://lb:300000.2.2";
*/


const char* CPI_RCC_CONT_COMMS_EP    = "cpi-socket-rdma://172.16.89.91;40005:600000.1.4";
const char* CPI_RCC_LBCONT_COMMS_EP  = "cpi-socket-rdma://172.16.89.93;40006:600000.2.4";

/*
//const char* CPI_RCC_CONT_COMMS_EP    = "cpi-ppp-pio://1.0:300000.1.2";
//const char* CPI_RCC_LBCONT_COMMS_EP  = "cpi-ppp-pio://17.0:300000.2.2";
*/


int  CPI_RCC_DATA_BUFFER_SIZE   = 128;
int  CPI_RCC_CONT_NBUFFERS      = 1;
extern volatile bool CPI_TRACE_TX;

volatile int CPI_RUN_TEST = 1;
volatile int CPI_YIELD = 1;
volatile int CPI_INPUT_ID;
volatile int CPI_USE_EVENTS=0;


static CPI::Container::Interface*     gpp_container;
static CPI::Container::Application*   gpp_app;

static CPI::Container::Interface*     loopback_container;
static CPI::Container::Application*   loopback_app;

Port *pc_inputPort, *pc_outputPort, *lb_inputPort, *lb_outputPort;


#define CS_DISPATCH if (server)server->dispatch();if(client)client->dispatch();

// Program globals
static std::string server_end_point;
static std::string loopback_end_point;
static volatile int circuit_count=0;
static CPI::DataTransport::MessageCircuit  *gpp_circuits[10];
static CPI::DataTransport::MessageCircuit  *loopback_circuit;
static CPI::DataTransport::MessageCircuit  *circuit;
static CPI::DataTransport::Server *server=NULL;
static CPI::DataTransport::Client *client=NULL;


#define CS_DISPATCH if (server)server->dispatch();if(client)client->dispatch();

class TransportSEventHandler : public CPI::DataTransport::ServerEventHandler
{
public:

  void newMessageCircuitAvailable( CPI::DataTransport::MessageCircuit* new_circuit )
  {
    //                printf("TransportEventHandler::newCircuitAvailable new circuit available\n");
    circuit = gpp_circuits[circuit_count++] = new_circuit;
    
  }

  void dataAvailable( CPI::DataTransport::MessageCircuit* circuit ) {

  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  void error( CPI::Util::EmbeddedException& ex )
  {
    printf("TransportEventHandler: Got an exception, (%d%s)\n", ex.getErrorCode(), ex.getAuxInfo() );
  }

};


class TransportCEventHandler : public CPI::DataTransport::ClientEventHandler
{
public:

  /**********************************
   *  This method gets called when data is available on a circuit
   **********************************/        
  void dataAvailable( CPI::DataTransport::MessageCircuit* circuit )
  {

  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  virtual void error( CPI::Util::EmbeddedException& ex )
  {

  }

};


const char* fpath = "/h/jmiller/tmp/";
const int MAX_DESC_LEN = 1024;

void writeDesc( std::string& desc, const char* file_name )
{

#ifdef USE_FS
  std::string fn(fpath);
  int fd;
  fn += file_name;
  if ( (fd = open( fn.c_str(), O_CREAT | O_RDWR | O_DIRECT | O_DSYNC, 0666 )) < 0 ) {
    printf("Could not read the port descriptor file (%s)\n",  fn.c_str() );
    printf("Good bye\n");
    exit(-1);
  }

  ssize_t br;
  br = write(fd,desc.c_str(), desc.length() + 1);
  if ( br <= 0  ) {
    printf("Attempted to write %u bytes and only wrote  %lu bytes to %s\n", MAX_DESC_LEN, br, fn.c_str() );
    exit(-1);
  }

  fsync( fd );
  close(fd);
  chmod( fn.c_str(), S_IRWXO | S_IRWXU | S_IRWXG | S_ISUID | S_ISGID | S_ISVTX );

  
  DIR * dir = opendir( fpath );
  closedir( dir );

#else 

  CPI::DataTransport::Buffer* buffer=NULL;

  while ( buffer == NULL ) {
    buffer = circuit->getSendMessageBuffer();
    if ( buffer ) {
      int len =desc.length()+1;
      memcpy(buffer, desc.c_str(), len);
      circuit->sendMessage( buffer, len );
    }
    else{
      CPI::OS::sleep( 1000 );
      printf("Waiting for another send buffer \n");
    }
  }


#endif
}


void writeDesc( Port * desc, const char* file_name )
{
  std::string sdesc;
  if ( ! loopback ) {
    sdesc = gpp_container->packPortDesc(*desc);
  }
  else {
    sdesc = loopback_container->packPortDesc(*desc);
  }
  writeDesc( sdesc, file_name );
}


std::string readDesc( const char* file_name )
{

#ifdef USE_FS
  std::string fn(fpath);
  int fd;
  fn += file_name;
  int retries = 1000;
  while ( retries-- > 0 ) {
    printf("About to test file access\n");
    DIR * dir = opendir( fpath );
    closedir( dir );
    if ( access( fn.c_str(), R_OK ) ) {
      printf("access() failed for %s \n", fn.c_str() );
      CPI::OS::sleep( 2000 );
      continue;
    }
    else {
      break;
    }
  }
  while ( retries-- > 0 ) {
    if ( (fd = open( fn.c_str(), O_RDONLY )) < 0 ) {
      printf("Read failed for %s, fd = %d\n", fn.c_str(), fd );
      CPI::OS::sleep( 2000 );
      continue;
    }
    else {
      break;
    }
  }
  if ( fd  < 0 ) {
    printf("Could not read the port descriptor file (%s)\n",  fn.c_str() );
    printf("Good bye\n");
    exit(-1);
  }

  ssize_t br;
  const int MAX_DESC_LEN = 1024;
  char buf[MAX_DESC_LEN];
  
  retries = 10;
  while ( retries-- > 0 ) {
    br = read(fd,buf,MAX_DESC_LEN);
    printf("read %ld bytes of data\n",(long int) br );
    if ( br > 0 ) {
      break;
    }
    CPI::OS::sleep(1000);
  }

  if ( br <= 0  ) {
    printf("Attempted to read %u bytes and only read %ld bytes from descriptor file\n", MAX_DESC_LEN, (long int)br );

    close(fd);
    exit(-1);
  }
  close(fd);
  std::string tbuf;
  tbuf.assign(buf,br);
  return tbuf;
#else


  CPI::DataTransport::Buffer* buffer=NULL;  
  while ( buffer == NULL ) {

    printf("Checking for Buffer\n");

    if ( circuit->messageAvailable() ) {

      printf("GOT A BUFFER !!\n");

      buffer = circuit->getNextMessage();
      printf("Message from server = %s\n", (char*)buffer->getBuffer() );
      std::string tbuf;
      tbuf.assign( (const char*)buffer->getBuffer(), buffer->getLength() );    
      loopback_circuit->freeMessage( buffer );
      return tbuf;
    }
    else {
      CPI::OS::sleep( 1000 );
      printf("Waiting for a message\n");
    }
  }



#endif






}



void dumpPd( PortData& pd ) 
{
  /*
      CPI::OS::uint32_t  nBuffers;
      CPI::OS::uint64_t  dataBufferBaseAddr;
      CPI::OS::uint32_t  dataBufferPitch;
      CPI::OS::uint32_t  dataBufferSize;
      CPI::OS::uint64_t  metaDataBaseAddr;
      CPI::OS::uint32_t  metaDataPitch;
      CPI::OS::uint64_t  fullFlagBaseAddr; 
      CPI::OS::uint32_t  fullFlagSize;
      CPI::OS::uint32_t  fullFlagPitch;
      CPI::OS::uint64_t  fullFlagValue;
      CPI::OS::uint64_t  emptyFlagBaseAddr; // when consumer is passive
      CPI::OS::uint32_t  emptyFlagSize;
      CPI::OS::uint32_t  emptyFlagPitch;
      CPI::OS::uint64_t  emptyFlagValue;
      OutOfBandData       oob;
*/

      printf(" Desscriptor:\n");
      printf("   nBuffers = %d\n", pd.connectionData.data.desc.nBuffers );
      printf("   data off = %lld\n", (long long)pd.connectionData.data.desc.dataBufferBaseAddr );
      printf("   data pitch = %d\n", pd.connectionData.data.desc.dataBufferPitch );
      printf("   data size = %d\n", pd.connectionData.data.desc.dataBufferSize );
      printf("   meta off = %lld\n", (long long)pd.connectionData.data.desc.metaDataBaseAddr );
      printf("   metapitch = %d\n", pd.connectionData.data.desc.metaDataPitch );
      printf("   full flag off = %lld\n", (long long)pd.connectionData.data.desc.fullFlagBaseAddr );
      printf("   ff size = %d\n", pd.connectionData.data.desc.fullFlagSize);
      printf("   ff pitch = %d\n", pd.connectionData.data.desc.fullFlagPitch );
      printf("   ff value = %lld\n", (long long)pd.connectionData.data.desc.fullFlagValue );
      printf("   empty flag off = %lld\n", (long long)pd.connectionData.data.desc.emptyFlagBaseAddr );
      printf("   ef size = %d\n", pd.connectionData.data.desc.emptyFlagSize);
      printf("   ef pitch = %d\n", pd.connectionData.data.desc.emptyFlagPitch );
      printf("   ef value = %lld\n", (long long)pd.connectionData.data.desc.emptyFlagValue );





}


void setupForPCMode()
{
  try {
    WORKER_CONSUMER_ID = &gpp_app->createWorker( NULL,NULL, (const char*)&ConsumerWorkerDispatchTable, NULL );
    WORKER_PRODUCER_ID = &gpp_app->createWorker( NULL,NULL, (const char*)&ProducerWorkerDispatchTable, NULL);
  }
  CATCH_ALL_RETHROW( "creating workers" )

  try { 
    pc_outputPort = &WORKER_PRODUCER_ID->createOutputPort( PORT_0,
                                                          CPI_RCC_CONT_NBUFFERS,
                                                          CPI_RCC_DATA_BUFFER_SIZE, NULL);
  }
  CATCH_ALL_RETHROW( "creating output port" );
        

  try {


    //    static CPI::Util::PValue c_port_props[] = {CPI::Util::PVString("protocol","cpi-socket-rdma"),
    static CPI::Util::PValue c_port_props[] = {CPI::Util::PVString("protocol","cpi-ppp-dma"),
                                                                                  CPI::Util::PVEnd };
  pc_inputPort = &WORKER_CONSUMER_ID->createInputPort( PORT_0,
                                                       CPI_RCC_CONT_NBUFFERS,
                                                       CPI_RCC_DATA_BUFFER_SIZE,
                                                       c_port_props
                                                       );
  }
  CATCH_ALL_RETHROW("creating input port")

  // Now we need to make the connections.  We are connecting our producer to the loopback consumer
  // and the loopback consumer to our producer.
  std::string localShadowPort;

  try {

    // We will write our input desc to a file named pc_input.desc
    writeDesc( pc_inputPort, "pc_input.desc" );

    std::string s_lb_tDesc;
    // We will read the loopback input descriptor  lb_input.desc 
    s_lb_tDesc = readDesc( "lb_input.desc" );  


    printf("Got the input descriptor from lb !!\n");

    // Connect our output to the remote input port
    localShadowPort = pc_outputPort->setFinalProviderInfo( s_lb_tDesc );

    printf("Done setting the IP\n");

    // We will write our output descriptor pc_output.desc
    writeDesc(localShadowPort, "pc_output.desc" );


    std::string s_lb_sDesc;
    // We will wait until the loopback writes its descriptor in lb_output.desc
    s_lb_sDesc =  readDesc( "lb_output.desc" );

    printf("Got the output descriptor from lb !!\n");

    // Setup our input to provide flow control to the connected output
    pc_inputPort->setFinalUserInfo( s_lb_sDesc );


    printf("Done setting fc\n");


  }
  catch( ... ) {                
    printf("gpp: Caught an unknown exception while connecting external ports\n" );
    throw;
  }

  printf("Setup is complete\n");
}





void setupForLoopbackMode() 
{


  printf("*** Create worker\n");

  try {
    WORKER_LOOPBACK_ID = &loopback_app->createWorker(NULL,NULL, (const char*)&LoopbackWorkerDispatchTable, NULL );
  }
  CATCH_ALL_RETHROW( "creating workers");



  printf("*** Create ports\n");

  try {
    lb_outputPort = &WORKER_LOOPBACK_ID->createOutputPort( PORT_0,
                                                          CPI_RCC_CONT_NBUFFERS,
                                                          CPI_RCC_DATA_BUFFER_SIZE, NULL);

    static CPI::Util::PValue c_port_props[] = {CPI::Util::PVString("protocol","cpi-socket-rdma"),
                                               //    static CPI::Util::PValue c_port_props[] = {CPI::Util::PVString("protocol","cpi-ppp-dma"),
                                               CPI::Util::PVEnd };
    lb_inputPort = &WORKER_LOOPBACK_ID->createInputPort( PORT_1,
                                                          CPI_RCC_CONT_NBUFFERS,
                                                         CPI_RCC_DATA_BUFFER_SIZE,
                                                         c_port_props
                                                         );
  }
  CATCH_ALL_RETHROW( "creating ports");


  printf("*** write desc 1\n");


  // We will write our output descriptor pc_output.desc
  writeDesc(lb_inputPort, "lb_input.desc" );


  printf("*** read desc 1\n");

  // Now wait till we get our producers' consumer descriptor
  std::string pc_tDesc;
  pc_tDesc = readDesc("pc_input.desc" );  


  // We will connect our producer to the remote consumer.  We got the consumers descriptor out of band
  // but it is complete and will allow us to establish this connection without any other information.  The
  // Consumer however is waiting for us to provide it with feedback control information so that it can complete
  // its portion of this conection.  We are not able to provide this information until we create the connections
  // since it is dependent on our "shadow" input buffers.

  printf("*** connect input\n");

  std::string localShadowPort;
  try {
    localShadowPort = lb_outputPort->setFinalProviderInfo( pc_tDesc );
  }
  CATCH_ALL_RETHROW( "connecting input port");

  printf("*** write desc 2\n");

  // So now we can send the "shadow" port descriptor back to the consumer so that they can complete the 
  // circuit
  writeDesc( localShadowPort, "lb_output.desc");


  printf("*** read desc 1\n");

  // Now we will block until we get the producer's "buffer empty" flag descriptor back.
  std::string pc_sDesc;
  pc_sDesc = readDesc("pc_output.desc" );

  try {
    lb_inputPort->setFinalUserInfo( pc_sDesc );
  }
  CATCH_ALL_RETHROW( "configuring output flow control");

  printf("Setup is complete\n");

}


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


static CPI::Util::DriverManager dm("Container");
int gpp_cont(int argc, char** argv)
{
  printf("In gpp_cont, Instrumentation turned on\n");
  DataTransfer::EventManager* event_manager = NULL;

    try {
      loopback = parseArgs(argc,argv);

      static CPI::Util::PValue cprops[] = {
          CPI::Util::PVBool("polling",1),
          CPI::Util::PVEnd };

        dm.discoverDevices(0,0);


#ifndef USE_FS
      server_end_point = CPI_RCC_CONT_COMMS_EP;
      TransportCEventHandler* eh=NULL;
      TransportSEventHandler *tcb=NULL;
      if ( !loopback ) {

        // Create the server endpoint and its processing thread
        tcb = new TransportSEventHandler();
        server = new CPI::DataTransport::Server( server_end_point, tcb );
        printf("setServerURL \"%s\"\n", server_end_point.c_str() );

        // We need a client to continue
        while ( circuit_count == 0 ) {
          server->dispatch();
          CPI::OS::sleep( 500 );
          printf("Waiting for a client to connect\n");
        }
        printf("***** Got a new client !! \n");
        for (int n=0; n<20; n++) {
          CPI::OS::sleep( 5 );
          server->dispatch();
        }
      }
      else {
        loopback_end_point = CPI_RCC_LBCONT_COMMS_EP;
        eh = new TransportCEventHandler();
        client = new CPI::DataTransport::Client( loopback_end_point, 1024, eh );
        circuit = loopback_circuit = client->createCircuit( server_end_point );           
        printf("***** Established a new connection  !! \n");
      }
#endif




      // First thing here on the GPP container is that we need to create the workers and
      // the worker ports.  We will use CPIRDT mode 3 for this test. 
      if ( ! loopback ) {


        // Bootstrap the communications
        




        try { 

          // Create the container
          CPI::Util::Device* d = dm.getDevice( cprops, "RCC");
          if ( ! d ) {
            throw CPI::Util::EmbeddedException("No Containers found\n");
          }
          gpp_container = static_cast<CPI::Container::Interface*>(d);
          gpp_app = gpp_container->createApplication();

        }
        CATCH_ALL_RETHROW( "creating container");

      }
      else {

        CPI::Util::Device* d = dm.getDevice( cprops, "RCC" );
        if ( ! d ) {
          throw CPI::Util::EmbeddedException("No Containers found\n");
        }
        loopback_container = static_cast<CPI::Container::Interface*>(d);
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

        printf("\n\nWaiting 2 secs to continue ....\n");
        for (int y=0; y<10; y++ ) {
          gpp_container->dispatch(event_manager);
          CPI::OS::sleep( 200 );                
        }


        try {

          WORKER_PRODUCER_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
          WORKER_PRODUCER_ID->control( WCI_CONTROL_START, WCI_DEFAULT );

          WORKER_CONSUMER_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
          WORKER_CONSUMER_ID->control( WCI_CONTROL_START, WCI_DEFAULT );

        }
        CATCH_ALL_RETHROW("initializing workers");

      }

      CPI_RUN_TEST = 1;

      if ( event_manager ) {
        printf("Running with an event manager\n");
      }
      else {
        printf("Running without a event manager\n");
      }      
      

      while( CPI_RUN_TEST ) {
                        
        if (!loopback) {

          // Block here until we get a PPP mailbox notification
          // We will get a notification for the following reasons
          // 1) One of our input buffers has been filled
          // 2) A DMA has completed and we have a output buffer that is now empty
          // 3) A timeout has occured
          if ( event_manager ) {
            do {
              gpp_container->dispatch(event_manager);

              /*
              if ( event_manager->wait_for_event( 100, event_id, evalue ) == DataTransfer::EventTimeout ) {
                printf("We have not recieved an event for 100 uSec.\n");
              }
              else {
                gpp_container->dispatch( event_manager );
                break;
              }
              */

            } while(1);
          }
          else {
            gpp_container->dispatch( event_manager );
          }

        }
        else {
          loopback_container->dispatch( event_manager );
        }

        //        CPI::OS::sleep( 10 );        

      }

      // Cleanup
      if ( loopback ) {
        delete loopback_app;
        delete loopback_container;
        loopback_container = NULL;
      }
      else {
        CPI::OS::sleep( 3000 );
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
    catch ( CPI::Util::EmbeddedException& eex ) {                        
      printf(" \n gpp main: Caught an embedded exception");  
      printf( " error number = %d", eex.m_errorCode );                        
    }                                                               
    catch( ... ) {
      printf("gpp: Caught an unknown exception\n" );
    }

    printf("gpp_container: Good Bye\n");

    return 0;
}


int gpp_lb()
{
  const char* argv[] = {"main","-loopback","-ep", CPI_RCC_CONT_COMMS_EP, 0};
  return gpp_cont( 2, (char**)argv );
}



void cpi_stop()
{
  CPI_RUN_TEST = 0;
}

void mh(int sn )
{
  exit(-1);
}


int main( int argc, char** argv)
{
  SignalHandler sh(mh);

  // Start the container in a thead
  gpp_cont(argc,argv);

  //#define RESTART_TEST
#ifdef RESTART_TEST
  // rerun a few times for test
  gpp_cont(argc,argv);
  gpp_cont(argc,argv);
  gpp_cont(argc,argv);
  gpp_cont(argc,argv);
#endif


}

