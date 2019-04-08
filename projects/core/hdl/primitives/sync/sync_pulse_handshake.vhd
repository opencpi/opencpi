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
-- Single Bit - Pulse (Toggle) Synchronizer with Handshake
--
-- Description:
--  Pulse (Toggle) Synchronizer with Handshake (Input and Output are Pulses)
--  Closed-loop solution (i.e. with Feedback Acknowledgement)
--
--  In the Handshake based Pulse Synchronizer, synchronization of a pulse generated
--  into source clock domain is guaranteed into destination clock domain.
--  However, back to back (i.e. without proper inactive clock cycles) pulses are
--  not supported. A 'ready' signal is provides to the to indicate when a new pulse
--  may be inputted.
--
--  For NORMAL operation, the input signal must be sampled two or more times
--  by the destination clock.
--
--  To achieve this requirement when synchronizing from:
--  a) Slow Signals into Fast Clock Domain:
--     - Must obey the src_rdy signal.
--     - The destination clock must be >= 2x frequency of the source clock.
--  b) Fast Signals into Slow Clock Domain:
--     - Must obey the src_rdy signal.
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
-- Latency:
--  src_rdy  : Cleared 1 src_clk and Through(Set) (N src_clk and N dest_clk delays)
--  dest_pulse : 1 src_clk and N dest_clk delays
--
-- Background:
--  VHDL replacement for core/hdl/primitives/bsv/imports/SyncHandshake.v
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_pulse_handshake is
  generic (
    N : natural := 2);
  port (
    src_clk    : in  std_logic;
    src_rst    : in  std_logic;
    src_pulse  : in  std_logic;
    src_rdy    : out std_logic;
    dest_clk   : in  std_logic;
    dest_rst   : in  std_logic;
    dest_pulse : out std_logic);
end entity sync_pulse_handshake;

architecture rtl of sync_pulse_handshake is

  signal s_src_ack    : std_logic;
  signal s_src_toggle : std_logic;
  signal s_dest_reg0  : std_logic;
  signal s_dest_reg1  : std_logic;

begin

  -- Indicates when circuit is 'ready' to accept new input
  src_rdy <= s_src_ack xnor s_src_toggle;

  -- Level detection used to generate output
  dest_pulse <= s_dest_reg0 xor s_dest_reg1;

  input_enabled_toggle : process(src_clk)
  begin
    if (rising_edge(src_clk)) then
      if (src_rst = '1') then
        s_src_toggle <= '0';
      elsif (src_pulse = '1') then
        s_src_toggle <= not s_src_toggle;
      end if;
    end if;
  end process;

  nff_synchronizer : entity work.sync_bit
    generic map (
      N    => N,
      IREG => '0')
    port map (
      src_clk  => '0',
      src_rst  => '0',
      src_en   => '0',
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

  acknowledge_nff_synchronizer : entity work.sync_bit
    generic map (
      N         => N,
      IREG      => '0',
      RST_LEVEL => '1')
    port map (
      src_clk  => '0',
      src_rst  => '0',
      src_en   => '0',
      src_in   => s_dest_reg0,
      dest_clk => src_clk,
      dest_rst => src_rst,
      dest_out => s_src_ack);

end architecture rtl;
