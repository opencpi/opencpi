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

$(if $(realpath $(OCPI_CDK_DIR)),,\
  $(error The OCPI_CDK_DIR environment variable is not set correctly.))
# To restrict the primitives that are built or run, you can set the Libraries or Cores
# variables to the specific list of which ones you want to build and run, e.g.:
ImportCoreDirs=\
  $(OCPI_HDL_IMPORTS_DIR)/coregen/temac_v6 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_trn_v6_gtx_x4_250 \
  $(OCPI_HDL_IMPORTS_DIR)/coregen/pcie_4243_hip_s4gx_gen2_x4_128
PrimitiveLibraries=comms_prims \
                   misc_prims \
                   util_prims \
                   dsp_prims \
                   temac_v6 \
                   pcie_4243_trn_v6_gtx_x4_250 \
                   pcie_4243_hip_s4gx_gen2_x4_128 \
                   virtex6 \
                   stratix4 \
                   xilinx \
                   zynq \
                   i2c
# PrimitiveCores=core1 core2
# Otherwise all primitives will be built
include $(OCPI_CDK_DIR)/include/hdl/hdl-primitives.mk
