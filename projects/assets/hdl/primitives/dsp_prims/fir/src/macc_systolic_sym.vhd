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
-- MACC Module for FIRs - Symmetric Systolic Architecture
-------------------------------------------------------------------------------
--
-- File: macc_systolic_sym.vhd
--
-- Description:
--
-- This Multiply-Accumulate module is the basic building block for
-- systolic symmetric FIRs.
--
-- Block Diagram:
--
--    DLY_IN_F ----R---R---*--------- DLY_OUT
--                         |
--    DLY_IN_R ----R------(+)
--                         |
--                         R
--                         |
--    COEFF    -----------(X)
--                         |
--                         R
--                         |
--    ACC_IN   -----------(+)---R---- ACC_OUT
--
--     R   : Register
--    (X)  : Multiplier
--    (+)  : Adder
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use ieee.math_real.all;
library misc_prims;

entity macc_systolic_sym is

generic (

   DATA_WIDTH   : positive;
   COEFF_WIDTH  : positive;
   ACC_PREC     : positive);

port (

   CLK        : in std_logic;
   RST        : in std_logic;
   DIN_VLD    : in std_logic;
   DLY_IN_F   : in std_logic_vector(DATA_WIDTH-1 downto 0);
   DLY_IN_R   : in std_logic_vector(DATA_WIDTH-1 downto 0);
   DLY_OUT    : out std_logic_vector(DATA_WIDTH-1 downto 0);
   COEFF      : in std_logic_vector(COEFF_WIDTH-1 downto 0);
   ACC_IN     : in std_logic_vector(ACC_PREC-1 downto 0);
   ACC_OUT    : out std_logic_vector(ACC_PREC-1 downto 0) := (others => '0'));

end macc_systolic_sym;

architecture rtl of macc_systolic_sym is

   signal dly_in_f_z1      : std_logic_vector(DATA_WIDTH-1 downto 0) := (others => '0');
   signal dly_in_f_z2      : std_logic_vector(DATA_WIDTH-1 downto 0) := (others => '0');
   signal dly_in_r_z1      : std_logic_vector(DATA_WIDTH-1 downto 0) := (others => '0');
   signal pre_add_a        : std_logic_vector(DATA_WIDTH downto 0);
   signal pre_add_b        : std_logic_vector(DATA_WIDTH downto 0);
   signal pre_add_y        : std_logic_vector(DATA_WIDTH downto 0);
   signal pre_add_reg      : std_logic_vector(DATA_WIDTH downto 0) := (others => '0');
   signal mult_out_full    : std_logic_vector(DATA_WIDTH+COEFF_WIDTH downto 0);
   signal mult_out_reg     : std_logic_vector(DATA_WIDTH downto 0) := (others => '0');
   signal acc_out_pre      : std_logic_vector(ACC_PREC-1 downto 0);

begin

   -- Delay Chain Registers

   process (CLK)
   begin
      if rising_edge(CLK) then
         if (DIN_VLD = '1') then
            dly_in_f_z1 <= DLY_IN_F;
            dly_in_f_z2 <= dly_in_f_z1;
            dly_in_r_z1 <= DLY_IN_R;
         end if;
      end if;
   end process;

   DLY_OUT <= dly_in_f_z2;

   -- Pre-Adder

   pre_add_a <= std_logic_vector(resize(signed(dly_in_f_z2), DATA_WIDTH+1));
   pre_add_b <= std_logic_vector(resize(signed(dly_in_r_z1), DATA_WIDTH+1));

   pre_add_y <= std_logic_vector(signed(pre_add_a) + signed(pre_add_b));

   process (CLK)
   begin
      if rising_edge(CLK) then
         if (DIN_VLD = '1') then
            pre_add_reg <= pre_add_y;
         end if;
      end if;
   end process;

   -- Multiplier : Full width is COEFF_WIDTH + DATA_WIDTH + 1

   mult_out_full <= std_logic_vector(signed(COEFF) * signed(pre_add_reg));

   mult_out_rnd : misc_prims.misc_prims.round_conv
   generic map (
      DIN_WIDTH  => DATA_WIDTH+COEFF_WIDTH,
      DOUT_WIDTH => DATA_WIDTH+1)
   port map (
      CLK      => CLK,
      RST      => RST,
      DIN      => mult_out_full(COEFF_WIDTH+DATA_WIDTH-1 downto 0), -- remove extra sign bit
      DIN_VLD  => DIN_VLD,
      DOUT     => mult_out_reg,
      DOUT_VLD => open);

   -- Accumulator

   acc_out_pre <= std_logic_vector(signed(ACC_IN) + signed(mult_out_reg));

   process (CLK)
   begin
      if rising_edge(CLK) then
         if (DIN_VLD = '1') then
            ACC_OUT <= acc_out_pre;
         end if;
      end if;
   end process;

end;

