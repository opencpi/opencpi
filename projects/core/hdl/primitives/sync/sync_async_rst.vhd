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
---------------------------------------------------------------------------------
--
-- Asynchronous "Reset Synchronizer"
--
-- Description:
--  Level Synchronizer (Input and Output are Level signals)
--  Open-loop solution (i.e. without Feedback Acknowledgement)
-- 
--  Reset assertion is asynchronous, while deassertion is synchronized to the clock.
--  The width of the reset signal is at least (RSTDELAY * dest_clk) period.
--
-- Generics:
--  RSTDELAY : Depth of shift register. Valid range 2 to 10 (default = 2)
--
-- Background:
--  - "Reset Synchronizer" in
--    http://www.sunburst-design.com/papers/CummingsSNUG2002SJ_Resets.pdf.
--  - Matches the performance of Xilinx XPM: xpm_cdc_async_rst module.
--  - VHDL replacement for Blue-Spec generated Verilog module, "SyncResetA.v",
--    with the execption that it holds the output active for 1 less clock cycle.
--
---------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity sync_async_rst is
  generic (
    RSTDELAY : integer := 2             --Width of reset shift reg
    );
  port (
    src_arst  : in  std_logic;
    dest_clk  : in  std_logic;
    dest_arst : out std_logic
    );
end entity sync_async_rst;

architecture rtl of sync_async_rst is

  signal s_reset_hold : std_logic_vector(RSTDELAY-1 downto 0) := (others => '0');  

begin

  assert (RSTDELAY >= 2 and RSTDELAY <= 10) report "Out of valid range (2-10) RSTDELAY = " & integer'image(RSTDELAY) severity failure;
  
  dest_arst <= s_reset_hold(s_reset_hold'length-1);  

  sync : process (dest_clk, src_arst)
  begin
    if src_arst = '1' then              --async assert of output
      s_reset_hold <= (others => '1');
    elsif rising_edge(dest_clk) then    --sync deassert of output
      s_reset_hold <= s_reset_hold(s_reset_hold'length-2 downto 0) & '0';      
    end if;
  end process;

end architecture rtl;
