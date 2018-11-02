-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Oct 17 10:49:07 2018 EDT
-- BASED ON THE FILE: virtex6_plat.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: virtex6_plat

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of virtex6_plat_worker is
begin
  ctl_out.finished <= btrue; -- remove or change this line for worker to be finished when appropriate
                             -- workers that are never "finished" need not drive this signal
  -- Skeleton assignments for virtex6_plat's volatile properties
  props_out.UUID <= (to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0));
  props_out.romData <= to_ulong(0);
  props_out.nSwitches <= to_ulong(0);
  props_out.nLEDs <= to_ulong(0);
  props_out.memories_length <= to_ulong(0);
  props_out.memories <= (to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0));
  props_out.dna <= to_ulonglong(0);
  props_out.switches <= to_ulong(0);
  props_out.slotCardIsPresent <= (bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse,bfalse);
end rtl;
