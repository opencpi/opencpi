-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Sep  8 18:57:44 2015 EDT
-- BASED ON THE FILE: lime_adc_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_adc_em

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of lime_adc_em_worker is
begin
  props_out.violation <= bfalse;
  rx_clk_out <= '0';
  rx_iq_sel <= '0';
  rxd <= (others => '0');
  rx_clk_in <= '0';
end rtl;
