-- A register synchronization module across clock domains.
-- Uses a Handshake Pulse protocol to trigger the load on
-- destination side registers
-- Transfer takes 3 dCLK for destination side to see data,
-- sRDY recovery takes 3 sCLK + 3 dCLK
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
entity SyncRegister is
  generic (
      width : positive := 1;
      init  : natural := 0);
    port (
      sCLK   : in std_logic;
      sRST   : in std_logic;
      dCLK   : in std_logic;
      sEN    : in std_logic;
      sRDY   : out std_logic;
      sD_IN  : in std_logic_vector(width-1 downto 0);      
      dD_OUT : out std_logic_vector(width-1 downto 0));
end entity SyncRegister;
architecture rtl of SyncRegister is
  signal s_sToggleReg  : std_logic;
  signal s_sDataSyncIn : std_logic_vector(width-1 downto 0);
  signal s_sSyncReg    : std_logic_vector(1 downto 0);
  signal s_dSyncReg    : std_logic_vector(2 downto 0);
  signal s_dPulse      : std_logic;
begin
  -----------------------------------------------------------------------------
  proc_iReg : process(sCLK, sRST)       -- source clk and source rst
  begin
    if (sRST = '1') then                -- async rst
      s_sDataSyncIn <= (others => '0');
    elsif (rising_edge(sCLK)) then
      if (sEN = '1') then
        s_sDataSyncIn <= sD_IN;
      end if;
    end if;
  end process;

  proc_sSyncReg : process(sCLK, sRST)   -- source clk and source rst
  begin
    if (sRST = '1') then                -- async rst
      s_sToggleReg <= '0';
      s_sSyncReg   <= (others => '1');
    elsif (rising_edge(sCLK)) then
      if (sEN = '1') then
        s_sToggleReg <= not s_sToggleReg;
      end if;
      s_sSyncReg <= s_sSyncReg(0) & s_dSyncReg(1);  -- clk domain crossing
    end if;
  end process;

  sRDY <= '1' when (s_sSyncReg(1) xnor s_sToggleReg) else '0';

  proc_dSyncReg : process(dCLK, sRST)   -- destination clk and source rst
  begin
    if (sRST = '1') then                -- async rst
      s_dSyncReg <= (others => '0');
    elsif (rising_edge(dCLK)) then
      s_dSyncReg <= s_dSyncReg(1 downto 0) & s_sToggleReg;  -- clk domain crossing 
    end if;
  end process;

  s_dPulse <= '1' when (s_dSyncReg(2) xor s_dSyncReg(1)) else '0';

  -- Transfer the data to destination domain when s_dPulsed is asserted.
  -- Setup and hold time are assured since at least 2 dClks occured since
  -- s_sDataSyncIn have been written. 
  proc_oReg : process(dCLK, sRST)       -- destination clk and source rst
  begin
    if (sRST = '1') then                -- async rst
      dD_OUT <= (others => '0');
    elsif (rising_edge(dCLK)) then
      if (s_dPulse = '1') then
        dD_OUT <= s_sDataSyncIn;        -- clk domain crossing
      end if;
    end if;
  end process;
  
end architecture rtl;
