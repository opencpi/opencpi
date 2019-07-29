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

-- This package defines constants relating to the WSI profile
-- These records have all possible signals.

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.ocp, ocpi.util;
package wsi is

  -- Generic slave to slave - no variability at all
  type s2m_t is record
    SReset_n            : std_logic;
    SThreadBusy         : std_logic_vector(0 downto 0);
 end record s2m_t;
  -- Generic master, WITHOUT DATA.  Accepts imprecise and precise.
  -- Maximum message burst size is enough for 16Kb with a data width of 8
  --
  type m2s_t is record
    MBurstLength        : std_logic_vector(14 downto 0);
    MCmd                : ocpi.ocp.MCmd_t;
    MReqInfo            : std_logic_vector(7 downto 0);
    MReqLast            : std_logic;
    MReset_n            : std_logic;
  end record m2s_t;

  function MData_width(dataWidth : natural; byteWidth : natural) return natural;
  function MDataInfo_width(dataWidth : natural; byteWidth : natural) return natural;
  function MByteEn_width(dataWidth : natural; byteWidth : natural) return natural;

  component slave
    generic (precise          : boolean; -- are we precise-only?
             mdata_width      : natural; -- width of data path
             mdata_info_width : natural; -- width of data path
             burst_width      : natural; -- burst width
             n_bytes          : natural; -- number of bytes
             byte_width       : natural; -- byte_width
             opcode_width     : natural; -- bits in reqinfo
             own_clock        : boolean; -- does the port have a clock different thanthe wci?
             hdl_version      : natural; -- hdl interface version
             early_request    : boolean  -- are datavalid and datalast used? 
             );
    port (
      -- Exterior OCP input/master signals
      --- this is the same as wci_clock unless metadata says it isn't
      Clk              : in  std_logic;
      --- only used if burst are precise
      MBurstLength     : in  std_logic_vector(burst_width - 1 downto 0);
      --- only used if bytesize < data width or zlm 
      MByteEn          : in  std_logic_vector(n_bytes - 1 downto 0);
      MCmd             : in  ocpi.ocp.MCmd_t;
      MData            : in  std_logic_vector(mdata_width-1 downto 0);
      --- only used for aborts or bytesize not 8 and less than datawidth
      MDataInfo        : in  std_logic_vector(mdata_info_width-1 downto 0);
      MDataLast        : in  std_logic := '0';
      MDataValid       : in  std_logic := '0';
      --- only used if number of opcodes > 1
      MReqInfo         : in  std_logic_vector(opcode_width-1 downto 0);
      MReqLast         : in  std_logic;
      MReset_n         : in  std_logic;
      -- Exterior OCP output/slave signals
      SReset_n         : out std_logic;
      SThreadBusy      : out std_logic_vector(0 downto 0);
      -- Signals connected from the worker's WCI to this module;
      wci_clk          : in  std_logic;
      wci_reset        : in  Bool_t;
      wci_is_operating : in  Bool_t;
      -- Non-worker internal signals
      first_take       : out Bool_t; -- the first datum after is_operating taken, a pulse
      -- Interior signals used by worker logic
      take             : in  Bool_t; -- the worker is taking data
      reset            : out Bool_t; -- this port is being reset from outside/peer
      ready            : out Bool_t; -- data can be taken
      -- user visible metadata
      som, eom, valid, eof, abort : out Bool_t;
      data             : out std_logic_vector(ocpi.util.max(1,byte_width)*n_bytes-1 downto 0);
      -- only used if bytes are required (zlm or byte size < data width)
      byte_enable      : buffer std_logic_vector(n_bytes-1 downto 0);
      -- only used if precise is required
      burst_length     : out std_logic_vector(burst_width-1 downto 0);
      -- only used if number of opcodes > 1
      opcode           : out std_logic_vector(opcode_width-1 downto 0)
      );
  end component slave;

  component master
    generic (precise          : boolean; -- are we precise-only?
             mdata_width      : natural; -- width of data path
             mdata_info_width : natural; -- width of data path
             burst_width      : natural; -- width of burst width
             n_bytes          : natural; -- number of bytes
             byte_width       : natural; -- byte_width
             opcode_width     : natural; -- bits in reqinfo
             own_clock        : boolean; -- does the port have a clock different thanthe wci?
             hdl_version      : natural; -- hdl interface version
             early_request    : boolean; -- are datavalid and datalast used? 
             insert_eom       : boolean; -- create output messages
             max_bytes        : natural; -- max possible supported message size in bytes
             max_latency      : natural := 256; -- max supported latency value
             worker_eof       : boolean; -- worker will push an EOF when appropriate
             fixed_buffer_size: boolean;
             debug            : boolean
             );
    port (
      -- Exterior OCP input/slave signals
      Clk              : in  std_logic; -- MIGHT BE THE SAME AS wci_clk
      SReset_n         : in  std_logic;
      SThreadBusy      : in  std_logic_vector(0 downto 0);
      -- Exterior OCP output/master signals
      MBurstLength     : out std_logic_vector(burst_width - 1 downto 0);
      MByteEn          : out std_logic_vector(n_bytes - 1 downto 0);
      MCmd             : out ocpi.ocp.MCmd_t;
      MData            : out std_logic_vector(mdata_width-1 downto 0);
      MDataInfo        : out std_logic_vector(mdata_info_width-1 downto 0);
      MDataLast        : out std_logic;
      MDataValid       : out std_logic;
      MReqInfo         : out std_logic_vector(opcode_width-1 downto 0);
      MReqLast         : out std_logic;
      MReset_n         : out std_logic;
      -- Signals connected from the worker's WCI to this interface;
      wci_clk          : in  std_logic;
      wci_reset        : in  Bool_t;
      wci_is_operating : in  Bool_t;
      -- Non-worker internal signals
      first_take       : in  Bool_t; -- the first datum after is_operating taken, a pulse
      input_eof        : in  Bool_t;   -- an EOF is pending from (the first) input port
      buffer_size      : in  UShort_t;
      latency          : out UShort_t;
    --====== Signals to and from the worker=====
      -- only used if abortable
      abort            : in  Bool_t; -- message is aborted
      -- only used if precise is required
      burst_length     : in  std_logic_vector(burst_width-1 downto 0);
      -- only used if number of opcodes > 1
      opcode           : in  std_logic_vector(opcode_width-1 downto 0);
      eof              : in  Bool_t; -- an EOF is pending from worker for this port
      give             : in  Bool_t := bfalse;
      data             : in  std_logic_vector(n_bytes * ocpi.util.max(1,byte_width)-1 downto 0);
      byte_enable      : in  std_logic_vector(n_bytes-1 downto 0) := (others => '1');
      som              : in  Bool_t := bfalse;
      eom              : in  Bool_t := bfalse;
      valid            : in  Bool_t := bfalse;
      reset            : out Bool_t;  -- this port is being reset from outside/peer
      ready            : out Bool_t); -- data can be given
  end component master;

  component part_slave
    generic (precise            : boolean; -- are we precise-only?
             mdata_width        : natural; -- width of data path
             mdata_info_width   : natural; -- width of data path
             burst_width        : natural; -- burst width
             n_bytes            : natural; -- number of bytes
             byte_width         : natural; -- byte_width
             opcode_width       : natural; -- bits in reqinfo
             own_clock          : boolean; -- does the port have a clock different thanthe wci?
             early_request      : boolean;  -- are datavalid and datalast used? 
             part_size_width    : natural := 16; -- width of part size path
             part_offset_width  : natural := 16 -- width of part offset path
             );
    port (
      -- Exterior OCP input/master signals
      --- this is the same as wci_clock unless metadata says it isn't
      Clk              : in  std_logic;
      --- only used if burst are precise
      MBurstLength     : in  std_logic_vector(burst_width - 1 downto 0);
      --- only used if bytesize < data width or zlm 
      MByteEn          : in  std_logic_vector(n_bytes - 1 downto 0);
      MCmd             : in  ocpi.ocp.MCmd_t;
      MData            : in  std_logic_vector(mdata_width-1 downto 0);
      --- only used for aborts or bytesize not 8 and less than datawidth
      MDataInfo        : in  std_logic_vector(mdata_info_width-1 downto 0);
      MDataLast        : in  std_logic := '0';
      MDataValid       : in  std_logic := '0';
      --- only used if number of opcodes > 1
      MReqInfo         : in  std_logic_vector(opcode_width-1 downto 0);
      MReqLast         : in  std_logic;
      MReset_n         : in  std_logic;
      -- Exterior OCP output/slave signals
      SReset_n         : out std_logic;
      SThreadBusy      : out std_logic_vector(0 downto 0);
      -- Signals connected from the worker's WCI to this module;
      wci_clk          : in  std_logic;
      wci_reset        : in  Bool_t;
      wci_is_operating : in  Bool_t;
      -- Interior signals used by worker logic
      take             : in  Bool_t; -- the worker is taking data
      reset            : out Bool_t; -- this port is being reset from outside/peer
      ready            : out Bool_t; -- data can be taken
      som, eom, valid  : out Bool_t;
      data             : out std_logic_vector(byte_width*n_bytes-1 downto 0);
      -- only used if abortable
      abort            : out Bool_t; -- message is aborted
      -- only used if bytes are required (zlm or byte size < data width)
      byte_enable      : out std_logic_vector(n_bytes-1 downto 0);
      -- only used if precise is required
      burst_length     : out std_logic_vector(burst_width-1 downto 0);
      -- only used if number of opcodes > 1
      opcode           : out std_logic_vector(opcode_width-1 downto 0);
      -- Partition signals
      part_size        : out unsigned(part_size_width-1 downto 0);
      part_offset      : out unsigned(part_offset_width-1 downto 0);
      part_ready       : out Bool_t;
      part_start       : out Bool_t;
      part_take        : in Bool_t
      );
  end component part_slave;

  component part_master
    generic (precise            : boolean; -- are we precise-only?
             mdata_width        : natural; -- width of data path
             mdata_info_width   : natural; -- width of data path
             burst_width        : natural; -- width of burst width
             n_bytes            : natural; -- number of bytes
             byte_width         : natural; -- byte_width
             opcode_width       : natural; -- bits in reqinfo
             own_clock          : boolean; -- does the port have a clock different thanthe wci?
             early_request      : boolean;  -- are datavalid and datalast used? 
             part_size_width    : natural := 16; -- width of part size path
             part_offset_width  : natural := 16 -- width of part offset path
             );
    port (
      -- Exterior OCP input/slave signals
      Clk              : in  std_logic; -- MIGHT BE THE SAME AS wci_clk
      SReset_n         : in  std_logic;
      SThreadBusy      : in  std_logic_vector(0 downto 0);
      -- Exterior OCP output/master signals
      MBurstLength     : out std_logic_vector(burst_width - 1 downto 0);
      MByteEn          : out std_logic_vector(n_bytes - 1 downto 0);
      MCmd             : out ocpi.ocp.MCmd_t;
      MData            : out std_logic_vector(mdata_width-1 downto 0);
      MDataInfo        : out std_logic_vector(mdata_info_width-1 downto 0);
      MDataLast        : out std_logic;
      MDataValid       : out std_logic;
      MReqInfo         : out std_logic_vector(opcode_width-1 downto 0);
      MReqLast         : out std_logic;
      MReset_n         : out std_logic;
      -- Signals connected from the worker's WCI to this interface;
      wci_clk          : in  std_logic;
      wci_reset        : in  Bool_t;
      wci_is_operating : in  Bool_t;
      -- Interior signals used by worker logic
      reset            : out Bool_t; -- this port is being reset from outside/peer
      ready            : out Bool_t; -- data can be given
      -- only used if abortable
      abort            : in  Bool_t; -- message is aborted
      -- only used if precise is required
      burst_length     : in  std_logic_vector(burst_width-1 downto 0);
      -- only used if number of opcodes > 1
      opcode           : in  std_logic_vector(opcode_width-1 downto 0);
      give             : in  Bool_t;
      data             : in  std_logic_vector(byte_width*n_bytes-1 downto 0);
      byte_enable      : in  std_logic_vector(n_bytes-1 downto 0) := (n_bytes-1 downto 0 => '1');
      som, eom, valid  : in  Bool_t;
      -- Partition signals
      part_size        : in unsigned(part_size_width-1 downto 0);
      part_offset      : in unsigned(part_offset_width-1 downto 0);
      part_ready       : out Bool_t;
      part_start       : in Bool_t;
      part_give        : in Bool_t
      );
  end component part_master;
  
  component delayline is
    generic (
      g_latency     : integer);
    port (
      i_clk         : in std_logic;
      i_reset       : in std_logic;
      i_enable      : in std_logic;
      i_ready       : in std_logic;
      i_som         : in std_logic;
      i_eom         : in std_logic;
      i_opcode      : in std_logic_vector;
      i_valid       : in std_logic;
      i_byte_enable : in std_logic_vector;
      i_data        : in std_logic_vector;
      i_eof         : in std_logic;
      o_ready       : out std_logic;
      o_som         : out std_logic;
      o_eom         : out std_logic;
      o_opcode      : out std_logic_vector;
      o_valid       : out std_logic;
      o_byte_enable : out std_logic_vector;
      o_data        : out std_logic_vector;
      o_eof         : out std_logic
      );
  end component;
end package wsi;
