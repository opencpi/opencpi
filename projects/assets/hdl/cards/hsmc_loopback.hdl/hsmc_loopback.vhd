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
architecture rtl of hsmc_loopback_worker is
  signal s_num_clkout1p_rising_edges : unsigned(15 downto 0) := (others => '0');
  signal s_num_clkout1n_rising_edges : unsigned(15 downto 0) := (others => '0');
begin
  -- start bank 1
  --gen_xcvr_txp_7_0 : for idx in 0 to 7 generate
  --  xcvr_txp_7_0(idx) <= props_in.xcvr_txp_7_0(idx);
  --end generate gen_xcvr_txp_7_0;

  props_out.sda      <= sda;
  scl                <= props_in.scl;

  --gen_xcvr_rxp_7_0 : for idx in 0 to 7 generate
  --  props_out.xcvr_rxp_7_0(idx) <= xcvr_rxp_7_0(idx);
  --end generate gen_xcvr_rxp_7_0;

  clkout0            <= props_in.clkout0;
  props_out.clkin0   <= clkin0;

  -- end bank 1


  -- start bank 2
  d0_oe             <= '1' when (props_in.d0_d1_pin_directions   = even_d_to_odd_d_e) else '0';
  d1_oe             <= '0' when (props_in.d0_d1_pin_directions   = even_d_to_odd_d_e) else '1';
  d2_oe             <= '1' when (props_in.d2_d3_pin_directions   = even_d_to_odd_d_e) else '0';
  d3_oe             <= '0' when (props_in.d2_d3_pin_directions   = even_d_to_odd_d_e) else '1';
  d4_oe             <= '1' when (props_in.d4_d5_pin_directions   = even_d_to_odd_d_e) else '0';
  d5_oe             <= '0' when (props_in.d4_d5_pin_directions   = even_d_to_odd_d_e) else '1';
  d6_oe             <= '1' when (props_in.d6_d7_pin_directions   = even_d_to_odd_d_e) else '0';
  d7_oe             <= '0' when (props_in.d6_d7_pin_directions   = even_d_to_odd_d_e) else '1';
  d8_oe             <= '1' when (props_in.d8_d9_pin_directions   = even_d_to_odd_d_e) else '0';
  d9_oe             <= '0' when (props_in.d8_d9_pin_directions   = even_d_to_odd_d_e) else '1';
  d10_oe            <= '1' when (props_in.d10_d11_pin_directions = even_d_to_odd_d_e) else '0';
  d11_oe            <= '0' when (props_in.d10_d11_pin_directions = even_d_to_odd_d_e) else '1';
  d12_oe            <= '1' when (props_in.d12_d13_pin_directions = even_d_to_odd_d_e) else '0';
  d13_oe            <= '0' when (props_in.d12_d13_pin_directions = even_d_to_odd_d_e) else '1';
  d14_oe            <= '1' when (props_in.d14_d15_pin_directions = even_d_to_odd_d_e) else '0';
  d15_oe            <= '0' when (props_in.d14_d15_pin_directions = even_d_to_odd_d_e) else '1';
  d16_oe            <= '1' when (props_in.d16_d17_pin_directions = even_d_to_odd_d_e) else '0';
  d17_oe            <= '0' when (props_in.d16_d17_pin_directions = even_d_to_odd_d_e) else '1';
  d18_oe            <= '1' when (props_in.d18_d19_pin_directions = even_d_to_odd_d_e) else '0';
  d19_oe            <= '0' when (props_in.d18_d19_pin_directions = even_d_to_odd_d_e) else '1';
  d20_oe            <= '1' when (props_in.d20_d21_pin_directions = even_d_to_odd_d_e) else '0';
  d21_oe            <= '0' when (props_in.d20_d21_pin_directions = even_d_to_odd_d_e) else '1';
  d22_oe            <= '1' when (props_in.d22_d23_pin_directions = even_d_to_odd_d_e) else '0';
  d23_oe            <= '0' when (props_in.d22_d23_pin_directions = even_d_to_odd_d_e) else '1';
  d24_oe            <= '1' when (props_in.d24_d25_pin_directions = even_d_to_odd_d_e) else '0';
  d25_oe            <= '0' when (props_in.d24_d25_pin_directions = even_d_to_odd_d_e) else '1';
  d26_oe            <= '1' when (props_in.d26_d27_pin_directions = even_d_to_odd_d_e) else '0';
  d27_oe            <= '0' when (props_in.d26_d27_pin_directions = even_d_to_odd_d_e) else '1';
  d28_oe            <= '1' when (props_in.d28_d29_pin_directions = even_d_to_odd_d_e) else '0';
  d29_oe            <= '0' when (props_in.d28_d29_pin_directions = even_d_to_odd_d_e) else '1';
  d30_oe            <= '1' when (props_in.d30_d31_pin_directions = even_d_to_odd_d_e) else '0';
  d31_oe            <= '0' when (props_in.d30_d31_pin_directions = even_d_to_odd_d_e) else '1';
  d32_oe            <= '1' when (props_in.d32_d33_pin_directions = even_d_to_odd_d_e) else '0';
  d33_oe            <= '0' when (props_in.d32_d33_pin_directions = even_d_to_odd_d_e) else '1';
  d34_oe            <= '1' when (props_in.d34_d35_pin_directions = even_d_to_odd_d_e) else '0';
  d35_oe            <= '0' when (props_in.d34_d35_pin_directions = even_d_to_odd_d_e) else '1';

  d0_o              <= props_in.d0_d1_output_voltage;
  d1_o              <= props_in.d0_d1_output_voltage;
  d2_o              <= props_in.d2_d3_output_voltage;
  d3_o              <= props_in.d2_d3_output_voltage;
  d4_o              <= props_in.d4_d5_output_voltage;
  d5_o              <= props_in.d4_d5_output_voltage;
  d6_o              <= props_in.d6_d7_output_voltage;
  d7_o              <= props_in.d6_d7_output_voltage;
  d8_o              <= props_in.d8_d9_output_voltage;
  d9_o              <= props_in.d8_d9_output_voltage;
  d10_o             <= props_in.d10_d11_output_voltage;
  d11_o             <= props_in.d10_d11_output_voltage;
  d12_o             <= props_in.d12_d13_output_voltage;
  d13_o             <= props_in.d12_d13_output_voltage;
  d14_o             <= props_in.d14_d15_output_voltage;
  d15_o             <= props_in.d14_d15_output_voltage;
  d16_o             <= props_in.d16_d17_output_voltage;
  d17_o             <= props_in.d16_d17_output_voltage;
  d18_o             <= props_in.d18_d19_output_voltage;
  d19_o             <= props_in.d18_d19_output_voltage;
  d20_o             <= props_in.d20_d21_output_voltage;
  d21_o             <= props_in.d20_d21_output_voltage;
  d22_o             <= props_in.d22_d23_output_voltage;
  d23_o             <= props_in.d22_d23_output_voltage;
  d24_o             <= props_in.d24_d25_output_voltage;
  d25_o             <= props_in.d24_d25_output_voltage;
  d26_o             <= props_in.d26_d27_output_voltage;
  d27_o             <= props_in.d26_d27_output_voltage;
  d28_o             <= props_in.d28_d29_output_voltage;
  d29_o             <= props_in.d28_d29_output_voltage;
  d30_o             <= props_in.d30_d31_output_voltage;
  d31_o             <= props_in.d30_d31_output_voltage;
  d32_o             <= props_in.d32_d33_output_voltage;
  d33_o             <= props_in.d32_d33_output_voltage;
  d34_o             <= props_in.d34_d35_output_voltage;
  d35_o             <= props_in.d34_d35_output_voltage;
  clkout1p          <= ctl_in.clk;
  clkout1n          <= ctl_in.clk;

  props_out.d_0_35(0)   <= d0_i;
  props_out.d_0_35(1)   <= d1_i;
  props_out.d_0_35(2)   <= d2_i;
  props_out.d_0_35(3)   <= d3_i;
  props_out.d_0_35(4)   <= d4_i;
  props_out.d_0_35(5)   <= d5_i;
  props_out.d_0_35(6)   <= d6_i;
  props_out.d_0_35(7)   <= d7_i;
  props_out.d_0_35(8)   <= d8_i;
  props_out.d_0_35(9)   <= d9_i;
  props_out.d_0_35(10)  <= d10_i;
  props_out.d_0_35(11)  <= d11_i;
  props_out.d_0_35(12)  <= d12_i;
  props_out.d_0_35(13)  <= d13_i;
  props_out.d_0_35(14)  <= d14_i;
  props_out.d_0_35(15)  <= d15_i;
  props_out.d_0_35(16)  <= d16_i;
  props_out.d_0_35(17)  <= d17_i;
  props_out.d_0_35(18)  <= d18_i;
  props_out.d_0_35(19)  <= d19_i;
  props_out.d_0_35(20)  <= d20_i;
  props_out.d_0_35(21)  <= d21_i;
  props_out.d_0_35(22)  <= d22_i;
  props_out.d_0_35(23)  <= d23_i;
  props_out.d_0_35(24)  <= d24_i;
  props_out.d_0_35(25)  <= d25_i;
  props_out.d_0_35(26)  <= d26_i;
  props_out.d_0_35(27)  <= d27_i;
  props_out.d_0_35(28)  <= d28_i;
  props_out.d_0_35(29)  <= d29_i;
  props_out.d_0_35(30)  <= d30_i;
  props_out.d_0_35(31)  <= d31_i;
  props_out.d_0_35(32)  <= d32_i;
  props_out.d_0_35(33)  <= d33_i;
  props_out.d_0_35(34)  <= d34_i;
  props_out.d_0_35(35)  <= d35_i;

  props_out.num_clkout1p_rising_edges <= s_num_clkout1p_rising_edges;
  process(clkin1p)
  begin
    if rising_edge(clkin1p) then
      if s_num_clkout1p_rising_edges < x"7fff" then
        s_num_clkout1p_rising_edges <= s_num_clkout1p_rising_edges + 1;
      end if;
    end if;
  end process;

  props_out.num_clkout1n_rising_edges <= s_num_clkout1n_rising_edges;
  process(clkin1n)
  begin
    if rising_edge(clkin1n) then
      if s_num_clkout1n_rising_edges < x"7fff" then
        s_num_clkout1n_rising_edges <= s_num_clkout1n_rising_edges + 1;
      end if;
    end if;
  end process;

  -- end bank 2


  -- start bank 3
  d40_oe            <= '1' when (props_in.d40_d41_pin_directions = even_d_to_odd_d_e) else '0';
  d41_oe            <= '0' when (props_in.d40_d41_pin_directions = even_d_to_odd_d_e) else '1';
  d42_oe            <= '1' when (props_in.d42_d43_pin_directions = even_d_to_odd_d_e) else '0';
  d43_oe            <= '0' when (props_in.d42_d43_pin_directions = even_d_to_odd_d_e) else '1';
  d44_oe            <= '1' when (props_in.d44_d45_pin_directions = even_d_to_odd_d_e) else '0';
  d45_oe            <= '0' when (props_in.d44_d45_pin_directions = even_d_to_odd_d_e) else '1';
  d46_oe            <= '1' when (props_in.d46_d47_pin_directions = even_d_to_odd_d_e) else '0';
  d47_oe            <= '0' when (props_in.d46_d47_pin_directions = even_d_to_odd_d_e) else '1';
  d48_oe            <= '1' when (props_in.d48_d49_pin_directions = even_d_to_odd_d_e) else '0';
  d49_oe            <= '0' when (props_in.d48_d49_pin_directions = even_d_to_odd_d_e) else '1';
  d50_oe            <= '1' when (props_in.d50_d51_pin_directions = even_d_to_odd_d_e) else '0';
  d51_oe            <= '0' when (props_in.d50_d51_pin_directions = even_d_to_odd_d_e) else '1';
  d52_oe            <= '1' when (props_in.d52_d53_pin_directions = even_d_to_odd_d_e) else '0';
  d53_oe            <= '0' when (props_in.d52_d53_pin_directions = even_d_to_odd_d_e) else '1';
  d54_oe            <= '1' when (props_in.d54_d55_pin_directions = even_d_to_odd_d_e) else '0';
  d55_oe            <= '0' when (props_in.d54_d55_pin_directions = even_d_to_odd_d_e) else '1';
  d56_oe            <= '1' when (props_in.d56_d57_pin_directions = even_d_to_odd_d_e) else '0';
  d57_oe            <= '0' when (props_in.d56_d57_pin_directions = even_d_to_odd_d_e) else '1';
  d58_oe            <= '1' when (props_in.d58_d59_pin_directions = even_d_to_odd_d_e) else '0';
  d59_oe            <= '0' when (props_in.d58_d59_pin_directions = even_d_to_odd_d_e) else '1';
  d60_oe            <= '1' when (props_in.d60_d61_pin_directions = even_d_to_odd_d_e) else '0';
  d61_oe            <= '0' when (props_in.d60_d61_pin_directions = even_d_to_odd_d_e) else '1';
  d62_oe            <= '1' when (props_in.d62_d63_pin_directions = even_d_to_odd_d_e) else '0';
  d63_oe            <= '0' when (props_in.d62_d63_pin_directions = even_d_to_odd_d_e) else '1';
  d64_oe            <= '1' when (props_in.d64_d65_pin_directions = even_d_to_odd_d_e) else '0';
  d65_oe            <= '0' when (props_in.d64_d65_pin_directions = even_d_to_odd_d_e) else '1';
  d66_oe            <= '1' when (props_in.d66_d67_pin_directions = even_d_to_odd_d_e) else '0';
  d67_oe            <= '0' when (props_in.d66_d67_pin_directions = even_d_to_odd_d_e) else '1';
  d68_oe            <= '1' when (props_in.d68_d69_pin_directions = even_d_to_odd_d_e) else '0';
  d69_oe            <= '0' when (props_in.d68_d69_pin_directions = even_d_to_odd_d_e) else '1';
  d70_oe            <= '1' when (props_in.d70_d71_pin_directions = even_d_to_odd_d_e) else '0';
  d71_oe            <= '0' when (props_in.d70_d71_pin_directions = even_d_to_odd_d_e) else '1';
  d72_oe            <= '1' when (props_in.d72_d73_pin_directions = even_d_to_odd_d_e) else '0';
  d73_oe            <= '0' when (props_in.d72_d73_pin_directions = even_d_to_odd_d_e) else '1';
  d74_oe            <= '1' when (props_in.d74_d75_pin_directions = even_d_to_odd_d_e) else '0';
  d75_oe            <= '0' when (props_in.d74_d75_pin_directions = even_d_to_odd_d_e) else '1';
  d76_oe            <= '1' when (props_in.d76_d77_pin_directions = even_d_to_odd_d_e) else '0';
  d77_oe            <= '0' when (props_in.d76_d77_pin_directions = even_d_to_odd_d_e) else '1';
  d78_oe            <= '1' when (props_in.d78_d79_pin_directions = even_d_to_odd_d_e) else '0';
  d79_oe            <= '0' when (props_in.d78_d79_pin_directions = even_d_to_odd_d_e) else '1';

  d40_o             <= props_in.d40_d41_output_voltage;
  d41_o             <= props_in.d40_d41_output_voltage;
  d42_o             <= props_in.d42_d43_output_voltage;
  d43_o             <= props_in.d42_d43_output_voltage;
  d44_o             <= props_in.d44_d45_output_voltage;
  d45_o             <= props_in.d44_d45_output_voltage;
  d46_o             <= props_in.d46_d47_output_voltage;
  d47_o             <= props_in.d46_d47_output_voltage;
  d48_o             <= props_in.d48_d49_output_voltage;
  d49_o             <= props_in.d48_d49_output_voltage;
  d50_o             <= props_in.d50_d51_output_voltage;
  d51_o             <= props_in.d50_d51_output_voltage;
  d52_o             <= props_in.d52_d53_output_voltage;
  d53_o             <= props_in.d52_d53_output_voltage;
  d54_o             <= props_in.d54_d55_output_voltage;
  d55_o             <= props_in.d54_d55_output_voltage;
  d56_o             <= props_in.d56_d57_output_voltage;
  d57_o             <= props_in.d56_d57_output_voltage;
  d58_o             <= props_in.d58_d59_output_voltage;
  d59_o             <= props_in.d58_d59_output_voltage;
  d60_o             <= props_in.d60_d61_output_voltage;
  d61_o             <= props_in.d60_d61_output_voltage;
  d62_o             <= props_in.d62_d63_output_voltage;
  d63_o             <= props_in.d62_d63_output_voltage;
  d64_o             <= props_in.d64_d65_output_voltage;
  d65_o             <= props_in.d64_d65_output_voltage;
  d66_o             <= props_in.d66_d67_output_voltage;
  d67_o             <= props_in.d66_d67_output_voltage;
  d68_o             <= props_in.d68_d69_output_voltage;
  d69_o             <= props_in.d68_d69_output_voltage;
  d70_o             <= props_in.d70_d71_output_voltage;
  d71_o             <= props_in.d70_d71_output_voltage;
  d72_o             <= props_in.d72_d73_output_voltage;
  d73_o             <= props_in.d72_d73_output_voltage;
  d74_o             <= props_in.d74_d75_output_voltage;
  d75_o             <= props_in.d74_d75_output_voltage;
  d76_o             <= props_in.d76_d77_output_voltage;
  d77_o             <= props_in.d76_d77_output_voltage;
  d78_o             <= props_in.d78_d79_output_voltage;
  d79_o             <= props_in.d78_d79_output_voltage;

  props_out.d_40_79(0)   <= d40_i;
  props_out.d_40_79(1)   <= d41_i;
  props_out.d_40_79(2)   <= d42_i;
  props_out.d_40_79(3)   <= d43_i;
  props_out.d_40_79(4)   <= d44_i;
  props_out.d_40_79(5)   <= d45_i;
  props_out.d_40_79(6)   <= d46_i;
  props_out.d_40_79(7)   <= d47_i;
  props_out.d_40_79(8)   <= d48_i;
  props_out.d_40_79(9)   <= d49_i;
  props_out.d_40_79(10)  <= d50_i;
  props_out.d_40_79(11)  <= d51_i;
  props_out.d_40_79(12)  <= d52_i;
  props_out.d_40_79(13)  <= d53_i;
  props_out.d_40_79(14)  <= d54_i;
  props_out.d_40_79(15)  <= d55_i;
  props_out.d_40_79(16)  <= d56_i;
  props_out.d_40_79(17)  <= d57_i;
  props_out.d_40_79(18)  <= d58_i;
  props_out.d_40_79(19)  <= d59_i;
  props_out.d_40_79(20)  <= d60_i;
  props_out.d_40_79(21)  <= d61_i;
  props_out.d_40_79(22)  <= d62_i;
  props_out.d_40_79(23)  <= d63_i;
  props_out.d_40_79(24)  <= d64_i;
  props_out.d_40_79(25)  <= d65_i;
  props_out.d_40_79(26)  <= d66_i;
  props_out.d_40_79(27)  <= d67_i;
  props_out.d_40_79(28)  <= d68_i;
  props_out.d_40_79(29)  <= d69_i;
  props_out.d_40_79(30)  <= d70_i;
  props_out.d_40_79(31)  <= d71_i;
  props_out.d_40_79(32)  <= d72_i;
  props_out.d_40_79(33)  <= d73_i;
  props_out.d_40_79(34)  <= d74_i;
  props_out.d_40_79(35)  <= d75_i;
  props_out.d_40_79(36)  <= d76_i;
  props_out.d_40_79(37)  <= d77_i;
  props_out.d_40_79(38)  <= d78_i;
  props_out.d_40_79(39)  <= d79_i;

  props_out.psntn <= psntn;

  -- end bank 3

end rtl;

