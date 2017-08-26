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
entity signals_lvds is
  port (w_DATA_CLK_P : out std_logic;
        w_P0_D       : in  std_logic_vector(11 downto 0);
        w_P1_D       : out std_logic_vector(11 downto 0);
        DATA_CLK_P   : in  std_logic;
        DATA_CLK_N   : in  std_logic;
        P0_D         : out std_logic_vector(11 downto 0);
        P1_D         : in  std_logic_vector(11 downto 0));
end entity signals_lvds;
architecture rtl of signals_lvds is
begin

  -- device worker signal DATA_CLK_(input)
  DATA_CLK_P_buffer : BUFFER_IN_1
    generic map(DIFFERENTIAL => true)
    port map(I    => DATA_CLK_P,
             IBAR => DATA_CLK_N,
             O    => w_DATA_CLK_P);

  -- device worker signal P0_D (output)
  P0_D_buffer_loop : for idx in ((P0_D'length/2)-1) downto 0 generate
    P0_D_buffer : BUFFER_OUT_1
      generic map(DIFFERENTIAL => true)
      port map(I    => w_P0_D((idx+1)*2-1),
               O    => P0_D((idx+1)*2-1),
               OBAR => P0_D((idx+1)*2-2));
  end generate;

  -- device worker signal P1_D (input)
  P1_D_buffer_loop : for idx in ((P1_D'length/2)-1) downto 0 generate
    P1_D_buffer : BUFFER_IN_1
      generic map(DIFFERENTIAL => true)
      port map(I    => P1_D((idx+1)*2-1),
               IBAR => P1_D((idx+1)*2-2),
               O    => w_P1_D((idx+1)*2-1));
  end generate;

end rtl;

