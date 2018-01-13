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

-- non-vendor-specific "input buffer" (needed for sim platforms), supports both
-- single ended and differential iostandards
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.types.all;
entity BUFFER_IN_N is
  generic (width        :     natural;
           STANDARD     :     iostandard_t := UNSPECIFIED;
           DIFFERENTIAL :     boolean; -- only used if IOSTANDARD is UNSPECIFIED
           GLOBAL_CLOCK :     boolean       := FALSE);
  port (   I            : in  std_logic_vector(width-1 downto 0);
           IBAR         : in  std_logic_vector(width-1 downto 0) := (others => 'X'); -- only used if relevant to IOSTANDARD
           O            : out std_logic_vector(width-1 downto 0));
end entity BUFFER_IN_N;
architecture rtl of BUFFER_IN_N is
begin
  O <= I;
end rtl;

