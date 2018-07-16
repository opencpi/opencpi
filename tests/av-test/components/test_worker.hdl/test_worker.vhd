-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Jun 25 16:00:59 2018 EDT
-- BASED ON THE FILE: test_worker.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: test_worker

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of test_worker_worker is
begin
  ctl_out.finished <= btrue; -- remove or change this line for worker to be finished when appropriate
                             -- workers that are never "finished" need not drive this signal
  -- Skeleton assignments for test_worker's volatile properties
  props_out.test_double <= to_double(0.0);
  props_out.test_ulong <= to_ulong(0);
  props_out.test_bool <= bfalse;
  props_out.test_char <= to_char(character'val(0));
  props_out.test_float <= to_float(0.0);
  props_out.test_long <= to_long(0);
  props_out.test_longlong <= to_longlong(0);
  props_out.test_short <= to_short(0);
  props_out.test_uchar <= to_uchar(0);
  props_out.test_ulonglong <= to_ulonglong(0);
  props_out.test_ushort <= to_ushort(0);
end rtl;
