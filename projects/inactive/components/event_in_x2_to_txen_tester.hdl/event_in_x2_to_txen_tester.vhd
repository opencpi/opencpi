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

-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Jun 28 13:23:40 2018 EDT
-- BASED ON THE FILE: event_in_x2_to_txen_tester.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: event_in_x2_to_txen_tester

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of event_in_x2_to_txen_tester_worker is

  -- mandatory input port logic, don't touch this
  signal event_in_0_out_take            : std_logic := '0';
  signal ready_for_event_in_0_port_data : std_logic := '0';

  -- mandatory input port logic, don't touch this
  signal event_in_1_out_take            : std_logic := '0';
  signal ready_for_event_in_1_port_data : std_logic := '0';

  signal event_in_0_opcode_on_off : std_logic := '0';
  signal event_in_1_opcode_on_off : std_logic := '0';

  signal txon_pulse_0         : std_logic := '0';
  signal txoff_pulse_0        : std_logic := '0';
  signal txon_pulse_1         : std_logic := '0';
  signal txoff_pulse_1        : std_logic := '0';
  signal event_in_connected_0 : std_logic := '0';
  signal is_operating_0       : std_logic := '0';
  signal event_in_connected_1 : std_logic := '0';
  signal is_operating_1       : std_logic := '0';
begin

  -- ready_for_event_in_1_port_data MUST have the same clock latency as the latency
  -- between event_in_1 port's data and any associated output port's data)
  event_in_1_out.take <= event_in_1_out_take;
  event_in_1_out_take <= ctl_in.is_operating and event_in_1_in.ready and
                       ready_for_event_in_1_port_data; -- this applies backpressure

  -- ready_for_event_in_0_port_data MUST have the same clock latency as the latency
  -- between event_in_0 port's data and any associated output port's data)
  event_in_0_out.take <= event_in_0_out_take;
  event_in_0_out_take <= ctl_in.is_operating and event_in_0_in.ready and
                       ready_for_event_in_0_port_data; -- this applies backpressure

  -- no associated output port or other backpressure, so we are always ready for
  -- data 
  ready_for_event_in_0_port_data <= '1';
  ready_for_event_in_1_port_data <= '1';

  event_in_0_opcode_on_off <= '1' when (event_in_0_in.opcode = tx_event_txOn_op_e) else '0';
  event_in_1_opcode_on_off <= '1' when (event_in_1_in.opcode = tx_event_txOn_op_e) else '0';

  event_in_to_txen_0 : entity work.event_in_to_txen
    port map (ctl_in_clk             => ctl_in.clk,
              ctl_in_reset           => ctl_in.reset,
              ctl_in_is_operating    => ctl_in.is_operating,
              event_in_in_reset      => event_in_0_in.reset,
              event_in_in_som        => '1', -- event_in_0_in.som,
              event_in_in_valid      => '0', -- event_in_0_in.valid,
              event_in_in_eom        => '1', -- event_in_0_in.eom,
              event_in_in_ready      => event_in_0_in.ready,
              event_in_out_take      => event_in_0_out_take,
              event_in_opcode_on_off => event_in_0_opcode_on_off,
              txon_pulse             => txon_pulse_0,
              txoff_pulse            => txoff_pulse_0,
              txen                   => open,
              event_in_connected     => event_in_connected_0,
              is_operating           => is_operating_0);

  event_in_to_txen_1 : entity work.event_in_to_txen
    port map (ctl_in_clk             => ctl_in.clk,
              ctl_in_reset           => ctl_in.reset,
              ctl_in_is_operating    => ctl_in.is_operating,
              event_in_in_reset      => event_in_1_in.reset,
              event_in_in_som        => '1', -- event_in_1_in.som,
              event_in_in_valid      => '0', -- event_in_1_in.valid,
              event_in_in_eom        => '1', -- event_in_1_in.eom,
              event_in_in_ready      => event_in_1_in.ready,
              event_in_out_take      => event_in_1_out_take,
              event_in_opcode_on_off => event_in_1_opcode_on_off,
              txon_pulse             => txon_pulse_1,
              txoff_pulse            => txoff_pulse_1,
              txen                   => open,
              event_in_connected     => event_in_connected_1,
              is_operating           => is_operating_1);

  -- Skeleton assignments for event_in_x2_to_txen_tester's volatile properties
  event_in_x2_to_txen : entity work.event_in_x2_to_txen
    port map (ctl_in_clk           => ctl_in.clk,
              ctl_in_reset         => ctl_in.reset,
              txon_pulse_0         => txon_pulse_0,
              txoff_pulse_0        => txoff_pulse_0,
              event_in_connected_0 => event_in_connected_0,
              is_operating_0       => is_operating_0,
              txon_pulse_1         => txon_pulse_1,
              txoff_pulse_1        => txoff_pulse_1,
              event_in_connected_1 => event_in_connected_1,
              is_operating_1       => is_operating_1,
              txen                 => props_out.txen);

  ctl_out.done <= '1';

end rtl;
