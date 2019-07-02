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
-- Cascaded Integrator-Comb (CIC) Decimator
-------------------------------------------------------------------------------
-- Revision Log:
-------------------------------------------------------------------------------
-- 10/12/15:
-- File Created
-------------------------------------------------------------------------------
-- 07/01/17:
-- Change in architecture to allow for back pressure. The input and output
-- data now make use of DIN_RDY and DOUT_RDY to accomplish this.
-------------------------------------------------------------------------------
-- 11/07/17:
-- Another major rewrite. Simplified control logic and fixed corner cases
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity cic_dec_gen is

generic(
   g_n          : positive;  -- CIC Stages: Number of Comb & Integrator sections
   g_m          : positive;  -- Differential Delay: delay in comb feedforward paths (usually 1 or 2)
   g_r          : positive;  -- Decimation Rate: 1 output value for every R input values
   g_din_width  : positive;
   g_acc_width  : positive;  -- >= n*log2(rm)+din_width
   g_dout_width : positive);

port(
   i_clk        : in std_logic;
   i_rst        : in std_logic;
   i_din        : in std_logic_vector(g_din_width-1 downto 0);
   i_din_vld    : in std_logic;
   o_din_rdy    : out std_logic;
   o_dout       : out std_logic_vector(g_dout_width-1 downto 0);
   o_dout_vld   : out std_logic;
   i_dout_rdy   : in std_logic);

end cic_dec_gen;

architecture rtl of cic_dec_gen is

   constant c_rcnt_width : integer := integer(ceil(log(real(g_r))/log(2.0)))+1;

   type t_integ is array (0 to g_n) of std_logic_vector(g_acc_width-1 downto 0);
   type t_comb is array (0 to g_n) of std_logic_vector(g_acc_width-1 downto 0);
   type t_comb_dly is array (0 to g_n, 0 to g_m) of std_logic_vector(g_acc_width-1 downto 0);

   signal s_rcnt         : unsigned(c_rcnt_width-1 downto 0);
   signal s_integ_en     : std_logic;
   signal s_comb_en      : std_logic;
   signal s_comb         : t_comb;
   signal s_comb_dly     : t_comb_dly;
   signal s_integ        : t_integ;

begin

   ----------------------------------------------------------------------------
   -- Timing Control for Comb and Integrator Sections:
   ----------------------------------------------------------------------------
   -- * Integrator stages enabled when working = 0 and i_din_vld = 1
   -- * A single i_din_vld sets working = 1
   -- * Comb stages enabled when working = 1 and o_dout_rdy = 1
   -- * Counter s_rcnt tracks processed comb samples
   -- * done flag set when s_rcnt reaches g_r-1, setting working = 0
   -- * Process repeats
   ----------------------------------------------------------------------------

   process(i_clk)
   begin
      if rising_edge(i_clk) then
         if (i_rst = '1') or (s_comb_en = '1') then
            s_rcnt <= (others => '0');
         elsif (s_integ_en = '1') then
            s_rcnt <= s_rcnt+1;
         end if;
      end if;
   end process;

   s_integ_en <= '1' when (s_rcnt < g_r) and (i_din_vld = '1') else '0';
   s_comb_en <= '1' when  (s_rcnt = g_r) and (i_dout_rdy = '1') else '0';

   o_din_rdy <= '1' when (s_rcnt < g_r) else '0';

   ----------------------------------------------------------------------------
   -- Integrator Stages : Operate at max rate of 1 * i_clk Rate
   ----------------------------------------------------------------------------


   s_integ(0)(g_acc_width-1 downto g_din_width) <= (others => i_din(g_din_width-1));
   s_integ(0)(g_din_width-1 downto 0) <= i_din;

   gen_integ: for i in 1 to g_n generate

      integ_i: process (i_clk)
      begin
         if rising_edge(i_clk) then
            if (i_rst = '1') then
               s_integ(i) <= (others => '0');
            elsif (s_integ_en = '1') then
               s_integ(i) <= std_logic_vector( signed(s_integ(i)) + signed(s_integ(i-1)) );
            end if;
         end if;
      end process;

   end generate;

   ----------------------------------------------------------------------------
   -- Comb Stages: Operate at max rate of 1/R * i_clk Rate
   ----------------------------------------------------------------------------

   s_comb(0) <= s_integ(g_n);

   gen_comb: for i in 1 to g_n generate

      comb_i: process (i_clk)
      begin
         if rising_edge(i_clk) then
            if (i_rst = '1') then
               s_comb(i) <= (others => '0');
            elsif (s_comb_en = '1') then
               s_comb(i) <= std_logic_vector( signed(s_comb(i-1)) - signed(s_comb_dly(i,g_m)) );
            end if;
         end if;
      end process;

      s_comb_dly(i,0) <= s_comb(i-1);

      gen_comb_dly: for j in 1 to g_m generate
         comb_dly_j: process(i_clk)
         begin
            if rising_edge(i_clk) then
               if (i_rst = '1') then
                  s_comb_dly(i,j) <= (others => '0');
               elsif (s_comb_en = '1') then
                  s_comb_dly(i,j) <= s_comb_dly(i,j-1);
               end if;
            end if;
         end process;
      end generate;

   end generate;

   ----------------------------------------------------------------------------
   -- Final Output
   ----------------------------------------------------------------------------

   o_dout <= s_comb(g_n)(g_acc_width-1 downto g_acc_width-g_dout_width);
   o_dout_vld <= s_comb_en;

   ----------------------------------------------------------------------------

end rtl;
