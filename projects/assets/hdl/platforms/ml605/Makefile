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

# The ml605 platform.
Libraries=virtex6 xilinx
Cores=pcie_4243_trn_v6_gtx_x4_250
#Cores=pcie_4243_trn_v6_gtx_x4_250 ./chipscope_icon ./chipscope_ila
ExportFiles=jtagSupport_ml605 loadFlash_ml605 \
            ml605.ucf ml605.ut ml605.mk testbias_ml605_base.bitz
# We specify these to get the order we want
SourceFiles= ml605_pkg.vhd ml605.vhd pci_ml605.v 
#SourceFiles=chipscope_pkg.vhd ml605_pkg.vhd ml605.vhd pci_ml605.v 
Configurations=\
	base \
	cfg_0rx_1tx_fmcomms_2_3_hpc_lvds \
	cfg_0rx_1tx_fmcomms_2_3_lpc_lvds \
	cfg_0rx_2tx_fmcomms_2_3_hpc_lvds \
	cfg_0rx_2tx_fmcomms_2_3_lpc_lvds \
	cfg_1rx_0tx_fmcomms_2_3_hpc_lvds \
	cfg_1rx_0tx_fmcomms_2_3_lpc_lvds \
	cfg_1rx_1tx_fmcomms_2_3_hpc_lvds \
	cfg_1rx_1tx_fmcomms_2_3_lpc_lvds \
	cfg_1rx_2tx_fmcomms_2_3_hpc_lvds \
	cfg_1rx_2tx_fmcomms_2_3_lpc_lvds \
	cfg_2rx_0tx_fmcomms_2_3_hpc_lvds \
	cfg_2rx_0tx_fmcomms_2_3_lpc_lvds \
	cfg_2rx_1tx_fmcomms_2_3_hpc_lvds \
	cfg_2rx_1tx_fmcomms_2_3_lpc_lvds \
	cfg_2rx_2tx_fmcomms_2_3_hpc_lvds \
	cfg_2rx_2tx_fmcomms_2_3_lpc_lvds \
	ml605_flash \
	ml605_lime_tx \
	ml605_zipper_fmc_hpc_rx_tx \
	ml605_zipper_fmc_hpc_rx \
	ml605_zipper_fmc_hpc_tx \
	ml605_zipper_fmc_lpc_rx_tx \
	ml605_zipper_fmc_lpc_rx \
	ml605_zipper_fmc_lpc_tx
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk
