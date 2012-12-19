-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Dec 17 10:40:22 2012 EST
-- BASED ON THE FILE: bias_vhdl.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: bias_vhdl

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of bias_vhdl_worker is
begin
-- WSI input interface outputs
  in_out.take <= in_in.ready and out_in.ready;
-- WSI output interface outputs
  out_out.give <= in_in.ready and out_in.ready;
  out_out.data <= std_logic_vector(unsigned(in_in.data) + props_write.biasValue);
  out_out.som <= in_in.som;
  out_out.eom <= in_in.eom;
  out_out.valid <= in_in.valid;
end rtl;
