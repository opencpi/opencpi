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


-------------------------------------------------------------------------------
-- GPI_EM
-------------------------------------------------------------------------------
--
-- Description:
--
-- The GPI Emulator uses the data received from a data source to write the GPIO
-- pins. It waits for numClockCycles number of clock cylces before taking data
-- because the GPI device worker should be in the operating state by then.


library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of worker is


-----------------------------------------------------------------------------
-- Constants
-----------------------------------------------------------------------------
constant c_NUM_INPUTS   : integer := to_integer(NUM_INPUTS);

signal s_counter        : ushort_t  := (others => '0');
signal s_gpi_ready      : std_logic := '0';
signal s_gpio_data      : std_logic_vector(31 downto 0) := (others => '0');
signal s_clr            : std_logic_vector(c_NUM_INPUTS-1 downto 0) := (others => '0');

begin

  em_in_out.take <= em_in_in.valid and s_gpi_ready;
  s_gpio_data <= em_in_in.data when (its(s_gpi_ready)) else (others => '0');

  -- Using the set_clr primitive to set the GPIO pins and also preserve current GPIO pin state
  output_condition_i : for ii in 0 to c_NUM_INPUTS-1 generate
    set_clr : util_prims.util_prims.set_clr
      port map(
        clk => ctl_in.clk,
        rst => ctl_in.reset,
        set => s_gpio_data(ii),
        clr => s_clr(ii),
        q   => gpi_pin(ii),
        q_r => open);

  s_clr(ii) <= not s_gpio_data(ii);
  end generate output_condition_i;

  counter : process (ctl_in.clk)
    begin
      if rising_edge(ctl_in.clk) then
        if ctl_in.reset = '1' then
          s_counter <= (others=>'0');
          s_gpi_ready <= '0';
        elsif (ctl_in.is_operating = '1' and s_counter < props_in.numClockCycles) then
            s_counter <= s_counter + 1;
        elsif (ctl_in.is_operating = '1' and s_counter = props_in.numClockCycles) then
            s_gpi_ready <= '1';
        end if;
      end if;
  end process counter;

end rtl;
