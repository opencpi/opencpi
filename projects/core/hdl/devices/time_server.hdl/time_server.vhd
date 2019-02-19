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

-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun 23 17:57:52 2015 EDT
-- BASED ON THE FILE: time_server.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: time_server

--
-- This module is normalized the interface when the BSV was converted to VHDL
-- 
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library platform;
architecture rtl of time_server_worker is
begin
  ts : entity work.time_service
    generic map(
      g_TIMECLK_FREQ => from_float(frequency))
    port map(
      CLK                 => ctl_in.clk,
      RST                 => ctl_in.reset,
      timeCLK             => timebase_in.clk,
      timeRST             => timebase_in.reset,
      ppsIn               => timebase_in.ppsIn,
      timeControl         => props_in.control,
      timeControl_written => props_in.control_written,
      timeStatus          => props_out.status,
      timeNowIn           => props_in.timeNow,
      timeNow_written     => props_in.timeNow_written,
      timeNowOut          => props_out.timeNow,
      timeDeltaIn         => props_in.delta,
      timeDelta_written   => props_in.delta_written,
      timeDeltaOut        => props_out.delta,
      ticksPerSecond      => props_out.ticksPerSecond,
      ppsOut              => timebase_out.ppsOut,
      time_service        => time_out);
end rtl;
