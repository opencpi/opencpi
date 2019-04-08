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

-------------------------------------------------------------------------------
-- Testbench for sync_srl_fifo_oreg.vhd
-------------------------------------------------------------------------------
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity UUT_tb is
end UUT_tb;

architecture rtl of UUT_tb is

  constant CLK_PER_1  : time     := 10 ns;
  constant CLK_PER_2  : time     := (10 ns)/1.5;
  constant CLK_PER_3  : time     := (10 ns)/2;
  constant CLK_PER_4  : time     := (10 ns)/3.3;
  constant DATA_WIDTH : positive := 16;

  --------------------------------------------------------------------------------
  -- Test Jig declaration
  --------------------------------------------------------------------------------

  component UUT_jig
    generic (
      SRC_FNAME     : string;
      VER_CHK_FNAME : string;
      UUT_NAME      : string;
      TEST_NUM      : integer;
      DATA_WIDTH    : positive;
      CLK_PER_1     : time;
      CLK_PER_2     : time);
  end component;

  --------------------------------------------------------------------------------

begin

  test1 : UUT_jig
    generic map (
      SRC_FNAME     => "../din.txt",
      VER_CHK_FNAME => "../din_chk.txt",
      UUT_NAME      => "sync_srl_fifo_oreg",
      TEST_NUM      => 1,
      DATA_WIDTH    => DATA_WIDTH,
      CLK_PER_1     => CLK_PER_1,
      CLK_PER_2     => CLK_PER_2);

end rtl;
