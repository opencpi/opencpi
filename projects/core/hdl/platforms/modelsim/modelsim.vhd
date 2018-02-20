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

-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Sep 26 15:32:12 2013 EDT
-- BASED ON THE FILE: modelsim.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: modelsim

library IEEE, ocpi, platform, sdp;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, platform.platform_pkg.all,
  sdp.all;

architecture rtl of modelsim_worker is
  signal   ctl_clk          : std_logic;
  signal   ctl_reset        : std_logic := '0';
  signal   ctl_rst_n        : std_logic;
begin
  ctl_rst_n <= not ctl_reset; -- for those that need it
  timebase_out.clk   <= ctl_clk;
  timebase_out.reset <= ctl_reset;
  timebase_out.ppsIn <= '0';

  -- generate a clock
  clock : sim_clk
    port map(clk => ctl_clk, reset => ctl_reset);


  sdp_sim_i : sdp.sdp.sdp_sim
    generic map(ocpi_debug => ocpi_debug,
                sdp_width  => sdp_width)
    port map(clk => ctl_clk,
             reset => ctl_reset,
             sdp_in => sdp_in,
             sdp_out => sdp_out,
             sdp_in_data => sdp_in_data,
             sdp_out_data => sdp_out_data);

  props_out.dna               <= (others => '0');
  props_out.nSwitches         <= (others => '0');
  props_out.switches          <= (others => '0');
  props_out.memories_length   <= to_ulong(1);
  props_out.memories          <= (others => to_ulong(0));
  props_out.nLEDs             <= (others => '0');
  props_out.UUID              <= metadata_in.UUID;
  props_out.romData           <= metadata_in.romData;
  props_out.sdpDropCount      <= sdp_in.dropCount;
  metadata_out.clk            <= ctl_clk;
  metadata_out.romAddr        <= props_in.romAddr;
  metadata_out.romEn          <= props_in.romData_read;
end rtl;
