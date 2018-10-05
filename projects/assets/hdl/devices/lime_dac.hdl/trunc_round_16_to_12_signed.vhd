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

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity trunc_round_16_to_12_signed is
  port(
    DIN  : in  std_logic_vector(15 downto 0); -- treated as signed Q0.15
    DOUT : out std_logic_vector(11 downto 0)); -- treated as signed Q0.11
end trunc_round_16_to_12_signed;
architecture rtl of trunc_round_16_to_12_signed is

  signal frac : std_logic_vector(3 downto 0) := (others => '0');
  signal intg : std_logic_vector(11 downto 0) := (others => '0');
  signal lsb  : std_logic := '0';
  signal inc  : std_logic := '0';

  -- indicates that an round up would cause an overflow
  signal will_overflow : std_logic := '0';

  signal tmp           : std_logic_vector(intg'range) := (others => '0');

begin

  frac <= DIN(3 downto 0);
  intg <= DIN(15 downto 4);
  lsb  <= DIN(4);

  -- will overflow logic would change if DIN were unsigned
  will_overflow <= (not intg(11)) and
                   intg(10) and
                   intg(9) and
                   intg(8) and
                   intg(7) and
                   intg(6) and
                   intg(5) and
                   intg(4) and
                   intg(3) and
                   intg(2) and
                   intg(1) and
                   intg(0);

  inc <= frac(frac'left) and (not will_overflow);

  -- TODO / FIXME add generic which allows for pipelining

  -- implement the round up by adding one (works for both signed and unsigned
  -- input data)
  DOUT <= std_logic_vector(unsigned(intg)+1) when inc = '1' else intg;

end;
