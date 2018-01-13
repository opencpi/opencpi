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
  
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.all; use ocpi.types.all; use ocpi.util.all;

entity clock_limiter is
  generic(width : natural);
  port(clk      : in std_logic;
       rst      : in bool_t;
       factor   : in unsigned(width-1 downto 0);
       enabled  : in bool_t;
       ready    : out bool_t);
end entity clock_limiter;
architecture rtl of clock_limiter is
  signal ready_r : bool_t;
begin
  ready <= ready_r;
  process(clk) is
    variable pclk_count : unsigned(width-1 downto 0);
  begin
    if rising_edge(clk) then
      if rst or pclk_count >= factor then
        ready_r <= btrue;
        pclk_count := (others => '0');
      elsif not ready_r then
        pclk_count := pclk_count + 1;
      elsif its(enabled) then
        ready_r <= bfalse;
      end if;
    end if;
  end process;
end rtl;
