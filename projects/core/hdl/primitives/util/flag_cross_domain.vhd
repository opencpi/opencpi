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

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;

entity flag_cross_domain is
  port (
    clkA         : in  std_logic;
    flagIn_clkA  : in  std_logic;  -- this is a one-clock pulse from the clkA domain
    busy_clkA    : out std_logic;
    clkB         : in  std_logic;
    flagOut_clkB : out std_logic   -- from which we generate a one-clock pulse in clkB domain
    );
end entity flag_cross_domain;

architecture rtl of flag_cross_domain is

  signal flagToggle_clkA : std_logic := '0';
  signal syncA_clkB      : std_logic_vector(2 downto 0) := (others => '0');
  signal syncB_clkA      : std_logic_vector(1 downto 0) := (others => '0');
  signal busy            : std_logic;

begin

  -- when flag is asserted, this signal toggles (clkA domain)
  clkA_proc : process (clkA)
  begin
    if rising_edge(clkA) then
      flagToggle_clkA <= flagToggle_clkA xor (flagIn_clkA and not(busy));
      syncB_clkA      <= syncB_clkA(0) & syncA_clkB(2);
      busy            <= flagToggle_clkA xor syncB_clkA(1);
    end if;
  end process clkA_proc;

  -- now we cross the clock domains and create the clkB flag_cross_domain
  clkB_proc : process (clkB)
  begin
    if rising_edge(clkB) then
      syncA_clkB   <= syncA_clkB(1 downto 0) & flagToggle_clkA;
      flagOut_clkB <= syncA_clkB(2) xor syncA_clkB(1);
    end if;
  end process clkB_proc;

  busy_clkA <= busy;

end rtl;
