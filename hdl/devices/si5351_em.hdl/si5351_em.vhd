-- THIS FILE WAS ORIGINALLY GENERATED ON Sat Oct 14 16:06:13 2017 EDT
-- BASED ON THE FILE: si5351_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: si5351_em

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of si5351_em_worker is
begin
  ctl_out.finished <= btrue; -- remove or change this line for worker to be finished when appropriate
                             -- workers that are never "finished" need not drive this signal
  -- Skeleton assignments for si5351_em's volatile properties
  props_out.violation <= bfalse;
  intr <= '0';
end rtl;
