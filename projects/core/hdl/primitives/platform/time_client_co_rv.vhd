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

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all; use work.time_client_co_defs.all; 
entity time_client_co_rv is
  port(
    wci_Clk     : in std_logic;
    wci_Reset_n : in std_logic;
    time_in     : in  time_service_t;
    wti_in      : in  wti_in_t;
    wti_out     : out wti_out_t
    );
end entity time_client_co_rv;
architecture rtl of time_client_co_rv is
begin
  wti_out.Clk   <= time_in.clk; -- WARNING THIS IS A DELTA CYCLE IN SIM (but should be ok).
  wti_out.MData <= std_logic_vector(time_in.now);
  wti_out.MCmd  <= ocp.MCmd_WRITE;
end architecture rtl;
