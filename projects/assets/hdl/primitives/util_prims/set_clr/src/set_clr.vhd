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

library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
entity set_clr is
  port (clk : in std_logic;
        rst : in std_logic;   -- active-high reset
        set : in std_logic;   -- set q w/ zero latency
        clr : in std_logic;   -- clear q w/ zero latency (override set)
        q   : out std_logic;  -- forced low one clock after rising rst
        q_r : out std_logic); -- q delayed by one clock cycle,
                              -- forced low one clock after rising rst
end entity set_clr;
architecture rtl of set_clr is
  signal tmp   : std_logic := '0';
  signal tmp_r : std_logic := '0';
begin

  tmp_reg : process(clk)
  begin
    if rising_edge(clk) then
      if rst = '1' then
        tmp_r <= '0';
      else
        tmp_r <= tmp;
      end if;
    end if;
  end process;

  tmp <= (set or tmp_r) and (not clr);

  q   <= tmp;
  q_r <= tmp_r;

end rtl;
