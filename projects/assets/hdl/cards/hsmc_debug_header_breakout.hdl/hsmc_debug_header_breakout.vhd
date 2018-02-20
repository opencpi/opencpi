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

library ieee, ocpi;
use ieee.std_logic_1164.all, ieee.numeric_std.all, ocpi.types.all;
architecture rtl of hsmc_debug_header_breakout_worker is
begin
  -- bank 1
  clkout0            <= props_in.clkout0;
  props_out.clkin0   <= clkin0;
  
  -- start bank 2
  d0_oe             <= '1' when (props_in.d_0_37_direction_is_out(0) = '1') else '0';
  d1_oe             <= '1' when (props_in.d_0_37_direction_is_out(1) = '1') else '0';
  d2_oe             <= '1' when (props_in.d_0_37_direction_is_out(2) = '1') else '0';
  d3_oe             <= '1' when (props_in.d_0_37_direction_is_out(3) = '1') else '0';
  d4_oe             <= '1' when (props_in.d_0_37_direction_is_out(4) = '1') else '0';
  d5_oe             <= '1' when (props_in.d_0_37_direction_is_out(5) = '1') else '0';
  d6_oe             <= '1' when (props_in.d_0_37_direction_is_out(6) = '1') else '0';
  d7_oe             <= '1' when (props_in.d_0_37_direction_is_out(7) = '1') else '0';
  d8_oe             <= '1' when (props_in.d_0_37_direction_is_out(8) = '1') else '0';
  d9_oe             <= '1' when (props_in.d_0_37_direction_is_out(9) = '1') else '0';
  d10_oe            <= '1' when (props_in.d_0_37_direction_is_out(10) = '1') else '0';
  d11_oe            <= '1' when (props_in.d_0_37_direction_is_out(11) = '1') else '0';
  d12_oe            <= '1' when (props_in.d_0_37_direction_is_out(12) = '1') else '0';
  d13_oe            <= '1' when (props_in.d_0_37_direction_is_out(13) = '1') else '0';
  d14_oe            <= '1' when (props_in.d_0_37_direction_is_out(14) = '1') else '0';
  d15_oe            <= '1' when (props_in.d_0_37_direction_is_out(15) = '1') else '0';
  d16_oe            <= '1' when (props_in.d_0_37_direction_is_out(16) = '1') else '0';
  d17_oe            <= '1' when (props_in.d_0_37_direction_is_out(17) = '1') else '0';
  d18_oe            <= '1' when (props_in.d_0_37_direction_is_out(18) = '1') else '0';
  d19_oe            <= '1' when (props_in.d_0_37_direction_is_out(19) = '1') else '0';
  d20_oe            <= '1' when (props_in.d_0_37_direction_is_out(20) = '1') else '0';
  d21_oe            <= '1' when (props_in.d_0_37_direction_is_out(21) = '1') else '0';
  d22_oe            <= '1' when (props_in.d_0_37_direction_is_out(22) = '1') else '0';
  d23_oe            <= '1' when (props_in.d_0_37_direction_is_out(23) = '1') else '0';
  d24_oe            <= '1' when (props_in.d_0_37_direction_is_out(24) = '1') else '0';
  d25_oe            <= '1' when (props_in.d_0_37_direction_is_out(25) = '1') else '0';
  d26_oe            <= '1' when (props_in.d_0_37_direction_is_out(26) = '1') else '0';
  d27_oe            <= '1' when (props_in.d_0_37_direction_is_out(27) = '1') else '0';
  d28_oe            <= '1' when (props_in.d_0_37_direction_is_out(28) = '1') else '0';
  d29_oe            <= '1' when (props_in.d_0_37_direction_is_out(29) = '1') else '0';
  d30_oe            <= '1' when (props_in.d_0_37_direction_is_out(30) = '1') else '0';
  d31_oe            <= '1' when (props_in.d_0_37_direction_is_out(31) = '1') else '0';
  d32_oe            <= '1' when (props_in.d_0_37_direction_is_out(32) = '1') else '0';
  d33_oe            <= '1' when (props_in.d_0_37_direction_is_out(33) = '1') else '0';
  d34_oe            <= '1' when (props_in.d_0_37_direction_is_out(34) = '1') else '0';
  d35_oe            <= '1' when (props_in.d_0_37_direction_is_out(35) = '1') else '0';
  d36_oe            <= '1' when (props_in.d_0_37_direction_is_out(36) = '1') else '0';

  props_out.d_0_37_direction_is_out(0)  <= props_in.d_0_37_direction_is_out(0);
  props_out.d_0_37_direction_is_out(1)  <= props_in.d_0_37_direction_is_out(1);
  props_out.d_0_37_direction_is_out(2)  <= props_in.d_0_37_direction_is_out(2);
  props_out.d_0_37_direction_is_out(3)  <= props_in.d_0_37_direction_is_out(3);
  props_out.d_0_37_direction_is_out(4)  <= props_in.d_0_37_direction_is_out(4);
  props_out.d_0_37_direction_is_out(5)  <= props_in.d_0_37_direction_is_out(5);
  props_out.d_0_37_direction_is_out(6)  <= props_in.d_0_37_direction_is_out(6);
  props_out.d_0_37_direction_is_out(7)  <= props_in.d_0_37_direction_is_out(7);
  props_out.d_0_37_direction_is_out(8)  <= props_in.d_0_37_direction_is_out(8);
  props_out.d_0_37_direction_is_out(9)  <= props_in.d_0_37_direction_is_out(9);
  props_out.d_0_37_direction_is_out(10) <= props_in.d_0_37_direction_is_out(10);
  props_out.d_0_37_direction_is_out(11) <= props_in.d_0_37_direction_is_out(11);
  props_out.d_0_37_direction_is_out(12) <= props_in.d_0_37_direction_is_out(12);
  props_out.d_0_37_direction_is_out(13) <= props_in.d_0_37_direction_is_out(13);
  props_out.d_0_37_direction_is_out(14) <= props_in.d_0_37_direction_is_out(14);
  props_out.d_0_37_direction_is_out(15) <= props_in.d_0_37_direction_is_out(15);
  props_out.d_0_37_direction_is_out(16) <= props_in.d_0_37_direction_is_out(16);
  props_out.d_0_37_direction_is_out(17) <= props_in.d_0_37_direction_is_out(17);
  props_out.d_0_37_direction_is_out(18) <= props_in.d_0_37_direction_is_out(18);
  props_out.d_0_37_direction_is_out(19) <= props_in.d_0_37_direction_is_out(19);
  props_out.d_0_37_direction_is_out(20) <= props_in.d_0_37_direction_is_out(20);
  props_out.d_0_37_direction_is_out(21) <= props_in.d_0_37_direction_is_out(21);
  props_out.d_0_37_direction_is_out(22) <= props_in.d_0_37_direction_is_out(22);
  props_out.d_0_37_direction_is_out(23) <= props_in.d_0_37_direction_is_out(23);
  props_out.d_0_37_direction_is_out(24) <= props_in.d_0_37_direction_is_out(24);
  props_out.d_0_37_direction_is_out(25) <= props_in.d_0_37_direction_is_out(25);
  props_out.d_0_37_direction_is_out(26) <= props_in.d_0_37_direction_is_out(26);
  props_out.d_0_37_direction_is_out(27) <= props_in.d_0_37_direction_is_out(27);
  props_out.d_0_37_direction_is_out(28) <= props_in.d_0_37_direction_is_out(28);
  props_out.d_0_37_direction_is_out(29) <= props_in.d_0_37_direction_is_out(29);
  props_out.d_0_37_direction_is_out(30) <= props_in.d_0_37_direction_is_out(30);
  props_out.d_0_37_direction_is_out(31) <= props_in.d_0_37_direction_is_out(31);
  props_out.d_0_37_direction_is_out(32) <= props_in.d_0_37_direction_is_out(32);
  props_out.d_0_37_direction_is_out(33) <= props_in.d_0_37_direction_is_out(33);
  props_out.d_0_37_direction_is_out(34) <= props_in.d_0_37_direction_is_out(34);
  props_out.d_0_37_direction_is_out(35) <= props_in.d_0_37_direction_is_out(35);
  props_out.d_0_37_direction_is_out(36) <= props_in.d_0_37_direction_is_out(36);
  props_out.d_0_37_direction_is_out(37) <= '0';

  d0_o              <= props_in.d_0_37(0);
  d1_o              <= props_in.d_0_37(1);
  d2_o              <= props_in.d_0_37(2);
  d3_o              <= props_in.d_0_37(3);
  d4_o              <= props_in.d_0_37(4);
  d5_o              <= props_in.d_0_37(5);
  d6_o              <= props_in.d_0_37(6);
  d7_o              <= props_in.d_0_37(7);
  d8_o              <= props_in.d_0_37(8);
  d9_o              <= props_in.d_0_37(9);
  d10_o             <= props_in.d_0_37(10);
  d11_o             <= props_in.d_0_37(11);
  d12_o             <= props_in.d_0_37(12);
  d13_o             <= props_in.d_0_37(13);
  d14_o             <= props_in.d_0_37(14);
  d15_o             <= props_in.d_0_37(15);
  d16_o             <= props_in.d_0_37(16);
  d17_o             <= props_in.d_0_37(17);
  d18_o             <= props_in.d_0_37(18);
  d19_o             <= props_in.d_0_37(19);
  d20_o             <= props_in.d_0_37(20);
  d21_o             <= props_in.d_0_37(21);
  d22_o             <= props_in.d_0_37(22);
  d23_o             <= props_in.d_0_37(23);
  d24_o             <= props_in.d_0_37(24);
  d25_o             <= props_in.d_0_37(25);
  d26_o             <= props_in.d_0_37(26);
  d27_o             <= props_in.d_0_37(27);
  d28_o             <= props_in.d_0_37(28);
  d29_o             <= props_in.d_0_37(29);
  d30_o             <= props_in.d_0_37(30);
  d31_o             <= props_in.d_0_37(31);
  d32_o             <= props_in.d_0_37(32);
  d33_o             <= props_in.d_0_37(33);
  d34_o             <= props_in.d_0_37(34);
  d35_o             <= props_in.d_0_37(35);
  d36_o             <= props_in.d_0_37(36);

  props_out.d_0_37(0)   <= d0_i when (props_in.d_0_37_direction_is_out(0) = '1') else props_in.d_0_37(0);
  props_out.d_0_37(1)   <= d1_i when (props_in.d_0_37_direction_is_out(1) = '1') else props_in.d_0_37(1);
  props_out.d_0_37(2)   <= d2_i when (props_in.d_0_37_direction_is_out(2) = '1') else props_in.d_0_37(2);
  props_out.d_0_37(3)   <= d3_i when (props_in.d_0_37_direction_is_out(3) = '1') else props_in.d_0_37(3);
  props_out.d_0_37(4)   <= d4_i when (props_in.d_0_37_direction_is_out(4) = '1') else props_in.d_0_37(4);
  props_out.d_0_37(5)   <= d5_i when (props_in.d_0_37_direction_is_out(5) = '1') else props_in.d_0_37(5);
  props_out.d_0_37(6)   <= d6_i when (props_in.d_0_37_direction_is_out(6) = '1') else props_in.d_0_37(6);
  props_out.d_0_37(7)   <= d7_i when (props_in.d_0_37_direction_is_out(7) = '1') else props_in.d_0_37(7);
  props_out.d_0_37(8)   <= d8_i when (props_in.d_0_37_direction_is_out(8) = '1') else props_in.d_0_37(8);
  props_out.d_0_37(9)   <= d9_i when (props_in.d_0_37_direction_is_out(9) = '1') else props_in.d_0_37(9);
  props_out.d_0_37(10)  <= d10_i when (props_in.d_0_37_direction_is_out(10) = '1') else props_in.d_0_37(10);
  props_out.d_0_37(11)  <= d11_i when (props_in.d_0_37_direction_is_out(11) = '1') else props_in.d_0_37(11);
  props_out.d_0_37(12)  <= d12_i when (props_in.d_0_37_direction_is_out(12) = '1') else props_in.d_0_37(12);
  props_out.d_0_37(13)  <= d13_i when (props_in.d_0_37_direction_is_out(13) = '1') else props_in.d_0_37(13);
  props_out.d_0_37(14)  <= d14_i when (props_in.d_0_37_direction_is_out(14) = '1') else props_in.d_0_37(14);
  props_out.d_0_37(15)  <= d15_i when (props_in.d_0_37_direction_is_out(15) = '1') else props_in.d_0_37(15);
  props_out.d_0_37(16)  <= d16_i when (props_in.d_0_37_direction_is_out(16) = '1') else props_in.d_0_37(16);
  props_out.d_0_37(17)  <= d17_i when (props_in.d_0_37_direction_is_out(17) = '1') else props_in.d_0_37(17);
  props_out.d_0_37(18)  <= d18_i when (props_in.d_0_37_direction_is_out(18) = '1') else props_in.d_0_37(18);
  props_out.d_0_37(19)  <= d19_i when (props_in.d_0_37_direction_is_out(19) = '1') else props_in.d_0_37(19);
  props_out.d_0_37(20)  <= d20_i when (props_in.d_0_37_direction_is_out(20) = '1') else props_in.d_0_37(20);
  props_out.d_0_37(21)  <= d21_i when (props_in.d_0_37_direction_is_out(21) = '1') else props_in.d_0_37(21);
  props_out.d_0_37(22)  <= d22_i when (props_in.d_0_37_direction_is_out(22) = '1') else props_in.d_0_37(22);
  props_out.d_0_37(23)  <= d23_i when (props_in.d_0_37_direction_is_out(23) = '1') else props_in.d_0_37(23);
  props_out.d_0_37(24)  <= d24_i when (props_in.d_0_37_direction_is_out(24) = '1') else props_in.d_0_37(24);
  props_out.d_0_37(25)  <= d25_i when (props_in.d_0_37_direction_is_out(25) = '1') else props_in.d_0_37(25);
  props_out.d_0_37(26)  <= d26_i when (props_in.d_0_37_direction_is_out(26) = '1') else props_in.d_0_37(26);
  props_out.d_0_37(27)  <= d27_i when (props_in.d_0_37_direction_is_out(27) = '1') else props_in.d_0_37(27);
  props_out.d_0_37(28)  <= d28_i when (props_in.d_0_37_direction_is_out(28) = '1') else props_in.d_0_37(28);
  props_out.d_0_37(29)  <= d29_i when (props_in.d_0_37_direction_is_out(29) = '1') else props_in.d_0_37(29);
  props_out.d_0_37(30)  <= d30_i when (props_in.d_0_37_direction_is_out(30) = '1') else props_in.d_0_37(30);
  props_out.d_0_37(31)  <= d31_i when (props_in.d_0_37_direction_is_out(31) = '1') else props_in.d_0_37(31);
  props_out.d_0_37(32)  <= d32_i when (props_in.d_0_37_direction_is_out(32) = '1') else props_in.d_0_37(32);
  props_out.d_0_37(33)  <= d33_i when (props_in.d_0_37_direction_is_out(33) = '1') else props_in.d_0_37(33);
  props_out.d_0_37(34)  <= d34_i when (props_in.d_0_37_direction_is_out(34) = '1') else props_in.d_0_37(34);
  props_out.d_0_37(35)  <= d35_i when (props_in.d_0_37_direction_is_out(35) = '1') else props_in.d_0_37(35);
  props_out.d_0_37(36)  <= d35_i when (props_in.d_0_37_direction_is_out(36) = '1') else props_in.d_0_37(36);
  props_out.d_0_37(37)  <= d37;

  -- end bank 2


  -- bank 3
  gen_d_40_79 : for idx in 40 to 79 generate
    d_40_79(idx-40) <= props_in.d_40_79(idx-40);
  end generate gen_d_40_79;
  props_out.psntn <= psntn;

end rtl;

