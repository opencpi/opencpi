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
#include "OcpiOsEther.h"
#include "OcpiUtilException.h"
#include "OcpiUtilMisc.h"
#include "XferException.h"
#include "DtDataGramXfer.h"

namespace DataTransfer {

  namespace XF = DataTransfer;
  namespace DG = DataTransfer::Datagram;
  namespace Ether {

    namespace OE = OCPI::OS::Ether;
    namespace OU = OCPI::Util;

#define OCPI_ETHER_RDMA "ocpi-ether-rdma"
#define DATAGRAM_PAYLOAD_SIZE OE::MaxPacketSize;

    class Socket;
    class XferFactory;
    class EndPoint : public DG::DGEndPoint {
      OE::Address m_addr;
      std::string m_ifname;
      ocpi_sockaddr_t m_sockaddr;

      friend class Socket;
      friend class XferFactory;
    protected:
      EndPoint(XF::XferFactory &a_factory, const char *protoInfo, const char *eps,
	       const char *other, bool a_local, size_t a_size, const OU::PValue *params)
	: DG::DGEndPoint(a_factory, eps, other, a_local, a_size, params) { 
	if (protoInfo) {
	  m_protoInfo = protoInfo;
	  const char *cp = strchr(protoInfo, '/');
	  if (cp) {
	    m_ifname.assign(protoInfo, OCPI_SIZE_T_DIFF(cp, protoInfo));
	    protoInfo = cp + 1;
	  }
	  m_addr.setString(protoInfo);
	  if (!cp || m_addr.hasError())
	    throw DataTransfer::DataTransferEx(UNSUPPORTED_ENDPOINT,
					       OCPI_ETHER_RDMA
					       ": invalid ethernet address in endpoint string");
	} else {
	  if (0) { // if (other) {
	    const char *sp = strchr(other, '/');
	    m_ifname.assign(other, OCPI_SIZE_T_DIFF(sp, other));
	  } else {
	    const char *l_name;
	    if (!OU::findString(params, "interface", l_name))
	      l_name = getenv("OCPI_ETHER_INTERFACE");
	    if (!l_name)
	      throw OU::Error(OCPI_ETHER_RDMA ": no interface specified");
	    m_ifname = l_name;
	  }
	  // FIXME: use first/only interface if there is one?
	  std::string error;
	  OE::Interface ifc(m_ifname.c_str(), error);
	  if (error.size())
	    throw OU::Error(OCPI_ETHER_RDMA ": bad ethernet interface: %s", error.c_str());
	  OU::format(m_protoInfo, "%s/%s", ifc.name.c_str(), ifc.addr.pretty());
	}
      }
      // boilerplate
      XF::SmemServices &createSmemServices();
      const std::string &ifname() const { return m_ifname; }
      OE::Address &addr() { return m_addr; } // not const
    public:
      bool isCompatibleLocal(const char *remote) const {
	std::string interface;
	const char *sp;
	if (!remote || !(sp = strchr(remote, '/')))
	  return true;
	interface.assign(remote, OCPI_SIZE_T_DIFF(sp, remote));
	return m_ifname == interface;
      }
    };

    class Socket : public DG::Socket {
      friend class XferFactory;
      EndPoint   &m_lep;
      OE::Socket *m_socket;
    public:
      Socket(EndPoint &lep) : DG::Socket(lep), m_lep(lep), m_socket(NULL) {
      }
      uint16_t maxPayloadSize() { return DATAGRAM_PAYLOAD_SIZE; }
    public:
      void start() {
	std::string error;
	OE::Interface ifc(m_lep.ifname().c_str(), error);
	if (error.size())
	  throw OU::Error("Invalid ethernet interface name: %s", m_lep.ifname().c_str());
	m_socket = new OE::Socket(ifc, ocpi_data, NULL, m_lep.mailBox(), error);
	if (error.size()) {
	  delete m_socket;
	  throw OU::Error("Error opening opencpi ethernet socket: %s", error.c_str());
	}
	OCPI::Util::Thread::start();
      }
      void send(DG::Frame &frame, DG::DGEndPoint &destEp) {
	// FIXME: multithreaded..
	std::string error;
	for (unsigned n = 0; error.empty() && n < 10; n++) {
	  if (m_socket->send(frame.iov, frame.iovlen, static_cast<EndPoint *>(&destEp)->addr(), 0,
			     NULL, error))
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

    class Device;
    class XferServices;
    const char *datagram_ether = "datagram_ether";
    class XferFactory
      : public DriverBase<XferFactory, Device, XferServices, datagram_ether, DG::XferFactory>
    {
    protected:
      ~XferFactory() throw () {}

    public:
      // Boilerplate methods that could be templatized
      DG::SmemServices *
      createSmemServices(XF::EndPoint *ep);
      XF::XferServices &createXferServices(XF::EndPoint &source, XF::EndPoint &target);
      DG::Socket &createSocket(XF::EndPoint&);
      XF::EndPoint &createEndPoint(const char *protoInfo, const char *eps, const char *other,
				   bool local, size_t size, const OCPI::Util::PValue *params);
      // End boilerplate methods

      const char* getProtocol() { return OCPI_ETHER_RDMA; }
    };

#include "DtDataGramBoilerplate.h"
    // Used to register with the data transfer system;
    RegisterTransferDriver<XferFactory> etherDatagramDriver;
  }
}

