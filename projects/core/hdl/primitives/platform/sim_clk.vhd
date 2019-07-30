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

-- The sim clock/reset generator

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity sim_clk is
  generic (frequency : real := 100000000.0;
           offset    : natural := 0);
  port(
    clk   : out std_logic := '0';
    reset : out std_logic := '1');
end sim_clk;
architecture rtl of sim_clk is
  constant period_ns        : natural := natural(1000000000.0/frequency);
  constant clk_half_period  : time := (period_ns/2) * 1 ns;       -- 100 mhz
  constant reset_clocks     : natural := 17;
  signal myclk              : std_logic := '0';
  signal myreset            : std_logic := '1';
  signal reset_count        : unsigned(7 downto 0) := (others => '0');
begin
  clk   <= myclk;
  reset <= myreset;
  -- generate a clock
  clock : process
  begin
    myclk <= '0';
    wait for offset * 1 ns;
    myclk <= '1';
    wait for clk_half_period;
    myclk <= '0';
    wait for clk_half_period - offset * 1 ns;
  end process;

  -- generate a reset for some number of clocks
  work : process(myclk)
  begin
    if rising_edge(myclk) then
      if myreset = '1' then
        if reset_count < reset_clocks then
          reset_count <= reset_count + 1;
          -- report "half: " & integer'image(integer(frequency)) & " " & integer'image(period_ns);
        else
          myreset <= '0';
        end if;
      end if;
    end if;
  end process;
end architecture rtl;
