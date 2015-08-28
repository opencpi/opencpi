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
#include <inttypes.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <OcpiOsSocket.h>
#include <OcpiOsServerSocket.h>
#include <OcpiOsAssert.h>
#include <OcpiUtilMisc.h>
#include <DtDataGramXfer.h>

#define DATAGRAM_PAYLOAD_SIZE 512

namespace DataTransfer {

  namespace UDP {

    static void 
    setEndpointString(std::string &ep, const char *ipAddr, unsigned port,
		      size_t size, uint16_t mbox, uint16_t maxCount)
    {
      OCPI::Util::formatString(ep, "ocpi-udp-rdma:%s;%u;%zu.%" PRIu16 ".%" PRIu16,
			       ipAddr, port, size, mbox, maxCount);
    }

    class DatagramSocket;
    class DatagramEndPoint : public EndPoint 
    {
      friend class DatagramSocket;
      friend class DatagramXferFactory;
    public:
      DatagramEndPoint( std::string& ep, bool local, uint32_t size=0)
	: EndPoint(ep, size, local) { 
	char ipaddr[80];
	int rv = sscanf(ep.c_str(), "ocpi-udp-rdma:%[^;];%" SCNu16 ";", ipaddr, &m_portNum);
	if (rv != 2) {
	  fprintf( stderr, "DatagramEndPoint  ERROR: Bad socket endpoint format (%s)\n", ep.c_str() );
	  throw DataTransfer::DataTransferEx( UNSUPPORTED_ENDPOINT, ep.c_str() );	  
	}
	m_ipAddress = ipaddr;  
	memset(&m_sockaddr, 0, sizeof(m_sockaddr));
	m_sockaddr.sin_family = AF_INET;
	m_sockaddr.sin_port = htons(m_portNum);
	inet_aton(m_ipAddress.c_str(), &m_sockaddr.sin_addr);
      }
    protected:
      ~DatagramEndPoint() {
	// FIXME:  this is generic behavior and belongs in a datagram endpoint base class
	if (resources.sMemServices) {
	  DatagramSmemServices &sm = *static_cast<DatagramSmemServices *>(resources.sMemServices);
	  sm.stop();
	  sm.join();
	}
      }
      inline struct sockaddr_in &sockaddr() { return m_sockaddr; }
      void updatePortNum(uint16_t portNum) {
	if (portNum != m_portNum) {
	  m_portNum = portNum;
	  m_sockaddr.sin_port = htons(m_portNum);
	  setEndpointString(end_point, m_ipAddress.c_str(), m_portNum, size, mailbox, maxCount);
	}
      }
      DataTransfer::SmemServices &createSmemServices() {
	DataTransfer::DatagramSmemServices *ss = new DatagramSmemServices(*this);
	// FIXME: this "start" is generic behavior and could be in a base class?
	ss->start();
	return *ss;
      }
    public:
      virtual const char* getAddress(){return m_ipAddress.c_str();}
      uint16_t & getId() { return m_portNum;}
    private:
      std::string m_ipAddress;
      uint16_t  m_portNum;
      struct sockaddr_in m_sockaddr;
    };

    class DatagramSocket : public DataTransfer::DatagramSocket {
      friend class DatagramXferFactory;
    public:
      DatagramSocket( DatagramSmemServices*  lsmem) //, DatagramTransmissionLayerDriver * driver )
	: DataTransfer::DatagramSocket(lsmem) {//,driver) {
	m_msghdr.msg_namelen = sizeof(struct sockaddr_in);
	m_msghdr.msg_iov = 0;
	m_msghdr.msg_iovlen = 0;
	m_msghdr.msg_control = 0;
	m_msghdr.msg_controllen = 0;
	m_msghdr.msg_flags = 0;    
      }
      uint16_t maxPayloadSize() { return DATAGRAM_PAYLOAD_SIZE; }
    public:
      void start() {
	DatagramEndPoint *sep = (DatagramEndPoint*)m_lsmem->endpoint();
	try {
	  m_server.bind(sep->getId(), false, true);
	}
	catch( std::string & err ) {
	  m_error=true;
	  ocpiBad("DatagramSocket bind error. %s", err.c_str() );
	  ocpiAssert("Unable to bind to socket"==0);
	  return;
	}
	catch( ... ) {
	  m_error=true;
	  ocpiAssert("Unable to bind to socket"==0);
	  return;
	}
	sep->updatePortNum(m_server.getPortNo());
	OCPI::Util::Thread::start();
      }

      void send(Frame &frame) {
	// FIXME: multithreaded..
	DatagramEndPoint *dep = static_cast<DatagramEndPoint *>(frame.endpoint);
	m_msghdr.msg_name = &dep->sockaddr();
	// We are depending on structure compatibility
	m_msghdr.msg_iov = (struct iovec *)frame.iov;
	m_msghdr.msg_iovlen = frame.iovlen;
	m_server.sendmsg(&m_msghdr, 0);
      }
      size_t
      receive(uint8_t *buffer, size_t &offset) {
	struct sockaddr sad;
	size_t size = sizeof(struct sockaddr);
	size_t n = m_server.recvfrom((char*)buffer, DATAGRAM_PAYLOAD_SIZE, 0, (char*)&sad, &size, 200);
	offset = 0;
#ifdef DEBUG_TxRx_Datagram
	// All DEBUG
	if (n != 0) {
	  int port = ntohs ( ((struct sockaddr_in *)&sad)->sin_port );
	  char * a  = inet_ntoa ( ((struct sockaddr_in *)&sad)->sin_addr );
	  ocpiDebug(" Recved %lld bytes of data on port %lld from addr %s port %d\n",
		    (long long)n , (long long)m_server.getPortNo(), a, port);
	}
#endif
	return n;
      }
    private:
      struct msghdr                 m_msghdr;
      OCPI::OS::ServerSocket        m_server;
    };

    class DatagramDevice;
    class DatagramXferServices;
    const char *datagram_udp = "datagram_udp";
    class DatagramXferFactory
      : public DriverBase<DatagramXferFactory, DatagramDevice, DatagramXferServices, datagram_udp,
			  DataTransfer::DatagramXferFactory>
    {
    protected:
      ~DatagramXferFactory() throw () {}

    public:
      DataTransfer::DatagramXferServices *createXferServices(DatagramSmemServices*source,
							     DatagramSmemServices*target);
      DatagramSocket *createSocket(DatagramSmemServices *smem);
      EndPoint* createEndPoint(std::string& endpoint, bool local);
      // End boilerplate methods

      const char* getProtocol(){return "ocpi-udp-rdma";}

      std::string 
      allocateEndpoint( const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes)
      {
	std::string ep;
	char ip_addr[128];
	const char* env = getenv("OCPI_UDP_TRANSFER_IP_ADDR");
	if( !env || (env[0] == 0)) {
	  ocpiDebug("Set ""OCPI_TRANSFER_IP_ADDR"" environment variable to set socket IP address");
	  gethostname(ip_addr,128); // FIXME: get a numeric address to avoid DNS problems
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
	  mailBox = (uint16_t)atoi(mb);
	}
	setEndpointString(ep, ip_addr, port, parent().getSMBSize(), mailBox, maxMailBoxes);
	return ep;
      }
      
    };

#include "DtDataGramBoilerplate.h"
#define Datagram_UDP_RDMA_SUPPORT
#ifdef  Datagram_UDP_RDMA_SUPPORT
    // Used to register with the data transfer system;
    RegisterTransferDriver<DatagramXferFactory> UDPDatagramDriver;
#endif
  }
}

