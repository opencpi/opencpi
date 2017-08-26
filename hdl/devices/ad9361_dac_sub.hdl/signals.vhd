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

library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.util.all;
entity signals is
  generic (DIFFERENTIAL : boolean);
  port (w_TX_FRAME_P : in  std_logic;
        w_FB_CLK_P   : in  std_logic;
        TX_FRAME_P   : out std_logic;
        TX_FRAME_N   : out std_logic;
        FB_CLK_P     : out std_logic;
        FB_CLK_N     : out std_logic);
end entity signals;
architecture rtl of signals is
begin

  -- device worker signal TX_FRAME_P (output)
  TX_FRAME_P_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => DIFFERENTIAL)
    port map(I    => w_TX_FRAME_P,
             O    => TX_FRAME_P,
             OBAR => TX_FRAME_N);

  -- device worker signal FB_CLK_P (output)
  FB_CLK_P_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => DIFFERENTIAL)
    port map(I    => w_FB_CLK_P,
             O    => FB_CLK_P,
             OBAR => FB_CLK_N);

end rtl;

