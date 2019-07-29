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

-- Adapt the input width to the output width.
-- Most of the complexity here is dealing with message sizes that do not divide evenly
-- with the larger width, and to not add any latency (insert idle cycles).
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all; -- remove this to avoid all ocpi name collisions
architecture rtl of worker is
  signal s_out_data : std_logic_vector(to_integer(width_out)-1 downto 0);
begin
  adapt_width : entity work.adapt_width
    generic map(width_in         => to_integer(width_in),
                width_out        => to_integer(width_out))
       port map(clk              => wci_clk,
                reset            => wci_reset,
                in_som           => in_in.som,
                in_valid         => in_in.valid,
                in_eom           => in_in.eom,
                in_eof           => in_in.eof,
                in_data          => in_in.data,
                in_byte_enable   => in_in.byte_enable,
                in_opcode        => in_in.opcode,
                in_ready         => in_in.ready,
                in_take          => in_out.take,
                out_ready        => out_in.ready,
                out_som          => out_out.som,
                out_valid        => out_out.valid,
                out_eom          => out_out.eom,
                out_eof          => out_out.eof,
                out_data         => s_out_data,
                out_byte_enable  => out_out.byte_enable,
                out_opcode       => out_out.opcode,
                out_give         => out_out.give);

  -- this only needed to avoid build bug for xsim:
  -- ERROR: [XSIM 43-3316] Signal SIGSEGV received.
  out_out.data <= s_out_data;
end rtl;
