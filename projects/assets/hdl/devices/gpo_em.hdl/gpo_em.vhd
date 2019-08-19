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
-- GPO_EM
-------------------------------------------------------------------------------
--
-- Description:
--
-- The GPO Emulator detects if the state of the GPIO pins have changed
-- and sends the current state of the GPIO pins as a message if there
-- has been a change.

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of worker is


-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Constants
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Data port has same width as property so these constants can be
-- used for indexing data and signals relating to the data port
constant c_NUM_OUTPUTS          : integer := to_integer(NUM_OUTPUTS);
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for emulator logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal s_msg_ctr               : ushort_t := (others => '0');
signal s_data_vld_i            : std_logic := '0';
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for combinatorial logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal s_ready                 : std_logic := '0'; -- Used for logic when ready to send messages
signal s_data_ready            : std_logic := '0'; -- Used for combinatorial logic to drive s_data_ready_for_out_port
signal s_gpo_pin_r             : std_logic_vector(c_NUM_OUTPUTS-1 downto 0) := (others => '0'); -- Registered version of s_gpo_pin
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Mandatory output port logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal s_data_ready_for_out_port  : std_logic := '0';
signal s_out_valid                : std_logic := '0';
signal s_out_eof                  : std_logic := '0';

begin


  out_out.valid <= s_out_valid;
  s_out_valid <= s_data_ready_for_out_port;
  s_data_ready_for_out_port <= s_data_vld_i;

  out_out.eof <= s_out_eof;

  -- Used to control when to start sending messages and to drive s_data_ready_for_out_port
  s_data_vld_i <= out_in.ready and s_data_ready;

  s_data_ready <= '1' when (s_gpo_pin_r /= gpo_pin) else '0';

  output_condition_gen : for ii in 0 to c_NUM_OUTPUTS-1 generate
    out_out.data(ii) <=  gpo_pin(ii);
  end generate output_condition_gen;

  regs: process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      s_gpo_pin_r <= gpo_pin;
    end if;
  end process regs;


  -- Process for sending messages
  msg_counter : process (ctl_in.clk)
    begin
      if rising_edge(ctl_in.clk) then
        if ctl_in.reset = '1' then
          s_msg_ctr <= (others=>'0');
          s_out_eof <= '0';
        elsif (s_data_vld_i = '1' and s_msg_ctr < props_in.numOutputSamples) then
            s_msg_ctr <= s_msg_ctr + 1;
        -- No more samples to send, so send an EOF
        elsif (ctl_in.is_operating = '1' and s_msg_ctr = props_in.numOutputSamples) then
            s_out_eof <= '1';
        end if;
      end if;
  end process msg_counter;

end rtl;
