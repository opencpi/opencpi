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
-- Single Bit - Pulse (Toggle) Synchronizer without Feedback Acknowledgement
--
-- Description:
--  Pulse (Toggle) Synchronizer (Input and Output are Pulses)
--  Open-loop solution (i.e. without Feedback Acknowledgement)
--
--  The basic function of a Pulse (Toggle) synchronizer is to take a single clockwide
--  pulse from the source clock domain and create a single clockwide pulse in the
--  destination clock domain.
--
--  For NORMAL operation, the input signal must be sampled two or more times
--  by the destination clock.
--
--  To achieve this requirement when synchronizing from:
--  a) Slow Signals into Fast Clock Domain:
--     - The destination clock must be >= 2x frequency of the source clock.
--     - While the src_en is active, this module has the affect of producing a pulse train.
--     - When dest_clk/src_clk >= 2 and src_en = 1,
--      an output pulse of 1 dest_clk will be produced.
--  b) Fast Signals into Slow Clock Domain:
--     - The minimum input signal width must be 2x the period of the destination clock
--       and the input pulses must be separated by 2x src_clk cycles to
--       ensure proper crossing of the CDC boundary.
--
-- Generics:
--  N     : Number of synchronizing FFs. Valid range 2 to 10 (default = 2)
--
-- Latency:
--  dest_pulse: 1 src_clk and N dest_clk delays
--
-- Background:
--  NOTE: VHDL replacement for Blue-Spec generated Verilog module, "SyncPulse.v".
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_pulse is
  generic (
    N : natural := 2);
  port (
    src_clk    : in  std_logic;
    src_rst    : in  std_logic;
    src_en     : in  std_logic;
    dest_clk   : in  std_logic;
    dest_rst   : in  std_logic;
    dest_pulse : out std_logic);
end entity sync_pulse;

architecture rtl of sync_pulse is

  signal s_src_toggle : std_logic;
  signal s_dest_reg0  : std_logic;
  signal s_dest_reg1  : std_logic;

begin

  -- Level detection used to generate output
  dest_pulse <= s_dest_reg0 xor s_dest_reg1;

  input_enabled_toggle : process(src_clk)
  begin
    if (rising_edge(src_clk)) then
      if (src_rst = '1') then
        s_src_toggle <= '0';
      elsif (src_en = '1') then
        s_src_toggle <= not s_src_toggle;
      end if;
    end if;
  end process;

  nff_synchronizer : entity work.sync_bit  -- N-FF and input register disabled
    generic map (
      N    => N,
      IREG => '0')
    port map (
      src_clk  => '0',
      src_rst  => '0',
      src_en   => src_en,
      src_in   => s_src_toggle,
      dest_clk => dest_clk,
      dest_rst => dest_rst,
      dest_out => s_dest_reg0);

  level_detection_prep_reg : process(dest_clk)
  begin
    if (rising_edge(dest_clk)) then
      if (dest_rst = '1') then
        s_dest_reg1 <= '0';
      else
        s_dest_reg1 <= s_dest_reg0;
      end if;
    end if;
  end process;

end architecture rtl;
