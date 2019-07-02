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

-- Allows forcing an output port's SOM/EOM/valid signals to inject a ZLM,
-- but only as soon as the output port's backpressure allows it. zlm_req_sent
-- indicates that backpressure finally allowed ZLM to be injected (and that the
-- som_i/eom_i/valid_i values were ignored)
library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library util_prims;
entity out_port_inject_zlm is
  port (clk          : in  std_logic;
        reset        : in  std_logic; -- active-high reset
        zlm_req      : in  std_logic; -- ZLM request
        som_i        : in  std_logic;
        eom_i        : in  std_logic;
        valid_i      : in  std_logic;
        out_in_ready : in  std_logic; -- enforces backpressure
        zlm_req_sent : out std_logic; -- request sent
        som_o        : out std_logic;
        eom_o        : out std_logic;
        valid_o      : out std_logic);
end entity out_port_inject_zlm;
architecture rtl of out_port_inject_zlm is
  signal out_in_ready_r : std_logic := '0';
  signal force_zlm      : std_logic := '0';
  signal pending_zlm    : std_logic := '0';
begin

  -- because 1) force_zlm is generated from pending_zlm and-ed with
  -- out_in_ready, and 2) the clr has zero latency, delaying out_in_ready by
  -- one clock cycle is required for force_zlm to ever go high

  out_in_ready_reg : process(clk)
  begin
    if rising_edge(clk) then
      if reset = '1' then
        out_in_ready_r <= '0';
      else
        out_in_ready_r <= out_in_ready;
      end if;
    end if;
  end process;

  pending_zlm_set_clr : util_prims.util_prims.set_clr
    port map(
      clk => clk,
      rst => reset,
      set => zlm_req,
      clr => out_in_ready_r,
      q   => pending_zlm,
      q_r => open);

  -- same as zlm_req, but obeys backpressure
  force_zlm <= pending_zlm and out_in_ready;

  zlm_req_sent <= force_zlm;
  som_o        <= som_i or force_zlm;
  eom_o        <= eom_i or force_zlm;
  valid_o      <= valid_i and (not force_zlm);

end rtl;
