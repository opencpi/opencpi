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
-- edge detector
-------------------------------------------------------------------------------
--
-- Description:
--
-- The edge detector detects rising and falling edge of an input signal
-- on the rising edge of the clock.
--
-- The output of the edge detector has latency of once clock cycle.
--
-- Can use enable to control when to do edge detection


library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;

entity edge_detector is
  generic(
    width : positive := 1); -- width of in port and out port
  port(
    clk               : in  std_logic;  -- input clock
    reset             : in  std_logic;  -- reset (active-high)
    enable            : in  std_logic; -- controls when to do edge detection
    input             : in  std_logic_vector(width-1 downto 0);  -- input signal
    rising_pulse      : out std_logic_vector(width-1 downto 0);  -- rising edge pulse
    falling_pulse     : out std_logic_vector(width-1 downto 0)); -- falling edge pulse
end entity edge_detector;

architecture rtl of edge_detector is

signal s_input_r1     : std_logic_vector(width-1 downto 0) := (others => '0'); -- First stage register for edge detection
signal s_input_r2     : std_logic_vector(width-1 downto 0) := (others => '0'); -- Second stage register for edge detection

begin

edgeDetect : process (clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        s_input_r1 <= input;
        s_input_r2 <= input;
      elsif enable = '1' then
        s_input_r1 <= input;
        s_input_r2 <= s_input_r1;
      end if;
    end if;
end process edgeDetect;


rising_pulse <= s_input_r1 and not s_input_r2 when (enable = '1') else (others =>'0');
falling_pulse <= (not s_input_r1) and s_input_r2 when (enable = '1') else (others =>'0');

end rtl;
