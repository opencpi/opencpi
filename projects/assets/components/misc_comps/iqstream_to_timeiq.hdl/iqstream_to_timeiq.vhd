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

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
architecture rtl of iqstream_to_timeiq_worker is
  signal doit           : bool_t;
begin
-- Pure combinatorial implementation
  doit                <= ctl_in.is_operating and in_in.ready and out_in.ready;
-- WSI input interface outputs
  in_out.take         <= doit;
-- WSI output interface outputs
  out_out.give        <= doit;
  out_out.data        <= in_in.data;
  out_out.som         <= in_in.som;
  out_out.eom         <= in_in.eom;
  out_out.valid       <= in_in.valid;
  out_out.byte_enable <= in_in.byte_enable; -- only necessary due to BSV protocol sharing
end rtl;
