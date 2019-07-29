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

-- THIS FILE WAS ORIGINALLY GENERATED ON Sat Mar  9 14:09:15 2019 EST
-- BASED ON THE FILE: wsi_adapt_from_zero.xml
-- YOU *ARE* EXPECTED TO EDIT IT
-- This file initially contains the architecture skeleton for worker: wsi_adapt_from_zero

library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;
library ocpi; use ocpi.types.all; -- remove this to avoid all ocpi name collisions
architecture rtl of worker is
  constant width_c : positive := in_in.opcode'length + 1;
  signal in_reset : std_logic;
  signal fifo_enq, fifo_deq : bool_t;
  signal fifo_not_full, fifo_not_empty, fifo_out_eof : bool_t;
  signal fifo_in, fifo_out : std_logic_vector(width_c-1 downto 0);
begin
  fifo_in <= in_in.eof & in_in.opcode;
  reset : cdc.cdc.reset
    port map(src_rst  => wci_reset,
             dst_clk  => in_in.clk,
             dst_rst => in_reset);
  fifo : cdc.cdc.fifo
    generic map (width       => width_c,
                 depth       => 2)
    port map (   src_CLK     => in_in.clk,
                 src_RST     => in_reset,
                 src_ENQ     => fifo_enq,
                 src_in      => fifo_in,
                 src_FULL_N  => fifo_not_full,
                 dst_CLK     => out_in.clk,
                 dst_DEQ     => fifo_deq,
                 dst_EMPTY_N => fifo_not_empty,
                 dst_out     => fifo_out);
  fifo_enq       <= fifo_not_full and (in_in.ready or in_in.eof);
  fifo_deq       <= fifo_not_empty and out_in.ready and not fifo_out_eof;
  fifo_out_eof   <= fifo_out(fifo_out'left);
  in_out.take    <= fifo_not_full; -- AXI style: we are ready to receive data
  out_out.give   <= fifo_not_empty;
  out_out.eof    <= fifo_out_eof;
  out_out.opcode <= fifo_out(in_in.opcode'range);
  out_out.som    <= btrue;
  out_out.eom    <= btrue;
  out_out.valid  <= bfalse;
end rtl;
