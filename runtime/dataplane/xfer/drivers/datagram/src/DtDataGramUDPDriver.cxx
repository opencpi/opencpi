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
#include "OcpiOsEther.h"
#include "OcpiUtilMisc.h"
#include "XferException.h"
#include "DtDataGramXfer.h"

#define DATAGRAM_PAYLOAD_SIZE 512

namespace XF = DataTransfer;
namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace DG = DataTransfer::Datagram;
namespace OE = OCPI::OS::Ether;
namespace DataTransfer {

  namespace UDP {

    class Socket;
    class EndPoint : public DG::DGEndPoint {
      friend class Socket;
      friend class XferFactory;
      std::string m_ipAddress;
      uint16_t  m_portNum;
      struct sockaddr_in m_sockaddr;
    public:
      EndPoint(XF::XferFactory &a_factory, const char *protoInfo, const char *eps,
	       const char *other, bool a_local, size_t a_size, const OU::PValue *params)
	: DG::DGEndPoint(a_factory, eps, other, a_local, a_size, params) { 
	if (protoInfo) {
	  m_protoInfo = protoInfo;
	  // FIXME: this is redundant to what is in the socket driver.
	  // Note that IPv6 addresses may have colons, even though colons are commonly used to
	  // separate addresses from ports.  Since there must be a port, it will be after the last
	  // colon.  There is also a convention that IPV6 addresses embedded in URLs are in fact
	  // enclosed in square brackets, like [ipv6-addr-with-colons]:port
	  // So this scheme will work whether the square bracket convention is used or not
	  const char *colon = strrchr(protoInfo, ':');  // before the port
	  if (!colon || sscanf(colon+1, "%hu;", &m_portNum) != 1)
	    throw OU::Error("Invalid UDP datagram endpoint format in \"%s\"", protoInfo);
	  // FIXME: we could do more parsing/checking on the ipaddress
	  m_ipAddress.assign(protoInfo, OCPI_SIZE_T_DIFF(colon, protoInfo));
	} else {
	  const char *env = getenv("OCPI_TRANSFER_IP_ADDRESS");
	  if (env && env[0])
	    m_ipAddress = env;
	  else {
	    ocpiDebug("Set OCPI_TRANSFER_IP_ADDRESS environment variable to set socket IP address");
	    static std::string myAddr;
	    if (myAddr.empty()) {
	      std::string error;
	      if (OE::IfScanner::findIpAddr(getenv("OCPI_SOCKET_INTERFACE"), myAddr, error))
		throw OU::Error("Cannot obtain a local IP address:  %s", error.c_str());
	    }
	    m_ipAddress = myAddr;
	  }
	  const char* penv = getenv("OCPI_UDP_TRANSFER_PORT");
	  if( !penv || (penv[0] == 0)) {
	    ocpiDebug("Set the OCPI_TRANSFER_PORT environment variable to set socket IP port");
	    m_portNum = 0;
	  } else {
	    static uint16_t s_port = 0;
	    if ( s_port == 0 )
	      s_port = (uint16_t)atoi(penv);
	    m_portNum = s_port++;
	  }
	  OU::format(m_protoInfo, "%s:%" PRIu16, m_ipAddress.c_str(), m_portNum);
	}
	memset(&m_sockaddr, 0, sizeof(m_sockaddr));
	m_sockaddr.sin_family = AF_INET;
	m_sockaddr.sin_port = htons(m_portNum);
	if (!inet_aton(m_ipAddress.c_str(), &m_sockaddr.sin_addr))
	  throw OU::Error("Unable to parse/resolve internet address for UDP: %s", m_ipAddress.c_str());
      }
    private:
      void
      setProtoInfo() {
	OU::format(m_protoInfo, "%s:%u", m_ipAddress.c_str(), m_portNum);
      }
      inline struct sockaddr_in &sockaddr() { return m_sockaddr; }
      void updatePortNum(uint16_t portNum) {
	if (portNum != m_portNum) {
	  m_portNum = portNum;
	  m_sockaddr.sin_port = htons(m_portNum);
	  setProtoInfo();
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

      void send(DG::Frame &frame, DG::DGEndPoint &destEp) {
	// FIXME: multithreaded..
	m_msghdr.msg_name = &static_cast<EndPoint *>(&destEp)->sockaddr();
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
	// All DEBUG
	if (OS::logWillLog(10) && n != 0) {
	  int port = ntohs ( ((struct sockaddr_in *)&sad)->sin_port );
	  char * a  = inet_ntoa ( ((struct sockaddr_in *)&sad)->sin_addr );
	  ocpiDebug(" Recved %lld bytes of data on port %lld from addr %s port %d\n",
		    (long long)n , (long long)m_server.getPortNo(), a, port);
	}
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
