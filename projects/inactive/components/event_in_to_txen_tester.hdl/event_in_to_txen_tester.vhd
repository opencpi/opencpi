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

-- THIS FILE WAS ORIGINALLY GENERATED ON Wed Jun 27 18:01:09 2018 EDT
-- BASED ON THE FILE: event_in_to_txen_tester.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: event_in_to_txen_tester

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of event_in_to_txen_tester_worker is

  -- mandatory input port logic, don't touch this
  signal event_in_out_take            : std_logic := '0';
  signal ready_for_event_in_port_data : std_logic := '0';

  signal event_in_opcode_on_off : std_logic := '0';
begin

  -- ready_for_event_in_port_data MUST have the same clock latency as the latency
  -- between in port's data and any associated output port's data)
  event_in_out.take <= event_in_out_take;
  event_in_out_take <= ctl_in.is_operating and event_in_in.ready and
                       ready_for_event_in_port_data; -- this applies backpressure

  -- no associated output port or other backpressure, so we are always ready for
  -- data 
  ready_for_event_in_port_data <= '1';

  event_in_opcode_on_off <= '1' when (event_in_in.opcode = tx_event_txOn_op_e) else '0';

  event_in_to_txen : entity work.event_in_to_txen
    port map (ctl_in_clk             => ctl_in.clk,
              ctl_in_reset           => ctl_in.reset,
              ctl_in_is_operating    => ctl_in.is_operating,
              event_in_in_reset      => event_in_in.reset,
              event_in_in_som        => '1', -- event_in_in.som,
              event_in_in_valid      => '0', -- event_in_in.valid,
              event_in_in_eom        => '1', -- event_in_in.eom,
              event_in_in_ready      => event_in_in.ready,
              event_in_out_take      => event_in_out_take,
              event_in_opcode_on_off => event_in_opcode_on_off,
              txon_pulse             => props_out.txon_pulse,
              txoff_pulse            => props_out.txoff_pulse,
              txen                   => props_out.txen,
              event_in_connected     => open,
              is_operating           => open);

end rtl;
