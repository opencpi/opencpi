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

#define USE_FS


/*
 * This is a test container that runs on a GPP.  It is used as a loopback tester for ay other OCPI compatible
 * conatiner.  The workers in this container generate a test pattern and send it to the remote worker.  It is 
 * expected that the remote worker send the same data back.
 * When it is setup as a server, it will wait until another container makes a connection and will wait 
 * until that container sends its consumer descriptor.  This container will send back its producer descriptor.
 * Then it will exchange its consumer descriptor.
 */


#include <stdio.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <OcpiContainerInterface.h>
#include <OcpiContainerPort.h>
#include <OcpiContainerPort.h>
#include <OcpiContainerApplication.h>
#include <OcpiWorker.h>
#include <OcpiRDTInterface.h>
#include <ConsumerWorker.h>
#include <ProdWorker.h>
#include <LoopbackWorker.h>
#include <test_utilities.h>

#include <OcpiTransportServer.h>
#include <OcpiTransportClient.h>
#include <OcpiBuffer.h>

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
  OCPI::RDT::PortDescriptorTypes    messageType;
  OCPI::OS::uint32_t                length;
};

using namespace OCPI::Container;
using namespace OCPI;

// Constants
static OCPI::Container::Worker * WORKER_PRODUCER_ID;
static OCPI::Container::Worker * WORKER_CONSUMER_ID;
static OCPI::Container::Worker * WORKER_LOOPBACK_ID;
static bool loopback;


int  OCPI_RCC_DATA_BUFFER_SIZE   = 1*1024;
int  OCPI_RCC_CONT_NBUFFERS      = 4;
extern volatile bool OCPI_TRACE_TX;

volatile int OCPI_RUN_TEST = 1;
volatile int OCPI_YIELD = 1;
volatile int OCPI_INPUT_ID;
volatile int OCPI_USE_EVENTS=0;


static OCPI::API::Container*     gpp_container;
static OCPI::API::ContainerApplication*   gpp_app;

static OCPI::API::Container*     loopback_container;
static OCPI::API::ContainerApplication*   loopback_app;

Port *pc_inputPort, *pc_outputPort, *lb_inputPort, *lb_outputPort;


#define CS_DISPATCH if (server)server->dispatch();if(client)client->dispatch();

// Program globals
static std::string server_end_point;
static std::string loopback_end_point;
static volatile int circuit_count=0;
static OCPI::DataTransport::MessageCircuit  *gpp_circuits[10];
static OCPI::DataTransport::MessageCircuit  *circuit;

#define CS_DISPATCH if (server)server->dispatch();if(client)client->dispatch();

class TransportSEventHandler : public OCPI::DataTransport::ServerEventHandler
{
public:

  void newMessageCircuitAvailable( OCPI::DataTransport::MessageCircuit* new_circuit )
  {
    //                printf("TransportEventHandler::newCircuitAvailable new circuit available\n");
    circuit = gpp_circuits[circuit_count++] = new_circuit;
    
  }

  void dataAvailable( OCPI::DataTransport::MessageCircuit* circuit ) {
    ( void ) circuit;
  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  void error( OCPI::Util::EmbeddedException& ex )
  {
    printf("TransportEventHandler: Got an exception, (%d%s)\n", ex.getErrorCode(), ex.getAuxInfo() );
  }

};


class TransportCEventHandler : public OCPI::DataTransport::ClientEventHandler
{
public:

  /**********************************
   *  This method gets called when data is available on a circuit
   **********************************/        
  void dataAvailable( OCPI::DataTransport::MessageCircuit* circuit )
  {
    ( void ) circuit;
  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  virtual void error( OCPI::Util::EmbeddedException& ex )
  {
    ( void ) ex;
  }

};


const char* fpath = "./tmp/";
const int MAX_DESC_LEN = 1024;

void writeDesc( std::string& desc, const char* file_name )
{

#ifdef USE_FS
  struct stat s;
  if (stat(fpath, &s) != 0 &&
      mkdir(fpath, 0777) != 0) {
    printf("Can't create directory %s\n", fpath);
    exit(1);
  }
  std::string fn(fpath);
  int fd;
  fn += file_name;
#ifndef O_DIRECT
#define O_DIRECT 0
#endif
  if ( (fd = open( fn.c_str(), O_CREAT | O_RDWR , 0666 )) < 0 ) {
    printf("Could not read the port descriptor file (%s)\n",  fn.c_str() );
    printf("Good bye\n");
    exit(-1);
  }

  ssize_t br;
  br = write(fd,desc.c_str(), desc.length() + 1);
  if ( br <= 0  ) {
    printf("Attempted to write %u bytes and only wrote  %" PRIsize_t " bytes to %s\n",
            MAX_DESC_LEN, br, fn.c_str() );
    exit(-1);
  }

  fsync( fd );
  close(fd);
  chmod( fn.c_str(), S_IRWXO | S_IRWXU | S_IRWXG | S_ISUID | S_ISGID | S_ISVTX );

  
  DIR * dir = opendir( fpath );
  closedir( dir );

#else 

  OCPI::DataTransport::Buffer* buffer=NULL;

  while ( buffer == NULL ) {
    buffer = circuit->getSendMessageBuffer();
    if ( buffer ) {
      int len =desc.length()+1;
      memcpy(buffer, desc.c_str(), len);
      circuit->sendMessage( buffer, len );
    }
    else{
      OCPI::OS::sleep( 1000 );
      printf("Waiting for another send buffer \n");
    }
  }


#endif
}


void writeDesc( Port * desc, const char* file_name )
{
  std::string sdesc;
  sdesc = OCPI::Container::Container::packPortDesc(*desc);
  writeDesc( sdesc, file_name );
}


std::string readDesc( const char* file_name )
{

#ifdef USE_FS
  std::string fn(fpath);
  int fd = -1;
  fn += file_name;
  int retries = 1000;
  while ( retries-- > 0 ) {
    printf("About to test file access\n");
    DIR * dir = opendir( fpath );
    closedir( dir );
    if ( access( fn.c_str(), R_OK ) ) {
      printf("access() failed for %s \n", fn.c_str() );
      OCPI::OS::sleep( 2000 );
      continue;
    }
    else {
      break;
    }
  }
  while ( retries-- > 0 ) {
    if ( (fd = open( fn.c_str(), O_RDONLY )) < 0 ) {
      printf("Read failed for %s, fd = %d\n", fn.c_str(), fd );
      OCPI::OS::sleep( 2000 );
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
    OCPI::OS::sleep(1000);
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


  OCPI::DataTransport::Buffer* buffer=NULL;  
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
      OCPI::OS::sleep( 1000 );
      printf("Waiting for a message\n");
    }
  }



#endif






}



void dumpPd( PortData& pd ) 
{
  /*
      OCPI::OS::uint32_t  nBuffers;
      OCPI::OS::uint64_t  dataBufferBaseAddr;
      OCPI::OS::uint32_t  dataBufferPitch;
      OCPI::OS::uint32_t  dataBufferSize;
      OCPI::OS::uint64_t  metaDataBaseAddr;
      OCPI::OS::uint32_t  metaDataPitch;
      OCPI::OS::uint64_t  fullFlagBaseAddr; 
      OCPI::OS::uint32_t  fullFlagSize;
      OCPI::OS::uint32_t  fullFlagPitch;
      OCPI::OS::uint64_t  fullFlagValue;
      OCPI::OS::uint64_t  emptyFlagBaseAddr; // when consumer is passive
      OCPI::OS::uint32_t  emptyFlagSize;
      OCPI::OS::uint32_t  emptyFlagPitch;
      OCPI::OS::uint64_t  emptyFlagValue;
      OutOfBandData       oob;
*/

      printf(" Desscriptor:\n");
      printf("   nBuffers = %d\n", pd.getData().data.desc.nBuffers );
      printf("   data off = %lld\n", (long long)pd.getData().data.desc.dataBufferBaseAddr );
      printf("   data pitch = %d\n", pd.getData().data.desc.dataBufferPitch );
      printf("   data size = %d\n", pd.getData().data.desc.dataBufferSize );
      printf("   meta off = %lld\n", (long long)pd.getData().data.desc.metaDataBaseAddr );
      printf("   metapitch = %d\n", pd.getData().data.desc.metaDataPitch );
      printf("   full flag off = %lld\n", (long long)pd.getData().data.desc.fullFlagBaseAddr );
      printf("   ff size = %d\n", pd.getData().data.desc.fullFlagSize);
      printf("   ff pitch = %d\n", pd.getData().data.desc.fullFlagPitch );
      printf("   ff value = %lld\n", (long long)pd.getData().data.desc.fullFlagValue );
      printf("   empty flag off = %lld\n", (long long)pd.getData().data.desc.emptyFlagBaseAddr );
      printf("   ef size = %d\n", pd.getData().data.desc.emptyFlagSize);
      printf("   ef pitch = %d\n", pd.getData().data.desc.emptyFlagPitch );
      printf("   ef value = %lld\n", (long long)pd.getData().data.desc.emptyFlagValue );





}


void setupForPCMode()
{
  try {
    WORKER_CONSUMER_ID = OCPI::CONTAINER_TEST::createWorker(gpp_app, &ConsumerWorkerDispatchTable);
    WORKER_PRODUCER_ID = OCPI::CONTAINER_TEST::createWorker(gpp_app,&ProducerWorkerDispatchTable );
  }
  CATCH_ALL_RETHROW( "creating workers" )

  try { 
    pc_outputPort = &WORKER_PRODUCER_ID->createOutputPort( PORT_0,
                                                          OCPI_RCC_CONT_NBUFFERS,
                                                          OCPI_RCC_DATA_BUFFER_SIZE, NULL);
  }
  CATCH_ALL_RETHROW( "creating output port" );
        

  try {


//   static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-socket-rdma"),
//static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-ofed-rdma"),
    static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-smb-pio"),
                                                                                  OCPI::Util::PVEnd };
  pc_inputPort = &WORKER_CONSUMER_ID->createInputPort( PORT_0,
                                                       OCPI_RCC_CONT_NBUFFERS,
                                                       OCPI_RCC_DATA_BUFFER_SIZE,
                                                       c_port_props
                                                       );
  }
  CATCH_ALL_RETHROW("creating input port")

  // Now we need to make the connections.  We are connecting our producer to the loopback consumer
  // and the loopback consumer to our producer.
  std::string localShadowPort;

  try {

    std::string desc, fb;

    desc = pc_inputPort->getInitialProviderInfo(NULL);
    writeDesc( desc, "pc_input1.desc" );
    desc = readDesc( "lb_output1.desc" );  
    fb = pc_inputPort->setInitialUserInfo( desc );
    writeDesc( fb, "pc_input2.desc" );
    desc = readDesc( "lb_output2.desc" );  
    pc_inputPort->setFinalUserInfo( desc );


    desc = readDesc( "lb_input1.desc" );  
    fb  = pc_outputPort->setInitialProviderInfo( NULL, desc );
    writeDesc(fb, "pc_output1.desc" );
    desc = readDesc( "lb_input2.desc" );  
    fb = pc_outputPort->setFinalProviderInfo( desc);
    writeDesc(fb, "pc_output2.desc" );

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
    WORKER_LOOPBACK_ID = OCPI::CONTAINER_TEST::createWorker(loopback_app, &LoopbackWorkerDispatchTable);
  }
  CATCH_ALL_RETHROW( "creating workers");



  printf("*** Create ports\n");

  try {
    lb_outputPort = &WORKER_LOOPBACK_ID->createOutputPort( PORT_0,
                                                          OCPI_RCC_CONT_NBUFFERS,
                                                          OCPI_RCC_DATA_BUFFER_SIZE, NULL);

            static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-smb-pio"),
    //static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-ofed-rdma"),
                     //    static OCPI::Util::PValue c_port_props[] = {OCPI::Util::PVString("protocol","ocpi-ppp-dma"),
                                               OCPI::Util::PVEnd };
    lb_inputPort = &WORKER_LOOPBACK_ID->createInputPort( PORT_1,
                                                          OCPI_RCC_CONT_NBUFFERS,
                                                         OCPI_RCC_DATA_BUFFER_SIZE,
                                                         c_port_props
                                                         );
  }
  CATCH_ALL_RETHROW( "creating ports");


  try {
    std::string desc, fb;

    desc = readDesc( "pc_input1.desc" );  
    fb  = lb_outputPort->setInitialProviderInfo( NULL, desc );
    writeDesc(fb, "lb_output1.desc" );
    desc = readDesc( "pc_input2.desc" );  
    fb = lb_outputPort->setFinalProviderInfo( desc);
    writeDesc(fb, "lb_output2.desc" );

    desc = lb_inputPort->getInitialProviderInfo(NULL);
    writeDesc( desc, "lb_input1.desc" );
    desc = readDesc( "pc_output1.desc" );  
    fb = lb_inputPort->setInitialUserInfo( desc );
    writeDesc( fb, "lb_input2.desc" );
    desc = readDesc( "pc_output2.desc" );  
    lb_inputPort->setFinalUserInfo( desc );
  }
  CATCH_ALL_RETHROW( "connecting ports");

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


//extern OCPI::Util::DriverManager dm;
int gpp_cont(int argc, char** argv)
{
  printf("In gpp_cont, Instrumentation turned on\n");

  //  OCPI::Util::DriverManager dm("Container");

  DataTransfer::EventManager* event_manager = NULL;

    try {
      loopback = parseArgs(argc,argv);

      static OCPI::Util::PValue cprops[] = {
          OCPI::Util::PVBool("polling",1),
          OCPI::Util::PVEnd };

      //        dm.discoverDevices(0,0);


#ifndef USE_FS
      server_end_point = OCPI_RCC_CONT_COMMS_EP;
      TransportCEventHandler* eh=NULL;
      TransportSEventHandler *tcb=NULL;
      if ( !loopback ) {

        // Create the server endpoint and its processing thread
        tcb = new TransportSEventHandler();
        server = new OCPI::DataTransport::Server( server_end_point, tcb );
        printf("setServerURL \"%s\"\n", server_end_point.c_str() );

        // We need a client to continue
        while ( circuit_count == 0 ) {
          server->dispatch();
          OCPI::OS::sleep( 500 );
          printf("Waiting for a client to connect\n");
        }
        printf("***** Got a new client !! \n");
        for (int n=0; n<20; n++) {
          OCPI::OS::sleep( 5 );
          server->dispatch();
        }
      }
      else {
        loopback_end_point = OCPI_RCC_LBCONT_COMMS_EP;
        eh = new TransportCEventHandler();
        client = new OCPI::DataTransport::Client( loopback_end_point, 1024, eh );
        circuit = loopback_circuit = client->createCircuit( server_end_point );           
        printf("***** Established a new connection  !! \n");
      }
#endif


      
      OCPI::API::Container *c;
      OCPI::API::ContainerApplication *a;

      try { 
	c =  OCPI::API::ContainerManager::find("rcc", NULL, cprops);
	if ( ! c )
	  throw OCPI::Util::EmbeddedException("No Containers found\n");
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
      if ( loopback ) {

        setupForLoopbackMode();

#if 0
        WORKER_LOOPBACK_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
        WORKER_LOOPBACK_ID->control( WCI_CONTROL_START, WCI_DEFAULT );
#else
	WORKER_LOOPBACK_ID->start();
#endif

      }
      else {

        try {
          setupForPCMode();
        }
        CATCH_ALL_RETHROW("setting mode");

        printf("\n\nWaiting 2 secs to continue ....\n");
        for (int y=0; y<10; y++ ) {
          gpp_container->run();
          OCPI::OS::sleep( 200 );                
        }


        try {

#if 0
          WORKER_PRODUCER_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
          WORKER_PRODUCER_ID->control( WCI_CONTROL_START, WCI_DEFAULT );

          WORKER_CONSUMER_ID->control( WCI_CONTROL_INITIALIZE, WCI_DEFAULT );
          WORKER_CONSUMER_ID->control( WCI_CONTROL_START, WCI_DEFAULT );
#else
	WORKER_PRODUCER_ID->start();
	WORKER_CONSUMER_ID->start();
#endif
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

          // Block here until we get a PPP mailbox notification
          // We will get a notification for the following reasons
          // 1) One of our input buffers has been filled
          // 2) A DMA has completed and we have a output buffer that is now empty
          // 3) A timeout has occured
          if ( event_manager ) {
            do {
              gpp_container->run();
            } while(1);
          }
          else {
            gpp_container->run(  );

	    //	    OCPI::OS::sleep( 100 );
          }

        }
        else {
          loopback_container->run();
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


void ocpi_stop()
{
  OCPI_RUN_TEST = 0;
}

void mh(int sn )
{
  ( void ) sn;
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

