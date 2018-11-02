-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Oct 17 12:25:49 2018 EDT
-- BASED ON THE FILE: util_wkr.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: util_wkr

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of util_wkr_worker is
begin
  ctl_out.finished <= btrue; -- remove or change this line for worker to be finished when appropriate
                             -- workers that are never "finished" need not drive this signal
end rtl;
