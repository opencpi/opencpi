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

-- two event_in ports drive single tx enable output signal
-- turn tx on as soon as we get txon for EITHER event port
-- turn tx off only after we get txoff for BOTH event ports,
-- also if one channel gets txoff and the other gets txon simultaneously,
-- txon will override
library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
entity event_in_x2_to_txen is
  port (ctl_in_clk             : in  std_logic;
        ctl_in_reset           : in  std_logic;
        -- TX channel 0 (channel indices are treated identically)
        txon_pulse_0           : in  std_logic;
        txoff_pulse_0          : in  std_logic;
        event_in_connected_0   : in  std_logic;
        is_operating_0         : in  std_logic;
        -- TX channel 1 (channel indices are treated identically)
        txon_pulse_1           : in  std_logic;
        txoff_pulse_1          : in  std_logic;
        event_in_connected_1   : in  std_logic;
        is_operating_1         : in  std_logic;
        -- tx enable output
        txen                   : out std_logic);
end entity event_in_x2_to_txen;
architecture rtl of event_in_x2_to_txen is
  signal pending_off_ch0_r    : std_logic := '0';
  signal pending_off_ch1_r    : std_logic := '0';
  signal txoff_10             : std_logic := '0';
  signal txoff_01             : std_logic := '0';

  signal nconnected           : std_logic := '0';
  signal noperating           : std_logic := '0';
  signal on_s                 : std_logic := '0';
  signal off_s                : std_logic := '0';
  signal en_set               : std_logic := '0';
  signal en_clear             : std_logic := '0';
  signal txen_s               : std_logic := '0';
  signal txen_r               : std_logic := '0';
begin

  -- only necessary because we have two pin ctrl channels and the desired
  -- functionality is to *wait* for both txoff pulses before turning transmitter
  -- off
  pending_off_ch0_reg : process(ctl_in_clk)
  begin
    if rising_edge(ctl_in_clk) then
      if ctl_in_reset = '1' then
        pending_off_ch0_r <= '0';
      else
        -- if one channel gets txoff and other gets txon simultaneously, txon
        -- will override, also reset register whever txen is cleared for any
        -- reason
        if ((txoff_10 = '1') and (on_s = '1')) or (en_clear = '1') then
          pending_off_ch0_r <= '0';
        elsif (txoff_pulse_1 = '1') and (txoff_pulse_0 = '0') and
              (event_in_connected_0 = '1') then
          pending_off_ch0_r <= '1';
        end if;
      end if;
    end if;
  end process;

  -- only necessary because we have two pin ctrl channels and the desired
  -- functionality is to *wait* for both txoff pulses before turning transmitter
  -- off
  pending_off_ch1_reg : process(ctl_in_clk)
  begin
    if rising_edge(ctl_in_clk) then
      if ctl_in_reset = '1' then
        pending_off_ch1_r <= '0';
      else
        -- if one channel gets txoff and other gets txon simultaneously, txon
        -- will override, also reset register whever txen is cleared for any
        -- reason
        if ((txoff_01 = '1') and (on_s = '1')) or (en_clear = '1') then
          pending_off_ch1_r <= '0';
        elsif (txoff_pulse_0 = '1') and (txoff_pulse_1 = '0') and
              (event_in_connected_1 = '1') then
          pending_off_ch1_r <= '1';
        end if;
      end if;
    end if;
  end process;

  -- previously received ch1 txoff, and received the final ch0 txoff we've been
  -- waiting for
  txoff_10 <= pending_off_ch0_r and txoff_pulse_0;

  -- previously received ch0 txoff, and received the final ch1 txoff we've been
  -- waiting for
  txoff_01 <= pending_off_ch1_r and txoff_pulse_1;

  -- TX off whenever both qdac are not operating 
  -- TX on when both qdac Event ports are disconnected
  nconnected <= (not event_in_connected_0) and (not event_in_connected_1);
  noperating <= (not is_operating_0) and (not is_operating_1);
  on_s <= txon_pulse_0 or txon_pulse_1;
  off_s <= (txoff_pulse_0 and (not event_in_connected_1)) or
           (txoff_pulse_1 and (not event_in_connected_0)) or
           (txoff_pulse_0 and txoff_pulse_1) or txoff_10 or txoff_01;
  en_set <= txen_r or on_s or nconnected;
  en_clear <= noperating or off_s;
  txen_s <= en_set and (not en_clear);

  tx_en_reg : process(ctl_in_clk)
  begin
    if rising_edge(ctl_in_clk) then
      if ctl_in_reset = '1' then
        txen_r <= '0';
      else
        txen_r <= txen_s;
      end if;
    end if;
  end process tx_en_reg;

  txen <= txen_s;

end rtl;
