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
	: m_bits(0), m_xid(0), m_actualByteLength(length) {
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
	set_node(OCPI_UTRUNCATE(unsigned, address >> 26)); // (addr_width + datum_addr_bits)));
	set_addr((address >> datum_addr_bits) & ~(UINT64_MAX << addr_width));
	set_extaddr(0); // headers constructed in SW never address
      }
      Header::
      Header()
	: m_xid(0), m_actualByteLength(0) {
      }
      Header::~Header() {
      }
      // return true on error, otherwise read the amount requested
      bool
      read(int fd, uint8_t *buf, size_t nRequested, std::string &error) {
	do {
	  ocpiDebug("SIM reading %zu from fd %d", nRequested, fd);
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
	    (void)write(2, "\nSDP Read interrupted2\n", 23);
	    //	    ocpiDebug("SDP Read interrupted");
	  }
	} while (error.empty() && nRequested);
	return !error.empty();
      }

      static bool write(int fd, uint8_t *data, size_t length, std::string &error) {
	ssize_t r, len = (ssize_t)length;
	return (r = ::write(fd, (char *)data, length)) == len ? false :
		OU::eformat(error,
			    "Error writing SDP response data to simulator: %zd/%zd %s %d",
			    r, len, strerror(errno), errno);
      }
      static uint8_t zero[Header::dword_bytes];
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
	if (write(sendFd, m_bytes, sizeof(m_bytes), error))
	  return true;
	length = sizeof(m_bytes);
	if (get_op() == WriteOp) {
	  if (get_lead()) {
	    if (write(sendFd, zero, get_lead(), error))
	      return true;
	    length += get_lead();
	  }
	  if (write(sendFd, data, m_actualByteLength, error))
	    return true;
	  length += m_actualByteLength;
	  if (get_trail()) {
	    if (write(sendFd, zero, get_trail(), error))
	      return true;
	    length += get_trail();
	  }
	  if (OS::logGetLevel() >= OCPI_LOG_DEBUG) {
	    fprintf(stderr, "Sending %p Write Data: ", this);
	    for (unsigned n = 0; n < m_actualByteLength; n++)
	      fprintf(stderr, " %02x", data[n]);
	    fprintf(stderr, "\n");
	  }
	}
	ocpiDebug("Request %p sent %zu bytes including header and padding", this, length);
	return false;
      }
      bool Header::
      sendResponse(int sendFd, uint8_t *data, size_t &length, std::string &error) {
	set_op(ResponseOp);
	ocpiDebug("Start response %p is:    op %u count %zu xid %u node %u lead %u trail %u",
		  this, get_op(), get_count(), get_xid(), get_node(), get_lead(), get_trail());
        if (OS::logGetLevel() >= OCPI_LOG_DEBUG) {
	  fprintf(stderr, "Sending response header: ");
	  for (unsigned n = 0; n < header_ndws; n++)
	    fprintf(stderr, " 0x%08x", ((uint32_t*)m_header)[n]);
	  fprintf(stderr, "\n");
	}
	if (write(sendFd, m_bytes, sizeof(m_bytes), error))
	  return true;
	length = sizeof(m_bytes);
	if (get_lead()) {
	  if (write(sendFd, zero, get_lead(), error))
	    return true;
	  length += get_lead();
	}
	if (write(sendFd, data, m_actualByteLength, error))
	  return true;
	length += m_actualByteLength;
	if (get_trail()) {
	  if (write(sendFd, zero, get_trail(), error))
	    return true;
	  length += get_trail();
	}
	if (OS::logGetLevel() >= OCPI_LOG_DEBUG) {
	  fprintf(stderr, "Sending %p response Data: ", this);
	  for (unsigned n = 0; n < m_actualByteLength; n++)
	    fprintf(stderr, " %02x", data[n]);
	  fprintf(stderr, "\n");
	}
	ocpiDebug("Response %p sent %zu bytes including header and padding", this, length);
	return false;
      }
      bool Header::
      getHeader(int recvFd, bool &request, std::string &error) {
	ocpiDebug("Getting incoming SDP header");
	size_t hlen = header_ndws * dword_bytes;
	if (read(recvFd, (uint8_t *)m_header, hlen, error))
	  return true;
	ocpiDebug("Received header: op %u count %zu xid %u node %u lead %u trail %u addr 0x%x"
		  " extaddr 0x%x whole 0x%" PRIx64,
		  get_op(), get_count(), get_xid(), get_node(), get_lead(), get_trail(),
		  get_addr(), get_extaddr(), getWholeByteAddress());
	m_actualByteLength = (get_count() + 1) * dword_bytes - get_lead() - get_trail();
	m_xid = get_xid();
	request = get_op() == ReadOp || get_op() == WriteOp;
	return false;
      }
      // Given a response that is in a header already, finish it
      bool Header::
      endRequest(Header &h, int recvFd, uint8_t *data, std::string &error) {
	ocpiDebug("Received SDP header: %x %x", h.m_header[0], h.m_header[1]);
	if (h.get_op() == ReadOp ||
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
	if (get_lead() && read(recvFd, junk, get_lead(), err))
	  return OU::eformat(error, "Bad SDP response padding to read request: %s", err.c_str());
	if (read(recvFd, data, m_actualByteLength, err))
	  return OU::eformat(error, "Bad SDP response data to read request: %s", err.c_str());
	if (OS::logGetLevel() >= OCPI_LOG_DEBUG) {
	  fprintf(stderr, "Received Data (%zu): ", m_actualByteLength);
	  for (unsigned n = 0; n < m_actualByteLength; n++)
	    fprintf(stderr, " %02x", data[n]);
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
	return h.getHeader(recvFd, request, error) || endRequest(h, recvFd, data, error);
      }
    }
  }
}
