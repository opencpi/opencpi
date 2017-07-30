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
    class DatagramXferFactory;
    class DatagramEndPoint : public EndPoint
    {
      OE::Address m_addr;
      std::string m_ifname;
      ocpi_sockaddr_t m_sockaddr;

      friend class DatagramSocket;
      friend class DatagramXferFactory;
    protected:
      DatagramEndPoint( std::string& endpoint, bool a_local, uint32_t a_size=0)
	: EndPoint(endpoint, a_size, a_local) {
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
	  if (!cp || m_addr.hasError())
	    error = OCPI_ETHER_RDMA ": invalid ethernet address in endpoint string";
	} else
	  error = OCPI_ETHER_RDMA ": invalid protocol name in endpoint string";
	if (error)
	  throw DataTransfer::DataTransferEx(UNSUPPORTED_ENDPOINT, error);
      }
      ~DatagramEndPoint() {
	// FIXME:  this is generic behavior and belongs in a datagram endpoint base class
	if (resources.sMemServices) {
	  DatagramSmemServices &sm = *static_cast<DatagramSmemServices *>(resources.sMemServices);
	  sm.stop();
	  sm.join();
	}
      }
      DatagramSmemServices &
      createSmemServices() {
	DataTransfer::DatagramSmemServices *ss = new DatagramSmemServices(*this);
	// FIXME: this "start" is generic behavior and could be in a base class?
	ss->start();
	return *ss;
      }
      const std::string &ifname() const { return m_ifname; }
      OE::Address &addr() { return m_addr; } // not const
    public:
      bool isCompatibleLocal(const char *remote) const {
	std::string interface;
	const char *sp = strchr(remote, '/');
	if (!sp)
	  return true;
	//	  throw OU::Error("Badly formed ether-rdma endpoint: %s", remote);
	interface.assign(remote, sp - remote);
	return m_ifname == interface;
      }
    };

    class DatagramSocket : public DataTransfer::DatagramSocket {
      friend class DatagramXferFactory;
      OE::Socket *m_socket;
    public:
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
	  if (m_socket->send(frame.iov, frame.iovlen, dep->addr(), 0, NULL, error))
	    return;
	  ocpiDebug("Sending packet error: %s", error.size() ? error.c_str() : "timeout");
	}
	throw OU::Error("Error sending ether packet: %s", error.empty() ? "timeout" : error.c_str());
      }
      size_t
      receive(uint8_t *buffer, size_t &offset) {
	size_t length;
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
      allocateEndpoint(const OCPI::Util::PValue*params, uint16_t mailBox, uint16_t maxMailBoxes,
		       size_t size = 0)
      {
	const char *ifname;
	if (!OU::findString(params, "interface", ifname))
	  ifname = getenv("OCPI_ETHER_INTERFACE");
	std::string error;
	OE::Interface ifc(ifname, error);
	if (error.size())
	  throw OU::Error(OCPI_ETHER_RDMA ": bad ethernet interface: %s", error.c_str());
	std::string ep;
	OCPI::Util::formatString(ep, OCPI_ETHER_RDMA ":%s/%s;%zu.%" PRIu16".%" PRIu16,
				 ifc.name.c_str(), ifc.addr.pretty(),
				 size ? size : parent().getSMBSize(), mailBox, maxMailBoxes);
	return ep;
      }

      std::string
      allocateCompatibleEndpoint(const OCPI::Util::PValue*, const char *remote,
				 uint16_t mailBox, uint16_t maxMailBoxes) {
	std::string interface;
	const char *sp = strchr(remote, '/');
	if (sp)
	  interface.assign(remote, sp - remote);
	else {
	  std::string error;
	  OE::Interface ifc(NULL, error);
	  if (error.size())
	    throw OU::Error(OCPI_ETHER_RDMA ": bad ethernet interface: %s", error.c_str());
	  interface = ifc.name;
	}
	//throw OU::Error("Badly formed ether-rdma endpoint: %s", remote);
	OU::PValue pvs[] = { OU::PVString("interface", interface.c_str()), OU::PVEnd };
	return allocateEndpoint(pvs, mailBox, maxMailBoxes);
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
