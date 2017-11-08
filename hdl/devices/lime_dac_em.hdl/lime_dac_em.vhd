-- THIS FILE WAS ORIGINALLY GENERATED ON Sat Oct 14 16:10:09 2017 EDT
-- BASED ON THE FILE: lime_dac_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_dac_em

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of lime_dac_em_worker is
begin
  ctl_out.finished <= btrue; -- remove or change this line for worker to be finished when appropriate
                             -- workers that are never "finished" need not drive this signal
  -- Skeleton assignments for lime_dac_em's volatile properties
  props_out.underrun <= bfalse;
  props_out.violation <= bfalse;
  TX_CLK_IN <= '0';
end rtl;
