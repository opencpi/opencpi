-- This file is protected by Copyright. Please refer to the COPYRIGHT file
-- distributed with this source distribution.
--
-- This file is part of OpenCPI <http://www.opencpi.org>
--
-- OpenCPI is free software: you can redistribute it and/or modify it under the
-- terms of the GNU Lesser General Public License as published by the Free
-- Software Foundation, either version 3 of the License, or (at your option) any
-- later version.
--
-- OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
-- details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program. If not, see <http://www.gnu.org/licenses/>.

-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Jun  6 11:26:56 2016 EDT
-- BASED ON THE FILE: ptest.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: ptest

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions
architecture rtl of ptest_worker is
begin
  -- Skeleton assignments for ptest's volatile properties
  props_out.p1 <= to_char(character'val(0));
  props_out.p3 <= to_float(0.0);
  props_out.p5 <= to_long(0);
  props_out.p7 <= to_ulong(0);
  props_out.p9 <= to_longlong(0);
  props_out.pString <= props_in.pString;
  props_out.ap1 <= (to_char(character'val(0)),to_char(character'val(0)));
  props_out.ap3 <= (to_float(0.0),to_float(0.0),to_float(0.0),to_float(0.0));
  props_out.ap5 <= (to_long(0),to_long(0),to_long(0),to_long(0),to_long(0),to_long(0));
  props_out.ap7 <= (to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0),to_ulong(0));
  props_out.ap9 <= (to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0),to_longlong(0));
  props_out.ap11 <= (to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20),to_string("", 20));
  props_out.ledv <= off_e;
  props_out.testvolseqbool_length <= to_ulong(4);
  props_out.testvolseqbool <= (bfalse, btrue, bfalse, btrue, others => bfalse);
  props_out.testvolsequlonglong_length <= to_ulong(4);
  props_out.testvolsequlonglong <= (to_ulonglong(0),
                                    to_ulonglong(slvn(4,ulonglong_t'length)),
                                    to_ulonglong(slv1(ulonglong_t'length)),
                                    to_ulonglong(slvn(1,ulonglong_t'length)) sll 36,
                                    others => to_ulonglong(0));
  props_out.testvolseqstring_length <= to_ulong(5);
  props_out.testvolseqstring <= (to_string("hell0",5),
                                 to_string("hell1",5),
                                 to_string("hell2",5),
                                 others => to_string("",5));
  props_out.testvolseqenum_length <= to_ulong(6);
  props_out.testvolseqenum <= (r_e, g_e, b_e, others => r_e);
  props_out.testrdseqbool_length <= to_ulong(4);
  props_out.testrdseqbool <= (bfalse, btrue, bfalse, btrue, others => bfalse);
  props_out.testrdsequlonglong_length <= to_ulong(4);
  props_out.testrdsequlonglong <= (to_ulonglong(0),
                                    to_ulonglong(slvn(4,ulonglong_t'length)),
                                    to_ulonglong(slv1(ulonglong_t'length)),
                                    to_ulonglong(slvn(1,ulonglong_t'length)) sll 36,
                                    others => to_ulonglong(0));
  props_out.testrdseqstring_length <= to_ulong(5);
  props_out.testrdseqstring <= (to_string("hell0",5),
                                 to_string("hell1",5),
                                 to_string("hell2",5),
                                 others => to_string("",5));
  props_out.testrdseqenum_length <= to_ulong(6);
  props_out.testrdseqenum <= (r_e, g_e, b_e, others => r_e);
  props_out.testvolarybool <= (bfalse, btrue, bfalse, btrue, others => bfalse);
  props_out.testrdarybool <= (bfalse, btrue, bfalse, btrue, others => bfalse);
  props_out.sp1_length <= to_ulong(0);
  props_out.sp3_length <= to_ulong(0);
  props_out.sp5_length <= to_ulong(0);
  props_out.sp7_length <= to_ulong(0);
  props_out.sp9_length <= to_ulong(0);
  props_out.sp11_length <= to_ulong(0);

  ctl_out.finished <= '1';
end rtl;
