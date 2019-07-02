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

-- This package defines constants relating to the WCI interface
library ieee, ocpi;
use ieee.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all, ocpi.ocp;
package wci is
type raw_in_t is record
  address     : ulong_t;
  byte_enable : std_logic_vector(3 downto 0);
  is_read     : bool_t;
  is_write    : bool_t;
  data        : word32_t;
end record raw_in_t;
constant raw_in_zero : raw_in_t
  := ((others => '0'), (others => '0'), bfalse, bfalse, (others => '0'));
type raw_out_t is record
  done        : bool_t;
  error       : bool_t;
  data        : word32_t;
end record raw_out_t;
constant raw_out_zero : raw_out_t := (bfalse, bfalse, (others => '0'));
-- The raw property interface for shared I2C and SPIs from the perspective of the
-- device worker.
constant raw_max_devices : natural := 6;

-- Output from device worker as master of the rawprop interface
type raw_prop_out_t is record
  present : bool_t;                       -- master is present - slave ties low
  reset   : bool_t;                       -- master worker is in reset
  raw     : raw_in_t;
end record raw_prop_out_t;
constant raw_prop_out_zero : raw_prop_out_t
  := ('0', '0', wci.raw_in_zero);
type raw_prop_out_array_t is array(natural range <>) of raw_prop_out_t;
-- Input to device worker as master of the rawprop interface
-- These signals are "broadcast" back to all masters from the one slave
type raw_prop_in_t is record
  present : bool_array_t(0 to raw_max_devices-1); -- which of all devices are present
  raw     : raw_out_t;
end record raw_prop_in_t;
constant raw_prop_in_zero : raw_prop_in_t
  := ((others => '0'), raw_out_zero);

type raw_prop_in_array_t is array(natural range <>) of raw_prop_in_t;

type control_op_t IS (INITIALIZE_e,
                      START_e,
                      STOP_e,
                      RELEASE_e,
                      BEFORE_QUERY_e,
                      AFTER_CONFIG_e,
                      TEST_e,
                      NO_OP_e);
subtype control_op_mask_t is std_logic_vector(control_op_t'pos(no_op_e) downto 0);
function get_op_pos(input: control_op_t) return natural;

type worker_t is record
  decode_width      : natural;
  raw_property_base : natural;
  allowed_ops       : control_op_mask_t;
end record worker_t;
type property_t is record
  data_width : natural; -- data width of datum in bits, but 32 for strings
  offset : unsigned(31 downto 0);    -- offset in property space in bytes
  bytes_1,              -- total bytes in this property minus 1, excluding seq header
  string_length,        -- bytes (excluding null) for string values
  seq_hdr,              -- bytes in sequence header
  nitems                -- nitems array
    : natural;          -- with of a single item
  writable, readable, volatile, debug : boolean;
end record property_t;

  -- Address Space Selection
  CONSTANT CONFIG  : std_logic := '1';
  CONSTANT CONTROL : std_logic := '0';

  -- Worker State
  -- These are not normative to the WCI interface, but are useful for bookkeepping
  -- Note we track the state where we have accepted a control operation but
  -- have not yet responded to it.  See OcpiWorker.h
TYPE State_t IS (EXISTS_e,            -- 0
                 INITIALIZED_e,       -- 1
                 OPERATING_e,         -- 2
                 SUSPENDED_e,         -- 3
                 FINISHED_e,          -- 4
                 UNUSABLE_e);         -- 5
function get_state_pos(input: state_t) return natural;

  type control_op_masks_t is array (natural range 0 to state_t'pos(unusable_e)) of control_op_mask_t;

  -- constant masks for what control op is allowed in each state
  constant next_ops : control_op_masks_t :=
    ("00000001",       -- EXISTS_e
     "01001010",       -- INITIALIZED_e
     "00111100",       -- OPERATING_e
     "00111010",       -- SUSPENDED_e
     "00111110",       -- FINISHED_e
     "00000000"        -- UNUSABLE_e: nothing to do but reset
     );

  type in_t is record
    Clk                 : std_logic;
    MAddr               : std_logic_vector(31 downto 0);
    MAddrSpace          : std_logic_vector(0 downto 0);
    MByteEn             : std_logic_vector(3 downto 0);
    MCmd                : ocp.MCmd_t;
    MData               : std_logic_vector(31 downto 0);
    MFlag               : std_logic_vector(18 downto 0);
    MReset_n            : std_logic;
  end record in_t;

  type out_t is record
    SData               : std_logic_vector(31 downto 0);
    SFlag               : std_logic_vector(2 downto 0);
    SResp               : ocp.SResp_t;
    SThreadBusy         : std_logic_vector(0 downto 0);
  end record out_t;

  -- This is the type of access to the property, or none
  type Access_t IS (Init_e, None_e, Error_e, Read_e, Write_e, Control_e);

  -- Return the currently decoded access
  function decode_access(input : in_t) return Access_t;

  -- Return the byte offset from the byte enables
  --subtype byte_offset_t is unsigned(1 downto 0);
--  function be2offset(input: in_t) return byte_offset_t;

  -- pull the value from the data bus, shifted and sized
--  function get_value(input : in_t; boffset : unsigned; width : natural) return std_logic_vector;

  function to_control_op(bits : std_logic_vector(2 downto 0)) return control_op_t;

  function resize(bits : std_logic_vector; n : natural) return std_logic_vector;


  subtype hword_t is std_logic_vector(15 downto 0);
  subtype byte_t is std_logic_vector(7 downto 0);
  type properties_t is array (natural range <>) of property_t;
  type data_a_t is array (natural range <>) of dword_t;
  type offset_a_t is array (natural range <>) of unsigned(31 downto 0);
  type boolean_array_t is array (natural range <>) of boolean;
  --function data_top (property : property_t;
  --                   byte_offset : byte_offset_t) -- xst v5 can't do this := to_unsigned(0,byte_offset_t'length))
  --                   return bit_offset_t;

-- These records are generic - i.e. a superset of what any given worker might have

type wci_m2s_t is record
  Clk        : std_logic;
  MReset_n   : std_logic;
  MCmd       : std_logic_vector( 3-1 downto 0);
  MAddr      : std_logic_vector(32-1 downto 0);
  MAddrSpace : std_logic_vector( 1-1 downto 0);
  MByteEn    : std_logic_vector( 4-1 downto 0);
  MData      : std_logic_vector(32-1 downto 0);
  MFlag      : std_logic_vector(19-1 downto 0);
end record wci_m2s_t;
type wci_m2s_array_t is array(natural range <>) of wci_m2s_t;
type wci_s2m_t is record
  SData       : std_logic_vector(32-1 downto 0);
  SResp       : std_logic_vector( 2-1 downto 0);
  SFlag       : std_logic_vector( 3-1 downto 0);
  SThreadBusy : std_logic_vector( 0 downto 0);
end record wci_s2m_t;
type wci_s2m_array_t is array(natural range <>) of wci_s2m_t;

  -- the wci convenience IP that makes it simple to implement a WCI interface
  component decoder
    generic(
      worker                 : worker_t;
      ocpi_debug             : bool_t;
      endian                 : endian_t;
      properties             : properties_t);
    port(
      ocp_in                 : in  in_t;
      done                   : in  bool_t := btrue;
      error                  : in  bool_t := btrue;
      finished               : in  bool_t := bfalse;
      resp                   : out ocp.SResp_t;
      busy                   : out bool_t;
      control_op             : out control_op_t;
      state                  : out state_t;
      is_operating           : out bool_t;  -- just a convenience for state = operating_e
      abort_control_op       : out bool_t;
      is_big_endian          : out bool_t;   -- for runtime dynamic endian
      raw_in                 : in  raw_out_t;
      raw_out                : out raw_in_t;
      barrier                : out bool_t;
      crew                   : out UChar_t;
      rank                   : out UChar_t;
      -- From here down, only for non-raw properties
      write_enables          : out bool_array_t(properties'range);
      read_enables           : out bool_array_t(properties'range);
      offsets                : out offset_a_t(properties'range);
--      indices                : out offset_a_t(properties'range);
      hi32                   : out bool_t;
      nbytes_1               : out byte_offset_t;
      data_outputs           : out data_a_t(properties'range);
      read_index             : out unsigned(width_for_max(properties'right)-1 downto 0)
    );
  end component;

  -- the wci convenience module used when there are no non-raw properties
  component no_props_decoder is
    generic (
      worker                 : worker_t;
      ocpi_debug             : bool_t;
      endian                 : endian_t);
    port (
      ocp_in                 : in  in_t;
      done                   : in  bool_t := btrue;
      error                  : in  bool_t := bfalse;
      finished               : in  bool_t := bfalse;
      resp                   : out ocp.SResp_t;
      busy                   : out bool_t;
      control_op             : out control_op_t;
      state                  : out state_t;
      is_operating           : out bool_t;  -- just a convenience for state = operating_e
      abort_control_op       : out bool_t;
      is_big_endian          : out bool_t;  -- for runtime dynamic endian
      raw_in                 : in  raw_out_t;
      raw_out                : out raw_in_t;
      barrier                : out bool_t;
      crew                   : out UChar_t;
      rank                   : out UChar_t
      );
  end component;

  component readback
    generic(properties : properties_t; ocpi_debug : bool_t);
    port(
      read_index  : in unsigned(width_for_max(properties'right)-1 downto 0);
      data_inputs  : in data_a_t(properties'range);
      data_output  : out std_logic_vector(31 downto 0)
      );
  end component readback;
  component property_decoder
  generic (
        property      : property_t;   -- property type
        decode_width  : natural;     -- decoder width in bits
        endian        : endian_t);
  port (
        reset         : in  bool_t;                            -- active-low WCI worker reset
        offset_in     : in  unsigned(decode_width-1 downto 0); -- offset in Bytes
        nbytes_1      : in  byte_offset_t;                     -- how many valid bytes
        is_read       : in  bool_t;
        is_write      : in  bool_t;
        data_in       : in  std_logic_vector(31 downto 0);     -- WCI master data
        is_big_endian : in bool_t;
        write_enable  : out bool_t;                            -- active-high write pulse
        read_enable   : out bool_t;                            -- active-high read pulse
        offset_out    : out unsigned(width_for_max(property.bytes_1+8)-1 downto 0); --
--        index_out     : out unsigned(decode_width-1 downto 0); --
        data_out      : out std_logic_vector(31 downto 0)); --
  end component property_decoder;
  component raw_arb is
    generic (nusers      : positive := 1);
    port    (clk         : in  std_logic;
             reset       : in  bool_t;
             from_users  : in  wci.raw_prop_out_array_t(0 to nusers-1);
             to_users    : out wci.raw_prop_in_array_t(0 to nusers-1);
             from_device : in  wci.raw_prop_in_t;
             to_device   : out wci.raw_prop_out_t;
             index       : out integer range 0 to nusers-1);
  end component raw_arb;
end package wci;
