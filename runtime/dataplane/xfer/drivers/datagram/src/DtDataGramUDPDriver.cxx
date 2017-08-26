//#define DEBUG_TxRx_Datagram 1
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
 * Abstract:
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
#include "OcpiOsSocket.h"
#include "OcpiOsServerSocket.h"
#include "OcpiOsAssert.h"
#include "OcpiUtilMisc.h"
#include "XferException.h"
#include "DtDataGramXfer.h"

#define DATAGRAM_PAYLOAD_SIZE 512

namespace XF = DataTransfer;
namespace OU = OCPI::Util;
namespace DG = DataTransfer::Datagram;
namespace DataTransfer {

  namespace UDP {

    class Socket;
    class EndPoint : public DG::DGEndPoint, public XF::EndPoint 
    {
      friend class Socket;
      friend class XferFactory;
      std::string m_ipAddress;
      uint16_t  m_portNum;
      struct sockaddr_in m_sockaddr;
    public:
      EndPoint(XF::XferFactory &factory, const char *protoInfo, const char *eps,
	       const char *other, bool local, size_t size, const OU::PValue *params)
	: XF::EndPoint(factory, eps, other, local, size, params) { 
	if (protoInfo) {
	  m_protoInfo = protoInfo;
	  char ipaddr[80];
	  if (sscanf(protoInfo, "%[^;];%" SCNu16 ";", ipaddr, &m_portNum) != 2)
	    throw DataTransfer::DataTransferEx( UNSUPPORTED_ENDPOINT, protoInfo);	  
	  m_ipAddress = ipaddr;  
	} else {
	  std::string ep;
	  char ip_addr[128];
	  const char* env = getenv("OCPI_UDP_TRANSFER_IP_ADDR");
	  if (!env || (env[0] == 0)) {
	    ocpiDebug("Set OCPI_TRANSFER_IP_ADDR environment variable to set socket IP address");
	    gethostname(ip_addr, 128); // FIXME: get a numeric address to avoid DNS problems
	  } else
	    strcpy(ip_addr, env);
	  m_ipAddress = ip_addr;
	  int port;
	  const char* penv = getenv("OCPI_UDP_TRANSFER_PORT");
	  if( !penv || (penv[0] == 0)) {
	    ocpiDebug("Set ""OCPI_TRANSFER_PORT"" environment variable to set socket IP address");
	    port = 0;
	  } else {
	    static uint16_t s_port = 0;
	    if ( s_port == 0 )
	      s_port = (uint16_t)atoi(penv);
	    m_portNum = s_port++;
	  }
	  OU::format(m_protoInfo, "%s;%" PRIu16, m_ipAddress.c_str(), m_portNum);
	}
	memset(&m_sockaddr, 0, sizeof(m_sockaddr));
	m_sockaddr.sin_family = AF_INET;
	m_sockaddr.sin_port = htons(m_portNum);
	inet_aton(m_ipAddress.c_str(), &m_sockaddr.sin_addr);
      }
    protected:
      ~EndPoint() {
	// FIXME:  this is generic behavior and belongs in a datagram endpoint base class
	DG::SmemServices &sm = *static_cast<DG::SmemServices *>(&sMemServices());
	sm.stop();
	stop();
      }
      inline struct sockaddr_in &sockaddr() { return m_sockaddr; }
      void updatePortNum(uint16_t portNum) {
	if (portNum != m_portNum) {
	  m_portNum = portNum;
	  m_sockaddr.sin_port = htons(m_portNum);
	  m_protoInfo = m_ipAddress.c_str();
	  setName();
	}
      }
      // boilerplate
      XF::SmemServices &createSmemServices();
    public:
      uint16_t & getId() { return m_portNum;}
    };

    class Socket : public DG::Socket {
      friend class XferFactory;
      EndPoint   &m_lep;
      bool        m_error;
    public:
      Socket(EndPoint &lep) : DG::Socket(lep), m_lep(lep), m_error(false) {
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
	try {
	  m_server.bind(m_lep.getId(), false, true);
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
	m_lep.updatePortNum(m_server.getPortNo());
	OCPI::Util::Thread::start();
      }

      void send(DG::Frame &frame) {
	// FIXME: multithreaded..
	EndPoint *dep = static_cast<EndPoint *>(frame.endpoint);
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

    class Device;
    class XferServices;
    const char *datagram_udp = "datagram_udp";
    class XferFactory
      : public DriverBase<XferFactory, Device, XferServices, datagram_udp,
			  DG::XferFactory>
    {
    protected:
      ~XferFactory() throw () {}

    public:
      XF::XferServices &createXferServices(XF::EndPoint &source, XF::EndPoint &target);
      DG::Socket &createSocket(XF::EndPoint &sep);
      XF::EndPoint &createEndPoint(const char *protoInfo, const char *eps, const char *other,
				   bool local, size_t size, const OCPI::Util::PValue *params);
      // End boilerplate methods
      const char* getProtocol() { return "ocpi-udp-rdma";}
    };

#include "DtDataGramBoilerplate.h"
    // Used to register with the data transfer system;
    RegisterTransferDriver<XferFactory> udpDatagramDriver;
  }
}

