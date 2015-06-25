library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all; use work.time_client_defs.all; 
entity time_client_rv is
  port(
    time_in : in  time_service_t;
    wti_in  : in  wti_in_t;
    wti_out : out wti_out_t
    );
end entity time_client_rv;
architecture rtl of time_client_rv is

  component SyncRegister is
    generic (
      width : positive;
      init  : natural);
    port (
      sCLK   : in  std_logic;
      dCLK   : in  std_logic;
      sRST   : in  std_logic;
      sD_IN  : in  std_logic_vector(width-1 downto 0);
      sEN    : in  std_logic;
      dD_OUT : out std_logic_vector(width-1 downto 0);
      sRDY   : out std_logic);
  end component;

  signal s_dSyncReg        : std_logic_vector(63 downto 0);
  signal s_dSyncRegCmd     : std_logic_vector(66 downto 0);
  signal wti_m_SThreadBusy : std_logic;
begin

  syncReg : SyncRegister
    generic map (
      width => 64,
      init  => 0)
    port map (
      sCLK   => time_in.clk,
      dCLK   => wti_in.Clk,
      sRST   => time_in.reset,
      sD_IN  => std_logic_vector(time_in.now),
      sEN    => '1',
      dD_OUT => s_dSyncReg,
      sRDY   => open);

  process(wti_in.Clk, wti_in.SReset_n)  --destination clk and destination rst 
  begin
    if (wti_in.SReset_n = '0') then
      s_dSyncRegCmd     <= (others => '0');
      wti_m_SThreadBusy <= '1';
    elsif (rising_edge(wti_in.Clk)) then
      s_dSyncRegCmd     <= "001" & s_dSyncReg;
      wti_m_SThreadBusy <= wti_in.SThreadBusy(0);
    end if;
  end process;



  wti_out.MCmd <= s_dSyncRegCmd(66 downto 64)
                   when (wti_m_SThreadBusy = '0') else "000";
  wti_out.MData <= s_dSyncRegCmd(63 downto 0)
                   when (wti_m_SThreadBusy = '0') else x"AAAA_AAAA_AAAA_AAAA";
  
end architecture rtl;
