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

entity sync_bit is
  generic (
    N    : integer := 4;
    IREG : std_logic := '1');
  port (
    src_clk    : in  std_logic;
    src_rst    : in  std_logic;
    src_pulse  : in  std_logic;
    dest_clk   : in  std_logic;
    dest_rst   : in  std_logic;
    dest_pulse : out std_logic);
end entity sync_bit;

architecture rtl of sync_bit is
begin

  xpm_cdc_bit_inst : xpm_cdc_bit
    generic map (
      DEST_SYNC_FF   => N,
      INIT_SYNC_FF   => 0,
      SRC_INPUT_REG  => to_integer(IREG),
      SIM_ASSERT_CHK => 0
      )
    port map (
      src_clk    => src_clk,
      src_pulse  => src_pulse,
      dest_clk   => dest_clk,
      dest_pulse => dest_pulse
      );

end architecture rtl;
