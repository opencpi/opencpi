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

-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Dec  4 14:38:54 2018 EST
-- BASED ON THE FILE: unit_tester.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: unit_tester

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of unit_tester_worker is
  signal enable1 : bool_t;
  signal enable2 : bool_t;
  signal enable3 : bool_t;
begin
  enable1              <= in1_in.ready and out1_in.ready and ctl_in.is_operating;
  enable2              <= in2_in.ready and out2_in.ready and ctl_in.is_operating;
  enable3              <= in3_in.ready and out3_in.ready and ctl_in.is_operating;

  in1_out.take         <= enable1;
  in2_out.take         <= enable2;
  in3_out.take         <= enable3;

  out1_out.give        <= enable1;
  out2_out.give        <= enable2;
  out3_out.give        <= enable3;

  out1_out.opcode      <= in1_in.opcode;
  out2_out.opcode      <= in2_in.opcode;
  out3_out.opcode      <= in3_in.opcode;

  out1_out.data        <= in1_in.data;
  out2_out.data        <= in2_in.data;
  out3_out.data        <= in3_in.data;

  out1_out.byte_enable <= in1_in.byte_enable;
  out2_out.byte_enable <= in2_in.byte_enable;
  out3_out.byte_enable <= in3_in.byte_enable;

  out1_out.som         <= in1_in.som;
  out2_out.som         <= in2_in.som;
  out3_out.som         <= in3_in.som;

  out1_out.eom         <= in1_in.eom;
  out2_out.eom         <= in2_in.eom;
  out3_out.eom         <= in3_in.eom;

  out1_out.valid       <= in1_in.valid;
  out2_out.valid       <= in2_in.valid;
  out3_out.valid       <= in3_in.valid;
end rtl;
