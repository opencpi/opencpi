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

--SI5351 Clock Generator I2C Device Worker
-- This worker has the register descriptions in its XML, but
-- delegates the control interface to an I2C subdevice.
architecture rtl of stm_mc24c02_worker is
begin
  -- Control plane outputs.  Raw props routed to underlying I2C
  rawprops_out.present    <= '1';
  rawprops_out.reset      <= ctl_in.reset;
  rawprops_out.raw        <= props_in.raw;
  props_out.raw           <= rawprops_in.raw;
end rtl;
