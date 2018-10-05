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

-- THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun  2 08:35:09 2015 EDT
-- BASED ON THE FILE: lime_tx.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_tx

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of lime_tx_worker is
  signal props_in_s  : worker_props_in_t;--work.lime_tx_defs.rawprops_out_t;
  signal event_rawprops  : worker_props_in_t;--work.lime_tx_defs.rawprops_out_t;
  signal eventEnable : std_logic;
  signal event_port_connected : std_logic;
  signal spi_busy : std_logic;
begin
  -- Route the raw property signals to the lime_spi
  rawprops_out.present           <= '1';
  rawprops_out.reset             <= ctl_in.reset;
  rawprops_out.raw               <= props_in_s.raw;
  props_out.raw                  <= rawprops_in.raw;
  props_out.other_present        <= rawprops_in.present(1);
  event_port_connected           <= dev_tx_event_in.connected;
  props_out.event_port_connected <= event_port_connected;
  spi_busy                       <= props_in.raw.is_read or props_in.raw.is_write;
  dev_tx_event_out.busy          <= spi_busy;
  --FIXME: Using reset because there is no other way to tell if port is connected (AV-4222)
  eventEnable <= event_port_connected and          --Port is connected
                 dev_tx_event_in.event_pending and --Event present on port
                 not spi_busy;                     --No other accesses in progress

  --TXEN must be high during SPI Read/Writes
  txen <= dev_txen_dac_in.txen or props_in.raw.is_read or props_in.raw.is_write;

  pin_control_true : if its(txen_pin_control_p) generate
    props_in_s.raw <= props_in.raw;   --No mux needed for pin control mode
    dev_tx_event_out.done_processing_event <= '1';
  end generate pin_control_true;

  pin_control_false : if not its(txen_pin_control_p) generate
    props_in_s.raw <= event_rawprops.raw when its(eventEnable) else props_in.raw;
    event_rawprops.raw.is_read <= '0';
    event_rawprops.raw.is_write <= eventEnable;
    event_rawprops.raw.address <= (6 => '1', others => '0'); --TXRF modules enable register 0x40
    event_rawprops.raw.data <= x"00000002" when its(dev_tx_event_in.opcode) else x"00000000";
    event_rawprops.raw.byte_enable <= (others => '1');
    dev_tx_event_out.done_processing_event <= rawprops_in.raw.done;
  end generate pin_control_false;

end rtl;
