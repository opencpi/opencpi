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
-- 10/12/15: RMM
-- File Created
-------------------------------------------------------------------------------
-- 07/01/17: RMM
-- Change in architecture to allow for back pressure. The input and output
-- data now make use of DIN_RDY and DOUT_RDY to accomplish this.
-------------------------------------------------------------------------------
-- 11/07/17: RMM
-- Another major rewrite. Simplified control logic and fixed corner cases
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;

entity cic_dec_gen2 is

generic(
   N          : positive;  -- CIC Stages: Number of Comb & Integrator sections
   M          : positive;  -- Differential Delay: delay in comb feedforward paths (usually 1 or 2)
   R          : positive;  -- Decimation Rate: 1 output value for every R input values
   DIN_WIDTH  : positive;
   ACC_WIDTH  : positive;  -- >= N*log2(RM)+DIN_WIDTH
   DOUT_WIDTH : positive);

port(
   CLK        : in std_logic;
   RST        : in std_logic;
   DIN        : in std_logic_vector(DIN_WIDTH-1 downto 0);
   DIN_VLD    : in std_logic;
   DIN_RDY    : out std_logic;
   DOUT       : out std_logic_vector(DOUT_WIDTH-1 downto 0);
   DOUT_VLD   : out std_logic;
   DOUT_RDY   : in std_logic);

end cic_dec_gen2;

architecture rtl of cic_dec_gen2 is

   constant RCNT_WIDTH : integer := integer(ceil(log(real(R))/log(2.0)))+1;

   type integ_t is array (0 to N) of std_logic_vector(ACC_WIDTH-1 downto 0);
   type comb_t is array (0 to N) of std_logic_vector(ACC_WIDTH-1 downto 0);
   type comb_dly_t  is array (0 to N, 0 to M) of std_logic_vector(ACC_WIDTH-1 downto 0);

   signal rcnt         : std_logic_vector(RCNT_WIDTH-1 downto 0);
   signal integ_en     : std_logic;
   signal comb_en      : std_logic;
   signal comb         : comb_t;
   signal comb_dly     : comb_dly_t;
   signal integ        : integ_t;

begin

   ----------------------------------------------------------------------------
   -- Timing Control for Comb and Integrator Sections:
   ----------------------------------------------------------------------------
   -- * Integrator stages enabled when working = 0 and DIN_VLD = 1
   -- * A single DIN_VLD sets working = 1
   -- * Comb stages enabled when working = 1 and DOUT_RDY = 1
   -- * Counter rcnt tracks processed comb samples
   -- * done flag set when rcnt reaches R-1, setting working = 0
   -- * Process repeats
   ----------------------------------------------------------------------------

   process(CLK)
   begin
      if rising_edge(CLK) then
         if (RST = '1') or (comb_en = '1') then
            rcnt <= (others => '0');
         elsif (integ_en = '1') then
            rcnt <= std_logic_vector(unsigned(rcnt)+1);
         end if;
      end if;
   end process;

   integ_en <= '1' when (unsigned(rcnt) < R) and (DIN_VLD = '1') else '0';
   comb_en <= '1' when (unsigned(rcnt) = R) and (DOUT_RDY = '1') else '0';

   DIN_RDY <= '1' when (unsigned(rcnt) < R) else '0';

   ----------------------------------------------------------------------------
   -- Integrator Stages : Operate at max rate of 1 * CLK Rate
   ----------------------------------------------------------------------------


   integ(0)(ACC_WIDTH-1 downto DIN_WIDTH) <= (others => DIN(DIN_WIDTH-1));
   integ(0)(DIN_WIDTH-1 downto 0) <= DIN;

   gen_integ: for i in 1 to N generate

      integ_i: process (CLK)
      begin
         if rising_edge(CLK) then
            if (RST = '1') then
               integ(i) <= (others => '0');
            elsif (integ_en = '1') then
               integ(i) <= std_logic_vector( signed(integ(i)) + signed(integ(i-1)) );
            end if;
         end if;
      end process;

   end generate;

   ----------------------------------------------------------------------------
   -- Comb Stages: Operate at max rate of 1/R * CLK Rate
   ----------------------------------------------------------------------------

   comb(0) <= integ(N);

   gen_comb: for i in 1 to N generate

      comb_i: process (CLK)
      begin
         if rising_edge(CLK) then
            if (RST = '1') then
               comb(i) <= (others => '0');
            elsif (comb_en = '1') then
               comb(i) <= std_logic_vector( signed(comb(i-1)) - signed(comb_dly(i,M)) );
            end if;
         end if;
      end process;

      comb_dly(i,0) <= comb(i-1);

      gen_comb_dly: for j in 1 to M generate
         comb_dly_j: process(CLK)
         begin
            if rising_edge(CLK) then
               if (RST = '1') then
                  comb_dly(i,j) <= (others => '0');
               elsif (comb_en = '1') then
                  comb_dly(i,j) <= comb_dly(i,j-1);
               end if;
            end if;
         end process;
      end generate;

   end generate;

   ----------------------------------------------------------------------------
   -- Final Output
   ----------------------------------------------------------------------------

   DOUT <= comb(N)(ACC_WIDTH-1 downto ACC_WIDTH-DOUT_WIDTH);
   DOUT_VLD <= comb_en;

   ----------------------------------------------------------------------------

end rtl;
