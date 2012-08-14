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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <OcpiOsEther.h>
#include <DtDataGramXfer.h>
#include <OcpiUtilException.h>
#include <OcpiUtilMisc.h>

namespace DataTransfer {

  namespace Ether {

    namespace OE = OCPI::OS::Ether;
    namespace OU = OCPI::Util;

#define OCPI_ETHER_RDMA "ocpi-ether-rdma"
#define DATAGRAM_PAYLOAD_SIZE OE::MaxPacketSize;

    class DatagramSocket;
    class DatagramEndPoint : public EndPoint 
    {
      OE::Address m_addr;
      std::string m_ifname;
      ocpi_sockaddr_t m_sockaddr;

      friend class DatagramSocket;
      friend class DatagramXferFactory;
    protected:
      DatagramEndPoint( std::string& endpoint, bool local, uint32_t size=0)
	: EndPoint(endpoint, size, local) { 
	const char *error = NULL;
	size_t len = strlen(OCPI_ETHER_RDMA);
	const char *ep = endpoint.c_str();
	if (!strncmp(ep, OCPI_ETHER_RDMA, len) && ep[len] == ':') {
	  ep += len + 1;
	  const char *cp = strchr(ep, '/');
	  if (cp) {
	    m_ifname.assign(ep, cp - ep);
	    ep = cp + 1;
	  }
	  cp = strchr(ep, ';');
	  m_addr.setString(ep);
	  if (!cp || m_addr.error())
	    error = OCPI_ETHER_RDMA ": invalid ethernet address in endpoint string";
	  else
	    ep = cp + 1;
	} else
	  error = OCPI_ETHER_RDMA ": invalid protocol name in endpoint string";
	if (error)
	  throw DataTransfer::DataTransferEx(UNSUPPORTED_ENDPOINT, error);
      }
      const std::string &ifname() const { return m_ifname; }
      OE::Address &addr() { return m_addr; } // not const
      const char* getAddress() { return m_addr.pretty(); }
#if 0
    public:
      virtual const char* getAddress(){return m_ipAddress.c_str();}
      unsigned & getId() { return m_portNum;}
    private:
      std::string m_ipAddress;
      unsigned  m_portNum;
      struct sockaddr_in m_sockaddr;
#endif
    };

    class DatagramSocket : public DataTransfer::DatagramSocket {
      friend class DatagramXferFactory;
      OE::Socket *m_socket;
    protected:
      DatagramSocket( DatagramSmemServices*  lsmem)
	: DataTransfer::DatagramSocket(lsmem), m_socket(NULL) {
      }
      uint16_t maxPayloadSize() { return DATAGRAM_PAYLOAD_SIZE; }
    public:
      void start() {
	DatagramEndPoint *sep = (DatagramEndPoint*)m_lsmem->endpoint();
	std::string error;
	OE::Interface ifc(sep->ifname().c_str(), error);
	if (error.size())
	  throw OU::Error("Invalid ethernet interface name: %s", sep->ifname().c_str());
	m_socket = new OE::Socket(ifc, ocpi_data, NULL, sep->mailbox, error);
	if (error.size()) {
	  delete m_socket;
	  throw OU::Error("Error opening opencpi ethernet socket: %s", error.c_str());
	}
	OCPI::Util::Thread::start();
      }
      void send(Frame &frame) {
	// FIXME: multithreaded..
	DatagramEndPoint *dep = static_cast<DatagramEndPoint *>(frame.endpoint);
	std::string error;
	for (unsigned n = 0; error.empty() && n < 10; n++) {
	  if (m_socket->send(frame.iov, frame.iovlen, dep->addr(), 0, error))
	    return;
	  ocpiDebug("Sending packet error: %s", error.size() ? error.c_str() : "timeout");
	}
	throw OU::Error("Error sending ether packet: %s", error.empty() ? "timeout" : error.c_str());
      }
      unsigned
      receive(uint8_t *buffer, unsigned &offset) {
	unsigned length;
	OE::Address from;
	std::string error;
	if (m_socket->receive(buffer, offset, length, 500, from, error))
	  return length;
	if (error.empty())
	  return 0;
	throw OU::Error("Ethernet socket read error: %s", error.c_str());
      }
    };

    class DatagramDevice;
    class DatagramXferServices;
    const char *datagram_ether = "datagram_ether";
    class DatagramXferFactory
      : public DriverBase<DatagramXferFactory, DatagramDevice, DatagramXferServices, datagram_ether,
			  DataTransfer::DatagramXferFactory>
    {
    protected:
      ~DatagramXferFactory() throw () {}

    public:
      // Boilerplate methods that could be templatized
      DataTransfer::DatagramSmemServices *createSmemServices(EndPoint *ep);
      DataTransfer::DatagramXferServices *createXferServices(DatagramSmemServices*source,
							     DatagramSmemServices*target);
      DatagramSocket *createSocket(DatagramSmemServices *smem);
      EndPoint* createEndPoint(std::string& endpoint, bool local);
      // End boilerplate methods

      const char* getProtocol() { return OCPI_ETHER_RDMA; }

      std::string 
      allocateEndpoint( const OCPI::Util::PValue*, uint16_t mailBox, uint16_t maxMailBoxes)
      {
	std::string ep;
	const char* env = getenv("OCPI_ETHER_INTERFACE");

	if (!env || (env[0] == 0)) {
	  ocpiDebug("Set ""OCPI_ETHER_INTERFACE"" environment variable to set socket Ethernet address");
	  env = 0;
	}
	std::string error;
	OE::Interface ifc(env, error);
	if (error.size())
	  throw OU::Error(OCPI_ETHER_RDMA ": bad ethernet interface: %s", error.c_str());
	OCPI::Util::formatString(ep, OCPI_ETHER_RDMA ":%s/%s;%u.%" PRIu16".%" PRIu16,
				 ifc.name.c_str(), ifc.addr.pretty(), parent().getSMBSize(),
				 mailBox, maxMailBoxes);
	return ep;
      }
    };

#include "DtDataGramBoilerplate.h"
#define Datagram_Ether_RDMA_SUPPORT
#ifdef  Datagram_Ether_RDMA_SUPPORT
    // Used to register with the data transfer system;
    RegisterTransferDriver<DatagramXferFactory> EtherDatagramDriver;
#endif
  }
}

