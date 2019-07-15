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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri Jun 21 09:49:30 2019 EDT
-- BASED ON THE FILE: optional_output.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: optional_output

-- This worker tests generating test assemblies for workers with a single optional port

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of worker is
    signal enable : bool_t;
begin
    enable              <= in_in.ready and out_in.ready and ctl_in.is_operating;
    in_out.take         <= enable;
    out_out.give        <= enable;
    out_out.data        <= in_in.data;
    out_out.som         <= in_in.som;
    out_out.eom         <= in_in.eom;
    out_out.valid       <= in_in.valid;
    ctl_out.finished    <= in_in.EOF;
end rtl;
