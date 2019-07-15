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
--
-- THIS FILE WAS ORIGINALLY GENERATED ON Thu May  2 08:43:35 2019 EDT
-- BASED ON THE FILE: two_in_four_out.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: two_in_four_out

-- This worker tests unit test generation for workers with mutiple inputs and output
--     some of which are optional

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of two_in_four_out_worker is
  signal enable1 : bool_t;
  signal enable2 : bool_t;
begin
  enable1              <= in1_in.ready and out1_in.ready and ctl_in.is_operating;
  enable2              <= in2_in.ready and out2_in.ready and ctl_in.is_operating;

  in1_out.take         <= enable1;
  in2_out.take         <= enable2;

  out1_out.give        <= enable1;
  out2_out.give        <= enable1;

  out3_out.give        <= enable2;
  out4_out.give        <= enable2;
  --
  -- out1_out.opcode      <= in1_in.opcode;
  -- out2_out.opcode      <= in1_in.opcode;
  -- out3_out.opcode      <= in2_in.opcode;
  -- out4_out.opcode      <= in2_in.opcode;

  out1_out.data        <= in1_in.data;
  out2_out.data        <= in1_in.data;

  out3_out.data        <= in2_in.data;
  out4_out.data        <= in2_in.data;

  -- out1_out.byte_enable <= in1_in.byte_enable;
  -- out2_out.byte_enable <= in1_in.byte_enable;
  -- out3_out.byte_enable <= in2_in.byte_enable;
  -- out4_out.byte_enable <= in2_in.byte_enable;

  out1_out.som         <= in1_in.som;
  out2_out.som         <= in1_in.som;

  out3_out.som         <= in2_in.som;
  out4_out.som         <= in2_in.som;


  out1_out.eom         <= in1_in.eom;
  out2_out.eom         <= in1_in.eom;

  out3_out.eom         <= in2_in.eom;
  out4_out.eom         <= in2_in.eom;

  out1_out.valid       <= in1_in.valid;
  out2_out.valid       <= in1_in.valid;

  out3_out.valid       <= in2_in.valid;
  out4_out.valid       <= in2_in.valid;
end rtl;
