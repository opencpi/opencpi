-- The control plane, wrapped for VHDL and no connecting logic
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library platform; use platform.platform_pkg.all, platform.time_client_defs.all;
entity time_client_rv is
  port(
    time_in : in  time_service_t;
    wti_in  : in  wti_in_t;
    wti_out : out wti_out_t
    );
end entity time_client_rv;
architecture rtl of time_client_rv is
  -- This is the verilog being wrapped
  component mkTimeClient is
    port(
      CLK_sys0_clk      : in  std_logic;
      RST_N_sys0_rst    : in  std_logic;
      CLK_wti_clk       : in  std_logic;
      RST_N_wti_rst     : in  std_logic;
      CLK               : in  std_logic;        
      RST_N             : in  std_logic;
      gpsTime_arg       : in  std_logic_vector(63 downto 0);
      EN_gpsTime        : in  std_logic;
      RDY_gpsTime       : out std_logic;
      wti_m_req         : out std_logic_vector(66 downto 0);
      wti_m_SThreadBusy : in std_logic;
      wti_m_SReset_n    : in std_logic
    );
  end component mkTimeClient;
begin
  tc : mkTimeClient
    port map(
      CLK_sys0_clk            => time_in.clk,
      RST_N_sys0_rst          => time_in.reset_n,
      CLK_wti_clk             => wti_in.Clk,
      RST_N_wti_rst           => wti_in.SReset_n,
      CLK                     => '0',
      RST_N                   => '0',
      gpsTime_arg             => std_logic_vector(time_in.now),
      EN_gpsTime              => '1',
      RDY_gpsTime             => open,  -- no flow control
      wti_m_req(66 downto 64) => wti_out.MCmd,
      wti_m_req(63 downto 0)  => wti_out.MData,
      wti_m_SThreadBusy       => wti_in.SThreadBusy(0),
      wti_m_SReset_n          => wti_in.SReset_n
      );
end architecture rtl;
