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
constant max_nodes              : natural := 1024; -- always includes one for control, implies
                                                -- interconnect addr width
constant datum_bits             : natural := 32; -- THIS IS ASSUMED BY VARIOUS CLIENTS

constant xid_width              : natural := width_for_max(max_reads_outstanding-1);
constant node_width             : natural := width_for_max(max_nodes-1);
constant datum_bytes            : natural := datum_bits/8;
constant max_message_units      : natural := (max_message_kbytes * 1024 * 8) / datum_bits;
constant count_width            : natural := width_for_max(max_message_units-1);
constant end_bytes_width        : natural := width_for_max(datum_bytes - 1);
constant addr_width             : natural := width_for_max(1024-1) +
                                             width_for_max(max_addressable_kbytes/datum_bytes-1);
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
subtype pkt_ndw_t is unsigned(count_width downto 0);
-- ASSUMPTION:  count fits in first DW...
-- UPDATE dws2header and header2dws if this is changed.
type header_t is record
  -- Note the count must be first.
  count : count_t;     -- like AXI: 0 means 1 etc.
  op    : op_t;
  xid   : xid_t;
  lead  : unsigned(end_bytes_width-1 downto 0); -- similar to AXI address LSB
  trail : unsigned(end_bytes_width-1 downto 0);
  node  : id_t; -- part of address for outbound requests
  addr  : addr_t;
end record header_t;
constant sdp_header_width : natural := width_for_max(op_t'pos(op_t'high)) + xid_width + end_bytes_width*2 +
                                     count_width + node_width + addr_width;
constant sdp_header_ndws  : natural := (sdp_header_width + (dword_t'length-1)) / dword_t'length;

constant whole_addr_bits_c : integer := addr_width + node_width;
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
type s2m_t is record
  sdp   : sdp_t;
end record s2m_t;

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
--function enable2which(enable : std_logic_vector) return which_t;
--function which64(which : which_t) return bool_t;
--function which2addr(which : which_t) return unsigned;
--function which2enable32(which : which_t) return unsigned;
--function which2enable64(which : which_t) return unsigned;
--type sdp_short_t is record         -- bits cum
--  is_short : bool_t;                --  1    1
--  op       : op_t;                  --  2    3
--  xid      : unsigned(2 downto 0);  --  3    6
--  which    : which_t;               --  4   10
--  -- after this is addressing
--  node     : unsigned(2 downto 0);  --  3   13 zero is known to be control plane
--  worker   : unsigned(4 downto 0);  --  5   18 zero is the platform worker
--  qwaddr   : unsigned(13 downto 0); -- 14   32 quad word (128KB) address  
--end record sdp_short_t;
--function from_sdp_short(sdp_short : sdp_short_t) return std_logic_vector;
--function to_sdp_short(slv : dword_t) return sdp_short_t;
--function from_sdp_long(sdp_long : sdp_long_t) return std_logic_vector;
--function to_sdp_long(dw : dword_t) return sdp_long_t;
--function is_short(dw : dword_t) return boolean;
--function payload_length(dw : dword_t) return unsigned;
-- The record that is an output from a data producer
--type sdp_oob_out_t is record
--  eom     : bool_t;  -- when valid, the data beat is the last of a message
--  valid   : bool_t;  -- the data path is valid
--end record sdp_oob_out_t;
---- The record that is an input to a data producer
--type sdp_oob_in_t is record
--  ready   : bool_t;  -- dequeue from producer
--end record sdp_oob_in_t;
--constant max_num_ids : positive := 4;
--constant id_width : positive := width_for_max(max_num_ids - 1);
--subtype id_t is unsigned(id_width-1 downto 0);
--constant max_num_positions : positive := 15;
--constant position_width : positive := width_for_max(max_num_positions - 1);
--subtype position_t is unsigned(position_width-1 downto 0);
---- Signals from the sdp to a client of it.
--type sdp2client_t is record
--  clk            : std_logic;
--  reset          : std_logic;
--  id             : id_t;
--  oob_for_s2c    : sdp_oob_out_t;
--  oob_for_c2s    : sdp_oob_in_t;
--end record sdp2client_t;
--type client2sdp_t is record
--  oob_for_s2c    : sdp_oob_in_t;
--  oob_for_c2s    : sdp_oob_out_t;
--end record client2sdp_t;

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
end package sdp;

-- modules instantiated as workers must have the component definition in their
-- own package.
--library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
--library ocpi; use ocpi.all, ocpi.types.all;
--use work.sdp.all;
--package sdp_node_defs is
--component sdp_node
--  generic(ocpi_debug      :     bool_t;
--          position        :     uchar_t;
--          sdp_width       :     uchar_t);
--  port(   up_in           : in  m2s_t;
--          up_out          : out s2m_t;
--          up_in_data      : in  dword_array_t(0 to to_integer(sdp_width)-1);
--          up_out_data     : out dword_array_t(0 to to_integer(sdp_width)-1);
--          client_in       : in  s2m_t;
--          client_out      : out m2s_t;
--          client_in_data  : in  dword_array_t(0 to to_integer(sdp_width)-1);
--          client_out_data : out dword_array_t(0 to to_integer(sdp_width)-1);
--          down_in         : in  s2m_t;
--          down_out        : out m2s_t;
--          down_in_data    : in  dword_array_t(0 to to_integer(sdp_width)-1);
--          down_out_data   : out  dword_array_t(0 to to_integer(sdp_width)-1));
--end component sdp_node;
--end package sdp_node_defs;

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.sdp.all;
package sdp_node_defs is
  -- Record for the SDP input signals for port "up" of worker "sdp_node"
  alias up_in_t is work.sdp.m2s_t;
  -- Record for the SDP output signals for port "up" of worker "sdp_node"
  alias up_out_t is work.sdp.s2m_t;

  -- Record for the SDP input signals for port "client" of worker "sdp_node"
  alias client_in_t is work.sdp.s2m_t;
  -- Record for the SDP output signals for port "client" of worker "sdp_node"
  alias client_out_t is work.sdp.m2s_t;

  -- Record for the SDP input signals for port "down" of worker "sdp_node"
  alias down_in_t is work.sdp.s2m_t;
  -- Record for the SDP output signals for port "down" of worker "sdp_node"
  alias down_out_t is work.sdp.m2s_t;

alias ocpi_endian_t is ocpi.types.endian_t;
component sdp_node_rv is
  generic (
      ocpi_debug            : in   bool_t := bfalse;
      ocpi_endian           : in   ocpi_endian_t := little_e;
      sdp_width             : in   uchar_t := to_uchar(1)
  );
  port (

  -- The SDP interface named "up", with "sdp_node" acting as slave:
  up_in      : in  up_in_t;
  up_out     : out up_out_t;
  up_in_data : in  dword_array_t(0 to to_integer(sdp_width)-1);
  up_out_data : out dword_array_t(0 to to_integer(sdp_width)-1);
  -- The SDP interface named "client", with "sdp_node" acting as master:
  client_in  : in  client_in_t;
  client_out : out client_out_t;
  client_in_data : in  dword_array_t(0 to to_integer(sdp_width)-1);
  client_out_data : out dword_array_t(0 to to_integer(sdp_width)-1);
  -- The SDP interface named "down", with "sdp_node" acting as master:
  down_in    : in  down_in_t;
  down_out   : out down_out_t;
  down_in_data : in  dword_array_t(0 to to_integer(sdp_width)-1);
  down_out_data : out dword_array_t(0 to to_integer(sdp_width)-1));
end component sdp_node_rv;
end package sdp_node_defs;
