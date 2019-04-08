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
-- Synchronizes Reset signal to the destination clock domain
--
-- Description:
--  Level Synchronizer (Input and Output are Level signals)
--  Open-loop solution (i.e. without Feedback Acknowledgement)
--
--  Reset assertion is synchronous and deassertion is synchronized to the
--  destination clock. Input data must be sampled two or more times by the
--  destination clock. As long as the input is sample two or more times by
--  the dest_clk, the output width is equal to the number of periods
--  that the input is sampled in the dest_clk, plus 1 depending on when
--  the input data is deasserted relative to the setup/hold requirements
--  of the dest_clk.
--
-- Generics:
--  RSTDELAY : Depth of N-FF Synchronizer. Valid range 2 to 10 (default = 2)
--
-- Background:
--  - Matches the performance of Xilinx XPM: xpm_cdc_async_rst module.
--
---------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity sync_sync_rst is
  generic (
    RSTDELAY : integer := 2
    );
  port (
    src_rst  : in  std_logic;
    dest_clk : in  std_logic;
    dest_rst : out std_logic
    );
end entity sync_sync_rst;

architecture rtl of sync_sync_rst is
begin

  nff_synchronizer : entity work.sync_bit  -- N-FF CDC with input register disable
    generic map (
      N    => RSTDELAY,
      IREG => '0')
    port map (
      src_clk  => '0',
      src_rst  => '0',
      src_en   => '0',
      src_in   => src_rst,
      dest_clk => dest_clk,
      dest_rst => '0',
      dest_out => dest_rst);

end architecture rtl;
