#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include "OcpiOsDebug.h"
#include "OcpiUtilMisc.h"
#include "OcpiUtilException.h"
#include "HdlSdp.h"
namespace OCPI {
  namespace HDL {
    namespace SDP {
      namespace OU = OCPI::Util;
      unsigned Header::s_xid = 0;
      Header::
      Header(bool read, uint64_t address, size_t length)
	: m_bits(0), m_xid(0), m_length(length) {
	ocpiDebug("SDH Header %p construct: read %d address %" PRIx64 " length %zu", this, read,
		  address, length);
	assert(length <= max_message_bytes);
	assert(header_ndws <= 2);
	assert(length);
	assert(!(address & 1) || length == 1);
	assert(!(address & 3) || length < 4);
	set_count((length + datum_bytes - 1) / datum_bytes - 1);
	set_op(read ? ReadOp : WriteOp);
	if (read) {
	  set_xid(m_xid = s_xid);
	  // FIXME: multithreaded control actions
	  if (++s_xid == max_reads_outstanding)
	    s_xid = 0;
	} else
	  set_xid(0);
	set_lead(address & ~(UINT64_MAX << datum_addr_bits));
	set_trail((datum_bytes - ((address + length) & (datum_bytes - 1))) & (datum_bytes - 1));
	set_node(OCPI_UTRUNCATE(uint8_t, address >> (addr_width + datum_addr_bits)));
	set_addr((address >> datum_addr_bits) & ~(UINT64_MAX << addr_width));
      }
      Header::
      Header()
	: m_xid(0), m_length(0) {
      }
      Header::~Header() {
      }
      // return true on error, otherwise read the amount requested
      bool
      read(int fd, uint8_t *buf, size_t nRequested, std::string &error) {
	ocpiDebug("SIM reading %zu from fd %d", nRequested, fd);
	do {
	  ssize_t nread = ::read(fd, (char *)buf, nRequested);
	  if (nread == 0)
	    // shouldn't happen because we have it open for writing too
	    error = "EOF on SDP channel, simulation server stopped";
	  else if (nread > 0) {
	    nRequested -= nread;
	    buf += nread;
	    ocpiDebug("SIM got %zu from fd %d", nread, fd);
	  } else if (errno != EINTR)
	    error = "error reading FIFO";
	  else {
	    write(2, "\nSDP Read interrupted2\n", 23);
	    //	    ocpiDebug("SDP Read interrupted");
	  }
	} while (error.empty() && nRequested);
	return !error.empty();
      }

      bool Header::
      startRequest(int sendFd, uint8_t *data, size_t &length, std::string &error) {
	ocpiDebug("Start request %p is:    op %u count %zu xid %u node %u lead %u trail %u",
		  this, get_op(), get_count(), get_xid(), get_node(), get_lead(), get_trail());
        if (OS::logGetLevel() >= OCPI_LOG_DEBUG) {
	  fprintf(stderr, "Sending header: ");
	  for (unsigned n = 0; n < header_ndws; n++)
	    fprintf(stderr, " 0x%08x", ((uint32_t*)m_header)[n]);
	  fprintf(stderr, "\n");
	}
	ssize_t r, len = header_ndws * dword_bytes;
	if ((r = write(sendFd, (char *)m_header, len) != len))
	  return OU::eformat(error, "Error writing SDP data to simulator: %zd/%zd %s %d",
			     r, len, strerror(errno), errno);
	length = (size_t)len;
	//s.send((char *)m_header, header_ndws * dword_bytes);
	if (get_op() == WriteOp) {
#if 0
	  ssize_t nw = m_length, offset;
	  if ((offset = m_address & (dword_bytes - 1))) {
	    nw += offset;
	    data -= offset;
	  }
	  if ((offset = nw & (dword_bytes - 1)))
	    nw += dword_bytes - offset;
	  // Now send the data, with padding
#else
	  ssize_t nw = m_length < sizeof(uint32_t) ? sizeof(uint32_t) : m_length;
#endif
	  length += nw;
	  if ((r = write(sendFd, (char *)data, nw) != nw))
	    return OU::eformat(error, "Error writing SDP data to simulator: %zd %zu %s %d",
			       r, nw, strerror(errno), errno);
	  //	  s.send((char *)data, nw);
	  if (OS::logGetLevel() >= OCPI_LOG_DEBUG) {
	    fprintf(stderr, "Sending %p Data: ", this);
	    for (unsigned n = 0; n < nw; n += 4)
	      fprintf(stderr, " 0x%08x", data[n/4]);
	    fprintf(stderr, "\n");
	  }
	}
	ocpiDebug("Request %p sent %zu bytes", this, length);
	return false;
      }
      bool Header::
      getHeader(int recvFd, bool &request, size_t &length, std::string &error) {
	ocpiDebug("Getting incoming SDP header");
	size_t hlen = header_ndws * dword_bytes;
	if (read(recvFd, (uint8_t *)m_header, hlen, error))
	  return true;
	ocpiDebug("Received header: op %u count %zu xid %u node %u lead %u trail %u addr 0x%"
		  PRIx64, get_op(), get_count(), get_xid(), get_node(), get_lead(), get_trail(),
		  get_addr());
        length = (get_count() + 1) * dword_bytes;
	request = get_op() == ReadOp || get_op() == WriteOp;
	return false;
      }
      // Given a response that is in a header already, finish it
      bool Header::
      endRequest(Header &h, int recvFd, uint8_t *data, std::string &error) {
	ocpiDebug("Received SDP header: %x %x", h.m_header[0], h.m_header[1]);
	if (h.get_op() != ResponseOp ||
	    h.get_count() != get_count() ||
	    h.get_xid() != get_xid() ||
	    h.get_node() != get_node()) {
	  ocpiDebug("Bad SDP header: op %u count %zu xid %u node %u lead %u trail %u",
		    h.get_op(), h.get_count(), h.get_xid(), h.get_node(), h.get_lead(),
		    h.get_trail());
	  ocpiDebug("Request was:    op %u count %zu xid %u node %u lead %u trail %u",
		    get_op(), get_count(), get_xid(), get_node(), get_lead(), get_trail());
	  return OU::eformat(error, "Bad SDP response header to read request");
	}
	ocpiDebug("Good SDP header: op %u count %zu xid %u node %u lead %u trail %u",
		  h.get_op(), h.get_count(), h.get_xid(), h.get_node(), h.get_lead(),
		  h.get_trail());
	ocpiDebug("Request was:     op %u count %zu xid %u node %u lead %u trail %u",
		  get_op(), get_count(), get_xid(), get_node(), get_lead(), get_trail());
	std::string err;
	uint8_t junk[dword_bytes];
#if 1
	size_t length = m_length;
	if (get_lead() && read(recvFd, junk, get_lead(), err))
	  return OU::eformat(error, "Bad SDP response padding to read request: %s", err.c_str());
#else
	size_t length = m_length + get_lead() + get_trail();
#endif
	if (read(recvFd, data, length, err))
	  return OU::eformat(error, "Bad SDP response data to read request: %s", err.c_str());
	if (OS::logGetLevel() >= OCPI_LOG_DEBUG) {
	  fprintf(stderr, "Received Data (%zu): ", length);
	  for (unsigned n = 0; n < length; n += 4)
	    fprintf(stderr, " 0x%08x", ((uint32_t *)data)[n/4]);
	  fprintf(stderr, "\n");
	}
	if (get_trail() && read(recvFd, junk, get_trail(), err))
	  return OU::eformat(error, "Bad SDP response padding to read request: %s", err.c_str());
	return false;
	
      }
      bool Header::
      endRequest(int recvFd, uint8_t *data, std::string &error) {
	if (get_op() == WriteOp)
	  return false;
	assert(get_op() == ReadOp);
	Header h;
	bool request;
	size_t length;
	return h.getHeader(recvFd, request, length, error) || endRequest(h, recvFd, data, error);
      }
    }
  }
}
