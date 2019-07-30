-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

library ieee, ocpi, platform;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all, ocpi.all, ocpi.types.all, ocpi.util.all,
  platform.all;
package sdp is
-- Constraints/assumptions:
-- We're in a DWORD (32-bit) world.  The data path is a mutiple of 32 bits.
-- The start byte offset and end byte offset are for DWORDS
-- The address is in DWORDS.
constant max_reads_outstanding  : natural := 8;  -- PER NODE, throttled down with properties
constant max_message_kbytes     : natural := 16; -- jumbo frames +
constant max_addressable_kbytes : natural := 64*1024; -- 64MB per node
constant max_nodes              : natural := 16; -- always includes one for control
constant datum_bits             : natural := 32; -- THIS IS ASSUMED BY VARIOUS CLIENTS

constant xid_width              : natural := width_for_max(max_reads_outstanding-1);
constant node_width             : natural := width_for_max(max_nodes-1);
constant datum_bytes            : natural := datum_bits/8;
constant max_message_units      : natural := (max_message_kbytes * 1024 * 8) / datum_bits;
constant count_width            : natural := width_for_max(max_message_units-1);
constant end_bytes_width        : natural := width_for_max(datum_bytes - 1);
constant addr_width             : natural := width_for_max(1024-1) +
                                             width_for_max(max_addressable_kbytes/datum_bytes-1);
constant extaddr_width          : natural := 36 - addr_width - width_for_max(datum_bytes-1);
constant max_pkt_dws            : natural := max_message_units;
type op_t is (read_e,
              write_e,
              response_e,
              reserved_e);
constant op_width : natural := width_for_max(op_t'pos(op_t'high));
subtype id_t is unsigned(node_width-1 downto 0);
subtype xid_t is unsigned(xid_width-1 downto 0);
subtype count_t is unsigned(count_width-1 downto 0);
subtype addr_t is unsigned(addr_width-1 downto 0);
subtype extaddr_t is unsigned(extaddr_width-1 downto 0);
subtype pkt_ndw_t is unsigned(count_width downto 0);
-- ASSUMPTION:  count fits in first DW...
-- UPDATE dws2header and header2dws if this is changed.
type header_t is record
  -- Note the count must be first.
  count   : count_t;     -- like AXI: 0 means 1 etc.
  op      : op_t;
  xid     : xid_t;
  lead    : unsigned(end_bytes_width-1 downto 0); -- similar to AXI address LSB
  trail   : unsigned(end_bytes_width-1 downto 0);
  node    : id_t; -- part of address for outbound requests
  addr    : addr_t;
  extaddr : extaddr_t; 
end record header_t;
constant sdp_header_width : natural := width_for_max(op_t'pos(op_t'high)) + xid_width + end_bytes_width*2 +
                                     count_width + node_width + addr_width;
constant sdp_header_ndws  : natural := (sdp_header_width + (dword_t'length-1)) / dword_t'length;

constant whole_addr_bits_c : integer := addr_width + extaddr_width;
subtype whole_addr_t is unsigned(whole_addr_bits_c-1 downto 0);

-- The record in both directions
type sdp_t is record
  header  : header_t; -- will be stable from sop to eop
  eop     : bool_t;   -- end of packet
  valid   : bool_t;   -- data/header is valid - like AXI
  ready   : bool_t;   -- can accept/is accepting data from other side - like AXI
end record sdp_t;
type m2s_t is record
  clk   : std_logic;
  reset : bool_t;
  id    : id_t;
  sdp   : sdp_t;
end record m2s_t;
type m2s_array_t is array(natural range <>) of m2s_t;
type s2m_t is record
  sdp       : sdp_t;
  dropCount : uchar_t;
end record s2m_t;
type s2m_array_t is array(natural range <>) of s2m_t;

function dword2header(dw : dword_t) return header_t;
function payload_in_dws(hdr : header_t) return unsigned;
function payload_in_dws(dw : dword_t) return unsigned;
function header2dws(h : header_t) return dword_array_t;
function dws2header(dws : dword_array_t(0 to sdp_header_ndws-1)) return header_t;
function count_in_dws(header : header_t) return unsigned;
-- Get the DW count from the first DW of the header
function count_in_dws(dw : dword_t) return unsigned;
-- Which DW in the frame does this transfer start at?
function start_dw(header : header_t; sdp_width : uchar_t) return natural;
-- A full byte enable for the indicated datum in the packet.
function header2be(h : header_t; word : unsigned) return std_logic_vector;

--------------------------------------------------------------------------------
-- Definitions for the compressed metadata dword used for DMA
-- Metadata (internal) definitions are consistent with DtHandshakeControl.h
--------------------------------------------------------------------------------
constant meta_length_width_c : natural := 21; -- MUST BE IN SYNC WITH SW DtHandshakeControl.h
constant meta_opcode_width_c : natural := 8;
constant meta_one_c          : natural := 0;
constant meta_length_c       : natural := meta_one_c + 1;
constant meta_eof_c          : natural := meta_length_c + meta_length_width_c;
constant meta_truncate_c     : natural := meta_eof_c + 1;
constant meta_opcode_c       : natural := meta_truncate_c + 1;
subtype metalength_t is unsigned(meta_length_width_c-1 downto 0);
subtype metalength_dws_t is unsigned(meta_length_width_c-1-dword_shift downto 0);
type metadata_t is record
  length   : metalength_t;
  eof      : bool_t;
  truncate : bool_t;
  opcode   : std_logic_vector(meta_opcode_width_c-1 downto 0);
end record metadata_t;
constant metawidth_c : integer := meta_length_width_c + 1 + 1 + 1 + meta_opcode_width_c;
constant meta_ndws_c : integer := (metawidth_c + dword_size - 1) / dword_size;
--subtype meta_dw_count_t is unsigned(meta_length_width_c-3 downto 0);
function meta2slv(meta : metadata_t) return std_logic_vector;
function slv2meta(s : std_logic_vector(metawidth_c-1 downto 0)) return metadata_t;

component sdp_term
  generic(ocpi_debug      :     bool_t := bfalse;
          sdp_width       :     natural);
  port(   up_in           : in  m2s_t;
          up_out          : out s2m_t;
          up_in_data      : in  dword_array_t(0 to sdp_width-1);
          up_out_data     : out dword_array_t(0 to sdp_width-1);
          drop_count      : out uchar_t);
end component sdp_term;
    
component sdp2cp
  generic(ocpi_debug :     bool_t;
          sdp_width  :     natural);
  port(sdp_in        : in  m2s_t;
       sdp_out       : out s2m_t;
       sdp_in_data   : in  dword_array_t(0 to sdp_width-1);
       sdp_out_data  : out dword_array_t(0 to sdp_width-1);
       cp_in         : in  platform_pkg.occp_out_t;
       cp_out        : out platform_pkg.occp_in_t);
end component sdp2cp;


component sdp_sim
  generic(ocpi_debug   :     bool_t;
          sdp_width    :     uchar_t);
  port(   clk          : in  std_logic;
          reset        : in  bool_t;
          sdp_in       : in  s2m_t;
          sdp_out      : out m2s_t;
          sdp_in_data  : in  dword_array_t(0 to to_integer(sdp_width)-1);
          sdp_out_data : out dword_array_t(0 to to_integer(sdp_width)-1));
end component sdp_sim;

-- This is used so that we can leave legacy unoc adapters in place.
component unoc2sdp
  generic(ocpi_debug   :     bool_t;
          sdp_width    :     natural);
  port (  unoc_in      : in  platform_pkg.unoc_master_out_t;
          unoc_out     : out platform_pkg.unoc_master_in_t;
          sdp_in       : in s2m_t;
          sdp_out      : out m2s_t;
          sdp_in_data  : in dword_array_t(0 to sdp_width-1);
          sdp_out_data : out dword_array_t(0 to sdp_width-1));
end component unoc2sdp;
component sdp_node_rv is
  generic(
    sdp_width       : in   uchar_t := to_uchar(1));
  port(
    wci_Clk         : in   std_logic;
    wci_Reset_n     : in   std_logic;
    up_in           : in  m2s_t;
    up_out          : out s2m_t;
    up_in_data      : in  dword_array_t(0 to to_integer(sdp_width)-1);
    up_out_data     : out dword_array_t(0 to to_integer(sdp_width)-1);
    client_in       : in  s2m_t;
    client_out      : out m2s_t;
    client_in_data  : in  dword_array_t(0 to to_integer(sdp_width)-1);
    client_out_data : out dword_array_t(0 to to_integer(sdp_width)-1);
    down_in         : in  s2m_t;
    down_out        : out m2s_t;
    down_in_data    : in  dword_array_t(0 to to_integer(sdp_width)-1);
    down_out_data   : out dword_array_t(0 to to_integer(sdp_width)-1));
end component;
component sdp2cp_clk_rv is
  generic(
    sdp_width    : in   uchar_t := to_uchar(1));
  port(
    wci_Clk      : in   std_logic;
    wci_Reset_n  : in   std_logic;
    cp_in        : in  platform.platform_pkg.occp_out_t;
    cp_out       : out platform.platform_pkg.occp_in_t;
    sdp_in       : in  m2s_t;
    sdp_out      : out s2m_t;
    sdp_in_data  : in  dword_array_t(0 to to_integer(sdp_width)-1);
    sdp_out_data : out dword_array_t(0 to to_integer(sdp_width)-1));
end component;


end package sdp;

