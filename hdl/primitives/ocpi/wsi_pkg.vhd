-- This package defines constants relating to the WSI profile
-- These records have all possible signals.

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; use ocpi.ocp;
package wsi is

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
             early_request    : boolean  -- are datavalid and datalast used? 
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
      som, eom, valid  : in  Bool_t);
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
end package wsi;
