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
  port (w_SPI_DI     : in  std_logic;
        w_SPI_CLK    : in  std_logic;
        w_SPI_ENB    : in  std_logic;
        w_SPI_DO     : out std_logic;
        w_RESETB     : in  std_logic;
        SPI_DI       : out std_logic;
        SPI_CLK      : out std_logic;
        SPI_ENB      : out std_logic;
        SPI_DO       : in  std_logic;
        RESETB       : out std_logic);
end entity signals;
architecture rtl of signals is
begin

  -- device worker signal SPI_DI (output)
  SPI_DI_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => false)
    port map(I => w_SPI_DI,
             O => SPI_DI);

  -- device worker signal SPI_CLK (output)
  SPI_CLK_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => false)
    port map(I => w_SPI_CLK,
             O => SPI_CLK);

  -- device worker signal SPI_ENB (output)
  SPI_ENB_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => false)
    port map(I => w_SPI_ENB,
             O => SPI_ENB);

  -- device worker signal SPI_DO (input)
  SPI_DO_buffer : BUFFER_IN_1
    generic map(DIFFERENTIAL => false)
    port map(I => SPI_DO,
             O => w_SPI_DO);

  -- device worker signal RESETB (output)
  RESETB_buffer : BUFFER_OUT_1
    generic map(DIFFERENTIAL => false)
    port map(I => w_RESETB,
             O => RESETB);

end rtl;

