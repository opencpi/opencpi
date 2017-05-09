-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Jun  6 11:26:56 2016 EDT
-- BASED ON THE FILE: ptest.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ptest

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of ptest_worker is
begin
  -- Skeleton assignments for ptest's volatile properties
  props_out.p1 <= to_char(character'val(0));
  props_out.p3 <= to_float(0.0);
  props_out.p5 <= to_long(0);
  props_out.p7 <= to_ulong(0);
  props_out.p9 <= to_longlong(0);
  props_out.pString <= to_string("", 20);
  props_out.ap1 <= (to_char(character'val(0)),to_char(character'val(0)));
  props_out.ap3 <= (to_float(0.0),to_float(0.0),to_float(0.0),to_float(0.0));
  props_out.ap5 <= (to_long(0),to_long(0),to_long(0),to_long(0),to_long(0),to_long(0));
  props_out.ap7 <= (to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0));
  props_out.ap9 <= (to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0));
  props_out.ap11 <= (to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20));
  props_out.ledv <= off_e;
  props_out.romData <= to_ulong(0);
  ctl_out.finished <= '1';
end rtl;
