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

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
library sdp; use sdp.sdp.all;
architecture rtl of sdp_term_rv is
  signal count_r : uchar_t;
begin
  up_out_data         <= (others => (others => '0'));
  up_out.sdp.valid    <= bfalse;
  up_out.sdp.eop      <= bfalse;
  up_out.sdp.ready    <= up_in.sdp.valid;
  up_out.sdp.header   <= dws2header((others => (others => '0')));
  up_out.dropCount    <= count_r;
  process (up_in.clk) is
  begin
    if rising_edge(up_in.clk) then
      if its(up_in.reset) then
        count_r <= (others => '0');
      elsif up_in.sdp.valid = '1' and count_r /= uchar_max then
        count_r <= count_r + 1;
      end if;
    end if;
  end process;
end rtl;
