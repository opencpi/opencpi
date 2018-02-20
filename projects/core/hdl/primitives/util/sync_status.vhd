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

-- A status bit that is set based on an event in the "other" clock domain,
-- only after the monitoring is "started".
library IEEE, ocpi, bsv;
use IEEE.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all, ocpi.util.all;
entity sync_status is
  port (-- ctl side signals
        clk         : in  std_logic;
        reset       : in  bool_t;
        operating   : in  bool_t;
        start       : in  bool_t;  -- condition to start looking for event
        clear       : in  bool_t;  -- pulse to clear status
        status      : out bool_t;  -- the reported status
        -- other clock domain signals
        other_clk   : in  std_logic;
        other_reset : out bool_t;
        event       : in  bool_t); -- the (transient) event indication in the other clock domain
end entity sync_status;
architecture rtl of sync_status is
  signal started_r     : bool_t;    -- monitoring is started
  signal status_r      : bool_t;    -- our registered sticky indicator
  signal status_sync   : bool_t;    -- sync'd indication from other side
  signal status_ready  : bool_t;
--  signal status_event  : bool_t;
  signal syncd_reset   : bool_t;
  signal other_reset_n : std_logic;
begin
  status       <= status_r;
--  status_event <= status_ready and event;
  input : process(clk)
  begin
    if rising_edge(clk) then
      if reset or clear then
        status_r  <= bfalse;
        started_r <= bfalse;
      elsif its(operating) then
        if started_r and status_sync then
          status_r <= btrue;
        end if;
        if its(start) then
          started_r <= btrue;
        end if;
      else
        started_r <= bfalse;
      end if;
    end if;  
  end process;        
  -- Synchronizing handshake to reliably send a pulse from other side to control side
  -- First we need a reset in the other clock domain
  sreset : bsv.bsv.SyncResetA
    generic map(RSTDELAY => 0)
    port map(   IN_RST    => reset,
                CLK       => other_clk,
                OUT_RST   => syncd_reset);
  other_reset   <= syncd_reset;     -- output port
  other_reset_n <= not syncd_reset; -- input to handshake
  sync : bsv.bsv.SyncHandshake
    port map(sCLK      => other_clk,
             sRST      => other_reset_n,
             dCLK      => clk,
             sEN       => event,
             sRDY      => status_ready,
             dPulse    => status_sync);
end rtl;
