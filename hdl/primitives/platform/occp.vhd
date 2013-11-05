-- The control plane, wrapped for VHDL and no connecting logic
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library platform; use platform.platform_pkg.all, platform.occp_defs.all;
entity occp_rv is
--  generic(
--    nWCIs      : natural := 15
--    );
  port(
    cp_in   : in  occp_in_t;
    cp_out  : out occp_out_t;
    wci_out : out wci_m2s_array_t(0 to 14); --wci_m2s_array_t(0 to nWCIs-1);
    wci_in  : in  wci_s2m_array_t(0 to 14) --wci_s2m_array_t(0 to nWCIs-1)
    );
end entity occp_rv;
architecture rtl of occp_rv is
  -- This is the verilog being wrapped
  component mkOCCP is
    port(
      CLK                     : in  std_logic;
      RST_N                   : in  std_logic;
      -- server request interface - we do the AND for: both sides ready
      server_request_put      : in std_logic_vector(58 downto 0);
      EN_server_request_put   : in  std_logic;
      RDY_server_request_put  : out std_logic;
      -- server response interface
      EN_server_response_get  : in  std_logic;
      server_response_get     : out std_logic_vector(39 downto 0);
      RDY_server_response_get : out std_logic;
      -- WCI signals for each one
      wci_Vm_0_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_0_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_0_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_0_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_0_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_0_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_0_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_0_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_0_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_0_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_1_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_1_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_1_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_1_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_1_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_1_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_1_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_1_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_1_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_1_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_2_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_2_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_2_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_2_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_2_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_2_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_2_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_2_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_2_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_2_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_3_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_3_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_3_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_3_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_3_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_3_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_3_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_3_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_3_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_3_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_4_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_4_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_4_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_4_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_4_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_4_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_4_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_4_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_4_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_4_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_5_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_5_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_5_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_5_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_5_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_5_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_5_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_5_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_5_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_5_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_6_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_6_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_6_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_6_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_6_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_6_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_6_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_6_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_6_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_6_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_7_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_7_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_7_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_7_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_7_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_7_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_7_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_7_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_7_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_7_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_8_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_8_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_8_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_8_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_8_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_8_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_8_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_8_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_8_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_8_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_9_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_9_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_9_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_9_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_9_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_9_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_9_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_9_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_9_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_9_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_10_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_10_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_10_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_10_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_10_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_10_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_10_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_10_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_10_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_10_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_11_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_11_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_11_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_11_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_11_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_11_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_11_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_11_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_11_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_11_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_12_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_12_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_12_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_12_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_12_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_12_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_12_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_12_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_12_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_12_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_13_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_13_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_13_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_13_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_13_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_13_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_13_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_13_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_13_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_13_MFlag          : out std_logic_vector( 1 downto 0);
      wci_Vm_14_MCmd           : out std_logic_vector( 2 downto 0);
      wci_Vm_14_MAddrSpace     : out std_logic; -- _vector( 0 downto 0);
      wci_Vm_14_MByteEn        : out std_logic_vector( 3 downto 0);
      wci_Vm_14_MAddr          : out std_logic_vector(31 downto 0);
      wci_Vm_14_MData          : out std_logic_vector(31 downto 0);
      wci_Vm_14_SResp          : in  std_logic_vector( 1 downto 0);
      wci_Vm_14_SData          : in  std_logic_vector(31 downto 0);
      wci_Vm_14_SThreadBusy    : in  std_logic; -- _vector( 0 downto 0);
      wci_Vm_14_SFlag          : in  std_logic_vector( 2 downto 0);
      wci_Vm_14_MFlag          : out std_logic_vector( 1 downto 0);
      RST_N_wci_Vm_0          : out std_logic;
      RST_N_wci_Vm_1          : out std_logic;
      RST_N_wci_Vm_2          : out std_logic;
      RST_N_wci_Vm_3          : out std_logic;
      RST_N_wci_Vm_4          : out std_logic;
      RST_N_wci_Vm_5          : out std_logic;
      RST_N_wci_Vm_6          : out std_logic;
      RST_N_wci_Vm_7          : out std_logic;
      RST_N_wci_Vm_8          : out std_logic;
      RST_N_wci_Vm_9          : out std_logic;
      RST_N_wci_Vm_10         : out std_logic;
      RST_N_wci_Vm_11         : out std_logic;
      RST_N_wci_Vm_12         : out std_logic;
      RST_N_wci_Vm_13         : out std_logic;
      RST_N_wci_Vm_14         : out std_logic
      );
  end component mkOCCP;
  -- Our locally computed enqueue signal for input data
  signal EN_server_request_put : std_logic;
  -- Our locally used input-FIFO-not-full signal for input data
  signal RDY_server_request_put : std_logic;
begin
  -- We tell mkOCCP to enqueue when it is ready (not full) and there is input data
  EN_server_request_put <= RDY_server_request_put and cp_in.valid;
  -- We tell the producer side to dequeue when we are enqueueing
  cp_out.take           <= EN_server_request_put;
  gen0: for i in 0 to 14 generate
    wci_out(i).CLK <= cp_in.clk;
  end generate gen0;
         
sm : mkOCCP
  port map(
    CLK                     => cp_in.clk,
    RST_N                   => cp_in.reset_n,
    -- server request interface
    RDY_server_request_put  => RDY_server_request_put,
    server_request_put      => cp_in.data,
    EN_server_request_put   => EN_server_request_put,
    -- server response interface
    RDY_server_response_get => cp_out.valid,
    server_response_get     => cp_out.data,
    EN_server_response_get  => cp_in.take,
    -- WCI signals for each one
    wci_Vm_0_MCmd        => wci_out(0).MCmd,
    wci_Vm_0_MAddrSpace  => wci_out(0).MAddrSpace(0),
    wci_Vm_0_MByteEn     => wci_out(0).MByteEn,
    wci_Vm_0_MAddr       => wci_out(0).MAddr,
    wci_Vm_0_MData       => wci_out(0).MData,   
    wci_Vm_0_MFlag       => wci_out(0).MFlag,
    RST_N_wci_Vm_0       => wci_out(0).MReset_n,
    wci_Vm_0_SResp       => wci_in(0).SResp,
    wci_Vm_0_SData       => wci_in(0).SData,
    wci_Vm_0_SFlag       => wci_in(0).SFlag,
    wci_Vm_0_SThreadBusy => wci_in(0).SThreadBusy(0),
    wci_Vm_1_MCmd        => wci_out(1).MCmd,
    wci_Vm_1_MAddrSpace  => wci_out(1).MAddrSpace(0),
    wci_Vm_1_MByteEn     => wci_out(1).MByteEn,
    wci_Vm_1_MAddr       => wci_out(1).MAddr,
    wci_Vm_1_MData       => wci_out(1).MData,   
    wci_Vm_1_MFlag       => wci_out(1).MFlag,
    RST_N_wci_Vm_1       => wci_out(1).MReset_n,
    wci_Vm_1_SResp       => wci_in(1).SResp,
    wci_Vm_1_SData       => wci_in(1).SData,
    wci_Vm_1_SFlag       => wci_in(1).SFlag,
    wci_Vm_1_SThreadBusy => wci_in(1).SThreadBusy(0),
    wci_Vm_2_MCmd        => wci_out(2).MCmd,
    wci_Vm_2_MAddrSpace  => wci_out(2).MAddrSpace(0),
    wci_Vm_2_MByteEn     => wci_out(2).MByteEn,
    wci_Vm_2_MAddr       => wci_out(2).MAddr,
    wci_Vm_2_MData       => wci_out(2).MData,   
    wci_Vm_2_MFlag       => wci_out(2).MFlag,
    RST_N_wci_Vm_2       => wci_out(2).MReset_n,
    wci_Vm_2_SResp       => wci_in(2).SResp,
    wci_Vm_2_SData       => wci_in(2).SData,
    wci_Vm_2_SFlag       => wci_in(2).SFlag,
    wci_Vm_2_SThreadBusy => wci_in(2).SThreadBusy(0),
    wci_Vm_3_MCmd        => wci_out(3).MCmd,
    wci_Vm_3_MAddrSpace  => wci_out(3).MAddrSpace(0),
    wci_Vm_3_MByteEn     => wci_out(3).MByteEn,
    wci_Vm_3_MAddr       => wci_out(3).MAddr,
    wci_Vm_3_MData       => wci_out(3).MData,   
    wci_Vm_3_MFlag       => wci_out(3).MFlag,
    RST_N_wci_Vm_3       => wci_out(3).MReset_n,
    wci_Vm_3_SResp       => wci_in(3).SResp,
    wci_Vm_3_SData       => wci_in(3).SData,
    wci_Vm_3_SFlag       => wci_in(3).SFlag,
    wci_Vm_3_SThreadBusy => wci_in(3).SThreadBusy(0),
    wci_Vm_4_MCmd        => wci_out(4).MCmd,
    wci_Vm_4_MAddrSpace  => wci_out(4).MAddrSpace(0),
    wci_Vm_4_MByteEn     => wci_out(4).MByteEn,
    wci_Vm_4_MAddr       => wci_out(4).MAddr,
    wci_Vm_4_MData       => wci_out(4).MData,   
    wci_Vm_4_MFlag       => wci_out(4).MFlag,
    RST_N_wci_Vm_4       => wci_out(4).MReset_n,
    wci_Vm_4_SResp       => wci_in(4).SResp,
    wci_Vm_4_SData       => wci_in(4).SData,
    wci_Vm_4_SFlag       => wci_in(4).SFlag,
    wci_Vm_4_SThreadBusy => wci_in(4).SThreadBusy(0),
    wci_Vm_5_MCmd        => wci_out(5).MCmd,
    wci_Vm_5_MAddrSpace  => wci_out(5).MAddrSpace(0),
    wci_Vm_5_MByteEn     => wci_out(5).MByteEn,
    wci_Vm_5_MAddr       => wci_out(5).MAddr,
    wci_Vm_5_MData       => wci_out(5).MData,   
    wci_Vm_5_MFlag       => wci_out(5).MFlag,
    RST_N_wci_Vm_5       => wci_out(5).MReset_n,
    wci_Vm_5_SResp       => wci_in(5).SResp,
    wci_Vm_5_SData       => wci_in(5).SData,
    wci_Vm_5_SFlag       => wci_in(5).SFlag,
    wci_Vm_5_SThreadBusy => wci_in(5).SThreadBusy(0),
    wci_Vm_6_MCmd        => wci_out(6).MCmd,
    wci_Vm_6_MAddrSpace  => wci_out(6).MAddrSpace(0),
    wci_Vm_6_MByteEn     => wci_out(6).MByteEn,
    wci_Vm_6_MAddr       => wci_out(6).MAddr,
    wci_Vm_6_MData       => wci_out(6).MData,   
    wci_Vm_6_MFlag       => wci_out(6).MFlag,
    RST_N_wci_Vm_6       => wci_out(6).MReset_n,
    wci_Vm_6_SResp       => wci_in(6).SResp,
    wci_Vm_6_SData       => wci_in(6).SData,
    wci_Vm_6_SFlag       => wci_in(6).SFlag,
    wci_Vm_6_SThreadBusy => wci_in(6).SThreadBusy(0),
    wci_Vm_7_MCmd        => wci_out(7).MCmd,
    wci_Vm_7_MAddrSpace  => wci_out(7).MAddrSpace(0),
    wci_Vm_7_MByteEn     => wci_out(7).MByteEn,
    wci_Vm_7_MAddr       => wci_out(7).MAddr,
    wci_Vm_7_MData       => wci_out(7).MData,   
    wci_Vm_7_MFlag       => wci_out(7).MFlag,
    RST_N_wci_Vm_7       => wci_out(7).MReset_n,
    wci_Vm_7_SResp       => wci_in(7).SResp,
    wci_Vm_7_SData       => wci_in(7).SData,
    wci_Vm_7_SFlag       => wci_in(7).SFlag,
    wci_Vm_7_SThreadBusy => wci_in(7).SThreadBusy(0),
    wci_Vm_8_MCmd        => wci_out(8).MCmd,
    wci_Vm_8_MAddrSpace  => wci_out(8).MAddrSpace(0),
    wci_Vm_8_MByteEn     => wci_out(8).MByteEn,
    wci_Vm_8_MAddr       => wci_out(8).MAddr,
    wci_Vm_8_MData       => wci_out(8).MData,   
    wci_Vm_8_MFlag       => wci_out(8).MFlag,
    RST_N_wci_Vm_8       => wci_out(8).MReset_n,
    wci_Vm_8_SResp       => wci_in(8).SResp,
    wci_Vm_8_SData       => wci_in(8).SData,
    wci_Vm_8_SFlag       => wci_in(8).SFlag,
    wci_Vm_8_SThreadBusy => wci_in(8).SThreadBusy(0),
    wci_Vm_9_MCmd        => wci_out(9).MCmd,
    wci_Vm_9_MAddrSpace  => wci_out(9).MAddrSpace(0),
    wci_Vm_9_MByteEn     => wci_out(9).MByteEn,
    wci_Vm_9_MAddr       => wci_out(9).MAddr,
    wci_Vm_9_MData       => wci_out(9).MData,   
    wci_Vm_9_MFlag       => wci_out(9).MFlag,
    RST_N_wci_Vm_9       => wci_out(9).MReset_n,
    wci_Vm_9_SResp       => wci_in(9).SResp,
    wci_Vm_9_SData       => wci_in(9).SData,
    wci_Vm_9_SFlag       => wci_in(9).SFlag,
    wci_Vm_9_SThreadBusy => wci_in(9).SThreadBusy(0),
    wci_Vm_10_MCmd        => wci_out(10).MCmd,
    wci_Vm_10_MAddrSpace  => wci_out(10).MAddrSpace(0),
    wci_Vm_10_MByteEn     => wci_out(10).MByteEn,
    wci_Vm_10_MAddr       => wci_out(10).MAddr,
    wci_Vm_10_MData       => wci_out(10).MData,   
    wci_Vm_10_MFlag       => wci_out(10).MFlag,
    RST_N_wci_Vm_10       => wci_out(10).MReset_n,
    wci_Vm_10_SResp       => wci_in(10).SResp,
    wci_Vm_10_SData       => wci_in(10).SData,
    wci_Vm_10_SFlag       => wci_in(10).SFlag,
    wci_Vm_10_SThreadBusy => wci_in(10).SThreadBusy(0),
    wci_Vm_11_MCmd        => wci_out(11).MCmd,
    wci_Vm_11_MAddrSpace  => wci_out(11).MAddrSpace(0),
    wci_Vm_11_MByteEn     => wci_out(11).MByteEn,
    wci_Vm_11_MAddr       => wci_out(11).MAddr,
    wci_Vm_11_MData       => wci_out(11).MData,   
    wci_Vm_11_MFlag       => wci_out(11).MFlag,
    RST_N_wci_Vm_11       => wci_out(11).MReset_n,
    wci_Vm_11_SResp       => wci_in(11).SResp,
    wci_Vm_11_SData       => wci_in(11).SData,
    wci_Vm_11_SFlag       => wci_in(11).SFlag,
    wci_Vm_11_SThreadBusy => wci_in(11).SThreadBusy(0),
    wci_Vm_12_MCmd        => wci_out(12).MCmd,
    wci_Vm_12_MAddrSpace  => wci_out(12).MAddrSpace(0),
    wci_Vm_12_MByteEn     => wci_out(12).MByteEn,
    wci_Vm_12_MAddr       => wci_out(12).MAddr,
    wci_Vm_12_MData       => wci_out(12).MData,   
    wci_Vm_12_MFlag       => wci_out(12).MFlag,
    RST_N_wci_Vm_12       => wci_out(12).MReset_n,
    wci_Vm_12_SResp       => wci_in(12).SResp,
    wci_Vm_12_SData       => wci_in(12).SData,
    wci_Vm_12_SFlag       => wci_in(12).SFlag,
    wci_Vm_12_SThreadBusy => wci_in(12).SThreadBusy(0),
    wci_Vm_13_MCmd        => wci_out(13).MCmd,
    wci_Vm_13_MAddrSpace  => wci_out(13).MAddrSpace(0),
    wci_Vm_13_MByteEn     => wci_out(13).MByteEn,
    wci_Vm_13_MAddr       => wci_out(13).MAddr,
    wci_Vm_13_MData       => wci_out(13).MData,   
    wci_Vm_13_MFlag       => wci_out(13).MFlag,
    RST_N_wci_Vm_13       => wci_out(13).MReset_n,
    wci_Vm_13_SResp       => wci_in(13).SResp,
    wci_Vm_13_SData       => wci_in(13).SData,
    wci_Vm_13_SFlag       => wci_in(13).SFlag,
    wci_Vm_13_SThreadBusy => wci_in(13).SThreadBusy(0),
    wci_Vm_14_MCmd        => wci_out(14).MCmd,
    wci_Vm_14_MAddrSpace  => wci_out(14).MAddrSpace(0),
    wci_Vm_14_MByteEn     => wci_out(14).MByteEn,
    wci_Vm_14_MAddr       => wci_out(14).MAddr,
    wci_Vm_14_MData       => wci_out(14).MData,   
    wci_Vm_14_MFlag       => wci_out(14).MFlag,
    RST_N_wci_Vm_14       => wci_out(14).MReset_n,
    wci_Vm_14_SResp       => wci_in(14).SResp,
    wci_Vm_14_SData       => wci_in(14).SData,
    wci_Vm_14_SFlag       => wci_in(14).SFlag,
    wci_Vm_14_SThreadBusy => wci_in(14).SThreadBusy(0)
    );
end architecture rtl;
