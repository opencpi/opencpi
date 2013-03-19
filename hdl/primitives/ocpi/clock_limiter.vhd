  
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.util.all;

entity clock_limiter is
  generic(width : natural);
  port(clk      : in std_logic;
       rst      : in bool_t;
       factor   : in unsigned(width-1 downto 0);
       enabled  : in bool_t;
       ready    : out bool_t);
end entity clock_limiter;
architecture rtl of clock_limiter is
  signal ready_r : bool_t;
begin
  ready <= ready_r;
  process(clk) is
    variable pclk_count : unsigned(width-1 downto 0);
  begin
    if rising_edge(clk) then
      if rst or pclk_count >= factor then
        ready_r <= btrue;
        pclk_count := (others => '0');
      elsif not ready_r then
        pclk_count := pclk_count + 1;
      elsif its(enabled) then
        ready_r <= bfalse;
      end if;
    end if;
  end process;
end rtl;
