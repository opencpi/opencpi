-- THIS FILE WAS ORIGINALLY GENERATED ON Sun Jan 20 14:00:41 2013 EST
-- BASED ON THE FILE: wsi_32_to_16.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: wsi_32_to_16

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of wsi_32_to_16_worker is
  signal hi16 : bool_t;
begin
  in_out.take <= in_in.ready and out_in.ready and (hi16 or not in_in.valid);
  out_out.give  <= in_in.ready and out_in.ready;
  out_out.data <= in_in.data(31 downto 16) when its(hi16) else in_in.data(15 downto 0);
  out_out.som <= bfalse when its(hi16) else in_in.som;
  out_out.eom <= bfalse when not its(hi16) else in_in.eom;
  out_out.valid <= in_in.valid;

  process (wci_clk) is
  begin
    if rising_edge(wci_clk) then
      if its(in_in.reset or out_in.reset) then
        hi16 <= bfalse;
      elsif in_in.ready and out_in.ready then
        if its(hi16) then
          hi16 <= bfalse;
        elsif its(in_in.valid) then
          hi16 <= btrue;
        end if;
      end if;
    end if;
  end process;
end rtl;
