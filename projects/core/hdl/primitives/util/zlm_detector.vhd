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

-- The zlm_detector detects all types (both single-cycle and multi-cycle) of
-- ZLMs
--
-- eozlm_pulse pulse-per-end-of-ZLM, zero latency, will immediately go high upon
--             ZLM appearing on input port (regardless of whether ZLM is
--             immediately taken or not)
--             example usage: enable for a counter that counts ZLMs
-- eozlm       same as EOM but only for end of ZLMs, zero latency,
--             will stay high until input port's ZLM is taken (same behavior as
--             EOM)

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;

entity zlm_detector is
  port(
    clk         : in  std_logic;  -- control plane clock
    reset       : in  std_logic;  -- control plane reset (active-high)
    som         : in  std_logic;  -- input port SOM
    valid       : in  std_logic;  -- input port valid
    eom         : in  std_logic;  -- input port EOM
    ready       : in  std_logic;  -- input port ready
    take        : in  std_logic;  -- input port take
    eozlm_pulse : out std_logic;  -- pulse-per-end-of-ZLM
    eozlm       : out std_logic); -- same as EOM but only for end of ZLMs
end zlm_detector;

architecture rtl of zlm_detector is
signal s_start  : std_logic := '0'; -- start of potential ZLM
signal s_pend_r : std_logic := '0'; -- aids in the detection of pending
                                    -- multi-cycle ZLMs (but does NOT directly
                                    -- indicate a multi-cycle ZLM)
signal s_take_r : std_logic := '0';
signal s_zlm    : std_logic := '0'; -- indicates ZLM
signal s_zlm_r  : std_logic := '0';
begin

  regs : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        s_pend_r <= '0';
        s_take_r <= '0';
        s_zlm_r  <= '0';
      else
        --          (set
        --           next ) or (pending last and(not (clear last           )))
        s_pend_r <= s_start or (s_pend_r and (not (ready and (valid or eom))));

        s_take_r <= take;
        s_zlm_r <= s_zlm;
      end if;
    end if;
  end process regs;

  s_start <= ready and som and (not valid);

  --       (end of a single-
  --        cycle ZLM      ) or (end of a multi-cycle ZLM                  )
  s_zlm <= (s_start and eom) or (s_pend_r and ready and (not valid) and eom);

  eozlm <= s_zlm;
  eozlm_pulse <= (s_zlm and (not s_zlm_r)) or (s_zlm and s_take_r);

end rtl;
