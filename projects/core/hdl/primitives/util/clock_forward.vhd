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

library IEEE;
use IEEE.std_logic_1164.all, IEEE.numeric_std.all;

entity clock_forward is
  generic (
    INVERT_CLOCK : boolean := false;
    SINGLE_ENDED : boolean := true
  );
  port (
    RST       : in  std_logic;
    CLK_IN    : in  std_logic;
    CLK_OUT_P : out std_logic;
    CLK_OUT_N : out std_logic
  );
end entity clock_forward;

architecture rtl of clock_forward is

  signal clk_fwd : std_logic;

begin

  clk_fwd   <= CLK_IN when (INVERT_CLOCK = false) else not(CLK_IN);
  CLK_OUT_P <= clk_fwd;
  CLK_OUT_N <= not(clk_fwd);

end rtl;
