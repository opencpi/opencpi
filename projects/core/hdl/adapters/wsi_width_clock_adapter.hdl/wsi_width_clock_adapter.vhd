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

-- Adapt the input width to the output width and the input clock to the output clock
-- Clock adaptation is on output, not input
-- Most of the complexity here is dealing with message sizes that do not divide evenly
-- with the larger width, and to not add any latency (insert idle cycles).
library IEEE; use IEEE.std_logic_1164.all, ieee.numeric_std.all;
library ocpi; use ocpi.types.all, ocpi.util.all;
architecture rtl of worker is
  constant width_out_c   : natural  := to_integer(width_out);
  constant fifo_width_c  : positive := width_out_c + out_out.opcode'length + out_out.byte_enable'length + 4;
  constant data_lsb_c    : natural  := fifo_width_c - width_out_c;
  constant opcode_lsb_c  : natural  := data_lsb_c - out_out.opcode'length;
  constant byte_en_lsb_c : natural  := opcode_lsb_c - out_out.byte_enable'length;
  signal in_reset        : bool_t;
  signal out_ready       : bool_t;
  signal out_give        : bool_t;
  signal out_som, out_valid, out_eom, out_eof : bool_t;
  signal out_data        : std_logic_vector(width_out_c-1 downto 0);
  signal out_byte_enable : std_logic_vector(width_out_c/8-1 downto 0);
  signal out_opcode      : std_logic_vector(7 downto 0);
  signal fifo_enq, fifo_deq : bool_t;
  signal fifo_not_full, fifo_not_empty, fifo_out_eof : bool_t;
  signal fifo_in, fifo_out : std_logic_vector(fifo_width_c-1 downto 0);
begin
  reset : cdc.cdc.reset
    port map(src_rst  => wci_reset,
             dst_clk  => in_in.clk,
             dst_rst  => in_reset);
  adapt_width : entity work.adapt_width
    generic map(width_in         => to_integer(width_in),
                width_out        => width_out_c)
       port map(clk              => in_in.clk,
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
                out_ready        => out_ready,
                out_som          => out_som,
                out_valid        => out_valid,
                out_eom          => out_eom,
                out_eof          => out_eof,
                out_data         => out_data,
                out_byte_enable  => out_byte_enable,
                out_opcode       => out_opcode,
                out_give         => out_give);

  -- cross the clock domain on the output side of the adapter
  -- we might enq multiple EOFs, but only one will be dequed
  fifo_enq            <= fifo_not_full and (out_give or out_eof);
  fifo_deq            <= fifo_not_empty and out_in.ready and not fifo_out_eof;
  fifo_in             <= out_data & out_opcode & out_byte_enable &
                         slv(out_som) & slv(out_valid) & slv(out_eom) & slv(out_eof);
  out_out.data        <= fifo_out(fifo_width_c - 1 downto data_lsb_c);
  out_out.opcode      <= fifo_out(data_lsb_c-1 downto opcode_lsb_c);
  out_out.byte_enable <= fifo_out(opcode_lsb_c-1 downto byte_en_lsb_c);
  out_out.som         <= fifo_out(3);
  out_out.valid       <= fifo_out(2) and fifo_not_empty;
  out_out.eom         <= fifo_out(1);
  fifo_out_eof        <= fifo_out(0);
  out_out.eof         <= fifo_out_eof and fifo_not_empty;
  out_ready           <= fifo_not_full;
  out_out.give        <= fifo_not_empty;
  fifo : cdc.cdc.fifo
    generic map (width       => fifo_width_c,
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
end rtl;
