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

echo "usage: ./test_hsmc_debug_header_breakout_is_plugged_into_port_a.sh    #this will run the whole script without prompts"
echo "usage: ./test_hsmc_debug_header_breakout_is_plugged_into_port_a.sh -p # this will prompt you for each LED"

echo "searching for OpenCPI-enabled PCIE devices..."
PCIE_DEV_NUM=$(ocpihdl search | grep "platform \"alst4\"" | sed "s/OpenCPI HDL device found: 'PCI:0000://g" | sed "s/:00.0.*//g")

echo "loading and initializing FPGA bitfile..."

ocpihdl -d $PCIE_DEV_NUM load ../../hdl/assemblies/empty/container-empty_alst4_base_cnt_hsmc_alst4_debug_header_breakout_card_hsmc_alst4_a/target-stratix4/empty_alst4_base_cnt_hsmc_alst4_debug_header_breakout_card_hsmc_alst4_a.bitz
if [ "$?" != "0" ]; then
  echo "ERROR loading bitstream"
  exit 1
fi
ocpihdl -d $PCIE_DEV_NUM wunreset 3 > /dev/null 2>&1

ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1

echo
echo -TESTING ALST4 HSMC PORT A BANK 3
echo "   turned all LEDs off"

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D1 (verifies HSM_D40, HSM_D41, HSM_D42, HSM_D43)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D2 (verifies HSM_D44, HSM_D45, HSM_D46, HSM_D47)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D3 (verifies HSM_D48, HSM_D49, HSM_D50, HSM_D51)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D4 (verifies HSM_D52, HSM_D53, HSM_D54, HSM_D55)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D5 (verifies HSM_D56, HSM_D57, HSM_D58, HSM_D59)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D6 (verifies HSM_D60, HSM_D61, HSM_D62, HSM_D63)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D7 (verifies HSM_D64, HSM_D65, HSM_D66, HSM_D67)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D8 (verifies HSM_D68, HSM_D69, HSM_D70, HSM_D71)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D9 (verifies HSM_D72, HSM_D73, HSM_D74, HSM_D75)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5

if [ ! -z "$1" ]; then
echo "   press any key to continue..."
read -n 1 -s
fi
echo "   testing D10 (verifies HSM_D76, HSM_D77, HSM_D78, HSM_D79)"
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 > /dev/null 2>&1
sleep 0.5
ocpihdl -d $PCIE_DEV_NUM set hsmc_debug_header_breakout d_40_79 0 > /dev/null 2>&1
sleep 0.5
echo "   all LEDs are off"

printf "   "
echo $(ocpihdl -d $PCIE_DEV_NUM get hsmc_debug_header_breakout psntn | sed "s/[a! -zA-Z ()/_.0-9]*with index [0-9]//g" | sed "s/ [0-9]*//g")

