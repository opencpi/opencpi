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
-- Single Bit (N-FF Synchronizer) with Feedback Acknowledgement
--
-- Description:
--  Level Synchronizer (Input and Output are Level signals)
--  Closed-loop solution (i.e. with Feedback Acknowledgement)
--
--  Synchronizes a single bit from the source clock domain to the destination
--  clock domain.
--
--  For NORMAL operation, the input signal must be sampled two or
--  more times by the destination clock.
--  To achieve this requirement when synchronizing from:
--  a) Slow Signals into Fast Clock Domain:
--     - The destination clock must be >= 2x frequency of the source clock.
--  b) Fast Signals into Slow Clock Domain:
--     - The minimum input signal width must be 2x the period of the destination
--       clock and the input pulses must be separated by 2x src_clk cycles to
--       ensure proper crossing of the CDC boundary. Depending on the phase
--       and frequency relationship of the source and destination clocks,
--       the generated pulse in the destination domain may be +1 cycle more
--       than the input pulse.
--
-- Generics:
--  N     : Number of synchronizing FFs. Valid range 2 to 10 (default = 2)
--
-- Ports:
--  src_ack : Acknowledgement (in source domain) that input has crossed CDC.
--
-- Latency:
--  src_ack   : 1+N src_clk and N dest_clk delays
--  dest_out  : 1 src_clk and N dest_clk delays
--
-- Background:
--  Much of the description below is from:
--  http://www.sunburst-design.com/papers/CummingsSNUG2008Boston_CDC.pdf - Section 4.5
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_bit_ack is
  generic (
    N : natural := 2);                  -- Range 2 - 10
  port (
    src_clk  : in  std_logic;
    src_rst  : in  std_logic;
    src_en   : in  std_logic;
    src_in   : in  std_logic;
    src_ack  : out std_logic;
    dest_clk : in  std_logic;
    dest_rst : in  std_logic;           -- optional; if not required tie '0'
    dest_out : out std_logic);
end entity sync_bit_ack;

architecture struct of sync_bit_ack is

  signal s_src_ack  : std_logic;
  signal s_dest_out : std_logic;

begin

  src_ack  <= s_src_ack;
  dest_out <= s_dest_out;

  ireg_nff_synchronizer : entity work.sync_bit  -- N-FF CDC with input register enable
    generic map (
      N    => N,
      IREG => '1')
    port map (
      src_clk  => src_clk,
      src_rst  => src_rst,
      src_en   => src_en,
      src_in   => src_in,
      dest_clk => dest_clk,
      dest_rst => dest_rst,
      dest_out => s_dest_out);

  acknowledge_nff_synchronizer : entity work.sync_bit  -- N-FF CDC with input register disable
    generic map (
      N    => N,
      IREG => '0')
    port map (
      src_clk  => '0',
      src_rst  => '0',
      src_en   => '0',
      src_in   => s_dest_out,
      dest_clk => src_clk,
      dest_rst => src_rst,
      dest_out => s_src_ack);

end architecture struct;
