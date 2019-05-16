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

#include <inttypes.h>
#include <unistd.h>  // FIXME for gethostname - use OS::
#include <deque>
#include "OcpiOsSocket.h"
#include "OcpiOsMisc.h"
#include "OcpiOsAssert.h"
#include "OcpiOsServerSocket.h"
#include "OcpiOsEther.h"
#include "OcpiUtilMisc.h"
#include "OcpiThread.h"
#include "XferDriver.h"
#include "XferEndPoint.h"
#include "XferPio.h"

namespace DataTransfer {
  namespace Socket {
    namespace OU = OCPI::Util;
    namespace OS = OCPI::OS;
    namespace OE = OCPI::OS::Ether;
    namespace XF = DataTransfer;

struct DataHeader {
  DtOsDataTypes::Offset offset;
  uint32_t   length;
  uint32_t   count;
};
const size_t TCP_BUFSIZE = 4096;

class XferFactory;
class EndPoint: public XF::EndPoint {
  friend class ServerT;
  friend class XferServices;
  friend class SmemServices;
  friend class ServerSocketHandler;
protected:
  std::string m_ipAddress;
  uint16_t    m_portNum;
public:
  EndPoint(XF::XferFactory &a_factory, const char *protoInfo, const char *eps, const char *other,
	   bool a_local, size_t a_size, const OU::PValue *params)
    : XF::EndPoint(a_factory, eps, other, a_local, a_size, params),
      m_portNum(0) {
    if (protoInfo) {
      m_protoInfo = protoInfo;
      // Note that IPv6 addresses may have colons, even though colons are commonly used to
      // separate addresses from ports.  Since there must be a port, it will be after the last
      // colon.  There is also a convention that IPV6 addresses embedded in URLs are in fact
      // enclosed in square brackets, like [ipv6-addr-with-colons]:port
      // So this scheme will work whether the square bracket convention is used or not
      const char *colon = strrchr(protoInfo, ':');  // before the port
      if (!colon || sscanf(colon+1, "%hu;", &m_portNum) != 1)
	throw OU::Error("Invalid socket endpoint format in \"%s\"", protoInfo);
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
      env = getenv("OCPI_TRANSFER_PORT");
      if (env && env[0]) {
	static uint16_t s_port = 0;
	if (!s_port)
	  s_port = (uint16_t)atoi(env);
	m_portNum = s_port++;
      } else {
	m_portNum = 0;
	ocpiDebug("Set the OCPI_TRANSFER_PORT environment variable to set socket IP port");
      }
      setProtoInfo();
    }
    // Socket endpoints need an address space too in come cases, so we provide one by
    // simply using the mailbox number as the high order bits.
    m_address = (uint64_t)mailBox() << 32;
  }
private:
  void
  setProtoInfo() {
      OU::format(m_protoInfo, "%s:%u", m_ipAddress.c_str(), m_portNum);
  }
  void
  updatePortNum(uint16_t portNum) {
    if (portNum != m_portNum) {
      m_portNum = portNum;
      setProtoInfo();
      setName();
    }
  }
  XF::SmemServices &createSmemServices();
};
class XferServices;
class Device;
const char *socket = "socket"; // name passed to inherited template class
class XferFactory : public DriverBase<XferFactory, Device, XferServices, socket> {
public:
  // Get our protocol string
  const char* getProtocol() { return "ocpi-socket-rdma"; }
  XF::XferServices &createXferServices(XF::EndPoint &source, XF::EndPoint &target);
protected:
  XF::EndPoint &
  createEndPoint(const char *protoInfo, const char *eps, const char *other, bool local,
		 size_t size, const OCPI::Util::PValue *params) {
    ocpiDebug("In Socket::XferFactory::createEndPoint(): %zu", m_SMBSize);
    return *new EndPoint(*this, protoInfo, eps, other, local, size, params);
  }
};

// Thread per peer writing to this endpoint
class ServerSocketHandler : public OU::Thread {
  EndPoint     &m_sep;
  SmemServices &m_smem;
  bool          m_run;
  OS::Socket    m_socket;
public:
  ServerSocketHandler(OS::ServerSocket &server, EndPoint &sep, SmemServices &smem)
    : m_sep(sep), m_smem(smem), m_run(true) {
    ocpiDebug("ServerSockletHandler accepting %u", sep.m_portNum);
    server.accept(m_socket);
    m_socket.linger(true); // give some time for data to the client FIXME timeout param?
    start();
  }

  virtual ~ServerSocketHandler() {
    stop();
    join();
    ocpiDebug("In ~ServerSocketHandler()");
  }

  void stop() {
    m_run = false;
  }

  void run() {
    try {
      size_t     n;
      uint8_t    buf[TCP_BUFSIZE];
      DataHeader header;
      uint8_t   *current_ptr = NULL;
      size_t     bytes_left = 0;
      bool       in_header = true;;
      while(m_run && (n = m_socket.recv((char*)buf, TCP_BUFSIZE, 500))) {
	if (n == SIZE_MAX)
	  continue; // allow timeout so m_run can go away and shut us down
	size_t copy_len;
	for (uint8_t *bp = buf; n; n -= copy_len, bp += copy_len) {
	  if (bytes_left == 0) { //starting the header or starting the payload
	    if (in_header) {
	      current_ptr = (uint8_t*)&header;
	      bytes_left = sizeof(header);
	    } else {
	      ocpiDebug("Received Header: %8x: %" PRIx32 " %" PRIx32,
			header.count, header.length, header.offset);
	      bytes_left = header.length;
	      current_ptr =
		m_sep.receiver() ? NULL : (uint8_t *)m_smem.map(header.offset, header.length);
	    }
	  }
	  copy_len = std::min(n, bytes_left);
	  ocpiDebug("Copying socket data to %p, size = %zu, in header %d, left %zu, first %x",
		    current_ptr, copy_len, in_header, bytes_left, *(uint32_t *)bp);
	  if (current_ptr) {
	    memcpy(current_ptr, bp, copy_len );
	    current_ptr += copy_len;
	  } else {
	    m_sep.receiver()->receive(header.offset, bp, copy_len);
	    header.offset += OCPI_UTRUNCATE(DtOsDataTypes::Offset, copy_len);
	  }
	  if (!(bytes_left -= copy_len))
	    in_header = !in_header;
	}
      }
      if (n == 0)
	ocpiInfo("Got a socket EOF for endpoint, terminating connection");
    } catch (std::string &s) {
      ocpiBad("Exception in endpoint socket receiver background thread: %s", s.c_str());
    } catch (...) {
      ocpiBad("Unknown exception in endpoint socket receiver background thread");
    }
    m_socket.close();
  }
};

// Master listener thread per endpoint to receive connection requests from peers
class ServerT : public OU::Thread {
  EndPoint                         &m_sep;
  SmemServices                     &m_smem;
  bool                              m_stop;
  bool                              m_started;
  bool                              m_error;
  OS::ServerSocket                  m_server;
  std::deque<ServerSocketHandler *> m_sockets;
public:  
  ServerT(EndPoint &sep, SmemServices &smem)
    : m_sep(sep), m_smem(smem), m_stop(false), m_started(false), m_error(false) {
    // This server socket setup must happen in the constructor because the port
    // must be determined before this returns.
    try {
      m_server.bind(m_sep.m_portNum, false);
    } catch(std::string & err) {
      m_error = true;
      ocpiBad("Socket bind error. %s", err.c_str() );
      ocpiAssert("Unable to bind to socket"==0);
      return;
    } catch( ... ) {
      m_error = true;
      ocpiAssert("Unable to bind to socket"==0);
      return;
    }
    if (m_sep.m_portNum == 0) {
      // We now know the real port, so we need to change the endpoint string.
      m_sep.updatePortNum(m_server.getPortNo());
      ocpiInfo("Finalizing socket endpoint with port: %s", m_sep.name().c_str());
    }
  }
  ~ServerT(){
    stop();
    join();
    while (!m_sockets.empty()) {
      ServerSocketHandler* ssh = m_sockets.front();
      m_sockets.pop_front();
      delete ssh;
    }
  }

  void run() {
    m_started = true;
    while (!m_stop)
      if (m_server.wait(500)) // give a chance to stop every 1/2 second
	m_sockets.push_back(new ServerSocketHandler(m_server, m_sep, m_smem));
    m_server.close();
  }
  void stop() { m_stop=true; }
  void btr() {
    while (!m_started)
      OS::sleep(10);
  }
  bool error(){return m_error;}
};

class SmemServices : public XF::SmemServices {
  ServerT  *m_socketServerT;
  char     *m_mem;
public:
  SmemServices(EndPoint& ep, bool local)
    : XF::SmemServices(ep), m_socketServerT(NULL), m_mem(NULL) {
    if (local) {
      if (!ep.receiver()) {
	m_mem = new char[ep.size()];
	memset(m_mem, 0, ep.size());
      }
      // Create our listener socket thread so that we can respond to incoming requests  
      m_socketServerT = new ServerT(ep, *this);
      m_socketServerT->start();
      m_socketServerT->btr();  
    }
  }
  ~SmemServices () {
    delete m_socketServerT;
    delete [] m_mem;
  }

  int32_t attach(XF::EndPoint* /*loc*/) { return 0; }
  int32_t detach() { return 0; }
  void *map(DtOsDataTypes::Offset offset, size_t/* size */)
  {
    //    assert(m_mem);
    return &m_mem[offset];
  }
  int32_t unMap () { return 0; }
};

XF::SmemServices &EndPoint::createSmemServices() {
  return *new SmemServices(*this, local());
}

class XferRequest;
class XferServices
  : public ConnectionBase<XferFactory,XferServices,XferRequest> {
  // So the destructor can invoke "remove"
  friend class XferRequest;
  // The handle returned by xfer_create
  XF_template        m_xftemplate;
  OS::Socket         m_socket;
public:
  XferServices(XF::EndPoint &source, XF::EndPoint &target)
    : ConnectionBase<XferFactory,XferServices,XferRequest>
      (*this, source, target) {
    xfer_create (source, target, 0, &m_xftemplate);
    EndPoint &rsep = *static_cast<EndPoint *>(&target);
    m_socket.connect(rsep.m_ipAddress, rsep.m_portNum);
    m_socket.linger(false);
  }
  ~XferServices() {
    // Invoke destroy without flags.
    xfer_destroy(m_xftemplate, 0);
    m_socket.close();
  }
  XF::XferRequest *createXferRequest();
protected:
  OS::Socket& socket(){ return m_socket; }
  void send(DtOsDataTypes::Offset offset, uint8_t *data, size_t nbytes) {
    DataHeader hdr;
    static uint32_t count = 0xabc00000;
    hdr.offset = offset;
    hdr.length = OCPI_UTRUNCATE(uint32_t, nbytes);
    hdr.count = count++;
    ocpiDebug("Sending IP header %zu %" PRIu32 " %" DTOSDATATYPES_OFFSET_PRIx" %" PRIx32,
	      sizeof(DataHeader), hdr.length, hdr.offset, hdr.count);
    m_socket.send((char*)&hdr, sizeof(DataHeader));
    m_socket.send((char *)data, nbytes);
  }
};

XF::XferServices &XferFactory::
createXferServices(XF::EndPoint &source, XF::EndPoint &target)
{
  return *new XferServices(source, target);
}

class XferRequest : public TransferBase<XferServices,XferRequest> {
  XF_transfer m_thandle;                // Transfer handle returned by xfer_xxx etal
public:
  XferRequest(XferServices &a_parent, XF_template temp)
    : TransferBase<XferServices,XferRequest>(a_parent, *this, temp) {
  }

  // Get Information about a Data Transfer Request
  inline DataTransfer::XferRequest::CompletionStatus getStatus() {
    return xfer_get_status (m_thandle) == 0 ?
      DataTransfer::XferRequest::CompleteSuccess : DataTransfer::XferRequest::Pending;
  }

  // Data members accessible from this/derived class
private:
  void action_transfer(PIO_transfer transfer, bool /*last*/) {
    //#define TRACE_PIO_XFERS  
#ifdef TRACE_PIO_XFERS
    ocpiDebug("Socket: copying %d bytes from 0x%llx to 0x%llx", transfer->nbytes,transfer->src_off,transfer->dst_off);
    ocpiDebug("source wrd 1 = %d", src1[0] );
#endif
    parent().send(transfer->dst_off, (uint8_t *)transfer->src_va, transfer->nbytes);
  }
};

XF::XferRequest* XferServices::
createXferRequest() {
  return new XferRequest(*this, m_xftemplate);
}

class Device : public XF::DeviceBase<XferFactory,Device> {
  Device(const char *a_name)
    : XF::DeviceBase<XferFactory,Device>(a_name, *this) {}
};

// Used to register with the data transfer system;
RegisterTransferDriver<XferFactory> driver;
}
}
