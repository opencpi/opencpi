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

-- Altera equivalent of global clock buffer from Example 5 on page 32 of https://www.altera.com/content/dam/altera-www/global/en_US/pdfs/literature/an/an307.pdf
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
entity buffer_clock_global is
  port (clk          : in  std_logic;
        clk_buffered : out std_logic);
end entity buffer_clock_global;
architecture rtl of buffer_clock_global is
begin
  clk_buffered <= clk;
end rtl;

