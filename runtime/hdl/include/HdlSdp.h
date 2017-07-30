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

#ifndef HDL_SDP_H
#define HDL_SDP_H
#include <stdint.h>
#include <climits>
#include "OcpiOsSocket.h"
// These definitions must match the VHDL
// constant max_reads_outstanding  : natural := 8;
// constant max_message_kbytes     : natural := 16; -- jumbo frames +
// constant max_addressable_kbytes : natural := 16*1024*1024; -- 16GB
// constant max_nodes              : natural := 1024; -- always includes one for control
// constant datum_bits             : natural := 32; -- THIS IS ASSUMED BY VARIOUS CLIENTS

// constant xid_width              : natural := width_for_max(max_reads_outstanding-1);
// constant node_width             : natural := width_for_max(max_nodes-1);
// constant datum_bytes            : natural := datum_bits/8;
// constant max_message_units      : natural := (max_message_kbytes * 1024 * 8) / datum_bits;
// constant count_width            : natural := width_for_max(max_message_units-1);
// constant end_bytes_width        : natural := width_for_max(datum_bytes - 1);
// constant addr_width             : natural := width_for_max(1024-1) +
//                                              width_for_max(max_addressable_kbytes/datum_bytes);
// type op_t is (read_e,
//               write_e,
//               response_e,
//               reserved_e);
// constant op_width : natural := width_for_max(op_t'pos(op_t'high));
// subtype id_t is unsigned(node_width-1 downto 0);
// -- ASSUMPTION:  count fits in first DW...
// -- UPDATE dws2header and header2dws if this is changed.
// type header_t is record
//   -- Note the count must be first.
//   count : unsigned(count_width-1 downto 0);     -- like AXI: 0 means 1 etc.
//   op    : op_t;
//   xid   : unsigned(xid_width-1 downto 0);
//   lead  : unsigned(end_bytes_width-1 downto 0); -- similar to AXI address LSB
//   trail : unsigned(end_bytes_width-1 downto 0);
//   node  : unsigned(node_width-1 downto 0); -- part of address for outbound requests
//   addr  : unsigned(addr_width-1 downto 0);
// end record header_t;
// constant sdp_header_width : natural := width_for_max(op_t'pos(op_t'high)) + xid_width + end_bytes_width*2 +
//                                      count_width + node_width + addr_width;
// constant sdp_header_ndws  : natural := (sdp_header_width + (dword_t'length-1)) / dword_t'length;

namespace OCPI {
  namespace HDL {
    namespace SDP {
      class Header {
      public:
	static const unsigned
	  dword_addr_bits        = 2,
	  dword_bytes            = 1 << dword_addr_bits,
          dword_bits             = dword_bytes * 8,
          datum_addr_bits        = dword_addr_bits, // Many things assume this!!!!!
          datum_bytes            = 1 << datum_addr_bits,
	  datum_bits             = datum_bytes * 8,
	  // Actual field widths
          count_width            = 12,     // 4K DWORDS, 16KB, at least jumbo frames
          count_shift            = 0,					
	  op_width               = 2,
          op_shift               = count_shift + count_width,
	  xid_width              = 3,
          xid_shift              = op_shift + op_width,
          lead_width             = datum_addr_bits,
          lead_shift             = xid_shift + xid_width,
          trail_width            = datum_addr_bits,
          trail_shift            = lead_shift + lead_width,
	  node_width             = 4,
          node_shift             = trail_shift + trail_width,
          addr_width             = 24, // addressing datums, dwords, 64MB per node
          addr_shift             = node_shift + node_width,
	  extaddr_width          = 36 - addr_width - dword_addr_bits,
	  extaddr_shift          = addr_shift + addr_width,
          header_width           = addr_shift + addr_width,
	  // derived max values
	  max_message_bytes      = ((1 << count_width) * datum_bytes),
	  max_reads_outstanding  = 1 << xid_width,
          max_addressable_kbytes = 1 << (addr_width - 8),
          max_nodes              = 1 << node_width, // always includes one for control
          header_ndws            = (header_width + dword_bits - 1) / dword_bits;
	static const uint64_t max_addressable_bytes = (uint64_t)max_addressable_kbytes << 10;
      private:
	union {
	  uint64_t m_bits;
	  uint32_t m_header[2]; // Worst case scenario of a header size, asserted
	  uint8_t  m_bytes[dword_bytes * header_ndws];
	};
	unsigned m_xid;
	size_t   m_actualByteLength;
	static unsigned s_xid;
      public:
	enum Op {
	  ReadOp,
	  WriteOp,
	  ResponseOp,
	  ReservedOp,
	  OpLimit
	};
	Header();
	Header(bool read, uint64_t address, size_t length);
	~Header();
	// Unfortunate effect of non-portable bitfields
#define SDP_FIELD(type, field)						             \
	void set_##field(type field) {					             \
	  assert(!(field & (UINT64_MAX << field##_width)));		             \
	  m_bits &= ~(~(UINT64_MAX << field##_width) << field##_shift);              \
	  m_bits |= (uint64_t)field << field##_shift;			\
        }								             \
	type get_##field() {						             \
	  return (type)((m_bits >> field##_shift) & ~(UINT64_MAX << field##_width)); \
	}
#define SDP_FIELDS \
	SDP_FIELD(size_t, count)		\
	SDP_FIELD(Op, op)			\
	SDP_FIELD(unsigned, xid)		\
	SDP_FIELD(unsigned, lead)		\
	SDP_FIELD(unsigned, trail)		\
	SDP_FIELD(unsigned, node)		\
	SDP_FIELD(unsigned, addr)               \
	SDP_FIELD(unsigned, extaddr)
	SDP_FIELDS
        inline uint64_t getWholeByteAddress() {
	  return
	    (get_addr() << 2) | get_lead() |
	    ((uint64_t)get_extaddr() << (addr_width + lead_width));
	}
        inline size_t getLength() {
	  return m_actualByteLength;
	}
	inline bool doRequest(OCPI::OS::Socket &s, uint8_t *data, std::string &error) {
	  size_t length;
	  return startRequest(s.fd(), data, length, error) ? true :
	    endRequest(s.fd(), data, error);
	}
	bool startRequest(int sendFd, uint8_t *data, size_t &length, std::string &error);
	bool sendResponse(int sendFd, uint8_t *data, size_t &length, std::string &error);
	bool endRequest(int recvFd, uint8_t *data, std::string &error);
	bool getHeader(int recvFd, bool &request, std::string &error);
	bool endRequest(Header &h, int recvFd, uint8_t *data, std::string &error);
        void respond();
      };
      bool read(int fd, uint8_t *buf, size_t nRequested, std::string &error);
      // The SDP data plane from an external point of view:
      // The property sets for sender and receivers are below and similar to the
      // analogous OCDP registers except that they support fan-in and fan-out.
      // The sender/receiver workers are assigned SDP node IDs which are available in
      // their registers, which determines the external addresses of the buffer memory
      // when it is visible (i.e. for passive or active-flow-control modes.).

      // The external address is a combination of node and address-within-node
      // So the external address is <node><addr><2lsbs>.
      // This is currently 3+24+2 = 29 bits or 1/2 GB.
      /*
	So for a given interconnect we need:
	1. Control space offset (like bar0 address in PCI, maybe not in bitstream anyway)
	2. Data space offset (like bar1 in PCI, maybe not in bitstream anyhow.)
	3. Data window size per node (could be in bitstream). 4k (2^12) up to 64MB (2^26).
        For a given sender we need:
	1. Memory size
	2. Node id
	3. Interconnect ordinal
      DMA METADATA
      -- for our purposes a single word of metadata can be the flag.
      -- this sacrifices the timestamp feature.
      -- this makes the logic simpler, and the transfers more efficient.
      -- some hardware needs an ad hoc random flag/mailbox that we can't control the contents of
      -- ideally the timestamp is optional.
      -- existing SW:
      --  expects flags separate
      -- so we make the following partial change:
      --   - the metadata format changes to a single word that hardware can use
      --   - the flag still operates for those that need it.
       */
      // For PCI, we divide control and data, but really it means that the control plane BAR
      // is for the whole 64MB for node 0, but the data BAR are divided differently,
      // So we need:
      //    Size of data bar - changed least often (PCI coregen)
      //    Window size per node - changeable
      //    Memory size per node - a property of each.
      // So each interconnect SDP adapter says the window size.
      // Node 0 is always the control plane.
      // Someday it would be nice to sync this with the property system.
      // MUST BE IN SYNC WITH sdp_send.xml
      // For SIM, we need an endpoint that is "active", with a hook to drive the simulated
      // SDP in SW.
      // For non-SIM, it just like 
      const unsigned NREMOTES = 1;
      const unsigned BYTES_PER_DWORD = 4;
      struct Properties {
	// Constant values available from the hardware
	const uint32_t memory_bytes;
	const uint8_t sdp_width, interconnect, window_log2, max_buffers, max_remotes;
	const uint8_t sdp_id; // not a parameter of the worker, but fed from the SDP itself
	// Configured by software
	uint32_t buffer_size;
	uint8_t buffer_count;
	uint8_t remote_count;
	uint16_t segment_size;
	// Changed by the worker to indicate messages too large
	volatile uint8_t  overflow;
	uint8_t readsAllowed;
	uint32_t role; // an enumeration 0:AM, 1:AFC, 2: passive
	// Configured by software for remote side
	uint64_t remote_data_addr[NREMOTES];
	uint32_t remote_data_pitch[NREMOTES];
	uint64_t remote_meta_addr[NREMOTES];
	uint32_t remote_meta_pitch[NREMOTES];
	uint64_t remote_flag_addr[NREMOTES];
	uint32_t remote_flag_pitch[NREMOTES];
	uint32_t remote_flag_value[NREMOTES];
	uint8_t remote_buffer_count[NREMOTES], remote_flag_required[NREMOTES];
	// Written by other side to indicate it processed a buffer
	uint32_t remote_doorbell[NREMOTES];
        uint8_t rem_idx, rem_bidx, rem_phase;
        uint64_t rem_addr, rem_seg;
      };
#if 0
      // Atomic metadata that can serve as flag, but doesn't always.
      struct MetaData {
	typedef uint32_t Data;
	Data m_data;
        static const unsigned c_opcode_bits = 8;
	static const uint8_t  c_eof_opcode = (1 << c_opcode_bits) - 1;
	static const unsigned c_metadata_bits = sizeof(Data) * CHAR_BIT;
	static const unsigned c_length_bits = sizeof(Data) * CHAR_BIT - 2 - c_opcode_bits;
      public:
	inline void set(uint32_t length, uint8_t opcode, bool eof) {
	  assert(length < ((uint64_t)1 << c_length_bits));
	  assert(opcode < c_eof_opcode);
	  m_data =
	    (1 << (c_metadata_bits-1)) | ((eof ? c_eof_opcode : opcode) << c_length_bits) |
	    length;
	}
	inline uint32_t data() const { return m_data; };
	inline bool valid() const { return m_data != 0; }
	inline void reset() { m_data = 0; }
	inline uint8_t opcode() const {
	  assert(valid());
	  uint8_t opcode = (m_data >> c_length_bits) & ~(-1 << c_opcode_bits);
	  assert(opcode != c_eof_opcode);
	  return opcode;
	}
	inline uint32_t length() const {
	  assert(valid());
	  uint8_t opcode = (m_data >> c_length_bits) & ~(-1 << c_opcode_bits);
	  assert(opcode != c_eof_opcode);
	  return m_data & ~(-1 << c_length_bits);
	}
	inline bool eof() const {
	  assert(valid());
	  uint8_t opcode = (m_data >> c_length_bits) & ~(-1 << c_opcode_bits);
	  return opcode == c_eof_opcode;
	}
      };
#endif
      // Constants for writing metadata and flags into the data space.
      static const uint32_t c_flag_offset = (Header::max_addressable_kbytes-4)*1024;
      static const uint32_t c_metadata_offset = (Header::max_addressable_kbytes-8)*1024;
    }      
  }
}

#endif
