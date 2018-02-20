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

-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Dec 17 10:40:22 2012 EST
-- BASED ON THE FILE: bias_vhdl.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: bias_vhdl

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of bias_spcm_worker is
  signal doit : bool_t;
begin
-- Pure combinatorial implementation
  doit                <= ctl_in.is_operating and in_in.ready and out_in.ready;
-- WSI input interface outputs
  in_out.take         <= doit;
-- WSI output interface outputs
  out_out.give        <= doit;
  out_out.data        <= std_logic_vector(unsigned(in_in.data) + props_in.biasValue);
  out_out.som         <= in_in.som;
  out_out.eom         <= in_in.eom;
  out_out.valid       <= in_in.valid;
  out_out.byte_enable <= in_in.byte_enable; -- only necessary due to BSV protocol sharing
  out_out.opcode      <= in_in.opcode;
end rtl;
