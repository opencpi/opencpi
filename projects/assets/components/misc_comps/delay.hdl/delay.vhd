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

-- This worker delays a WSI input by a specified number of cycles
library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.types.all; use ocpi.util.all;-- remove this to avoid all ocpi name collisions

architecture rtl of worker is
  
  constant c_latency : positive := to_integer(latency);

  signal s_dly_ready : std_logic;

  -- AV-3027 requires intermediate signals to drive output data ports
  signal data : std_logic_vector(ocpi_port_out_data_width-1 downto 0);
  
begin

  in_out.take <= in_in.ready and out_in.ready;
  
  delay_inst : ocpi.wsi.delayline
  generic map (
    g_latency     => c_latency)
  port map (
    i_clk         => wci_clk,
    i_reset       => wci_reset,
    i_enable      => out_in.ready,
    i_ready       => in_in.ready,
    i_som         => in_in.som,
    i_eom         => in_in.eom,
    i_opcode      => in_in.opcode,
    i_valid       => in_in.valid,
    i_byte_enable => in_in.byte_enable,
    i_data        => in_in.data,
    i_eof         => in_in.eof,
    o_ready       => s_dly_ready,
    o_som         => out_out.som,
    o_eom         => out_out.eom,
    o_opcode      => out_out.opcode,
    o_valid       => out_out.valid,
    o_byte_enable => out_out.byte_enable,
    o_data        => data,
    o_eof         => out_out.eof);

  out_out.give <= out_in.ready and s_dly_ready;

  -- AV-3027 requires intermediate signals to drive output data ports
  out_out.data <= data;

  
end rtl;
