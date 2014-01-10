-- VHDL component declarations for platform modules
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
package platform_pkg is


--------------------------------------------------------------------------------
-- Control Plane definitions
--------------------------------------------------------------------------------

-- The simple control interface.
-- The client (interconnect) can treat this as asynchronous, with a returned tag,
-- but the implementation -- is fully synchronous - one-at-a-time...
-- A synchronous client can not bother generating the tag.
subtype occp_address_t is std_logic_vector(21 downto 0);
subtype occp_data_t    is std_logic_vector(31 downto 0);
subtype occp_tag_t     is std_logic_vector(7  downto 0);
subtype occp_byte_en_t is std_logic_vector(3  downto 0);
type occp_in_t is record
  clk     : std_logic;      -- clock for control
  reset   : std_logic;      -- reset for control
  valid   : std_logic;      -- is the request valid/present?
  is_read : std_logic;      -- is this a read request
  address : occp_address_t; -- address of read or write
  byte_en : occp_byte_en_t; -- byte enable of read or write
  data    : occp_data_t;    -- write data or read tag
  take    : std_logic;      -- take response from occp
end record occp_in_t;
type occp_out_t is record
  valid   : std_logic;      -- is the response valid/present?
  tag     : occp_tag_t;     -- read tag in response
  data    : occp_data_t;    -- read data in response
  take    : std_logic;      -- take request presented to occp
end record occp_out_t;

constant worker_control_bits : natural := 8;
constant worker_config_bits  : natural := 16;
constant worker_control_size : natural := 2**worker_control_bits;
constant worker_config_size  : natural := 2**worker_config_bits;
constant worker_id_bits      : natural := 8;
subtype worker_timeout_t is unsigned(4 downto 0);
type worker_operation_t is (none_e,
                            control_op_e,
                            control_read_e,
                            control_write_e,
                            config_read_e,
                            config_write_e,
                            timeout_e);

type worker_response_t is (none_e,     -- no response yet
                           ok_e,       -- success for something with no data
                           data_e,     -- success for something with data
                           error_e,    -- error
                           timedout_e, -- worker timed out
                           reset_e);   -- worker was reset

-- Internal interface to WCI master modules, driven to all workers in parallel
type worker_in_t is record
  clk       : std_logic;
  reset     : std_logic;
  cmd       : ocp.MCmd_t;                    -- cmd per WCI
  address   : std_logic_vector(worker_config_bits-1 downto 0); -- ready for WCI MAddr
  id        : unsigned(worker_id_bits-1 downto 0);
  is_config : bool_t;                        -- same as WCI MAddrSpace;
  byte_en   : std_logic_vector(3 downto 0);  -- byte enable for read or write
  data      : std_logic_vector(31 downto 0); -- write data
  operation : worker_operation_t;            -- what op is in progress or starting
  timedout  : bool_t;                        -- operation has timed out
end record worker_in_t;

-- Internal interface from WCI master modules, each drives its own version
type worker_out_t is record
  data      : std_logic_vector(31 downto 0); -- worker's timeout value or data
  response  : worker_response_t;             -- worker responds
  attention : std_logic;                     -- to allow them to be consolidated
  present   : std_logic;
end record worker_out_t;


-- These records are generic - i.e. a superset of what any given worker might have

type wci_m2s_t is record
  Clk        : std_logic;
  MReset_n   : std_logic;
  MCmd       : std_logic_vector( 3-1 downto 0);
  MAddr      : std_logic_vector(32-1 downto 0);
  MAddrSpace : std_logic_vector( 1-1 downto 0);
  MByteEn    : std_logic_vector( 4-1 downto 0);
  MData      : std_logic_vector(32-1 downto 0);
  MFlag      : std_logic_vector( 2-1 downto 0);
end record wci_m2s_t;
type wci_m2s_array_t is array(natural range <>) of wci_m2s_t;
type wci_s2m_t is record
  SData       : std_logic_vector(32-1 downto 0);
  SResp       : std_logic_vector( 2-1 downto 0);
  SFlag       : std_logic_vector( 3-1 downto 0);
  SThreadBusy : std_logic_vector( 0 downto 0);
end record wci_s2m_t;
type wci_s2m_array_t is array(natural range <>) of wci_s2m_t;

--------------------------------------------------------------------------------
-- Metadata definitions
--------------------------------------------------------------------------------
-- Metadata to the platform worker.  Platform worker is master
type metadata_in_t is record
  romData : ulong_t;
  UUID    : ulong_array_t(0 to 15);
end record metadata_in_t;

type metadata_out_t is record
  clk     : std_logic;
  romAddr : ushort_t;
  romEn   : bool_t;
end record metadata_out_t;

--------------------------------------------------------------------------------
-- Time service definitions
--------------------------------------------------------------------------------

-- The time_server's (and platform worker's) output that is the time service.
type time_service_t is record
  clk     : std_logic;
  reset_n : std_logic;
  now     : ocpi.types.ulonglong_t;
end record time_service_t;

-- The time server module that is generally instantiated in the platform worker.

component time_server is
  port (
    CLK                 : in  std_logic;
    RST_N               : in  std_logic;
    timeCLK             : in  std_logic;
    timeRST_N           : in  std_logic;
    ppsIn               : in  std_logic;  -- ASYNC
    -- Property interface
    timeControl         : in  ulong_t;
    timeControl_written : in  bool_t;
    timeStatus          : out ulong_t;
    timeNowIn           : in  ulonglong_t;
    timeNow_written     : in  bool_t;
    timeNowOut          : out ulonglong_t;
    timeDeltaIn         : in  ulonglong_t;
    timeDelta_written   : in  bool_t;
    timeDeltaOut        : out ulonglong_t;
    ticksPerSecond      : out ulong_t;
    -- Outputs in tiock clock domain
    ppsOut              : out std_logic;
    time_service        : out time_service_t  -- time service clock domain
    );
end component time_server;

--------------------------------------------------------------------------------
-- uNoc definitions
--------------------------------------------------------------------------------

constant unoc_width : natural := 153;

-- The link in either direction among nodes and clients
type unoc_master_in_t is record
  data  : std_logic_vector(unoc_width-1 downto 0);
  valid : std_logic; -- this data is valid, can be dequeued
  take  : std_logic; -- take data in other direction's record: perform dequeue
end record unoc_master_in_t;

-- The master/source interface of the uNoc
-- This is the interface to the device worker or platform worker
-- that sprouts the uNoc, including its clocking
type unoc_master_out_t is record
  clk     : std_logic;
  reset_n : std_logic;
  id      : std_logic_vector(15 downto 0);
  data  : std_logic_vector(unoc_width-1 downto 0);
  valid : std_logic; -- this data is valid, can be dequeued
  take  : std_logic; -- take data in other direction's record: perform dequeue
end record unoc_master_out_t;


component unoc_terminator is
  port(
    up_in      : in  unoc_master_out_t;
    up_out     : out unoc_master_in_t;
    drop_count : out unsigned(7 downto 0)
    );
end component unoc_terminator;

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
  port(
    clk   : out std_logic;
    reset : out std_logic
    );
end component sim_clk;

component sim_dcp is
  port(
    clk    : in  std_logic;
    reset  : in  std_logic;
    cp_in  : in  occp_out_t;
    cp_out : out occp_in_t);
end component sim_dcp;

end package platform_pkg;

-- modules instantiated as workers must have the component definition in their
-- own package.

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package occp_defs is
  ---- Record for the WCI input signals for port "wci" of worker "ml605_flash"
  --alias wci_out_t is wci_m2s_t;
  ---- Record for the WCI output signals for port "wci" of worker "ml605_flash"
  --alias wci_in_t is wci_s2m_t;
  --type wci_in_array_t is array(0 to 14) of wci_in_t;
  --type wci_out_array_t is array(0 to 14) of wci_out_t;
component occp_rv is
--  generic(
--    nWCIs      : natural := 15
--    );
  port(
    cp_in    : in  occp_in_t;
    cp_out   : out occp_out_t;
    wci_out  : out wci_m2s_array_t(0 to 14); -- (0 to nWCIs-1);
    wci_in   : in  wci_s2m_array_t(0 to 14) --(0 to nWCIs-1)
    );
end component occp_rv;
end package occp_defs;

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package cp_master_defs is
component cp_master_rv is
  generic(
    debug      : boolean;
    nWCIs      : natural);
  port(
    wci_in     : in  wci_s2m_array_t(nWCIs downto 0);
    wci_out    : out wci_m2s_array_t(nWCIs downto 0);
    master_in  : in  occp_in_t;
    master_out : out occp_out_t);
end component cp_master_rv;
end package cp_master_defs;

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package time_client_defs is
type wti_in_t is record
  Clk         : std_logic;
  SThreadBusy : std_logic_vector(0 downto 0);
  SReset_n    : std_logic;
end record wti_in_t;
type wti_out_t is record
  MCmd        : ocpi.ocp.MCmd_t;
  MData       : std_logic_vector(63 downto 0);
end record wti_out_t;

component time_client_rv is
  port(
    time_in : in  time_service_t;
    wti_in  : in  wti_in_t;
    wti_out : out wti_out_t
    );
end component time_client_rv;
end package time_client_defs;
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
use work.platform_pkg.all;
package metadata_defs is
component metadata_rv is
  port(
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
