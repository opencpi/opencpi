-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Jun 24 09:04:32 2015 EDT
-- BASED ON THE FILE: time_gen.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: time_gen

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi, util; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of time_gen_worker is
  signal out_count : ushort_t;
begin
  props_out.seconds  <= time_in.seconds;
  props_out.fraction <= time_in.fraction;
  out_out.data       <= ocpi.util.slv(time_in.fraction);
  out_out.valid      <= btrue;
  out_out.som        <= to_bool(out_count = 0);
  out_out.eom        <= to_bool(out_count = props_in.messageLength - 4);
  out_out.give       <= out_in.ready and ctl_in.is_operating;
  p : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        out_count <= (others => '0');
      elsif out_in.ready and ctl_in.is_operating then
        if out_count = props_in.messageLength - 4 then
          out_count <= (others => '0');
        else
          out_count <= out_count + 4;
        end if;
      end if;
    end if;
  end process;
end rtl;
