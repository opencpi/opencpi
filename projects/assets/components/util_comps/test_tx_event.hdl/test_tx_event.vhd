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

-- THIS FILE WAS ORIGINALLY GENERATED ON Mon Dec 17 10:40:22 2012 EST
-- BASED ON THE FILE: bias_vhdl.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: bias_vhdl

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of worker is
  signal doit           : bool_t;
  signal opcode_bit     : std_logic := '0';
  signal ctr            : unsigned(31 downto 0);
  signal en_assert_eof  : std_logic := '0';
  signal assert_eof     : std_logic := '0';
begin
  doit <= event_out_in.ready and ctl_in.is_operating;
  event_out_out.give <= doit and to_bool(ctr = props_in.max_count_value-1);
  event_out_out.opcode <= tx_event_txOn_op_e when its(opcode_bit) else tx_event_txOff_op_e;
  
  FlipOpCode : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if its(ctl_in.reset) or not its(ctl_in.is_operating) then
        ctr <= (others => '0');
      elsif ctr = props_in.max_count_value-1 then
        ctr <= (others => '0');
        opcode_bit <= not opcode_bit;
      else 
        ctr <= ctr +1;
      end if;
    end if;
  end process;

  props_out.opcode_bit <= opcode_bit;

  en_assert_eof_gen : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        en_assert_eof <= '0';
      elsif (props_in.assert_eof_written = '1' )then
        en_assert_eof <= '1';
      end if;
    end if;
  end process en_assert_eof_gen;

  assert_eof_gen : process(ctl_in.clk)
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        assert_eof <= '0';
      elsif (en_assert_eof = '1') then
        assert_eof <= props_in.assert_eof;
      else
        assert_eof <= '0';
      end if;
    end if;
  end process assert_eof_gen;

  event_out_out.eof <= assert_eof;
end rtl;
