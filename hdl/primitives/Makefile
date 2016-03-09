#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #
ImportCoreDirs=\
  $(OCPI_HDL_IMPORTS_DIR)/coregen/temac_v6 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v6_mig37 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/dram_v5_mig34 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/fft_v5_4k_strm_nat \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v6_gtx_x4_250 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v5_gtp_x8_125 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v5_gtx_x8_125 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_hip_s4gx_gen2_x4_128 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/ddc_4243_4ch_v5

# These are order-sensitive
PrimitiveLibraries=bsv fixed_float ocpi util platform sdp zynq stratix4 virtex5 virtex6 xilinx i2c
# All cores here are imported
PrimitiveCores=

include $(OCPI_CDK_DIR)/include/hdl/hdl-primitives.mk


