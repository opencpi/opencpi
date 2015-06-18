-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Jun 17 21:24:21 2015 EDT
-- BASED ON THE FILE: si5351_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: si5351_em

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of si5351_em_worker is
begin
  props_out.violation <= bfalse;
  intr <= '0';
end rtl;
