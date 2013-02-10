-- THIS FILE WAS ORIGINALLY GENERATED ON Sun Jan 20 14:00:41 2013 EST
-- BASED ON THE FILE: wsi_32_to_16.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: wsi_32_to_16

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of wsi_32_to_16_worker is
  signal hi16_r   : bool_t; 
  signal hi_absent : bool_t;
begin
  hi_absent           <= to_bool(in_in.byte_enable(3) = '0' and in_in.byte_enable(2) = '0');
  in_out.take         <= in_in.ready and out_in.ready and
                         (hi16_r or not in_in.valid or hi_absent);

  out_out.give        <= in_in.ready and out_in.ready;
  out_out.data        <= in_in.data(31 downto 16) when its(hi16_r) else in_in.data(15 downto 0);
  out_out.byte_enable <= in_in.byte_enable(3 downto 2) when its(hi16_r) else in_in.byte_enable(1 downto 0);
  out_out.som         <= bfalse when its(hi16_r) else in_in.som;
  out_out.eom         <= in_in.eom and
                         (
                           -- bare eom (and possibly som)
                           not in_in.valid or 
                           -- high valid data word on input eom word
                           hi16_r or
                           -- low valid data word when no high word on input eom
                           hi_absent
                           );
  out_out.valid       <= in_in.valid;

  process (wci_clk) is
  begin
    if rising_edge(wci_clk) then
      if in_in.reset or out_in.reset then
        hi16_r <= bfalse;
      elsif in_in.ready and out_in.ready then
        if its(hi16_r) then
          hi16_r <= bfalse;
        elsif in_in.valid and not its(hi_absent) then
          hi16_r <= btrue;
        end if;
      end if;
    end if;
  end process;
end rtl;
