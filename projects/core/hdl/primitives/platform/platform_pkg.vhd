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

-- VHDL component declarations for platform modules
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all, ocpi.util.all;
package platform_pkg is


--------------------------------------------------------------------------------
-- Control Plane definitions
--------------------------------------------------------------------------------

-- The simple control interface.
-- The client (interconnect) can treat this as asynchronous, with a returned tag,
-- but the implementation -- is fully synchronous - one-at-a-time...
-- A synchronous client can not bother generating the tag.
subtype occp_address_t is std_logic_vector(23 downto 0); -- 64MB of control space
subtype occp_data_t    is std_logic_vector(31 downto 0);
subtype occp_tag_t     is std_logic_vector(7  downto 0);
subtype occp_byte_en_t is std_logic_vector(3  downto 0);
type occp_in_t is record
  clk     : std_logic;      -- clock for control
  reset   : bool_t;         -- reset for control
  valid   : bool_t;         -- is the request valid/present?
  is_read : bool_t;         -- is this a read request
  address : occp_address_t; -- address of read or write
  byte_en : occp_byte_en_t; -- byte enable of read or write
  data    : occp_data_t;    -- write data or read tag
  take    : bool_t;         -- take response from occp
end record occp_in_t;
type occp_out_t is record
  valid   : bool_t;         -- is the response valid/present?
  tag     : occp_tag_t;     -- read tag in response
  data    : occp_data_t;    -- read data in response
  take    : bool_t;         -- take request presented to occp
end record occp_out_t;

-- These are the number of bits of the DW address
constant worker_control_bits : natural := 12; -- 16KB control space per worker
constant worker_config_bits  : natural := 18; -- 1MB config space per worker
constant worker_control_size : natural := 2**worker_control_bits;
constant worker_config_size  : natural := 2**worker_config_bits;
constant worker_max_nworkers : natural := 63; -- for a 64 bit array of worker-present bits
constant worker_ncontrol_ops : natural := 8;
-- ID is wide enough for a sentinel value of all ones.
constant worker_id_bits      : natural := width_for_max(worker_max_nworkers);
subtype worker_timeout_t is unsigned(4 downto 0);
-- A log2 value
constant default_timeout_c   : positive := 4;
type worker_operation_t is (none_e,
                            control_op_e,
                            control_read_e,
                            control_write_e,
                            config_read_e,
                            config_write_e);

type worker_response_t is (none_e,     -- no response yet
                           ok_e,       -- success for something with no data
                           data_e,     -- success for something with data
                           error_e,    -- error
                           timedout_e, -- worker timed out
                           reset_e,    -- worker was reset
                           busy_e);    -- worker was busy

-- Internal interface to WCI master modules, driven to all workers in parallel
type worker_data_source_t is (status_e, control_e, last_addr_e, window_e, sdata_e);
type worker_in_t is record
  clk           : std_logic;
  reset         : bool_t;
  cmd           : ocp.MCmd_t;                    -- cmd per WCI
  address       : std_logic_vector(worker_config_bits-1+2 downto 0); -- Byte Addr
  source        : worker_data_source_t;
  id            : unsigned(worker_id_bits-1 downto 0);
  
  is_config     : bool_t;                        -- same as WCI MAddrSpace;
  byte_en       : std_logic_vector(3 downto 0);  -- byte enable for read or write
  data          : occp_data_t;                   -- write data
  operation     : worker_operation_t;            -- what op is in progress or starting
  timedout      : bool_t;                        -- operation has timed out
  is_big_endian : bool_t;
end record worker_in_t;

-- Internal interface from WCI master modules, each drives its own version
type worker_out_t is record
  data      : occp_data_t; -- worker's timeout value or data
  response  : worker_response_t;             -- worker responds
  attention : bool_t;                        -- to allow them to be consolidated
  present   : bool_t;
  timeout   : worker_timeout_t;
end record worker_out_t;


--------------------------------------------------------------------------------
-- Metadata definitions
--------------------------------------------------------------------------------
-- Metadata to the platform worker.  Platform worker is master
type metadata_in_t is record
  romData : ulong_t;
  UUID    : ulong_array_t(0 to 15);
end record metadata_in_t;

type metadata_out_t is record
  clk     : std_logic; -- THIS IS IGNORED AND IS ONLY HERE FOR COMPATIBILITY with platform workers
  romAddr : ushort_t;
  romEn   : bool_t;
end record metadata_out_t;

--------------------------------------------------------------------------------
-- Time service definitions
--------------------------------------------------------------------------------

-- Input from the platform worker to the time server.
type time_base_out_t is record
  clk     : std_logic;
  reset   : std_logic; -- assert hi
  ppsIn   : std_logic;
end record time_base_out_t;
type time_base_in_t is record
  ppsOut   : std_logic;
end record time_base_in_t;

-- The time_server's (and platform worker's) output that is the time service.
type time_service_t is record
  clk     : std_logic;
  reset   : std_logic;
  now     : ocpi.types.ulonglong_t;
end record time_service_t;

--------------------------------------------------------------------------------
-- uNoc definitions
--------------------------------------------------------------------------------

-- This is our "legacy" format, roughly aligned with Xilinx BLock-plus endpoint
-- interface (the headers).
--TLP request format: (bytes) usually 16
--  First the fields of uNoc packets, where any TLP headers are in the "data".
--  sof:           1
--  eof:           1
--  hit:           7 (bar) - routing code somehow
--  be:           16 - somewhat redundent with the TLP headers :first BE and last BE
--  data: 16*8 = 128
-- 3DW Header format of data:
--  pack-to-64(MemReqHdr1),address,rres.data)
-- So a single word write is 2DW header, 1DW address, and 1DW data
-- NOTE: the header is defined hi-to-low (MtoLSB), but the bytes on the wire
-- are placed high-to-low in the header. (a sort of big-endian in the spec)
--  63: 0
--  62:61: format: 0: 3DWnoData, 1: 4DWnoData, 2: 3DWwData, 3: 4DWwData
    -- 62: is WRITE
    -- 61: is 4 DW
--  60:56: type: 0: memory read/write, b'1010: completion
--  55: 0
--  54:52: traffic class 0 - 7: we use 0
--  51:48: 0000
--  47: has digest: false
--  46: poisoned: false
--  45: attrOrdering: false - could be considered for performance
--  44: attrNoSnoop: false - could be considered for performance
--  43:42: 00
--  41:32: length in DWORDS 9:0, value of zero means 1024DW (4KB)
--  31:16: id
--  15:8:  tag: 0 for writes
--  7:4:   last be
--  3:0:   first be
-- Completion header is different, basically it has the same
-- High 64 bits, and the 3rd DW is the transaction description (like address)
--  95: 0
--  94: has data
--  93: 0
--  92:88: type 5'b01010
--  87: 0
--  86:84: traffic class,
--  83:80: 0
--  79: hasDigest
--  78: isPoisoned
--  77: attrOrdering
--  76: attrNoSnoop
--  75:74: 0
--  73:64: length
--  63:48: completerID - the ID of the sender just like requests
--  47:45: status: 0 is success
--  44:    bytecount modified: always 0 for PCI express
--  43:32: bytecount remaining - 0 means 4096, INCLUDES BYTES IN THIS PACKET

----- From here is the 3rd DW
--  31:16: requesterID (bus8, device5, function3)
--  15:8:  tag - by default only 5 LSBs should be non-zero
--  7      0 reserved
--  6:0    lowerAddress BYTE address

-- This is pretty much out of the of the PCI spec,
-- but, like the spec, "left" is "first on the wire", bytewise
constant unoc_dw_length_bits_c : natural := 10;
type unoc_header_t is record -- DWORD 0, 32 bits
  mbz0       : std_logic;
  has_data   : std_logic;
  is_4dw     : std_logic;
  pkt_type   : std_logic_vector(4 downto 0);  -- 0 is mem rd/wr, 10 is completion
  mbz1       : std_logic;                     -- 0
  class      : std_logic_vector(2 downto 0);  -- we always use 0
  mbz2       : std_logic_vector(3 downto 0);  -- 0000
  has_digest : std_logic;                     -- we always use 0
  poisoned   : std_logic;
  ordering   : std_logic;
  no_snoop   : std_logic;
  mbz3       : std_logic_vector(1 downto 0);  -- 00
  dw_length  : std_logic_vector(unoc_dw_length_bits_c-1 downto 0);  -- but zero means 1024 DWs
end record unoc_header_t;
-- For values that don't use the funky zero-means-1024 convention
subtype unoc_ndw_t is unsigned(unoc_dw_length_bits_c downto 0);
function to_slv(d : unoc_header_t) return std_logic_vector;
function to_unoc_header(v : std_logic_vector) return unoc_header_t;
function from_unoc_header(h : unoc_header_t) return std_logic_vector;
function unoc_make_header(is_write      : bool_t;
                          is_completion : bool_t;
                          is_4dw        : bool_t;
                          dw_length     : unsigned(unoc_dw_length_bits_c downto 0))
  return unoc_header_t;
function unoc_get_ndw(h : unoc_header_t) return unoc_ndw_t;
function unoc_make_response(dw_length   : unsigned(unoc_dw_length_bits_c downto 0))
  return unoc_header_t;

-- This is the second dword of a unoc sof request
type unoc_request_t is record -- DWORD 1 for requests
  sender_id  : std_logic_vector(15 downto 0); -- usually bus:8, device:5, func:3  
  tag        : std_logic_vector(7 downto 0);  -- sender's tag
  last_be    : std_logic_vector(3 downto 0);  -- bit 0 is first byte of DW
  first_be   : std_logic_vector(3 downto 0);  -- bit 0 is first byte of DW
end record unoc_request_t;
function to_unoc_request(v : std_logic_vector) return unoc_request_t;

-- This is the second word of a unoc sof completion
type unoc_completion1_t is record -- DWORD 1 for completions
  completer_id : std_logic_vector(15 downto 0);
  status       : std_logic_vector(2 downto 0);
  bcm          : std_logic; -- always zero for us
  byte_count   : std_logic_vector(11 downto 0);
end record unoc_completion1_t;
function from_unoc_completion1(h : unoc_completion1_t) return std_logic_vector;
function unoc_make_completion1(dw_length : unsigned(unoc_dw_length_bits_c downto 0))
  return unoc_completion1_t;

-- This is the third dword of a unoc sof completion
constant unoc_low_addr_bits_c : natural := 7;
type unoc_completion2_t is record -- DWORD 2 for completion
  request_id : std_logic_vector(15 downto 0);
  tag        : std_logic_vector(7 downto 0);
  mbz0       : std_logic;
  low_addr   : std_logic_vector(unoc_low_addr_bits_c-1 downto 0);
end record unoc_completion2_t;
function from_unoc_completion2(h : unoc_completion2_t) return std_logic_vector;
function unoc_make_completion2(requester_id : std_logic_vector(15 downto 0);
                               tag          : std_logic_vector(7 downto 0);
                               addr         : std_logic_vector(31 downto 0))
  return unoc_completion2_t;

-- Constants for now, parameterized later
constant unoc_ndw_c        : natural := 4;
constant unoc_id_width_c   : natural := 16;
constant unoc_hit_width_c  : natural := 7;
constant unoc_addr_width_c : natural := 32;
-- The data type of the payload of each beat on the unoc
subtype unoc_payload_t is dword_array_t(0 to unoc_ndw_c-1);
-- This record carries DMA master read or write requests to the ic and also
-- responses to inbound (via ic2unoc) read requests
type unoc_data_t is record
  sof     : bool_t;
  eof     : bool_t;
  hit     : std_logic_vector(unoc_hit_width_c-1 downto 0); -- PCIish (1 << bar) value
  be      : std_logic_vector(unoc_ndw_c*4-1 downto 0);
  payload : unoc_payload_t; -- on sof, headers are first 2 or 3 words
end record unoc_data_t;
-- can't use 'left or any sort of sizeof() on records?
constant unoc_data_width : natural := 2 + unoc_hit_width_c + unoc_ndw_c*4 + unoc_ndw_c*32;
function to_slv(d : unoc_data_t) return std_logic_vector;
function to_unoc(v : std_logic_vector) return unoc_data_t;
-- The link in either direction among nodes and clients
type unoc_master_in_t is record
  data  : unoc_data_t; -- data from the uNoc to the interconnect
  valid : bool_t;      -- this data is valid, can be dequeued
  take  : bool_t;      -- take data in other direction's record: perform dequeue
  dropCount : uchar_t;
end record unoc_master_in_t;

-- The master/source interface of the uNoc
-- This is the interface to the device worker or platform worker
-- that sprouts the uNoc, including its clocking
-- 
type unoc_master_out_t is record
  clk     : std_logic;
  reset_n : std_logic;
  id      : std_logic_vector(unoc_id_width_c-1 downto 0); -- our interconnect id
  data    : unoc_data_t;                -- unoc data from the interconnect into the unoc
  valid   : bool_t;                     -- this data is valid, can be dequeued
  take    : bool_t;                     -- take data from the _in_t: perform dequeue
end record unoc_master_out_t;


-- component unoc_terminator is
--   port(
--     up_in      : in  unoc_master_out_t;
--     up_out     : out unoc_master_in_t;
--     drop_count : out unsigned(7 downto 0)
--     );
-- end component unoc_terminator;

component unoc_cp_adapter is
  port(
    client_in  : in  unoc_master_out_t;
    client_out : out unoc_master_in_t;
    cp_in      : in  occp_out_t;
    cp_out     : out occp_in_t
    );
end component unoc_cp_adapter;

-- Component to drive the OCCP in a simulator
component sim_clk is
  generic(frequency : real := 100000000.0;
          offset    : natural := 0);
  port   (clk   : out std_logic;
          reset : out bool_t);
end component sim_clk;

component sim_dcp is
  port(
    clk    : in  std_logic;
    reset  : in  bool_t;
    cp_in  : in  occp_out_t;
    cp_out : out occp_in_t);
end component sim_dcp;

component wci_master is
  generic(
    ocpi_debug   : bool_t;
    id_width, id : natural);
  port(
    -- worker-facing side - a WCI
    wci_in     : in  wci.wci_s2m_t;
    wci_out    : out wci.wci_m2s_t;
    worker_in  : in  worker_in_t;
    worker_out : out worker_out_t);
end component wci_master;

end package platform_pkg;

-- modules instantiated as workers must have the component definition in their
-- own package.

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package ocscp_defs is
component ocscp_rv is
  generic(
    ocpi_debug  : bool_t := bfalse;
    ocpi_endian : endian_t := little_e;
    nWorkers    : ulong_t);
  port(
    wci_in  : in  wci.wci_s2m_array_t(0 to to_integer(nWorkers)-1);
    wci_out : out wci.wci_m2s_array_t(0 to to_integer(nWorkers)-1);
    cp_in   : in  occp_in_t;
    cp_out  : out occp_out_t);
end component ocscp_rv;
end package ocscp_defs;


library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package time_client_defs is
type wti_in_t is record
  Clk         : std_logic;
  -- These signals will only be needed someday if the "request" feature is implemented
  SThreadBusy : std_logic_vector(0 downto 0);
  SReset_n    : std_logic;
end record wti_in_t;
type wti_out_t is record
  MCmd        : ocpi.ocp.MCmd_t;
  MData       : std_logic_vector(63 downto 0);
end record wti_out_t;

component time_client_rv is
  port(
    wci_Clk     : in std_logic;
    wci_Reset_n : in std_logic;
    time_in     : in  time_service_t;
    wti_in      : in  wti_in_t;
    wti_out     : out wti_out_t
    );
end component time_client_rv;
end package time_client_defs;

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package time_client_co_defs is
type wti_in_t is record
  -- These signals will only be needed someday if the "request" feature is implemented
  SThreadBusy : std_logic_vector(0 downto 0);
  SReset_n    : std_logic;
end record wti_in_t;
type wti_out_t is record
  Clk         : std_logic;
  MCmd        : ocpi.ocp.MCmd_t;
  MData       : std_logic_vector(63 downto 0);
end record wti_out_t;

component time_client_co_rv is
  port(
    wci_Clk     : in std_logic;
    wci_Reset_n : in std_logic;
    time_in     : in  time_service_t;
    wti_in      : in  wti_in_t;
    wti_out     : out wti_out_t
    );
end component time_client_co_rv;
end package time_client_co_defs;


library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package metadata_defs is
component metadata_rv is
  generic(romwords : natural := 2048);
  port(
    wci_Clk      : in std_logic;
    wci_Reset_n  : in std_logic;
    metadata_in  : in  metadata_out_t;
    metadata_out : out  metadata_in_t
    );
end component metadata_rv;
end package metadata_defs;


library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package unoc_node_defs is

-- a node in the unoc
component unoc_node_rv is
  generic(
    control          : bool_t;
    position         : ulong_t := to_ulong(0)
    );
  port (
    up_in      : in  unoc_master_out_t;
    up_out     : out unoc_master_in_t;
    client_in  : in  unoc_master_in_t;
    client_out : out unoc_master_out_t;
    down_in    : in  unoc_master_in_t;
    down_out   : out unoc_master_out_t
    );
end component unoc_node_rv;
end package unoc_node_defs;

package pci_pkg is
  constant pci_any_id_c         : natural := 16#ffff#;      -- wild card
  constant pci_vendor_id_c      : natural := 16#1df7#;      -- opencpi.org
  constant pci_device_id_c      : natural := pci_any_id_c;  -- device is our platform
  constant pci_class_c          : natural := 16#b#;         -- processor
  constant pci_subclass_c       : natural := 16#40#;        -- coprocessor
  constant pci_class_code_c     : natural := pci_class_c * 2 ** 16 + pci_subclass_c * 2 ** 8;
  constant pci_device_unknown_c : natural := 0;
  -- All other defined PCI device platforms are registered in the platform worker spec xml
end package pci_pkg;
