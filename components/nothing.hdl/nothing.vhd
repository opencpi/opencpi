-- THIS FILE WAS ORIGINALLY GENERATED ON Mon May 11 19:59:16 2015 EDT
-- BASED ON THE FILE: nothing.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: nothing

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of nothing_worker is
begin
  -- the minimum logic to force XST to actually include this module in the
  -- worker synthesis. PITA
  props_out.dummy <= btrue when its(ctl_in.is_operating) else bfalse;
  ctl_out.finished <= btrue;
end rtl;
