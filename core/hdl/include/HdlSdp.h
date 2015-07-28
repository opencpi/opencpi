#ifndef HDL_SDP_H
#define HDL_SDP_H
#include <stdint.h>
#include "OcpiOsSocket.h"
// These definitions must match the VHDL
// constant max_reads_outstanding  : natural := 8;
// constant max_message_kbytes     : natural := 16; -- jumbo frames +
// constant max_addressable_kbytes : natural := 16*1024*1024; -- 16GB
// constant max_nodes              : natural := 8; -- always includes one for control
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
	  node_width             = 3,
          node_shift             = trail_shift + trail_width,
          addr_width             = 22, // addressing datums, dwords, 16MB per node
          addr_shift             = node_shift + node_width,
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
	  uint32_t m_header[2]; // Worst case scenario of a header size, asserted
	  uint64_t m_bits;
	};
	unsigned m_xid;
	uint64_t m_address;
	size_t   m_length;
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
	  m_bits |= field << field##_shift;				             \
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
	SDP_FIELD(uint64_t, addr)
	SDP_FIELDS
	inline bool doRequest(OCPI::OS::Socket &s, uint8_t *data, std::string &error) {
	  size_t length;
	  return startRequest(s.fd(), data, length, error) ? true :
	    endRequest(s.fd(), data, error);
	}
	bool startRequest(int sendFd, uint8_t *data, size_t &length, std::string &error);
	bool endRequest(int recvFd, uint8_t *data, std::string &error);

      };
    }
  }
}

#endif
