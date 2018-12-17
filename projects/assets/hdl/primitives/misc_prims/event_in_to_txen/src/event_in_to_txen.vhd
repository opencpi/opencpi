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

-- single event_in port drives single tx enable output signal
-- turn tx on as soon as we get txon from event port (zero-latency)
-- turn tx off only after we get txoff from event port (zero-latency)
library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util; use util.util.all;
entity event_in_to_txen is
  port (event_in_clk           : in  std_logic;
        event_in_reset         : in  std_logic;
        ctl_in_is_operating    : in  std_logic;
        event_in_in_reset      : in  std_logic;
        event_in_in_som        : in  std_logic;
        event_in_in_valid      : in  std_logic;
        event_in_in_eom        : in  std_logic;
        event_in_in_ready      : in  std_logic;
        event_in_out_take      : in  std_logic;
        event_in_opcode_on_off : in  std_logic; -- '1'      /'0' corresponds to
                                                -- on opcode/off opcode
        -- use case 1: use tx enable to directly drive pin
        txen                   : out std_logic;
        -- use case 2: use intermediate signals to drive other logic which
        -- drives pin (useful when there are multiple channels/event ports)
        txon_pulse             : out std_logic;
        txoff_pulse            : out std_logic;
        event_in_connected     : out std_logic;
        is_operating           : out std_logic);
end entity event_in_to_txen;
architecture rtl of event_in_to_txen is
  signal eozlmp               : std_logic := '0';
  signal event_in_connected_s : std_logic := '0';
  signal op_and_con           : std_logic := '0';
  signal on_pulse_s           : std_logic := '0';
  signal off_pulse_s          : std_logic := '0';

  signal nconnected           : std_logic := '0';
  signal noperating           : std_logic := '0';
  signal on_s                 : std_logic := '0';
  signal off_s                : std_logic := '0';
  signal en_set               : std_logic := '0';
  signal en_clear             : std_logic := '0';
  signal txen_s               : std_logic := '0';
  signal txen_r               : std_logic := '0';
begin

  zlm_detector : util.util.zlm_detector
    port map (clk         => event_in_clk,
              reset       => event_in_reset,
              som         => event_in_in_som,
              valid       => event_in_in_valid,
              eom         => event_in_in_eom,
              ready       => event_in_in_ready,
              take        => event_in_out_take,
              eozlm_pulse => eozlmp, -- zero-latency one-clock-cyle
                                     -- pulse-per-ZLM which occurs at
                                     -- the end of each ZLM
              eozlm       => open);

  event_in_connected_s <= not event_in_in_reset;

  op_and_con <= ctl_in_is_operating and event_in_connected_s;

  on_pulse_s  <= op_and_con and eozlmp and event_in_opcode_on_off;
  off_pulse_s <= op_and_con and eozlmp and (not event_in_opcode_on_off);

  -- TX off whenever qdac not operating 
  -- TX on when qdac Event port is disconnected
  nconnected <= not event_in_connected_s;
  noperating <= not ctl_in_is_operating;
  on_s <= on_pulse_s;
  off_s <= off_pulse_s;
  en_set <= txen_r or on_s or nconnected;
  en_clear <= noperating or off_s;
  txen_s <= en_set and (not en_clear);

  tx_en_reg : process(event_in_clk)
  begin
    if rising_edge(event_in_clk) then
      if event_in_reset = '1' then
        txen_r <= '0';
      else
        txen_r <= txen_s;
      end if;
    end if;
  end process tx_en_reg;

  txen               <= txen_s;
  txon_pulse         <= on_pulse_s;
  txoff_pulse        <= off_pulse_s;
  event_in_connected <= event_in_connected_s;
  is_operating       <= ctl_in_is_operating;

end rtl;
