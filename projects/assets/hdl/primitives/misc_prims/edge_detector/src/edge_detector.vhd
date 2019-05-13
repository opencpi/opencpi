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
-- The edge detector detects rising and falling edge of an input bus
-- on the rising edge of the clock. A sync reset initializes to input,
-- e.g. you won't get a rising_pulse if it has always been high.
--
-- The output of the edge detector has latency of one clock cycle.
--
-- Can use enable to suppress the output buses.
--
-- It should be noted that the output ports can't be left unconnected since they
-- have unconstrained widths.


library IEEE;
use IEEE.std_logic_1164.all;

entity edge_detector is
  port(
    clk               : in  std_logic;  -- input clock
    reset             : in  std_logic;  -- reset (active-high)
    enable            : in  std_logic; -- can be used to suppress output buses
    din               : in  std_logic_vector;  -- input bus
    rising_pulse      : out std_logic_vector;  -- rising edge pulse
    falling_pulse     : out std_logic_vector); -- falling edge pulse
end entity edge_detector;

architecture rtl of edge_detector is

signal s_din_r1     : std_logic_vector(din'range) := (din'range => '0'); -- First stage register for edge detection
signal s_din_r2     : std_logic_vector(din'range) := (din'range => '0'); -- Second stage register for edge detection

begin

edgeDetect : process (clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        s_din_r2 <= din;
      else
        s_din_r2 <= s_din_r1;
      end if;
      s_din_r1 <= din;
    end if;
end process edgeDetect;

rising_pulse <= s_din_r1 and (not s_din_r2) when (enable = '1') else (rising_pulse'range =>'0');
falling_pulse <= (not s_din_r1) and s_din_r2 when (enable = '1') else (falling_pulse'range =>'0');

end rtl;
