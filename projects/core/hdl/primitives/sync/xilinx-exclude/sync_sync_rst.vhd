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

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library xpm;
use xpm.vcomponents.all;

entity sync_sync_rst is
  generic (
    RSTDELAY : integer := 2
    );
  port (
    src_rst  : in  std_logic;
    dest_clk : in  std_logic;
    dest_rst : out std_logic
    );
end entity sync_sync_rst;

architecture rtl of sync_sync_rst is
begin

  xpm_cdc_sync_rst_inst : xpm_cdc_sync_rst
    generic map(
      DEST_SYNC_FF   => RSTDELAY,       --range:2-10
      INIT           => 1,  --0=initializesynchronizationregistersto0,--1=initializesynchronizationregistersto1
      SIM_ASSERT_CHK => 0  --0=disablesimulationmessages,1=enablesimulationmessages
      )
    port map(
      src_arst  => src_arst,
      dest_clk  => dest_clk,
      dest_arst => dest_arst
      );

end architecture rtl;
