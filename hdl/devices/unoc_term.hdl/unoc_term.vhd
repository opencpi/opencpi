-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Oct 24 08:44:59 2016 EDT
-- BASED ON THE FILE: unoc_term.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: unoc_term
-- Note THIS IS THE OUTER skeleton, since the 'outer' attribute was set.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library platform; use platform.platform_pkg.all;
architecture rtl of unoc_term_rv is
  signal count_r : unsigned(up_out.dropCount'range);
begin
  up_out.data      <= to_unoc(util.slv0(unoc_data_width));
  up_out.valid     <= '0';
  up_out.take      <= up_in.valid;
  up_out.dropCount <= count_r;

  process (up_in.clk) is
  begin
    if rising_edge(up_in.clk) then
      if up_in.reset_n = '0' then
        count_r <= (others => '0');
      elsif up_in.valid = '1' and count_r /= 255 then
        count_r <= count_r + 1;
      end if;
    end if;
  end process;
end rtl;
