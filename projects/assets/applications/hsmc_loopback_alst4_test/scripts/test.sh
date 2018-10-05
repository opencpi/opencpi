# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

echo "usage: ./test.sh"

echo "searching for OpenCPI-enabled PCIE devices..."
PCIE_DEV_NUM=$(ocpihdl search | grep "platform \"alst4\"" | sed "s/OpenCPI HDL device found: 'PCI:0000://g" | sed "s/:00.0.*//g")
HSMC_LOOPBACK_WORKER_INSTANCE_NUM=c/HSMC_ALST4_A_hsmc_loopback

echo "loading and initializing FPGA bitfile..."

ocpihdl -d $PCIE_DEV_NUM load ../../hdl/assemblies/empty/container-empty_alst4_base_cnt_hsmc_loopback_card_hsmc_alst4_a_hsmc_alst4_b/target-stratix4/empty_alst4_base_cnt_hsmc_loopback_card_hsmc_alst4_a_hsmc_alst4_b.bitz 1> /dev/null
if [ "$?" != "0" ]; then
  echo "ERROR loading bitstream"
  exit 1
fi
ocpihdl -d $PCIE_DEV_NUM wunreset 3 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM wunreset 4 > /dev/null 2>&1

DO_PORT_A=0
printf "HSMC Port A card detected... "
if [ $(ocpihdl -d $PCIE_DEV_NUM get 3 psntn | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g" | sed "s/psntn://g") = "false" ]; then
  DO_PORT_A=1
  echo "true"
else
  echo "false"
fi
DO_PORT_B=0
printf "HSMC Port B card detected... "
if [ $(ocpihdl -d $PCIE_DEV_NUM get 4 psntn | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g" | sed "s/psntn://g") = "false" ]; then
  DO_PORT_B=1
  echo "true"
else
  echo "false"
fi

if [ "$DO_PORT_A" = "1" ]; then
echo
echo -TESTING ALST4 HSMC PORT A BANK 1
printf "   "
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM scl 1 > /dev/null 2>&1
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM sda | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM scl 0 > /dev/null 2>&1
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM sda | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkout0 1 > /dev/null 2>&1
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkin0 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkout0 0 > /dev/null 2>&1
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkin0 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")

echo
echo -TESTING ALST4 HSMC PORT A BANK 2
echo "   setting direction even Dxx -> odd  Dxx"
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
echo "          d0   d1   d2   d3   d4   d5   d6   d7   d8   d9   d10  d11  d12  d13  d14  d15  d16  d17  d18  d19  d20  d21  d22  d23  d24  d25  d26  d27  d28  d29  d30  d31  d32  d33  d34  d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
echo "          d0    d1    d2    d3    d4    d5    d6    d7    d8    d9    d10   d11   d12   d13   d14   d15   d16   d17   d18   d19   d20   d21   d22   d23   d24   d25   d26   d27   d28   d29   d30   d31   d32   d33   d34   d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")

echo "   setting direction odd  Dxx -> even Dxx"
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
echo "          d0   d1   d2   d3   d4   d5   d6   d7   d8   d9   d10  d11  d12  d13  d14  d15  d16  d17  d18  d19  d20  d21  d22  d23  d24  d25  d26  d27  d28  d29  d30  d31  d32  d33  d34  d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
echo "          d0    d1    d2    d3    d4    d5    d6    d7    d8    d9    d10   d11   d12   d13   d14   d15   d16   d17   d18   d19   d20   d21   d22   d23   d24   d25   d26   d27   d28   d29   d30   d31   d32   d33   d34   d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM num_clkout1p_rising_edges | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]*//g" | sed "s/[0-9]* //g")
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM num_clkout1n_rising_edges | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]*//g" | sed "s/[0-9]* //g")


echo
echo -TESTING ALST4 HSMC PORT A BANK 3

ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")

echo "   setting direction odd  Dxx -> even Dxx"
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
fi

HSMC_LOOPBACK_WORKER_INSTANCE_NUM=c/HSMC_ALST4_B_hsmc_loopback

if [ "$DO_PORT_B" = "1" ]; then
echo
echo -TESTING ALST4 HSMC PORT B BANK 1
printf "   "
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM scl 1 > /dev/null 2>&1
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM sda | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM scl 0 > /dev/null 2>&1
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM sda | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkout0 1 > /dev/null 2>&1
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkin0 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkout0 0 > /dev/null 2>&1
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM clkin0 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")

echo
echo -TESTING ALST4 HSMC PORT B BANK 2
echo "   setting direction even Dxx -> odd  Dxx"
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
echo "          d0   d1   d2   d3   d4   d5   d6   d7   d8   d9   d10  d11  d12  d13  d14  d15  d16  d17  d18  d19  d20  d21  d22  d23  d24  d25  d26  d27  d28  d29  d30  d31  d32  d33  d34  d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
echo "          d0    d1    d2    d3    d4    d5    d6    d7    d8    d9    d10   d11   d12   d13   d14   d15   d16   d17   d18   d19   d20   d21   d22   d23   d24   d25   d26   d27   d28   d29   d30   d31   d32   d33   d34   d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")

echo "   setting direction odd  Dxx -> even Dxx"
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
echo "          d0   d1   d2   d3   d4   d5   d6   d7   d8   d9   d10  d11  d12  d13  d14  d15  d16  d17  d18  d19  d20  d21  d22  d23  d24  d25  d26  d27  d28  d29  d30  d31  d32  d33  d34  d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d0_d1_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d2_d3_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d4_d5_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d6_d7_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d8_d9_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d10_d11_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d12_d13_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d14_d15_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d16_d17_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d18_d19_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d20_d21_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d22_d23_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d24_d25_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d26_d27_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d28_d29_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d30_d31_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d32_d33_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d34_d35_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
echo "          d0    d1    d2    d3    d4    d5    d6    d7    d8    d9    d10   d11   d12   d13   d14   d15   d16   d17   d18   d19   d20   d21   d22   d23   d24   d25   d26   d27   d28   d29   d30   d31   d32   d33   d34   d35"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_0_35 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM num_clkout1p_rising_edges | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]*//g" | sed "s/[0-9]* //g")
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM num_clkout1n_rising_edges | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]*//g" | sed "s/[0-9]* //g")

echo
echo -TESTING ALST4 HSMC PORT B BANK 3

ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")

echo "   setting direction odd  Dxx -> even Dxx"
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions odd_d_to_even_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 1 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 1 > /dev/null 2>&1
echo "   setting all outputs to 1"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_pin_directions even_d_to_odd_d  > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d40_d41_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d42_d43_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d44_d45_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d46_d47_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d48_d49_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d50_d51_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d52_d53_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d54_d55_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d56_d57_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d58_d59_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d60_d61_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d62_d63_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d64_d65_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d66_d67_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d68_d69_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d70_d71_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d72_d73_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d74_d75_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d76_d77_output_voltage 0 > /dev/null 2>&1
ocpihdl -d $PCIE_DEV_NUM set $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d78_d79_output_voltage 0 > /dev/null 2>&1
echo "   setting all outputs to 0"
printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get $HSMC_LOOPBACK_WORKER_INSTANCE_NUM d_40_79 | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")
fi


