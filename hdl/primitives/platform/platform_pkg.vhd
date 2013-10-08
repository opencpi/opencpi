-- VHDL component declarations for platform modules
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
package platform_pkg is


--------------------------------------------------------------------------------
-- Control Plane definitions
--------------------------------------------------------------------------------

constant occp_request_width : natural := 59;
constant occp_response_width : natural := 40;
type occp_in_t is record
  clk     : std_logic;
  reset_n : std_logic;
  data    : std_logic_vector(occp_request_width-1 downto 0);
  valid   : std_logic;
  take    : std_logic;
end record occp_in_t;
type occp_out_t is record
  data  : std_logic_vector(occp_response_width-1 downto 0);
  valid : std_logic;
  take  : std_logic;
end record occp_out_t;

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
    CLK, RST_N          : in  std_logic;
    timeCLK, timeRST_N  : in  std_logic;
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
type unoc_link_t is record
  data  : std_logic_vector(unoc_width-1 downto 0);
  valid : std_logic; -- this data is valid, can be dequeued
  take  : std_logic; -- take data in other direction's record: perform dequeue
end record unoc_link_t;

-- The master/source interface of the uNoc
-- This is the interface to the device worker or platform worker
-- that sprouts the uNoc, including its clocking
alias unoc_master_in_t is unoc_link_t;
type unoc_master_out_t is record
  clk     : std_logic;
  reset_n : std_logic;
  link    : unoc_link_t;
end record unoc_master_out_t;

-- a node in the unoc
component unoc_node is
  generic(
    -- Be a little proactive - this is more control than we have now
    control          : boolean := false;
    func             : natural := 0;      -- PCI function
    offset           : natural := 0;      -- Offset in bar: now 0 or 32k
    size             : natural := 0;      -- Window size in bar: now 32k
    up_producer_hack : boolean := false   -- hack described below for compatibility
    );
  port (
    CLK, RST_N : in  std_logic;
    up_in      : in  unoc_link_t;
    up_out     : out unoc_link_t;
    client_in  : in  unoc_link_t;
    client_out : out unoc_link_t;
    down_in    : in  unoc_link_t;
    down_out   : out unoc_link_t
    );
end component unoc_node;

component unoc_terminator is
  port(
    CLK, RST_N : in  std_logic;
    up_in      : in  unoc_link_t;
    up_out     : out unoc_link_t;
    drop_count : out unsigned(7 downto 0)
    );
end component unoc_terminator;

component unoc_cp_adapter is
  port(
    CLK, RST_N : in  std_logic;
    pciDevice  : in  std_logic_vector(15 downto 0);
    client_in  : in  unoc_link_t;
    client_out : out unoc_link_t;
    occp_in    : in  occp_out_t;
    occp_out   : out occp_in_t
    );
end component unoc_cp_adapter;

--------------------------------------------------------------------------------
-- Simulation module definitions
--------------------------------------------------------------------------------

component mkSimIO is
  port(
    CLK                   : in  std_logic;
    RST_N                 : in  std_logic;
    EN_host_request_get   : in  std_logic;
    host_request_get      : out std_logic_vector(7 downto 0);
    RDY_host_request_get  : out std_logic;
    host_response_put     : in  std_logic_vector(7 downto 0);
    EN_host_response_put  : in  std_logic;
    RDY_host_response_put : out std_logic);
end component mkSimIO;
component mkSimDCP is
  port(
    CLK                     : in  std_logic;
    RST_N                   : in  std_logic;

    host_request_put        : in  std_logic_vector(7 downto 0);
    EN_host_request_put     : in  std_logic;
    RDY_host_request_put    : out std_logic;

    EN_host_response_get    : in  std_logic;
    host_response_get       : out std_logic_vector(7 downto 0);
    RDY_host_response_get   : out std_logic;

    EN_client_request_get   : in  std_logic;
    client_request_get      : out std_logic_vector(58 downto 0);
    RDY_client_request_get  : out std_logic;
    
    client_response_put     : in  std_logic_vector(39 downto 0);
    EN_client_response_put  : in  std_logic;
    RDY_client_response_put : out std_logic);
end component mkSimDCP;

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
    metadata_in : in  metadata_out_t;
    metadata_out : in  metadata_in_t
    );
end component metadata_rv;
end package metadata_defs;
