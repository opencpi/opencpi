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

entity sync_async_rst is
  generic (
    RSTDELAY : integer := 2             --Width of reset shift reg
    );
  port (
    src_arst  : in  std_logic;
    dest_clk  : in  std_logic;
    dest_arst : out std_logic
    );
end entity sync_async_rst;

architecture rtl of sync_async_rst is
begin

  xpm_cdc_async_rst_inst : xpm_cdc_async_rst
    generic map(
      DEST_SYNC_FF    => RSTDELAY,      --range:2-10
      RST_ACTIVE_HIGH => 1              --0=activelowreset,1=activehighreset
      )
    port map(
      src_arst  => src_arst,
      dest_clk  => dest_clk,
      dest_arst => dest_arst
      );

end architecture rtl;
