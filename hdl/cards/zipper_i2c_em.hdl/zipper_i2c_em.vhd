-- THIS FILE WAS ORIGINALLY GENERATED ON Sat Oct 14 16:12:10 2017 EDT
-- BASED ON THE FILE: zipper_i2c_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: zipper_i2c_em

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of zipper_i2c_em_worker is
begin
  ctl_out.finished <= btrue; -- remove or change this line for worker to be finished when appropriate
                             -- workers that are never "finished" need not drive this signal
  -- Skeleton assignments for zipper_i2c_em's volatile properties
  props_out.violation <= bfalse;
  sda_i <= '0';
  scl_i <= '0';
end rtl;
