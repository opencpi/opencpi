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

-- THIS FILE WAS ORIGINALLY GENERATED ON Fri Sep 29 13:05:37 2017 EDT
-- BASED ON THE FILE: lime_tx_em.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: lime_tx_em

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of lime_tx_em_worker is
  signal num_write_ctr   : unsigned(31 downto 0);
  signal txen_r          : std_logic;
  signal txen_edge       : std_logic;
  signal give_sig        : std_logic;
begin
  tx_en_falling_edge_detect : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      txen_r <= txen;
    end if; -- rising edge
  end process;
  txen_edge <= not txen and txen_r;
    
  props_out.numWrites <= num_write_ctr;
  write_ctr : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) then
        num_write_ctr <= (others => '0');
      elsif its(give_sig) then
        num_write_ctr <= num_write_ctr+1;
      end if; -- not reset
    end if; -- rising edge
  end process;

  give_sig <= ctl_in.is_operating and out_in.ready and txen_edge;
  out_out.give <= give_sig;
  out_out.valid <= give_sig when num_write_ctr <= props_in.maxWrites else '0';
  out_out.som <= give_sig;
  out_out.eom <= give_sig;
  out_out.data <= std_logic_vector(num_write_ctr);
  out_out.byte_enable <= (others => '1') when num_write_ctr <= props_in.maxWrites else (others => '0');
  out_out.opcode <= (others => '0');

end rtl;
