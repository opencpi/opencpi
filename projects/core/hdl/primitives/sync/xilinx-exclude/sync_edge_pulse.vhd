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
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_edge_pulse is
  generic (
    N    : natural := 2;
    OREG : std_logic := '0');  
  port (
    src_clk    : in  std_logic;
    src_rst    : in  std_logic;
    src_pulse  : in  std_logic;
    dest_clk   : in  std_logic;
    dest_rst   : in  std_logic;
    dest_pulse : out std_logic);
end entity sync_edge_pulse;

architecture rtl of sync_edge_pulse is
begin

  xpm_cdc_pulse_inst : xpm_cdc_pulse
    generic map (
      DEST_SYNC_FF   => N,
      REG_OUTPUT     => OREG,
      RST_USED       => '1',
      SIM_ASSERT_CHK => '0'
      )
    port map (
      src_clk    => src_clk,
      src_rst    => src_rst,
      src_pulse  => src_pulse,
      dest_clk   => dest_clk,
      dest_rst   => dest_rst,
      dest_pulse => dest_pulse
      );

end architecture rtl;
