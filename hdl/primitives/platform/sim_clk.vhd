-- The sim clock/reset generator

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity sim_clk is
  port(
    clk   : out std_logic;
    reset : out std_logic);
end sim_clk;
architecture rtl of sim_clk is
  constant clk_half_period  : time := 5 ns;       -- 100 mhz
  constant reset_clocks     : natural := 17;
  signal myclk              : std_logic := '0';
  signal myreset            : std_logic := '1';
  signal reset_count        : unsigned(7 downto 0) := (others => '0');
begin
  clk   <= myclk;
  reset <= myreset;
  -- generate a clock
  clock : process
  begin
    myclk <= '0';
    wait for clk_half_period;
    myclk <= '1';
    wait for clk_half_period;
  end process;

  -- generate a reset for some number of clocks
  work : process(myclk)
  begin
    if rising_edge(myclk) then
      if myreset = '1' then
        if reset_count < reset_clocks then
          reset_count <= reset_count + 1;
        else
          myreset <= '0';
        end if;
      end if;   
    end if;
  end process;
end architecture rtl;
