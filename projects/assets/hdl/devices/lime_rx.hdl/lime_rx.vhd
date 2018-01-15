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

-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun  2 08:35:00 2015 EDT
-- BASED ON THE FILE: lime_rx.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_rx

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of lime_rx_worker is
begin
  -- Route the raw property signals to the lime_spi
  rawprops_out.present     <= '1';
  rawprops_out.reset       <= ctl_in.reset;
  rawprops_out.raw         <= props_in.raw;
  props_out.raw            <= rawprops_in.raw;
  props_out.other_present  <= rawprops_in.present(1);
  -- FIXME: should this be "not ctl_in.reset and ctl_in.is_operating"?
  rxen <= '1';
end rtl;
