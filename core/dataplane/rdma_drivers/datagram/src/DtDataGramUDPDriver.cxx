//#define DEBUG_TxRx_Datagram 1

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


/*
 * Abstact:
 *   This file contains the interface for the Ocpi Datagram transfer driver.
 *
 *  John Miller -  5-24-12
 *  Initial version
 *
 */

#include <DtSharedMemoryInternal.h>
#include <OcpiOsMisc.h>
#include <OcpiOsAssert.h>
#include <DtExceptions.h>
#include <OcpiThread.h>
#include <DtDataGramXfer.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fasttime.h>
#include <OcpiUtilAutoMutex.h>
#include <deque>

namespace DataTransfer {

  namespace UDP {

    class ServerDatagramSocketHandler;

    class DatagramSocket : public DataTransfer::DatagramSocket {

    public :
      DatagramSocket( DatagramSmemServices*  lsmem, DatagramTransmisionLayerDriver * driver )
	: DataTransfer::DatagramSocket(lsmem,driver){}
      virtual ~DatagramSocket();
      void start();

    private:
      ServerDatagramSocketHandler*   m_socketHandler;
    };



    class  DatagramEndPoint : public EndPoint 
    {
    public:
      virtual ~DatagramEndPoint(){};
      DatagramEndPoint( std::string& ep, bool local, uint32_t size=0)
	: EndPoint(ep, size, local) { parse(ep);}
      virtual const char* getAddress(){return ipAddress.c_str();}
      int32_t parse( std::string& ep )
      {
	char ipaddr[80];
	int rv = sscanf(ep.c_str(), "ocpi-udp-rdma:%[^;];%u:", ipaddr, &portNum);
	if (rv != 2) {
	  fprintf( stderr, "DatagramEndPoint  ERROR: Bad socket endpoint format (%s)\n", ep.c_str() );
	  throw DataTransfer::DataTransferEx( UNSUPPORTED_ENDPOINT, ep.c_str() );	  
	}
	ipAddress = ipaddr;  
	return 0;
      }
      unsigned & getId(){return portNum;}
      std::string ipAddress;
      unsigned  portNum;
    };



    class DatagramDriver;
    class ServerDatagramSocketHandler : public OCPI::Util::Thread
    {
    public:
      ServerDatagramSocketHandler( OCPI::OS::Socket & socket, DatagramSmemServices*  lsmem,  DatagramDriver * driver )
	: m_run(true), m_lsmem(lsmem), m_socket(socket),m_driver(driver) {}

      virtual ~ServerDatagramSocketHandler()
      {
	try {
	  stop();
	  m_socket.close();
	  join();
	}
	catch( ... ) {

	}
      }

#define RX_BUFFER_SIZE (1024*10)


      void stop(){m_run=false;}
      void run();

    private:
      bool   m_run;
      DatagramSmemServices*  m_lsmem;
      OCPI::OS::Socket & m_socket;
      DatagramDriver *               m_driver;
    };


    class DatagramDriver : public DatagramTransmisionLayerDriver  {
      friend class FrameMonitor;

    private:
      OCPI::OS::Socket * m_socket;
      std::vector<DatagramSmemServices*> m_smems;

    public:
      DatagramDriver()
      {

      }

      virtual ~DatagramDriver()
      {
	std::vector<DatagramSmemServices*>::iterator it;
	for ( it=m_smems.begin(); it!=m_smems.end(); it++ ) {
	  (*it)->stop();
	  (*it)->join();
	}
	m_smems.clear();
      }


      uint16_t maxPayloadSize()
      {
	return DATAGRAM_PAYLOAD_SIZE;
      }

      SmemServices * getSmemServices( XferFactory *f, EndPoint * ep )
      {
	ep->smem = new DatagramSmemServices(f, ep);
	DatagramSmemServices * smem = static_cast<DatagramSmemServices*>(ep->smem);
	m_smems.push_back( smem );
	if ( ep->local ) {
	  // Create our listener socket thread so that we can respond to incoming
	  // requests  
	  smem->socketServer() = new DatagramSocket( smem, this );
	  smem->socketServer()->start();
	  m_socket = & ((DatagramSmemServices*)ep->smem)->socketServer()->socket();
	}
	smem->start();
	return ep->smem;
      }
      EndPoint * getEndPoint(){return NULL;}
      EndPoint* createEndPoint(std::string& endpoint, bool local) {
	return new DatagramEndPoint(endpoint, local);
      }
      static void 
      setEndpointString(std::string &ep, const char *ipAddr, unsigned port,
			unsigned size, unsigned mbox, unsigned maxCount)
      {
	char tep[128];
	snprintf(tep, 128, "ocpi-udp-rdma:%s;%u:%u.%u.%u", ipAddr, port, size, mbox,
		 maxCount);
	ep = tep;
      }

      const char* getProtocol(){return "ocpi-udp-rdma";}

      std::string 
      allocateEndpoint( const OCPI::Util::PValue*, unsigned SMBsize, unsigned mailBox, unsigned maxMailBoxes)
      {
	std::string ep;
	char ip_addr[128];
	const char* env = getenv("OCPI_UDP_TRANSFER_IP_ADDR");
	if( !env || (env[0] == 0)) {
	  ocpiDebug("Set ""OCPI_TRANSFER_IP_ADDR"" environment variable to set socket IP address");
	  gethostname(ip_addr,128);
	}
	else {
	  strcpy(ip_addr,env);
	}
	int port;
	const char* penv = getenv("OCPI_UDP_TRANSFER_PORT");
	if( !penv || (penv[0] == 0)) {
	  ocpiDebug("Set ""OCPI_TRANSFER_PORT"" environment variable to set socket IP address");
	  port = 0;
	}
	else {
	  static int m_port = 0;
	  if ( m_port == 0 ) {
	    m_port = atoi(penv);
	  }
	  port = m_port++;
	}
	const char* mb = getenv("OCPI_MAILBOX");    
	if ( mb ) {
	  mailBox = atoi(mb);
	}
	setEndpointString(ep, ip_addr, port, SMBsize, mailBox, maxMailBoxes);
	return ep;
      }
      
    };


    class DatagramXferFactory : public DataTransfer::DatagramXferFactory {
    public:
      DatagramXferFactory(){
	printf(" ***** In UDP::DatagramXferFactory\n");
	setDriver(new DatagramDriver());
      }
      virtual ~DatagramXferFactory()
	throw () {};
    };




    // This handler services 
    void
    ServerDatagramSocketHandler::
    run() {

      uint64_t frames_processed = 0;
      DatagramFrameHeader * header;

	
      try {
	while ( m_run ) {
	  uint8_t   buf[RX_BUFFER_SIZE];
	  struct sockaddr sad;
	  unsigned long size = sizeof( struct sockaddr);
	  unsigned long long n = m_socket.recvfrom( (char*)buf,RX_BUFFER_SIZE, 0, (char*)&sad, &size, 200);
	  if ( n == 0 ) {
	    // timeout
	    continue;
	  }

#ifdef DEBUG_TxRx_Datagram
	  // All DEBUG
	  int port = ntohs ( ((struct sockaddr_in *)&sad)->sin_port );
	  char * a  = inet_ntoa ( ((struct sockaddr_in *)&sad)->sin_addr );
	  printf(" Recved %lld bytes of data on port %lld from addr %s port %d\n", (long long)n , (long long)m_socket.getPortNo(), 
		 a, port );
#endif

	  // This causes a frame drop for testing
	  //#define DROP_FRAME
#ifdef DROP_FRAME
	  const char* env = getenv("OCPI_Datagram_DROP_FRAMES");
	  if ( env != NULL ) 
	  {
	    static int dropit=1;
	    static int dt = 300;
	    static int m = 678900;
	    if ( dt && (((++dropit)%m)==0) ) {
	      printf("\n\n\n DROP A PACKET FOR TESTING \n\n\n");
	      dt--;
	      m = 500000 + rand()%10000;
	      continue;
	    }
	  }
#endif


	  header = reinterpret_cast<DatagramFrameHeader*>(&buf[2]);

	  // Get the xfer service that handles this conversation
	  DatagramXferServices * xferS = 
	    m_lsmem->xferServices(header->srcId);
	  xferS->processFrame( header );

	  frames_processed++;

	}
      }
      catch (std::string &s) {
	ocpiBad("Exception in socket background thread: %s", s.c_str());
      } catch (...) {
	ocpiBad("Unknown exception in socket background thread");
      }

    }

      void 
      DatagramSocket::
      start() {
	DatagramEndPoint *sep = (DatagramEndPoint*)m_lsmem->endpoint();
	try {
	  m_socket = m_server.bind(sep->getId(),false,true);
	}
	catch( std::string & err ) {
	  m_error=true;
	  ocpiBad("DatagramSocket bind error. %s", err.c_str() );
	  ocpiAssert(!"Unable to bind to socket");
	  return;
	}
	catch( ... ) {
	  m_error=true;
	  ocpiAssert(!"Unable to bind to socket");
	  return;
	}
	if (sep->getId() == 0) {
	  // We now know the real port, so we need to change the endpoint string.
	  sep->getId() = m_server.getPortNo();
	  DatagramDriver::setEndpointString(sep->end_point, sep->ipAddress.c_str(),
				      sep->portNum, sep->size, sep->mailbox,
				      sep->maxCount);
	}
	m_socket.linger(false); // we want to give some time for data to the client FIXME timeout param?
	m_socketHandler  = new ServerDatagramSocketHandler(m_socket ,m_lsmem,
							   static_cast<DatagramDriver*>(m_driver));
	m_socketHandler->start();
      }

    DatagramSocket::
    ~DatagramSocket() {
      delete m_socketHandler;	
    }

  }

  DatagramTransmisionLayerDriver * getUDPDriver(){return new UDP::DatagramDriver();}

#define Datagram_UDP_RDMA_SUPPORT
#ifdef  Datagram_UDP_RDMA_SUPPORT
  // Used to register with the data transfer system;
  RegisterTransferDriver<DatagramXferFactory> UDPDatagramDriver;
#endif


}

