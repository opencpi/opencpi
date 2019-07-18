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

-- Corrects time_in value by subtracting time_correction in a purely
-- combinatorial fashion. Both time_in and corrected_time are unsigned in
-- order to allow EPOCH format. Note that corrected time may be invalid
-- (indicated by logic-low corrected_time_valid), one cause of which would be
-- overflow.

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;

entity time_corrector is
  generic(
    TIME_WIDTH : positive := 64);
  port(
    time_in              : in  unsigned(TIME_WIDTH-1 downto 0);
    time_in_valid        : in  std_logic;
    time_correction      : in  signed(  TIME_WIDTH-1 downto 0);
    corrected_time       : out unsigned(TIME_WIDTH-1 downto 0);
    corrected_time_valid : out std_logic);
end time_corrector;

architecture rtl of time_corrector is
  signal tmp              : signed(time_in'length+1 downto 0) := (others => '0');
  signal tmp_lower_than_min  : std_logic := '0';
  signal tmp_larger_than_max : std_logic := '0';
  signal tmp_did_overflow    : std_logic := '0';
begin

  tmp <= resize(signed(time_in),         tmp'length) -
         resize(signed(time_correction), tmp'length);

  tmp_lower_than_min  <= tmp(tmp'left); -- sign bit
  tmp_larger_than_max <= tmp(tmp'left-1); -- largest amplitude bit
  tmp_did_overflow <= tmp_lower_than_min or tmp_larger_than_max;

  corrected_time <= unsigned(tmp(tmp'left-2 downto 0));
  corrected_time_valid <= time_in_valid and (not tmp_did_overflow);

end rtl;
