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

library ieee; use ieee.std_logic_1164.all;
library unisim; use unisim.vcomponents.all;
entity TSINOUT_1 is
  generic (DIFFERENTIAL : boolean := false);
  port    (I  : in    std_logic;  -- OUTPUT to PIN when OE = 1
           OE : in    std_logic;                           -- output enable, 1 = enabled
           O  : out   std_logic;  -- INPUT from pin, all the time
           IO : inout std_logic;  -- pin/pad
           IOBAR : inout std_logic := 'Z');
end entity TSINOUT_1;
architecture rtl of TSINOUT_1 is
signal NOE : std_logic;
begin
NOE <= NOT(OE);

 single_ended_prim : if DIFFERENTIAL = false generate
  buf : IOBUF
   port map(
    I  => I,
    T  => NOE,
    O  => O,
    IO => IO
    );
 end generate;

 differential_prim : if DIFFERENTIAL = true generate
  buf : IOBUFDS
   port map(
    I  => I,
    T  => NOE,
    O  => O,
    IO => IO,
    IOB => IOBAR
    );
 end generate;

end rtl;
