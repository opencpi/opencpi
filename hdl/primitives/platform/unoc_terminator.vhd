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

library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
library ocpi; use ocpi.all, ocpi.types.all;
library work; use work.platform_pkg.all;

entity unoc_terminator is
  port(
    up_in      : in  unoc_master_out_t;
    up_out     : out unoc_master_in_t;
    drop_count : out unsigned(7 downto 0)
    );
end entity unoc_terminator;
architecture rtl of unoc_terminator is
  signal count : unsigned(drop_count'range);
begin
  up_out.data  <= to_unoc(util.slv0(unoc_data_width));
  up_out.valid <= '0';
  up_out.take  <= up_in.valid;
  drop_count   <= count;
  process (up_in.clk) is
  begin
    if rising_edge(up_in.clk) then
      if up_in.reset_n = '0' then
        count <= (others => '0');
      elsif up_in.valid = '1' and count /= 255 then
        count <= count + 1;
      end if;
    end if;
  end process;
end architecture rtl;
