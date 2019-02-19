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

-- THIS FILE WAS ORIGINALLY GENERATED ON Thu Jul 19 09:02:32 2018 EDT
-- BASED ON THE FILE: backpressure.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: backpressure

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of backpressure_worker is
  signal take    : std_logic;
  signal take_en : std_logic;
  signal count   : unsigned(7 downto 0) := (others => '0');
  signal lfsr    : unsigned(15 downto 0); -- := (others => '0');

begin

  process (ctl_in.clk)
    variable take_v : std_logic;
  begin
    if rising_edge(ctl_in.clk) then
      if (ctl_in.reset = '1') then
        take_en <= '0';
        lfsr <= x"0001";
      else

        if (props_in.enable_select = '1') then
          take_en <= lfsr(15);
        else
          take_en <= take_v;
        end if;

        if (out_in.ready = '1' and in_in.ready = '1') then
          lfsr <= lfsr(14 downto 0) & (lfsr(15) xor lfsr(14));
        end if;

        if (count = props_in.enable_duty_cycle-1) then
          count  <= (others => '0');
          take_v := '1';
        elsif (out_in.ready = '1' and in_in.ready = '1') then
          count  <= count + 1;
          take_v := '0';
        end if;

      end if;
    end if;
  end process;

  take        <= '1' when (ctl_in.is_operating = '1' and out_in.ready = '1' and in_in.ready = '1' and take_en = '1') else '0';

  in_out.take <= take;

  out_out.give <= take;

  out_out.som   <= in_in.som;
  out_out.eom   <= in_in.eom;
  out_out.valid <= in_in.valid;
  out_out.data  <= in_in.data;
  out_out.byte_enable <= in_in.byte_enable;
  out_out.opcode <= in_in.opcode;
end rtl;
