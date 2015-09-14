-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Sep  8 18:59:18 2015 EDT
-- BASED ON THE FILE: zipper_i2c_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: zipper_i2c_em

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of zipper_i2c_em_worker is
begin
  props_out.violation <= bfalse;
  sda_i <= '0';
  scl_i <= '0';
end rtl;
