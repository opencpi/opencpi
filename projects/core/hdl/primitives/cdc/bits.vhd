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

-- simplest multi-bit cdc with no coherence between bits
-- good for status bits for unrelated conditions etc.
library IEEE; use IEEE.std_logic_1164.all, IEEE.numeric_std.all;
entity bits is
  generic (N         : natural   := 2;         -- Range 2 - 10
           IREG      : std_logic := '0';       -- 0=no, 1=yes input register
           RST_LEVEL : std_logic := '1';       -- 0=low, 1=high
           WIDTH     : positive  := 1);
  port    (src_clk : in  std_logic;
           src_rst : in  std_logic;
           src_in  : in  std_logic_vector(WIDTH-1 downto 0);
           dst_clk : in  std_logic;
           dst_rst : in  std_logic;           -- optional; if not required, tie '0'
           dst_out : out std_logic_vector(WIDTH-1 downto 0));
end bits;
architecture rtl of bits is
begin
    gen: for i in 0 to width-1 generate
      sbit : component work.cdc.bit
        generic map (
          N => N, IREG => IREG, RST_LEVEL => RST_LEVEL)
        port map (
          src_clk => src_clk,
          src_rst => src_rst,
          src_en  => '1',
          src_in  => src_in(i),
          dst_clk => dst_clk,
          dst_rst => dst_rst,
          dst_out => dst_out(i));
    end generate gen;
end architecture rtl;
