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

-- non-vendor-specific "output buffer" (needed for sim platforms), supports both
-- single ended and differential iostandards
library IEEE;
use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.types.all;
entity BUFFER_OUT_1 is
  generic (IOSTANDARD   :   iostandard_t := UNSPECIFIED;
           DIFFERENTIAL :   boolean); -- only used if IOSTANDARD is UNSPECIFIED
  port (   I          : in  std_logic;
           O          : out std_logic;
           OBAR       : out std_logic := 'X'); -- only use if relevant to IOSTANDARD
end entity BUFFER_OUT_1;
architecture rtl of BUFFER_OUT_1 is
begin
  O <= I;
end rtl;

