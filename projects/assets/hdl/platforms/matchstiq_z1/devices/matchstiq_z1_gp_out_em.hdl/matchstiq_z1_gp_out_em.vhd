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

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of matchstiq_z1_gp_out_em_worker is

-------------------------------------------------------------------------------
-- Matchstiq-Z1 GP Out Emulator
-------------------------------------------------------------------------------
--
-- Description:
--
-- The Matchstiq-Z1 GP Out Emulator uses an edge detector to detect the
-- rising edge of the emulator's input pins and then sends the outputs of the
-- edge detector as a message.

-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Constants
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
constant c_num_inputs          : integer := 3;
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for emulator logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal s_msg_ctr               : ulong_t := (others => '0');
signal s_done                  : std_logic := '0'; -- Done sending messages
signal s_enable                : std_logic := '0';
signal s_gpio                  : std_logic_vector(c_num_inputs-1 downto 0) := (others => '0'); -- Bus of GPIO signals.
signal s_rising_pulse          : std_logic_vector(c_num_inputs-1 downto 0) := (others => '0'); -- Output of edge detector rising pulse
signal s_dev_mask              : std_logic := '0'; -- Controls dev_gp_out.mask
signal s_dev_data              : std_logic := '0'; -- Control dev_gp_out.data
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Signals for combinatorial logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal s_som                   : std_logic := '0'; -- Used for combinatorial logic to drive out_som
signal s_eom                   : std_logic := '0'; -- Drives out_eom
signal s_zlm                   : std_logic := '0'; -- Used for combinatorial logic to drive out_som and out_valid
signal s_last_sample           : std_logic := '0'; -- Since the edge detector has 1 clock cycle delay, this signal is used to drive s_enable when dev_gp_em_in.enable
                                                   -- goes low on last sample, so that the last sample can be sent
signal s_ready                 : std_logic := '0'; -- Used for logic when ready to send messages
signal s_enable_r              : std_logic := '0'; -- Registered version of s_enable
signal s_data_ready            : std_logic := '0'; -- Used for combinatorial logic to drive data_ready_for_out_port
signal s_dev_ready             : std_logic := '0'; -- Used for combinatorial logic to drive data_ready_for_out_port when driving dev_gp_out signals
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- Mandatory output port logic
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
signal data_ready_for_out_port : std_logic := '0';
signal out_meta_is_reserved    : std_logic := '0';
signal out_som                 : std_logic := '0';
signal out_eom                 : std_logic := '0';
signal out_valid               : std_logic := '0';

begin

 -- Concatenate the signals together for the edge_detector input port
 s_gpio <= gpio3 & gpio2 & gpio1;

 -- edge detect the input signal
 edge_detect_gen : for i in 0 to c_num_inputs-1 generate
    edge_detector : misc_prims.misc_prims.edge_detector
      port map (
        clk    => ctl_in.clk,
        reset  => ctl_in.reset,
        din    => s_gpio(i),
        rising_pulse  => s_rising_pulse(i),
        falling_pulse => open);
 end generate edge_detect_gen;

  -- Mandatory output port logic, (note that
  -- data_ready_for_out_port MUST be clock-aligned with out_out.data)
  -- (note that reserved messages will be DROPPED ON THE FLOOR)
  out_out.give <= ctl_in.is_operating and out_in.ready and
                (not out_meta_is_reserved) and data_ready_for_out_port;


  out_meta_is_reserved <= (not out_som) and (not out_valid) and (not out_eom);
  out_out.som   <= out_som;
  out_out.eom   <= out_eom;
  out_out.valid <= out_valid;
  out_out.opcode <= (others => '0');

  s_dev_ready <= ctl_in.is_operating and out_in.ready and props_in.input_mask(2);

  data_ready_for_out_port <= s_ready and s_data_ready;

  -- Used to control when to start sending messages and to drive data_ready_for_out_port
  s_enable <= ctl_in.is_operating and out_in.ready and (dev_gp_em_in.enable or s_last_sample or s_dev_ready);

  -- If using property mask_data then used registered version of s_enable.
  -- Using registered version because s_enable pulses for one clock cycle and
  -- the registered version is coincident with when the edge_detector's new output
  -- is available. Otherwise use the non registered version of s_enable
  s_data_ready <= s_enable_r when (props_in.input_mask(0) = '1') else
                  s_enable;

  out_som <=  s_som or s_zlm;

  out_eom <= s_eom;

  out_valid <= ctl_in.is_operating and out_in.ready and
                             data_ready_for_out_port and not s_zlm;

  out_out.byte_enable <= (others => '1');
  out_out.data <= x"0000000" & '0' & s_rising_pulse;

  dev_gp_out.mask <= s_dev_mask;
  dev_gp_out.data <= s_dev_data;

  -- Set devsignal data and mask
  -- For this test it just toggles s_dev_data on and off
  toggle_dev : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        s_dev_data <= '0';
        s_dev_mask <= '0';
      elsif (s_dev_ready = '1') then
        s_dev_data <= not s_dev_data;
        s_dev_mask <= '1';
      end if;
    end if;
  end process toggle_dev;

  -- Register s_enable
  set_reg : process(ctl_in.clk)
  begin
  if rising_edge(ctl_in.clk) then
    if (ctl_in.reset = '1') then
      s_enable_r <= '0';
    else
      s_enable_r <= s_enable;
    end if;
  end if;
end process set_reg;


  -- Process for sending messages
  msg_counter : process (ctl_in.clk)
    begin
      if rising_edge(ctl_in.clk) then
        if ctl_in.reset = '1' then
          s_msg_ctr <= (others=>'0');
          s_som <= '0';
          s_eom <= '0';
          s_done <= '0';
          s_zlm <= '0';
          s_ready <= '0';
          s_last_sample <= '0';
        -- Logic for sending messages when numOutputSamples > 1
        elsif (s_enable = '1' and s_done = '0' and props_in.numOutputSamples > 1) then
            -- Since the edge detector has 1 clock cycle delay, wait a clock cycle after s_enable goes high
            -- to start sending messages
            if (s_msg_ctr = 0) then
              s_ready <= '1';
              s_som <= '1';
            end if;
            -- Next clock cycle will be last sample so set s_eom and s_last_sample now
            if (s_msg_ctr = props_in.numOutputSamples-2) then
              s_eom <= '1';
              s_last_sample <= '1';
            end if;
            -- Finished sending message. Get ready to send ZLM
            if (s_msg_ctr = props_in.numOutputSamples-1) then
              s_zlm <= '1';
              s_done <= '1';
            -- Start message counter
            elsif (s_ready = '1') then
              s_som <= '0';
              s_msg_ctr <= s_msg_ctr + 1;
            end if;
        -- Logic for single word messages
        elsif (s_enable = '1' and s_done = '0' and props_in.numOutputSamples = 1) then
            if (s_msg_ctr = 0) then
              s_ready <= '1';
              s_som <= '1';
              s_eom <= '1';
              s_last_sample <= '1';
            end if;
            if (s_ready = '1') then
              s_som <= '0';
              s_msg_ctr <= s_msg_ctr + 1;
              s_zlm <= '1';
              s_done <= '1';
            end if;
        end if;
        -- Finished message and gave ZLM so s_ready and s_last_sample should go low
        if (s_done = '1') then
          s_ready <= '0';
          s_last_sample <= '0';
        end if;
      end if;
  end process msg_counter;


end rtl;
