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

-- TMP100 I2C Device Worker
-- This worker has the register descriptions in its XML, but
-- delegates the control interface to an I2C subdevice.
architecture rtl of tmp100_worker is
begin
  -- Control plane outputs.  Raw props routed to underlying I2C
  rawprops_out.present         <= '1';
  rawprops_out.reset           <= ctl_in.reset;
  props_out.raw                <= rawprops_in.raw;
  --Need to shift to 16bit addressing
  rawprops_out.raw.address     <= props_in.raw.address srl 1;
  rawprops_out.raw.byte_enable <= props_in.raw.byte_enable;
  rawprops_out.raw.is_read     <= props_in.raw.is_read;
  rawprops_out.raw.is_write    <= props_in.raw.is_write;
  rawprops_out.raw.data        <= props_in.raw.data;
end rtl;
