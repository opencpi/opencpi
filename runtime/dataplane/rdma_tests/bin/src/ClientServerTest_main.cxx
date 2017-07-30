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

#include <stdio.h>
#include <sstream>
#include <memory>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtIntEventHandler.h>
#include <OcpiTransportServer.h>
#include <OcpiMessageEndpoint.h>
#include <OcpiRDTInterface.h>
#include <OcpiThread.h>
#include <OcpiBuffer.h>


#define CATCH_ALL_RETURN1( msg )						\
  catch ( int& ii ) {							\
    printf("gpp: Caught an int exception while %s = %d\n", msg,ii );	\
    return 1;								\
  }									\
  catch ( OCPI::Util::EmbeddedException& eex ) {			\
    printf(" gpp: Caught an embedded exception while %s:\n", msg);	\
    printf( " error number = %d", eex.m_errorCode );			\
    printf( " aux info = %s\n", eex.m_auxInfo.c_str() );                \
    return 1;								\
  }									\
  catch( std::string& stri ) {						\
    printf("gpp: Caught a string exception while %s = %s\n", msg, stri.c_str() ); \
    return 1;								\
  }									\
  catch( ... ) {                                                        \
    printf("gpp: Caught an unknown exception while %s\n",msg );		\
    return 1;								\
  }

using namespace OCPI::DataTransport;
using namespace DataTransport::Interface;
using namespace OCPI::Container;
using namespace OCPI;

/*
 *  These are the global parameters that are configured in the vxWorks startup script
 */
/*
char* OCPI_RCC_CONT_COMMS_EP    = "ocpi-smb-pio:s:300000.1.20";
char* OCPI_RCC_LBCONT_COMMS_EP  = "ocpi-smb-pio:lb:300000.3.20";
*/

int  OCPI_RCC_DATA_BUFFER_SIZE   = 1024;
int  OCPI_RCC_CONT_NBUFFERS      = 1;


// Program globals
static const char *server_end_point = "ocpi-socket-rdma:localhost;40006:600000.2.8";
static const char *loopback_end_point = "ocpi-socket-rdma:localhost;0:600000.3.8";
static volatile int circuit_count=0;
static MessageCircuit     *gpp_circuits[10];
//static Server *server=NULL;


#define CS_DISPATCH if (server)server->dispatch();if(client)client->dispatch();

class TransportSEventHandler : public ServerEventHandler
{
public:

  void newMessageCircuitAvailable( MessageCircuit* new_circuit )
  {
    printf("TransportEventHandler::newCircuitAvailable new circuit available\n");
    gpp_circuits[circuit_count++] = new_circuit;
  }

  void dataAvailable( MessageCircuit* /* circuit */ ) {

  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  void error( OCPI::Util::EmbeddedException& ex )
  {
    printf("TransportEventHandler: Got an exception, (%d%s)\n", ex.getErrorCode(), ex.getAuxInfo() );
  }

};


#if 0
class TransportCEventHandler : public ClientEventHandler
{
public:

  /**********************************
   *  This method gets called when data is available on a circuit
   **********************************/        
  void dataAvailable( MessageCircuit* /* circuit */ )
  {

  }

  /**********************************
   * This method gets called when an error gets generated
   *********************************/
  virtual void error( OCPI::Util::EmbeddedException& /* ex */ )
  {

  }

};
#endif

bool parseArgs( int argc, char** argv)
{
  bool ret =false;
  for ( int n=0; n<argc; n++ ) {
    if (strcmp(argv[n],"-loopback") == 0 ) {
      if (argv[n+1]) {
	loopback_end_point = argv[n+1];
	n++;
      }
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
    loopback = parseArgs(argc,argv);
    if ( !loopback ) {
      printf("Setting up for server mode using %s\n", server_end_point);
      MessageCircuit *mc;

#if 0
      // Create the server endpoint and its processing thread
      TransportSEventHandler *tcb = new TransportSEventHandler();
      server = new Server( server_end_point, tcb );
      printf("setServerURL \"%s\"\n", server_end_point );

      // We need a client to continue
      while ( circuit_count == 0 ) {
        server->dispatch();
        OCPI::OS::sleep( 500 );
        printf("Waiting for a client to connect\n");
      }
      MessageCircuit *mc = gpp_circuits[0];
#else
      MessageEndpoint &mep = MessageEndpoint::getMessageEndpoint(server_end_point);
      printf("Local server endpoint is: %s\n", mep.endpoint());
      do {
	OS::Timer timer(2, 0);
        printf("Waiting for a client to connect...\n");
	mc = mep.accept(&timer);
      } while (!mc);
#endif
      char *protocol = mc->getProtocol();
      printf("Server side:\n  local:  %s\n  remote: %s, protocol: %s\n ",
	     mc->localEndpoint(), mc->remoteEndpoint(), protocol);
      delete [] protocol;
      for (unsigned msg_count = 0; msg_count <= 101; msg_count++) {
        OCPI::DataTransport::BufferUserFacet* buffer;        
	void *data;
	size_t bufferLength;
	while (!(buffer = mc->getNextEmptyOutputBuffer(data, bufferLength)))
	  mc->dispatch();
	size_t length = 0;
	if (msg_count != 100) {
	  sprintf((char*)data, "message %d\n", msg_count);
	  length = strlen((char*)data) + 1 ;
	}
	printf("Sending buffer: %s, length %zu\n", (char*)data, length);
	mc->sendOutputBuffer( buffer, length, 0xe7);
      }
      printf("Server done, waiting 5 seconds\n");
      OCPI::OS::sleep(5000);
      delete mc;
    }
    else {
      printf("Setting up for loopback mode using %s\n", loopback_end_point);
      OS::Timer timer(10, 0);
      std::auto_ptr<MessageCircuit> c(&MessageEndpoint::connect(server_end_point, 4096,
								"Hello, World", &timer));
      //      MessageCircuit c(loopback_end_point, 1024);
      //      printf("***** Client connecting to: %s\n", server_end_point);
      //      c.connect( server_end_point );   
      printf("Client side:\n  local:  %s\n  remote: %s\n ",
	     c->localEndpoint(), c->remoteEndpoint());

      for (unsigned n = 0; n < 1000; n++) {
        OCPI::DataTransport::BufferUserFacet* buffer;        
	void *data = 0;// for debug
	size_t length;
	uint8_t opcode;
	while (!(buffer = c->getNextFullInputBuffer(data, length, opcode)))
	  OCPI::OS::sleep(1);
	if (length != 0) {
	  ocpiAssert(length == strlen((char *)data) + 1);
	  printf("Message %d from server = %s, op %xx\n", n, (char*)data, opcode);
	}
	c->releaseInputBuffer( buffer );
	if (length == 0) {
	  printf("Message %d empty\n", n);
	  break;
	}
	OCPI::OS::sleep(1);
      }
      printf("****Client done\n");
    }
  }
  CATCH_ALL_RETURN1("running client/server test")
    OCPI::DataTransport::MessageEndpoint::destroyMessageEndpoints();
  return 0;
}

int main( int argc, char** argv)
{
  setlinebuf(stdout);
  // Start the container in a thead
  return gpp_cont(argc,argv);
}

