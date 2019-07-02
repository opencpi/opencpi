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
-- Divide by Constant
-------------------------------------------------------------------------------
-- A/B implemented as A * (2^num_bits / B) * (1 / 2^num_bits)
-- 
-- (2^num_bits / B) is constant and precomputed and 1 / 2^(num_bits) is
-- implemented as a bit shift of num_bits
--
-- https://surf-vhdl.com/how-to-divide-an-integer-by-constant-in-vhdl/
-------------------------------------------------------------------------------

library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
use ieee.math_real.all;
library ocpi; use ocpi.util.all;

entity divide_by_constant is
  generic(
    g_const      : positive; --constant to divide by
    g_num_bits   : positive; --number of bits used to quantize the divider
    g_data_width : positive);
  port(
    i_clk        : in  std_logic;
    i_rst        : in  std_logic;
    i_din        : in  unsigned(g_data_width-1 downto 0);
    i_din_vld    : in  std_logic;
    o_din_rdy    : out std_logic;
    o_dout       : out unsigned(g_data_width-1 downto 0);
    o_dout_vld   : out std_logic;
    i_dout_rdy   : in  std_logic);
end divide_by_constant;

architecture rtl of divide_by_constant is
  constant c_two_exp_num_bits_div_const : unsigned(g_num_bits-1 downto 0) := to_unsigned(integer(ceil(real(2 ** g_num_bits) / real(g_const))),g_num_bits);
  signal   s_prod                       : unsigned(g_data_width-1+g_num_bits downto 0);
  signal   s_din_rdy, s_dout_vld        : std_logic;
begin

  o_din_rdy <= s_din_rdy;
  
  process (i_clk)
  begin
    if rising_edge(i_clk) then
      if i_rst = '1' then
        s_prod     <= (others => '0');
        s_din_rdy  <= '1';
        s_dout_vld <= '0';
      elsif i_din_vld = '1' and s_din_rdy = '1' then
        s_prod     <= i_din * c_two_exp_num_bits_div_const;
        s_din_rdy  <= '0';
        s_dout_vld <= '1';
      elsif i_dout_rdy = '1' and s_dout_vld = '1' then 
        s_din_rdy  <= '1';
        s_dout_vld <= '0';
      end if;
    end if;
  end process;

  o_dout     <= s_prod(g_data_width-1+g_num_bits downto g_num_bits);
  o_dout_vld <= s_dout_vld;
  
end rtl;
