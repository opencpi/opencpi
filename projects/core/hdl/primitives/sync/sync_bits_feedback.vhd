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
-- Multi-Bit Register Synchronizer with Feedback
--
-- Description:
--  Closed-loop solution (i.e. with Feedback)
--  Uses a Pulse (Toggle) Synchronizer with Feedback to CDC a multi-bit data
--  bus of signals. Provides an "automatic" feedback path that assumes that
--  the receiving clock domain will always be ready for the next data word
--  synchronized through an MCP formulation[1].
--
--  Synchronizing Slow Signals into Fast Clock Domain:
--   - Per functionality of sync_pulse_handshake.vhd
--  Synchronizing Fast Signals into Slow Clock Domain:
--   - Per functionality of sync_pulse_handshake.vhd
--
-- Generics:
--  WIDTH  : width of register to sync, (must be >= 1 (default = 1))
--
-- Latency:
--  src_rdy   : 3 src_clk + 3 dest_clk
--  dest_out  : 3 dest_clk for destination side to see data
--
-- Background:
--  NOTE: VHDL replacement for Blue-Spec generated Verilog module, "SyncRegister.v".
--
--  [1] The below description is from the following link:
--  http://www.sunburst-design.com/papers/CummingsSNUG2008Boston_CDC.pdf
--  "
--  Multi-Cycle Path (MCP) formulation
--  An MCP formulation refers to sending unsynchronized data to a receiving
--  clock domain paired with a synchronized control signal. The data and
--  control signals are sent simultaneously allowing the data to setup on
--  the inputs of the destination register while the control signal is
--  synchronized for two receiving clock cycles before it arrives at the
--  load input of the destination register
--
--  Advantages:
--  (1) The sending clock domain is not required to calculate the appropriate
--  pulse width to send between clock domains.
--  (2) The sending clock domain is only required to toggle an enable into the
--  receiving clock domain to indicate that data has been passed and is ready
--  to be loaded. The enable signal is not required to return to its initial
--  logic level.
--
--  This strategy passes multiple CDC signals without synchronization, and
--  simultaneously passes asynchronized enable signal to the receiving clock
--  domain. The receiving clock domain is not allowed to sample the multi-bit
--  CDC signals until the synchronized enable passes through synchronization
--  and arrives at the receiving register.
--
--  This strategy is called a Multi-Cycle Path Formulation[8] due to the fact
--  that the unsynchronized data word is passed directly to the receiving clock
--  domain and held for multiple receiving clock cycles, allowing an enable
--  signal to be synchronized and recognized into the receiving clock domain
--  before permitting the unsynchronized data word to change.
--
--  Because the unsynchronized data is passed and held stable for multiple
--  clock cycles before being sampled, there is no danger that the sampled
--  value will go metastable.
--  "
-- 
--  The below description is from the following link:
--  Similar to the circuit discussed in http://www.verilogpro.com/clock-domain-crossing-part-2/
--  "
--  The multi-bit data signal passes straight from source (clock) flip-flop to
--  destination (clock) flip-flop and to avoid problems with synchronizing multiple
--  bits, a single control signal is synchronized to allow time for the multi-bit
--  data to settle from possible metastable state. The load pulse from the source
--  clock domain first gets converted into a toggle. The toggle is synchronized
--  across the clock domain crossing (CDC), then gets converted back to a load
--  pulse in the destination clock domain. Finally that load pulse is used to load
--  the multi-bit data signal into flip-flops in the destination clock domain.
--  "
---------------------------------------------------------------------------------

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

entity sync_bits_feedback is
  generic (
    WIDTH : positive := 1);
  port (
    src_clk   : in  std_logic;
    src_rst   : in  std_logic;
    src_en    : in  std_logic;
    src_rdy   : out std_logic;
    src_data  : in  std_logic_vector(WIDTH-1 downto 0);
    dest_clk  : in  std_logic;
    dest_rst  : in  std_logic;
    dest_data : out std_logic_vector(WIDTH-1 downto 0)
    );
end entity sync_bits_feedback;

architecture rtl of sync_bits_feedback is

  signal s_src_data     : std_logic_vector(WIDTH-1 downto 0);
  signal s_dest_load_en : std_logic;

begin

  src_ireg : process(src_clk)
  begin
    if (rising_edge(src_clk)) then
      if (src_rst = '1') then
        s_src_data <= (others => '0');
      elsif (src_en = '1') then
        s_src_data <= src_data;
      end if;
    end if;
  end process;

  -- Transfer the data to destination domain when s_dest_load_en is asserted.
  -- Setup and hold time are assured since at least 2 dest_clk occured since
  -- s_src_data have been written.
  dest_oreg : process(dest_clk)
  begin
    if (rising_edge(dest_clk)) then
      if (src_rst = '1') then
        dest_data <= (others => '0');
      elsif (s_dest_load_en = '1') then
        dest_data <= s_src_data;
      end if;
    end if;
  end process;

  -- Synchronize enable pulse generation with feedback
  sync_en_puls_w_fdbk : entity work.sync_pulse_handshake
    port map(
      src_clk    => src_clk,
      src_rst    => src_rst,
      src_pulse  => src_en,
      src_rdy    => src_rdy,
      dest_clk   => dest_clk,
      dest_rst   => dest_rst,
      dest_pulse => s_dest_load_en
      );

end architecture rtl;
