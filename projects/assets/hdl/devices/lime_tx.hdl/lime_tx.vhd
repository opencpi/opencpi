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
  signal txen_s : std_logic;
begin
  -- Route the raw property signals to the lime_spi
  rawprops_out.present     <= '1';
  rawprops_out.reset       <= ctl_in.reset;
  rawprops_out.raw         <= props_in_s.raw;
  props_out.raw            <= rawprops_in.raw;
  props_out.other_present  <= rawprops_in.present(1);
  eventEnable <= not event_in_in.reset and                              --Port is connected
                 ctl_in.is_operating and event_in_in.ready and          --Event present on port
                 not props_in.raw.is_read and not props_in.raw.is_write;--No other accesses in progress
    
  UpdateTxEn : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if not its(ctl_in.is_operating) then               --TX whenever not operating 
        txen_s <= '0';
      else
        if its(event_in_in.reset) then                   --TX on when Event port is disconnected
          txen_s <= '1';                                   
        elsif its(eventEnable) then                      --Otherwise, update tx_en when event port has incoming opcode
          if event_in_in.opcode = tx_event_txOn_op_e then 
            txen_s <= '1';
          else
            txen_s <= '0';
          end if;
        end if;
      end if;
    end if;
  end process;

  txen <= txen_s or props_in.raw.is_read or props_in.raw.is_write;

  pin_control_true : if its(pin_control_p) generate
    props_in_s.raw <= props_in.raw;   --No mux needed for pin control mode
    event_in_out.take <= eventEnable; --Take events when enabled
  end generate pin_control_true;

  pin_control_false : if not its(pin_control_p) generate
    props_in_s.raw <= event_rawprops.raw when its(eventEnable) else props_in.raw;
    event_rawprops.raw.is_read <= '0';
    event_rawprops.raw.is_write <= eventEnable;
    event_rawprops.raw.address <= x"0040"; --TXRF modules enable register
    event_rawprops.raw.data <= x"00000002" when event_in_in.opcode = tx_event_txOn_op_e else x"00000000";
    event_rawprops.raw.byte_enable <= (others => '1');
    event_in_out.take <= rawprops_in.raw.done and eventEnable;
  end generate pin_control_false;

end rtl;
